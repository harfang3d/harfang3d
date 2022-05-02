// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/render_pipeline.h"
#include "engine/vertex.h"
#include "foundation/vector2.h"

#include <array>
#include <map>
#include <vector>

namespace hg {

using VtxIdxType = uint32_t;

enum ModelOptimisationLevel {
	MOL_None, // fastest
	MOL_Minimal, // improve index cache hit
	MOL_Full // slowest, improve index and vertex cache hit, reduce overdraw
};

struct ModelBuilder {
	ModelBuilder();

	VtxIdxType AddVertex(const Vertex &v);

	void AddTriangle(VtxIdxType a, VtxIdxType b, VtxIdxType c);
	void AddQuad(VtxIdxType a, VtxIdxType b, VtxIdxType c, VtxIdxType d);
	void AddPolygon(const std::vector<VtxIdxType> &idxs);
	void AddBoneIdx(uint16_t AddBoneIdx);

	size_t GetCurrentListIndexCount() const;

	/// End current list. Return false if list is empty, meaning no new list has been created.
	bool EndList(uint16_t material);

	void Clear();

	using end_list_cb = void (*)(const bgfx::VertexLayout &decl, const MinMax &minmax, const std::vector<VtxIdxType> &idx_data,
		const std::vector<uint8_t> &vtx_data, const std::vector<uint16_t> &bones_table, uint16_t mat, void *userdata);

	void Make(const bgfx::VertexLayout &decl, end_list_cb on_end_list, void *userdata, ModelOptimisationLevel optimisation_level = MOL_None,
		bool verbose = false) const;

	Model MakeModel(const bgfx::VertexLayout &decl, ModelOptimisationLevel optimisation_level = MOL_None, bool verbose = false) const;

private:
	size_t hash_collision{};

	struct List {
		std::vector<VtxIdxType> idx;
		std::vector<Vertex> vtx;
		std::vector<uint16_t> bones_table;

		std::map<uint64_t, VtxIdxType> vtx_lookup;
		uint16_t mat;

		MinMax minmax{Vec3::Max, Vec3::Min};
	};

	std::vector<List> lists;

	void NewList();
};

} // namespace hg
