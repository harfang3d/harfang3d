// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/recast_detour.h"
#include "engine/assets_rw_interface.h"
#include "engine/geometry.h"
#include "engine/render_pipeline.h"

#include "foundation/cext.h"
#include "foundation/color.h"
#include "foundation/data.h"
#include "foundation/file_rw_interface.h"
#include "foundation/log.h"

#if HG_ENABLE_RECAST_DETOUR_API

#include <DetourNavMeshBuilder.h>
#include <DetourNavMeshQuery.h>
#include <Recast.h>

#include <math.h>

namespace hg {

dtNavMesh *LoadNavMesh(const char *path, const Reader &ir, const ReadProvider &ip) {
	ScopedReadHandle h(ip, path);

	if (!ir.is_valid(h))
		return nullptr;

	struct NavMeshSetHeader {
		int magic;
		int version;
		int numTiles;
		dtNavMeshParams params;
	};

	struct NavMeshTileHeader {
		dtTileRef tileRef;
		int dataSize;
	};

	// read header
	NavMeshSetHeader header;

	if (!Read(ir, h, header))
		return nullptr;

	if (header.magic != NAVMESHSET_MAGIC)
		return nullptr;
	if (header.version != NAVMESHSET_VERSION)
		return nullptr;

	auto mesh = dtAllocNavMesh();
	if (!mesh)
		return nullptr;

	auto status = mesh->init(&header.params);
	if (dtStatusFailed(status)) {
		dtFree(mesh);
		return nullptr;
	}

	// read tiles
	for (int i = 0; i < header.numTiles; ++i) {
		NavMeshTileHeader tileHeader;

		if (!Read(ir, h, tileHeader)) {
			dtFree(mesh);
			return nullptr;
		}

		if (!tileHeader.tileRef || !tileHeader.dataSize)
			break;

		unsigned char *data = (unsigned char *)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);
		if (!data)
			break;
		memset(data, 0, tileHeader.dataSize);

		auto readLen = ir.read(h, data, tileHeader.dataSize);
		if (readLen != tileHeader.dataSize) {
			dtFree(data);
			dtFree(mesh);
			return nullptr;
		}

		mesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, 0);
	}

	return mesh;
}

dtNavMesh *LoadNavMeshFromFile(const char *path) { return LoadNavMesh(path, g_file_reader, g_file_read_provider); }
dtNavMesh *LoadNavMeshFromAssets(const char *path) { return LoadNavMesh(path, g_assets_reader, g_assets_read_provider); }

void DestroyNavMesh(dtNavMesh *mesh) { dtFree(mesh); }

//
dtNavMeshQuery *CreateNavMeshQuery(const dtNavMesh *mesh) {
	auto query = dtAllocNavMeshQuery();
	auto status = query->init(mesh, 2048);

	if (dtStatusFailed(status)) {
		dtFree(query);
		query = nullptr;
	}

	return query;
}

void DestroyNavMeshQuery(dtNavMeshQuery *query) { dtFree(query); }

//
std::vector<Vec3> FindNavigationPathTo(const dtNavMeshQuery *query, const Vec3 &from, const Vec3 &to) {
	dtQueryFilter m_filter; // map attributes flags

	m_filter.setIncludeFlags(0xffff);
	m_filter.setExcludeFlags(0);

	static const float m_polyPickExt[3] = {2, 4, 2}; // XYZ bounding box to pick nearby polygons

	const float *spos = &from.x, *epos = &to.x;

	static const int MAX_POLYS = 256;
	static const int MAX_SMOOTH = 2048;

	dtPolyRef polys[MAX_POLYS];
	int npolys = 0;
	int nstraightPath = 0;
	float straightPath[MAX_POLYS * 3];
	unsigned char straightPathFlags[MAX_POLYS];
	dtPolyRef straightPathPolys[MAX_POLYS];
	int straightPathOptions = DT_STRAIGHTPATH_ALL_CROSSINGS;

	dtPolyRef startRef = 0;
	dtPolyRef endRef = 0;
	query->findNearestPoly(spos, m_polyPickExt, &m_filter, &startRef, 0);
	query->findNearestPoly(epos, m_polyPickExt, &m_filter, &endRef, 0);

	if (startRef && endRef) {
		query->findPath(startRef, endRef, spos, epos, &m_filter, polys, &npolys, MAX_POLYS);
		if (npolys) {
			// in case of a partial path, make sure the end point is clamped to the last polygon
			float temp_epos[3] = {epos[0], epos[1], epos[2]};
			if (polys[npolys - 1] != endRef)
				query->closestPointOnPoly(polys[npolys - 1], epos, temp_epos, 0);

			query->findStraightPath(
				spos, temp_epos, polys, npolys, straightPath, straightPathFlags, straightPathPolys, &nstraightPath, MAX_POLYS, straightPathOptions);
		}
	} else {
		npolys = 0;
		nstraightPath = 0;
	}

	std::vector<Vec3> out(nstraightPath);
	for (int i = 0; i < nstraightPath; ++i)
		out[i] = {straightPath[i * 3], straightPath[i * 3 + 1], straightPath[i * 3 + 2]};

	return out;
}

