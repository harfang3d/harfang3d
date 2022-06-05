// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/vertex.h"

namespace hg {

Vertex MakeVertex(const Vec3 &pos, const Vec3 &nrm, const Vec2 &uv0, const Color &color0) {
	Vertex vtx;
	vtx.pos = pos;
	vtx.normal = nrm;
	vtx.uv0 = uv0;
	vtx.color0 = color0;
	return vtx;
}

} // namespace hg
