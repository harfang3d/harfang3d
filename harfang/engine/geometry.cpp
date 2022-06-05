// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/geometry.h"
#include "engine/file_format.h"

#include "foundation/file.h"
#include "foundation/file_rw_interface.h"
#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/math.h"
#include "foundation/pack_float.h"
#include "foundation/time.h"

#include "mikktspace.h"

#include <bgfx/bgfx.h>

#include <numeric>

namespace hg {

uint8_t GetModelBinaryFormatVersion() { return 2; }

size_t ComputeBindingCount(const Geometry &geo) {
	return std::accumulate(std::begin(geo.pol), std::end(geo.pol), 0, [](int v, const Geometry::Polygon &pol) { return v + pol.vtx_count; });
}

size_t ComputeTriangleCount(const Geometry &geo) {
	return std::accumulate(
		std::begin(geo.pol), std::end(geo.pol), 0, [](int v, const Geometry::Polygon &pol) { return v + (pol.vtx_count > 2 ? pol.vtx_count - 2 : 0); });
}

std::vector<uint32_t> ComputePolygonIndex(const Geometry &geo) {
	std::vector<uint32_t> out;
	out.reserve(geo.pol.size());

	uint32_t idx = 0;
	for (auto &pol : geo.pol) {
		out.push_back(idx);
		idx += pol.vtx_count;
	}
	return out;
}

std::vector<VertexToPolygon> ComputeVertexToPolygon(const Geometry &geo) {
	std::vector<VertexToPolygon> vtx_to_pol(geo.vtx.size());

	size_t tt = 0;
	for (auto &p : geo.pol) {
		for (auto j = 0; j < p.vtx_count; ++j)
			++vtx_to_pol[geo.binding[tt + j]].pol_count;
		tt += p.vtx_count;
	}

	for (auto j = 0; j < geo.vtx.size(); ++j) {
		vtx_to_pol[j].pol_index.resize(vtx_to_pol[j].pol_count);
		vtx_to_pol[j].vtx_index.resize(vtx_to_pol[j].pol_count);
		vtx_to_pol[j].pol_count = 0;
	}

	tt = 0;
	for (auto i = 0; i < geo.pol.size(); ++i) {
		for (auto j = 0; j < geo.pol[i].vtx_count; ++j) {
			const auto v = geo.binding[tt + j];
			vtx_to_pol[v].pol_index[vtx_to_pol[v].pol_count] = numeric_cast<uint32_t>(i);
			vtx_to_pol[v].vtx_index[vtx_to_pol[v].pol_count] = j;
			++vtx_to_pol[v].pol_count;
		}
		tt += geo.pol[i].vtx_count;
	}

	return vtx_to_pol;
}

std::vector<VertexToVertex> ComputeVertexToVertex(const Geometry &geo, const std::vector<VertexToPolygon> &vtx_to_pol) {
	std::vector<VertexToVertex> vtx_to_vtx(geo.vtx.size());

	static const int __VertexToVertexTempListSize = 1024;
	std::array<VertexToVertex::PolygonVertex, __VertexToVertexTempListSize> tmp_vtx_to_vtx;

	for (auto pass = 0; pass < 2; ++pass)
		for (auto v = 0; v < geo.vtx.size(); ++v) {
			vtx_to_vtx[v].vtx_count = 0;

			size_t vtx_vtx_count = 0, tt = 0;
			for (auto p = 0; p < vtx_to_pol[v].pol_count; ++p) {
				const uint32_t pol_index = vtx_to_pol[v].pol_index[p];

				const auto &pol = geo.pol[pol_index];

				int ci = 0;
				for (; ci < pol.vtx_count; ++ci)
					if (geo.binding[tt + ci] == v)
						break;

				for (auto _c = ci - 1; _c <= ci + 1; _c += 2) {
					int vtx_index = _c;
					if (vtx_index < 0)
						vtx_index += pol.vtx_count;
					if (vtx_index >= pol.vtx_count)
						vtx_index -= pol.vtx_count;

					// invalidate already registered candidate
					bool insert = true;
					for (auto nl = 0; nl < vtx_vtx_count; ++nl)
						if (tmp_vtx_to_vtx[nl].pol_index == pol_index && tmp_vtx_to_vtx[nl].vtx_index == uint32_t(vtx_index)) {
							insert = false;
							break;
						}

					if (insert) {
						if (vtx_vtx_count == __VertexToVertexTempListSize) {
							warn("Temporary list exceeded, vertex to vertex LUT corrupted");
							vtx_vtx_count = __VertexToVertexTempListSize - 1;
						}

						tmp_vtx_to_vtx[vtx_vtx_count].pol_index = pol_index;
						tmp_vtx_to_vtx[vtx_vtx_count].vtx_index = vtx_index;

						if (pass == 1) {
							vtx_to_vtx[v].vtx[vtx_vtx_count].pol_index = pol_index;
							vtx_to_vtx[v].vtx[vtx_vtx_count].vtx_index = vtx_index;
						}

						++vtx_vtx_count;
					}
				}

				tt += pol.vtx_count;
			}

			// allocate vertex container for this vertex
			if (pass == 0)
				vtx_to_vtx[v].vtx.reserve(vtx_vtx_count);
		}

	return vtx_to_vtx;
}

std::vector<Vec3> ComputePolygonNormal(const Geometry &geo) {
	std::vector<Vec3> out(geo.pol.size());

	size_t tt = 0;
	for (auto c = 0; c < geo.pol.size(); ++c) {
		const auto &pol = geo.pol[c];

		if (pol.vtx_count > 2) {
			const auto va = geo.vtx[geo.binding[tt + 2]] - geo.vtx[geo.binding[tt + 0]], vb = geo.vtx[geo.binding[tt + 1]] - geo.vtx[geo.binding[tt + 0]];
			out[c] = Normalize(Cross(vb, va));
		} else {
			out[c] = {0, 0, 0};
		}

		tt += pol.vtx_count;
	}

	return out;
}

std::vector<Vec3> ComputeVertexNormal(const Geometry &geo, const std::vector<VertexToPolygon> &vtx_to_pol, float msa) {
	const auto pol_normal = ComputePolygonNormal(geo);

	std::vector<Vec3> out(geo.binding.size());

	msa = Cos(msa);

	size_t tt = 0;
	for (auto cp = 0, ttp = 0; cp < geo.pol.size(); ++cp) {
		const auto &pol = geo.pol[cp];

		for (auto cv = 0; cv < pol.vtx_count; ++cv) {
			const auto gv = geo.binding[tt + cv];

			auto normal = pol_normal[cp];
			for (auto cg = 0; cg < vtx_to_pol[gv].pol_count; ++cg) {
				const auto cc = vtx_to_pol[gv].pol_index[cg];

				if (cc != cp)
					if (Dot(pol_normal[cp], pol_normal[cc]) >= msa)
						normal += pol_normal[cc];
			}
			out[ttp++] = Normalize(normal);
		}

		tt += pol.vtx_count;
	}

	return out;
}

//
struct SMikkTSpaceContextData {
	const Geometry &geo;
	const std::vector<Vec3> &nrm;

