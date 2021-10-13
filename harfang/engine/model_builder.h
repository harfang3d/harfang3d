// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/render_pipeline.h"
#include "engine/vertex.h"
#include "foundation/vector2.h"

#include <array>
#include <map>
#include <vector>

namespace hg {

enum ModelOptimisationLevel {
	MOL_None, // fastest
	MOL_Minimal, // improve index cache hit
	MOL_Full // slowest, improve index and vertex cache hit, reduce overdraw
};

struct ModelBuilder {
	ModelBuilder();

	uint16_t AddVertex(const Vertex &v);

	void AddTriangle(uint16_t a, uint16_t b, uint16_t c);
	void AddQuad(uint16_t a, uint16_t b, uint16_t c, uint16_t d);
	void AddPolygon(const std::vector<uint16_t> &idxs);
	void AddBoneIdx(uint16_t AddBoneIdx);

	size_t GetCurrentListIndexCount() const;

	/// End current list. Return false if list is empty, meaning no new list has been created.
	bool EndList(uint16_t material);

	void Clear();

	using end_list_cb = void (*)(const bgfx::VertexLayout &decl, const MinMax &minmax, const std::vector<uint32_t> &idx_data,
		const std::vector<uint8_t> &vtx_data, const std::vector<uint16_t> &bones_table, uint16_t mat, void *userdata);

	void Make(const bgfx::VertexLayout &decl, end_list_cb on_end_list, void *userdata, ModelOptimisationLevel optimisation_level = MOL_None,
		bool verbose = false) const;

	Model MakeModel(const bgfx::VertexLayout &decl, ModelOptimisationLevel optimisation_level = MOL_None, bool verbose = false) const;

private:
	uint32_t hash_collision{};

	struct List {
		std::vector<uint32_t> idx;
		std::vector<Vertex> vtx;
		std::vector<uint16_t> bones_table;

		std::map<uint64_t, uint16_t> vtx_lookup;
		uint16_t mat;

		MinMax minmax{Vec3::Max, Vec3::Min};
	};

	std::vector<List> lists;

	void NewList();
};

} // namespace hg
