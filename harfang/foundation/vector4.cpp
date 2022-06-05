// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/vector4.h"
#include "foundation/color.h"
#include "foundation/math.h"
#include "foundation/matrix4.h"
#include "foundation/rand.h"
#include "foundation/vector2.h"
#include "foundation/vector3.h"

namespace hg {

const Vec4 Vec4::Zero(0, 0, 0, 0);
const Vec4 Vec4::One(1, 1, 1, 1);

void Vec4::operator*=(const Mat4 &m) {
	const auto _x = x, _y = y, _z = z, _w = w;
	x = _x * m.m[0][0] + _y * m.m[0][1] + _z * m.m[0][2] + _w * m.m[0][3];
	y = _x * m.m[1][0] + _y * m.m[1][1] + _z * m.m[1][2] + _w * m.m[1][3];
	z = _x * m.m[2][0] + _y * m.m[2][1] + _z * m.m[2][2] + _w * m.m[2][3];
	w = _w;
}

Vec4 Abs(const Vec4 &v) { return {Abs(v.x), Abs(v.y), Abs(v.z), Abs(v.w)}; }

Vec4 Normalize(const Vec4 &v) {
	const auto l = Sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
	return l > 0.f ? v / l : v;
}

Vec4 RandomVec4(float min, float max) { return {FRRand(min, max), FRRand(min, max), FRRand(min, max), FRRand(min, max)}; }
Vec4 RandomVec4(const Vec4 &min, const Vec4 &max) { return {FRRand(min.x, max.x), FRRand(min.y, max.y), FRRand(min.z, max.z), FRRand(min.w, max.w)}; }

Vec4::Vec4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

Vec4::Vec4(const tVec2<int> &v) : x(float(v.x)), y(float(v.y)), z(0), w(1) {}
Vec4::Vec4(const tVec2<float> &v) : x(float(v.x)), y(float(v.y)), z(0), w(1) {}
Vec4::Vec4(const Vec3 &v, float w_) : x(v.x), y(v.y), z(v.z), w(w_) {}

Vec4::Vec4(const Color &c) : x(c.r), y(c.g), z(c.b), w(c.a) {}
Vec4::Vec4(float v) : x(v), y(v), z(v), w(v) {}

} // namespace hg