	std::vector<uint32_t> pol_idx;
	std::vector<Geometry::TangentFrame> &out;

	uint32_t uv_index{0};
};

static int MikkT_getNumFace(const SMikkTSpaceContext *pContext) {
	const auto data = reinterpret_cast<SMikkTSpaceContextData *>(pContext->m_pUserData);
	return int(data->geo.pol.size());
}

static int MikkT_getNumVerticesOfFace(const SMikkTSpaceContext *pContext, const int iFace) {
	const auto data = reinterpret_cast<SMikkTSpaceContextData *>(pContext->m_pUserData);
	return data->geo.pol[iFace].vtx_count;
}

static void MikkT_getPosition(const SMikkTSpaceContext *pContext, float fvPosOut[], const int iFace, const int iVert) {
	const auto data = reinterpret_cast<SMikkTSpaceContextData *>(pContext->m_pUserData);
	const auto &T = data->geo.vtx[data->geo.binding[data->pol_idx[iFace] + iVert]];

	fvPosOut[0] = T.x;
	fvPosOut[1] = T.y;
	fvPosOut[2] = T.z;
}

static void MikkT_getNormal(const SMikkTSpaceContext *pContext, float fvNormOut[], const int iFace, const int iVert) {
	const auto data = reinterpret_cast<SMikkTSpaceContextData *>(pContext->m_pUserData);
	const auto &N = data->nrm[data->pol_idx[iFace] + iVert];

	fvNormOut[0] = N.x;
	fvNormOut[1] = N.y;
	fvNormOut[2] = N.z;
}

static void MikkT_getTexCoord(const SMikkTSpaceContext *pContext, float fvTexcOut[], const int iFace, const int iVert) {
	const auto data = reinterpret_cast<SMikkTSpaceContextData *>(pContext->m_pUserData);

	if (data->uv_index < data->geo.uv.size()) {
		const auto &UV = data->geo.uv[data->uv_index][data->pol_idx[iFace] + iVert];
		fvTexcOut[0] = UV.x;
		fvTexcOut[1] = UV.y;
	} else {
		fvTexcOut[0] = 0.f;
		fvTexcOut[1] = 0.f;
	}
}

static void MikkT_setTSpaceBasic(const SMikkTSpaceContext *pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert) {
	const auto data = reinterpret_cast<SMikkTSpaceContextData *>(pContext->m_pUserData);
	const auto i = data->pol_idx[iFace] + iVert;

	auto &F = data->out[i];

	F.T.x = fvTangent[0];
	F.T.y = fvTangent[1];
	F.T.z = fvTangent[2];

	F.B = Cross(data->nrm[i], F.T) * fSign;
}

std::vector<Geometry::TangentFrame> ComputeVertexTangent(const Geometry &geo, const std::vector<Vec3> &vtx_normal, uint32_t uv_index, float msa) {
	std::vector<Geometry::TangentFrame> out(geo.binding.size());

	SMikkTSpaceInterface itf = {
		MikkT_getNumFace, MikkT_getNumVerticesOfFace, MikkT_getPosition, MikkT_getNormal, MikkT_getTexCoord, MikkT_setTSpaceBasic, nullptr};

	SMikkTSpaceContextData data = {geo, vtx_normal, ComputePolygonIndex(geo), out, uv_index};
	SMikkTSpaceContext ctx = {&itf, &data};

	genTangSpace(&ctx, RadianToDegree(msa));

	return out;
}

//
void ReverseTangentFrame(Geometry &geo, bool T, bool B) {
	if (T)
		for (auto &v : geo.tangent)
			v.T = Reverse(v.T);

	if (B)
		for (auto &v : geo.tangent)
			v.B = Reverse(v.B);
}

//
void SmoothVertexColor(Geometry &geo, const std::vector<uint32_t> &pol_index, const std::vector<VertexToVertex> &vtx_to_vtx) {
	std::vector<Color> out(geo.color.size());

	size_t tt = 0;
	for (auto np = 0; np < geo.pol.size(); ++np) {
		const auto &pol = geo.pol[np];

		for (auto nv = 0; nv < pol.vtx_count; ++nv) {
			const size_t imv = geo.binding[tt + nv], iv = tt + nv;

			out[iv] = geo.color[iv] * 4.f;

			float nrgb = 4.f;
			for (auto nvv = 0; nvv < vtx_to_vtx[imv].vtx_count; ++nvv)
				if (geo.pol[vtx_to_vtx[imv].vtx[nvv].pol_index].material == geo.pol[np].material) {
					out[iv] += geo.color[pol_index[vtx_to_vtx[imv].vtx[nvv].pol_index] + vtx_to_vtx[imv].vtx[nvv].vtx_index];
					nrgb += 1.f;
				}

			out[iv] /= float(nrgb);
		}

		tt += pol.vtx_count;
	}

	geo.color = std::move(out);
}

//
bool Validate(const Geometry &geo) {
	const size_t binding_count = ComputeBindingCount(geo);

	int error_count = 0;

	const auto validation_error = [&](const char *msg) {
		warn(msg);

		if (++error_count == 32) {
			warn("Too many errors in geometry, aborting validation");
			return false;
		}

		return true;
	};

	if (geo.binding.size() != binding_count)
		if (!validation_error("Invalid polygon vertex index count"))
			return false;

	if (!geo.color.empty() && (geo.color.size() != binding_count))
		if (!validation_error("Invalid vertex color count"))
			return false;

	if (!geo.normal.empty() && (geo.normal.size() != binding_count))
		if (!validation_error("Invalid vertex normal count"))
			return false;

	if (!geo.tangent.empty() && (geo.tangent.size() != binding_count))
		if (!validation_error("Invalid tangent frame count"))
			return false;

	for (auto &uv : geo.uv)
		if (!uv.empty() && (uv.size() != binding_count))
			if (!validation_error("Invalid UV count"))
				return false;

	for (auto &vtx_idx : geo.binding)
		if (vtx_idx >= geo.vtx.size())
			if (!validation_error("Invalid reference to non-existing vertex"))
				return false;

	const auto bone_count = geo.bind_pose.size();

	for (const auto &s : geo.skin)
		for (auto i : s.index)
			if (i >= bone_count)
				if (!validation_error("Invalid reference to non-existing bone"))
					return false;

	return error_count == 0;
}

//
template <typename T> bool ReadStdVector(const Reader &ir, const Handle &h, std::vector<T> &v) {
	const auto size = Read<uint32_t>(ir, h);
	v.resize(size);
	return ir.read(h, v.data(), size * sizeof(T)) == size * sizeof(T);
}

Geometry LoadGeometry(const Reader &ir, const Handle &h, const char *name) {
	Geometry geo;

	if (!ir.is_valid(h)) {
		warn(format("Cannot load model '%1', invalid file handle").arg(name));
		return geo;
	}

	if (Read<uint32_t>(ir, h) != HarfangMagic) {
		warn(format("Cannot load model '%1', invalid magic marker").arg(name));
		return geo;
	}

	if (Read<uint8_t>(ir, h) != ModelMarker) {
		warn(format("Cannot load model '%1', invalid model marker").arg(name));
		return geo;
	}

	/*
		version 0: initial
		version 1: add skin weights & bind pose
		version 2: added arbitrary number of bones
	*/
	const auto version = Read<uint32_t>(ir, h);
	if (version > 2) {
		warn(format("Cannot load model '%1', unsupported version").arg(name));
		return geo;
	}

	ReadStdVector(ir, h, geo.vtx);
	ReadStdVector(ir, h, geo.pol);
	ReadStdVector(ir, h, geo.binding);

	ReadStdVector(ir, h, geo.normal);
	ReadStdVector(ir, h, geo.color);

	ReadStdVector(ir, h, geo.tangent);

	for (auto &uv : geo.uv)
		ReadStdVector(ir, h, uv);

	if (version > 0) {
		if (version == 1) {

			// backward compat
			struct Skin_v1 { // 8B
				uint8_t index[4];
				uint8_t weight[4];
			};

			std::vector<Skin_v1> skin1;
			ReadStdVector(ir, h, skin1);
			geo.skin.resize(skin1.size());
			for (size_t i = 0; i < skin1.size(); i++) {
				for (size_t j = 0; j < 4; j++) {
					geo.skin[i].index[j] = skin1[i].index[j];
					geo.skin[i].weight[j] = skin1[i].weight[j];
				}
			}
		} else {
			ReadStdVector(ir, h, geo.skin);
		}
		ReadStdVector(ir, h, geo.bind_pose);
	}
	return geo;
}

Geometry LoadGeometryFromFile(const char *path) { return LoadGeometry(g_file_reader, ScopedReadHandle(g_file_read_provider, path), path); }

template <typename T> void WriteStdVector(const Writer &iw, const Handle &h, const std::vector<T> &v) {
	Write(iw, h, uint32_t(v.size()));
	iw.write(h, v.data(), v.size() * sizeof(T));
}

bool SaveGeometry(const Writer &iw, const Handle &h, const Geometry &geo) {
	if (!iw.is_valid(h))
		return false;

	Write(iw, h, HarfangMagic);
	Write(iw, h, ModelMarker);

	/*
		version 0: initial
		version 1: add skin weights & bind pose
		version 2: added arbitrary number of bones
	*/
	Write<uint32_t>(iw, h, 2); // version

	WriteStdVector(iw, h, geo.vtx);
	WriteStdVector(iw, h, geo.pol);
	WriteStdVector(iw, h, geo.binding);

	WriteStdVector(iw, h, geo.normal);
	WriteStdVector(iw, h, geo.color);

	WriteStdVector(iw, h, geo.tangent);

	for (auto &uv : geo.uv)
		WriteStdVector(iw, h, uv);

	WriteStdVector(iw, h, geo.skin);
	WriteStdVector(iw, h, geo.bind_pose);
	return true;
}

bool SaveGeometryToFile(const char *path, const Geometry &geo) { return SaveGeometry(g_file_writer, ScopedWriteHandle(g_file_write_provider, path), geo); }

//
static Vertex PreparePolygonVertex(const Geometry &geo, size_t i_bind, size_t i_vtp, const std::map<uint16_t, uint16_t> &bone_map) {
	Vertex vtx = {};

	const auto i_vtx = geo.binding[i_bind + i_vtp];
	vtx.pos = geo.vtx[i_vtx];

	if (!geo.normal.empty())
		vtx.normal = geo.normal[i_bind + i_vtp];

	if (!geo.tangent.empty()) {
		vtx.tangent = geo.tangent[i_bind + i_vtp].T;
		vtx.binormal = geo.tangent[i_bind + i_vtp].B;
	}

	if (!geo.color.empty())
		vtx.color0 = geo.color[i_bind + i_vtp];

	for (auto i = 0; i < geo.uv.size(); ++i)
		if (!geo.uv[i].empty())
			*(&vtx.uv0 + i) = geo.uv[i][i_bind + i_vtp];

	if (!geo.skin.empty())
		for (int i = 0; i < 4; ++i) {
			auto bone_idx = geo.skin[i_vtx].index[i];
			auto bone_map_it = bone_map.find(bone_idx);
			__ASSERT__(bone_map_it != bone_map.end());
			vtx.index[i] = numeric_cast<uint8_t>(bone_map_it->second);
			vtx.weight[i] = unpack_float(geo.skin[i_vtx].weight[i]);
		}

	return vtx;
}

static bgfx::VertexLayout GetGeometryVertexDeclaration(const Geometry &geo) {
	bgfx::VertexLayout vs_decl;

	vs_decl.begin();
	vs_decl.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);

