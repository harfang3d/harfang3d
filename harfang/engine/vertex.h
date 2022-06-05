// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/color.h"
#include "foundation/vector2.h"
#include "foundation/vector3.h"

namespace hg {

struct Vertex {
	Vec3 pos;

	Vec3 normal;
	Vec3 tangent;
	Vec3 binormal;

	Vec2 uv0, uv1, uv2, uv3, uv4, uv5, uv6, uv7;
	Color color0, color1, color2, color3;

	uint8_t index[4];
	float weight[4];
};

Vertex MakeVertex(const Vec3 &pos, const Vec3 &nrm = {0.f, 1.f, 0.f}, const Vec2 &uv = {0.f, 0.f}, const Color &color = {1.f, 1.f, 1.f, 1.f});

} // namespace hg