//
void DrawNavMesh(const dtNavMesh *mesh, bgfx::ViewId view_id, const bgfx::VertexLayout &vtx_layout, bgfx::ProgramHandle program,
	const std::vector<UniformSetValue> &values, const std::vector<UniformSetTexture> &textures, RenderState state) {
	Color col(0.54f, 0.81f, 0.84f, 0.5f);

	Vertices vtx(vtx_layout, 3);
	int vtx_count = 0;

	for (int i = 0; i < mesh->getMaxTiles(); ++i) {
		const dtMeshTile *tile = mesh->getTile(i);
		if (!tile->header)
			continue;

		dtPolyRef base = mesh->getPolyRefBase(tile);

		int tileNum = mesh->decodePolyIdTile(base);

		for (int j = 0; j < tile->header->polyCount; ++j) {
			const dtPoly *p = &tile->polys[j];
			if (p->getType() == DT_POLYTYPE_OFFMESH_CONNECTION) // Skip off-mesh links.
				continue;

			const dtPolyDetail *pd = &tile->detailMeshes[j];

			for (int k = 0; k < pd->triCount; ++k) {
				const unsigned char *t = &tile->detailTris[(pd->triBase + k) * 4];
				for (int l = 0; l < 3; ++l) {
					if (t[l] < p->vertCount) {
						float *v = &tile->verts[p->verts[t[l]] * 3];
						vtx.Begin(vtx_count++).SetPos(Vec3(v[0], v[1], v[2])).SetColor0(col).End();
					} else {
						float *v = &tile->detailVerts[(pd->vertBase + t[l] - p->vertCount) * 3];
						vtx.Begin(vtx_count++).SetPos(Vec3(v[0], v[1], v[2])).SetColor0(col).End();
					}
				}
			}
		}
	}

	DrawTriangles(view_id, vtx, program, values, textures, state);
}

//
void AddGeometryToNavMeshInput(NavMeshInput &input, const Geometry &geo, const Mat4 &world) {
	const auto start_index = input.vtx.size();

	input.vtx.reserve(input.vtx.size() + geo.vtx.size());
	for (auto &vtx : geo.vtx)
		input.vtx.push_back(world * vtx);

	//
	const auto tri_count = ComputeTriangleCount(geo);
	input.idx.reserve(input.idx.size() + tri_count * 3);

	const auto pol_idx = ComputePolygonIndex(geo);

	for (size_t i = 0; i < geo.pol.size(); ++i) {
		const auto &pol = geo.pol[i];
		const auto j = pol_idx[i];

		for (size_t v = 1; v < pol.vtx_count - 1; ++v) {
			input.idx.push_back(numeric_cast<int>(geo.binding[j] + start_index));
			input.idx.push_back(numeric_cast<int>(geo.binding[j + v] + start_index));
			input.idx.push_back(numeric_cast<int>(geo.binding[j + v + 1] + start_index));
		}
	}
}