	if (!geo.normal.empty())
		vs_decl.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Uint8, true, true);

	if (!geo.tangent.empty()) {
		vs_decl.add(bgfx::Attrib::Tangent, 3, bgfx::AttribType::Uint8, true, true);
		vs_decl.add(bgfx::Attrib::Bitangent, 3, bgfx::AttribType::Uint8, true, true);
	}

	if (!geo.color.empty())
		vs_decl.add(bgfx::Attrib::Color0, 3, bgfx::AttribType::Uint8, true);

	for (auto i = 0; i < geo.uv.size(); ++i)
		if (!geo.uv[i].empty())
			vs_decl.add(bgfx::Attrib::Enum(bgfx::Attrib::TexCoord0 + i), 2, bgfx::AttribType::Float);

	if (!geo.skin.empty()) {
		vs_decl.add(bgfx::Attrib::Indices, 4, bgfx::AttribType::Uint8, true, false);
		vs_decl.add(bgfx::Attrib::Weight, 4, bgfx::AttribType::Uint8, true, false);
	}

	vs_decl.end();
	return vs_decl;
}

uint8_t GetMaterialCount(const Geometry &geo) {
	uint8_t count = 0;
	for (auto &p : geo.pol)
		if (p.material >= count)
			count = p.material + 1;
	return count;
}

