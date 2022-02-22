// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/model_builder.h"
#include "engine/render_pipeline.h"
#include "engine/vertex.h"

#include "foundation/color.h"
#include "foundation/matrix4.h"
#include "foundation/minmax.h"
#include "foundation/rw_interface.h"
#include "foundation/unit.h"
#include "foundation/vector2.h"

#include <array>
#include <vector>

namespace hg {

uint8_t GetModelBinaryFormatVersion();

// Platform agnostic geometry
struct Geometry {
	struct Polygon { // 2B
		uint8_t vtx_count{0};
		uint8_t material{0};
	};

	std::vector<Vec3> vtx;
	std::vector<Polygon> pol;
	std::vector<uint32_t> binding;

	//
	std::vector<Vec3> normal; // per-polygon-vertex
	std::vector<Color> color; // per-polygon-vertex

	struct TangentFrame { // 24B
		Vec3 T, B;
	};

	std::vector<TangentFrame> tangent; // per-polygon-vertex

	using UVSet = std::vector<Vec2>; // 24B

	std::array<UVSet, 8> uv; // per-polygon-vertex

	//
	struct Skin { // 12B
		uint16_t index[4];
		uint8_t weight[4];
	};

	std::vector<Skin> skin; // per-polygon-vertex
	std::vector<Mat4> bind_pose; // per-bone
};

//
size_t ComputeBindingCount(const Geometry &geo);
size_t ComputeTriangleCount(const Geometry &geo);
std::vector<uint32_t> ComputePolygonIndex(const Geometry &geo);

uint8_t GetMaterialCount(const Geometry &geo);

//
struct VertexToPolygon {
	uint16_t pol_count;
	std::vector<uint32_t> pol_index; // polygon index in the geometry
	std::vector<uint8_t> vtx_index; // vertex index in the polygon
};

std::vector<VertexToPolygon> ComputeVertexToPolygon(const Geometry &geo);

struct VertexToVertex {
	uint16_t vtx_count;

	struct PolygonVertex {
		uint32_t pol_index;
		uint32_t vtx_index;
	};

	std::vector<PolygonVertex> vtx;
};

std::vector<VertexToVertex> ComputeVertexToVertex(const Geometry &geo, const std::vector<VertexToPolygon> &vtx_to_polygon);

//
std::vector<Vec3> ComputePolygonNormal(const Geometry &geo);

std::vector<Vec3> ComputeVertexNormal(const Geometry &geo, const std::vector<VertexToPolygon> &vtx_to_pol, float max_smoothing_angle = Deg(60.f));
std::vector<Geometry::TangentFrame> ComputeVertexTangent(
	const Geometry &geo, const std::vector<Vec3> &vtx_normal, uint32_t uv_index = 0, float max_smoothing_angle = Deg(60.f));

void ReverseTangentFrame(Geometry &geo, bool T, bool B);

//
bool Validate(const Geometry &geo);

//
void SmoothVertexColor(Geometry &geo, const std::vector<uint32_t> &pol_index, const std::vector<VertexToVertex> &vtx_to_vtx);

//
Model GeometryToModel(const Geometry &geo, ModelOptimisationLevel optimisation_level = MOL_None);

//
Geometry LoadGeometry(const Reader &ir, const Handle &h, const char *name);
Geometry LoadGeometryFromFile(const char *path);

bool SaveGeometry(const Writer &iw, const Handle &h, const Geometry &geo);
bool SaveGeometryToFile(const char *path, const Geometry &geo);

// Convert a geometry to a model and save it to file.
bool SaveGeometryModelToFile(const char *path, const Geometry &geo, ModelOptimisationLevel optimisation_level = MOL_None);

} // namespace hg