//
dtNavMesh *CreateNavMesh(const NavMeshInput &input, float radius, float height, float slope, float climb) {
	MinMax minmax = {Vec3::Max, Vec3::Min};

	for (const auto &vtx : input.vtx) {
		minmax.mn = Min(minmax.mn, vtx);
		minmax.mx = Max(minmax.mx, vtx);
	}

	const float *bmin = reinterpret_cast<float *>(&minmax.mn);
	const float *bmax = reinterpret_cast<float *>(&minmax.mx);

	const float *verts = reinterpret_cast<const float *>(input.vtx.data());
	const int nverts = numeric_cast<int>(input.vtx.size());

	const int *tris = input.idx.data();
	const int ntris = numeric_cast<int>(input.idx.size()) / 3;

	//
	// Step 1. Initialize build config.
	//

	// Init build configuration from GUI

	enum SamplePartitionType {
		SAMPLE_PARTITION_WATERSHED,
		SAMPLE_PARTITION_MONOTONE,
		SAMPLE_PARTITION_LAYERS,
	};

	float cellSize = 0.3f;
	float cellHeight = 0.2f;
	float agentHeight = height;
	float agentRadius = radius;
	float agentMaxClimb = climb;
	float agentMaxSlope = slope;
	float regionMinSize = 8;
	float regionMergeSize = 20;
	float edgeMaxLen = 12.0f;
	float edgeMaxError = 1.3f;
	float vertsPerPoly = 6.0f;
	float detailSampleDist = 6.0f;
	float detailSampleMaxError = 1.0f;
	int partitionType = SAMPLE_PARTITION_WATERSHED;

	rcConfig cfg;
	memset(&cfg, 0, sizeof(cfg));
	cfg.cs = cellSize;
	cfg.ch = cellHeight;
	cfg.walkableSlopeAngle = agentMaxSlope;
	cfg.walkableHeight = (int)ceilf(agentHeight / cfg.ch);
	cfg.walkableClimb = (int)floorf(agentMaxClimb / cfg.ch);
	cfg.walkableRadius = (int)ceilf(agentRadius / cfg.cs);
	cfg.maxEdgeLen = (int)(edgeMaxLen / cellSize);
	cfg.maxSimplificationError = edgeMaxError;
	cfg.minRegionArea = (int)rcSqr(regionMinSize); // Note: area = size*size
	cfg.mergeRegionArea = (int)rcSqr(regionMergeSize); // Note: area = size*size
	cfg.maxVertsPerPoly = (int)vertsPerPoly;
	cfg.detailSampleDist = detailSampleDist < 0.9f ? 0 : cellSize * detailSampleDist;
	cfg.detailSampleMaxError = cellHeight * detailSampleMaxError;

	rcContext ctx;

	// Set the area where the navigation will be build.
	// Here the bounds of the input mesh are used, but the
	// area could be specified by an user defined box, etc.
	rcVcopy(cfg.bmin, bmin);
	rcVcopy(cfg.bmax, bmax);
	rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

	//
	// Step 2. Rasterize input polygon soup.
	//

	// Allocate voxel height field where we rasterize our input data to.
	std::unique_ptr<rcHeightfield> solid(rcAllocHeightfield());

	if (!rcCreateHeightfield(&ctx, *solid, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch)) {
		warn("Navigation: Could not create solid height field.");
		return nullptr;
	}

	// Allocate array that can hold triangle area types.
	// If you have multiple meshes you need to process, allocate
	// and array which can hold the max number of triangles you need to process.
	std::vector<unsigned char> triareas(ntris);

	// Find triangles which are walkable based on their slope and rasterize them.
	// If your input data is multiple meshes, you can transform them here, calculate
	// the are type for each of the meshes and rasterize them.
	memset(triareas.data(), 0, ntris * sizeof(unsigned char));
	rcMarkWalkableTriangles(&ctx, cfg.walkableSlopeAngle, verts, nverts, tris, ntris, triareas.data());
	rcRasterizeTriangles(&ctx, verts, nverts, tris, triareas.data(), ntris, *solid, cfg.walkableClimb);

	triareas.clear();

	//
	// Step 3. Filter walkable surfaces.
	//

	// Once all geometry is rasterized, we do initial pass of filtering to
	// remove unwanted overhangs caused by the conservative rasterization
	// as well as filter spans where the character cannot possibly stand.
	rcFilterLowHangingWalkableObstacles(&ctx, cfg.walkableClimb, *solid);
	rcFilterLedgeSpans(&ctx, cfg.walkableHeight, cfg.walkableClimb, *solid);
	rcFilterWalkableLowHeightSpans(&ctx, cfg.walkableHeight, *solid);

	//
	// Step 4. Partition walkable surface to simple regions.
	//

	// Compact the height field so that it is faster to handle from now on.
	// This will result more cache coherent data as well as the neighbors
	// between walkable cells will be calculated.
	std::unique_ptr<rcCompactHeightfield> chf(rcAllocCompactHeightfield());

	if (!rcBuildCompactHeightfield(&ctx, cfg.walkableHeight, cfg.walkableClimb, *solid, *chf)) {
		warn("buildNavigation: Could not build compact data.");
		return nullptr;
	}

	solid = nullptr;

	// Erode the walkable area by agent radius.
	if (!rcErodeWalkableArea(&ctx, cfg.walkableRadius, *chf)) {
		warn("buildNavigation: Could not erode.");
		return nullptr;
	}

	// Partition the height field so that we can use simple algorithm later to triangulate the walkable areas.
	// There are 3 partitioning methods, each with some pros and cons:
	// 1) Watershed partitioning
	//   - the classic Recast partitioning
	//   - creates the nicest tessellation
	//   - usually slowest
	//   - partitions the height field into nice regions without holes or overlaps
	//   - the are some corner cases where this method creates produces holes and overlaps
	//      - holes may appear when a small obstacles is close to large open area (triangulation can handle this)
	//      - overlaps may occur if you have narrow spiral corridors (i.e stairs), this make triangulation to fail
	//   * generally the best choice if you precompute the navmesh, use this if you have large open areas
	// 2) Monotone partitioning
	//   - fastest
	//   - partitions the height field into regions without holes and overlaps (guaranteed)
	//   - creates long thin polygons, which sometimes causes paths with detours
	//   * use this if you want fast navmesh generation
	// 3) Layer partitioning
	//   - quite fast
	//   - partitions the height field into non-overlapping regions
	//   - relies on the triangulation code to cope with holes (thus slower than monotone partitioning)
	//   - produces better triangles than monotone partitioning
	//   - does not have the corner cases of watershed partitioning
	//   - can be slow and create a bit ugly tessellation (still better than monotone)
	//     if you have large open areas with small obstacles (not a problem if you use tiles)
	//   * good choice to use for tiled navmesh with medium and small sized tiles

	if (partitionType == SAMPLE_PARTITION_WATERSHED) {
		// Prepare for region partitioning, by calculating distance field along the walkable surface.
		if (!rcBuildDistanceField(&ctx, *chf)) {
			warn("buildNavigation: Could not build distance field.");
			return nullptr;
		}

		// Partition the walkable surface into simple regions without holes.
		if (!rcBuildRegions(&ctx, *chf, 0, cfg.minRegionArea, cfg.mergeRegionArea)) {
			warn("buildNavigation: Could not build watershed regions.");
			return nullptr;
		}
	} else if (partitionType == SAMPLE_PARTITION_MONOTONE) {
		// Partition the walkable surface into simple regions without holes.
		// Monotone partitioning does not need distance field.
		if (!rcBuildRegionsMonotone(&ctx, *chf, 0, cfg.minRegionArea, cfg.mergeRegionArea)) {
			warn("buildNavigation: Could not build monotone regions.");
			return nullptr;
		}
	} else { // SAMPLE_PARTITION_LAYERS
			 // Partition the walkable surface into simple regions without holes.
		if (!rcBuildLayerRegions(&ctx, *chf, 0, cfg.minRegionArea)) {
			warn("buildNavigation: Could not build layer regions.");
			return nullptr;
		}
	}

	//
	// Step 5. Trace and simplify region contours.
	//

	// Create contours.
	std::unique_ptr<rcContourSet> cset(rcAllocContourSet());

	if (!rcBuildContours(&ctx, *chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *cset)) {
		warn("buildNavigation: Could not create contours.");
		return nullptr;
	}

	//
	// Step 6. Build polygons mesh from contours.
	//

	// Build polygon navmesh from the contours.
	std::unique_ptr<rcPolyMesh> pmesh(rcAllocPolyMesh());

	if (!rcBuildPolyMesh(&ctx, *cset, cfg.maxVertsPerPoly, *pmesh)) {
		warn("buildNavigation: Could not triangulate contours.");
		return nullptr;
	}

	//
	// Step 7. Create detail mesh which allows to access approximate height on each polygon.
	//

	std::unique_ptr<rcPolyMeshDetail> dmesh(rcAllocPolyMeshDetail());

	if (!rcBuildPolyMeshDetail(&ctx, *pmesh, *chf, cfg.detailSampleDist, cfg.detailSampleMaxError, *dmesh)) {
		warn("buildNavigation: Could not build detail mesh.");
		return nullptr;
	}

	rcFreeCompactHeightfield(chf.release());
	rcFreeContourSet(cset.release());

	// At this point the navigation mesh data is ready, you can access it from pmesh.
	// See duDebugDrawPolyMesh or dtCreateNavMeshData as examples how to access the data.

	//
	// (Optional) Step 8. Create Detour data from Recast poly mesh.
	//

	// The GUI may allow more max points per polygon than Detour can handle.
	// Only build the detour navmesh if we do not exceed the limit.
	dtNavMesh *navMesh = nullptr;

	if (cfg.maxVertsPerPoly <= DT_VERTS_PER_POLYGON) {
		unsigned char *navData = 0;
		int navDataSize = 0;

		// Update poly flags from areas.
		for (int i = 0; i < pmesh->npolys; ++i) {
			if (pmesh->areas[i] == RC_WALKABLE_AREA)
				pmesh->flags[i] = 1;
			/*
			if (pmesh->areas[i] == RC_WALKABLE_AREA)
			pmesh->areas[i] = SAMPLE_POLYAREA_GROUND;

			if (pmesh->areas[i] == SAMPLE_POLYAREA_GROUND || pmesh->areas[i] == SAMPLE_POLYAREA_GRASS || pmesh->areas[i] == SAMPLE_POLYAREA_ROAD) {
			pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK;
			} else if (pmesh->areas[i] == SAMPLE_POLYAREA_WATER) {
			pmesh->flags[i] = SAMPLE_POLYFLAGS_SWIM;
			} else if (pmesh->areas[i] == SAMPLE_POLYAREA_DOOR) {
			pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK | SAMPLE_POLYFLAGS_DOOR;
			}
			*/
		}

		dtNavMeshCreateParams params;
		memset(&params, 0, sizeof(params));
		params.verts = pmesh->verts;
		params.vertCount = pmesh->nverts;
		params.polys = pmesh->polys;
		params.polyAreas = pmesh->areas;
		params.polyFlags = pmesh->flags;
		params.polyCount = pmesh->npolys;
		params.nvp = pmesh->nvp;
		params.detailMeshes = dmesh->meshes;
		params.detailVerts = dmesh->verts;
		params.detailVertsCount = dmesh->nverts;
		params.detailTris = dmesh->tris;
		params.detailTriCount = dmesh->ntris;
		/*
		params.offMeshConVerts = geom->getOffMeshConnectionVerts();
		params.offMeshConRad = geom->getOffMeshConnectionRads();
		params.offMeshConDir = geom->getOffMeshConnectionDirs();
		params.offMeshConAreas = geom->getOffMeshConnectionAreas();
		params.offMeshConFlags = geom->getOffMeshConnectionFlags();
		params.offMeshConUserID = geom->getOffMeshConnectionId();
		params.offMeshConCount = geom->getOffMeshConnectionCount();
		*/
		params.walkableHeight = agentHeight;
		params.walkableRadius = agentRadius;
		params.walkableClimb = agentMaxClimb;
		rcVcopy(params.bmin, pmesh->bmin);
		rcVcopy(params.bmax, pmesh->bmax);
		params.cs = cfg.cs;
		params.ch = cfg.ch;
		params.buildBvTree = true;

		if (!dtCreateNavMeshData(&params, &navData, &navDataSize)) {
			warn("Could not build Detour navmesh.");
			return nullptr;
		}

		navMesh = dtAllocNavMesh();
		if (!navMesh) {
			dtFree(navData);
			warn("Could not create Detour navmesh");
			return nullptr;
		}

		dtStatus status;

		status = navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
		if (dtStatusFailed(status)) {
			dtFree(navData);
			warn("Could not init Detour navmesh");
			return nullptr;
		}

		// free pmesh dmesh
		rcFreePolyMesh(pmesh.release());
		rcFreePolyMeshDetail(dmesh.release());
	}

	return navMesh;
}

} // namespace hg

#else

namespace hg {

dtNavMesh *LoadNavMesh(const char *path, const Reader &ir, const ReadProvider &ip) { return nullptr; }
dtNavMesh *LoadNavMeshFromFile(const char *path) { return nullptr; }
dtNavMesh *LoadNavMeshFromAssets(const char *path) { return nullptr; }

void DestroyNavMesh(dtNavMesh *mesh) {}

//
void DrawNavMesh(const dtNavMesh *mesh, bgfx::ViewId view_id, const bgfx::VertexLayout &vtx_layout, bgfx::ProgramHandle program, RenderState state) {}

//
void AddGeometryToNavMeshInput(NavMeshInput &input, const Geometry &geo, const Mat4 &world) {}
dtNavMesh *CreateNavMesh(const NavMeshInput &input, float radius, float height, float slope, float climb) { return nullptr; }

//
dtNavMeshQuery *CreateNavMeshQuery(const dtNavMesh *mesh) { return nullptr; }
void DestroyNavMeshQuery(dtNavMeshQuery *query) {}

//
std::vector<Vec3> FindNavigationPathTo(const dtNavMeshQuery *query, const Vec3 &from, const Vec3 &to) { return {}; }

} // namespace hg

#endif