static void GeometryToModelBuilder(const Geometry &geo, ModelBuilder &builder) {
	const time_ns t = time_now();

	const uint8_t mat_count = GetMaterialCount(geo);

	for (auto i_mat = 0; i_mat < mat_count; ++i_mat) {
		size_t i_bind = 0;

		std::map<uint16_t, uint16_t> bone_map;
		std::vector<uint16_t> bone_indices_to_add;

		for (auto &pol : geo.pol) {
			if (pol.material == i_mat) {
#if 0 // [EJ] we're seeing very large meshes coming in lately so allow 32 bit indexes, we'll introduce a force 16 bit indice flag if required
				if (builder.GetCurrentListIndexCount() + (pol.vtx_count - 2) * 3 > 65535) {
					builder.EndList(i_mat);
					bone_map.clear();
				}
#endif

				bool bone_map_ready = false;

				if (!geo.skin.empty()) {
					while (!bone_map_ready) {

						// if the bones on this polygon don't fit our max bones,
						// we'll clear this flag and start again with a new list
						bone_map_ready = true;
						bone_indices_to_add.clear();

						for (auto i_vtp = 0; i_vtp < pol.vtx_count; ++i_vtp) {
							// make sure our bone_map contains those bones

							// the bone limit has to be < 255 as long as we have a uint8_t vtx format for bone indices
							static_assert(max_skinned_model_matrix_count <= std::numeric_limits<uint8_t>::max() + 1, "max_skinned_model_matrix_count <= 256");

							const auto i_vtx = geo.binding[i_bind + i_vtp];

							bool need_new_list = false;
							for (int i = 0; i < 4; ++i) {
								const auto bone_idx = geo.skin[i_vtx].index[i];

								if (bone_map.find(bone_idx) == bone_map.end()) {
									auto redir_idx = uint16_t(bone_map.size());

									if (redir_idx < max_skinned_model_matrix_count) {
										bone_map[bone_idx] = redir_idx;
										bone_indices_to_add.push_back(bone_idx);
									} else {
										need_new_list = true; // too many bones, start a new list
										break;
									}
								}
							}

							if (need_new_list) {
								builder.EndList(i_mat);
								bone_map.clear();
								bone_map_ready = false;
								bone_indices_to_add.clear();
								break;
							}
						}

						for (auto bone_idx : bone_indices_to_add)
							builder.AddBoneIdx(bone_idx);
					}
				}

				for (auto i_vtp = 1; i_vtp < pol.vtx_count - 1; ++i_vtp) {
					const auto a = builder.AddVertex(PreparePolygonVertex(geo, i_bind, 0, bone_map));
					const auto b = builder.AddVertex(PreparePolygonVertex(geo, i_bind, i_vtp + 1, bone_map));
					const auto c = builder.AddVertex(PreparePolygonVertex(geo, i_bind, i_vtp, bone_map));
					builder.AddTriangle(a, b, c);
				}
			}

			i_bind += pol.vtx_count;
		}

		builder.EndList(i_mat);
		bone_map.clear();
	}

	log(format("Geometry to model builder took %1 ms").arg(time_to_ms(time_now() - t)));
}

