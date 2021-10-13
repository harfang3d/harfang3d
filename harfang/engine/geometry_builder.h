// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/geometry.h"
#include "engine/vertex.h"

#include "foundation/color.h"
#include "foundation/vector2.h"

#include <vector>

namespace hg {

struct GeometryBuilder {
	struct Polygon {
		std::vector<uint32_t> vidx;
		uint16_t material;
	};

	void AddVertex(const Vertex &vtx);
	void AddPolygon(const std::vector<uint32_t> &idxs, uint16_t material);
	void AddTriangle(uint32_t a, uint32_t b, uint32_t c, uint32_t material);
	void AddQuad(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t material);

	Geometry Make();
	void Clear();

private:
	std::vector<Vertex> vertices;
	std::vector<Polygon> polygons;
};

} // namespace hg
