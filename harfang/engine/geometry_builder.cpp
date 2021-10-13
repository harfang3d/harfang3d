// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/geometry_builder.h"

namespace hg {

void GeometryBuilder::AddVertex(const Vertex &vtx) { vertices.push_back(vtx); }

void GeometryBuilder::AddPolygon(const std::vector<uint32_t> &idxs, uint16_t material) {
	Polygon pol;
	for (int i = 0; i < idxs.size(); ++i)
		pol.vidx.push_back(idxs[i]);
	pol.material = material;
	polygons.push_back(pol);
}

void GeometryBuilder::AddTriangle(uint32_t a, uint32_t b, uint32_t c, uint32_t material) { AddPolygon({a, b, c}, material); }
void GeometryBuilder::AddQuad(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t material) { AddPolygon({a, b, c, d}, material); }

Geometry GeometryBuilder::Make() {
	Geometry geo;

	for (const auto &vtx : vertices) {
		geo.vtx.push_back(vtx.pos);
		geo.normal.push_back(vtx.normal);
		for (auto i = 0; i < 8; ++i)
			geo.uv[i].push_back((&vtx.uv0)[i]);
		geo.color.push_back(vtx.color0);
	}

	for (const auto &pol : polygons) {
		Geometry::Polygon p;
		p.vtx_count = numeric_cast<uint8_t>(pol.vidx.size());
		for (const auto &vidx : pol.vidx)
			geo.binding.push_back(vidx);
		p.material = numeric_cast<uint8_t>(pol.material);
		geo.pol.push_back(p);
	}

	return geo;
}

void GeometryBuilder::Clear() {
	vertices.clear();
	polygons.clear();
}

} // namespace hg