//
Model GeometryToModel(const Geometry &geo, ModelOptimisationLevel optimisation_level) {
	ModelBuilder builder;
	GeometryToModelBuilder(geo, builder);

	const auto vs_decl = GetGeometryVertexDeclaration(geo);
	Model model = builder.MakeModel(vs_decl, optimisation_level);

	model.bind_pose = geo.bind_pose; // copy bind pose over to model
	return model;
}

bool SaveGeometryModelToFile(const char *path, const Geometry &geo, ModelOptimisationLevel optimisation_level) {
	ScopedFile file(OpenWrite(path));
	if (!file)
		return false;

	Write(file, HarfangMagic);
	Write(file, ModelMarker);

	/*
		version 0: initial
		version 1: add bind poses
		version 2: add indices size
	*/
	const auto version = GetModelBinaryFormatVersion();
	Write<uint8_t>(file, version); // version

	const auto decl = GetGeometryVertexDeclaration(geo);
	Write(file, decl); // write vertex declaration

	ModelBuilder builder;
	GeometryToModelBuilder(geo, builder);

	auto on_end_list = [](const bgfx::VertexLayout &, const MinMax &minmax, const std::vector<VtxIdxType> &idx32, const std::vector<uint8_t> &vtx,
						   const std::vector<uint16_t> &bones_table, uint16_t mat, void *userdata) {
		const auto &file = *reinterpret_cast<File *>(userdata);

		uint8_t idx_type_size = 2;

		for (auto idx : idx32)
			if (idx > 65535) {
				idx_type_size = 4;
				break;
			}

		Write<uint8_t>(file, idx_type_size); // version 2: indices size in bytes

		size_t idx_size;

		if (idx_type_size == 4) {
			idx_size = idx32.size() * sizeof(uint32_t);
			Write(file, uint32_t(idx_size));
			Write(file, idx32.data(), idx_size); // 32 bit index buffer
		} else {
			std::vector<uint16_t> idx16(idx32.size());
			for (size_t i = 0; i < idx32.size(); ++i)
				idx16[i] = idx32[i];

			idx_size = idx16.size() * sizeof(uint16_t);
			Write(file, uint32_t(idx_size));
			Write(file, idx16.data(), idx_size); // 16 bit index buffer
		}

		const size_t vtx_size = vtx.size();
		Write(file, uint32_t(vtx_size));
		Write(file, vtx.data(), vtx_size); // vertex buffer

		Write(file, uint32_t(bones_table.size()));
		Write(file, bones_table.data(), bones_table.size() * sizeof(bones_table[0])); // bones table

		Write(file, minmax);
		Write(file, mat);

		log(format("Index size: %1, vertex size: %2").arg(idx_size).arg(vtx_size));
	};

	builder.Make(decl, on_end_list, &file.f, optimisation_level);

	Write<uint8_t>(file, 0); // EOLists

	Write(file, numeric_cast<uint32_t>(geo.bind_pose.size())); // version 1: add bind poses
	for (auto &mtx : geo.bind_pose)
		Write(file, mtx);

	return true;
}

} // namespace hg
