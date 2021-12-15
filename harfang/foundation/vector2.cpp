// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/math.h"

#include "foundation/vector2.h"
#include "foundation/matrix3.h"
#include "foundation/vector3.h"
#include "foundation/vector4.h"

namespace hg {

template <> tVec2<float>::tVec2(const Vec3 &v) {
	x = v.x;
	y = v.y;
}

template <> tVec2<float>::tVec2(const Vec4 &v) {
	x = v.x;
	y = v.y;
}

template <> tVec2<int>::tVec2(const Vec3 &v) {
	x = int(v.x);
	y = int(v.y);
}

template <> tVec2<int>::tVec2(const Vec4 &v) {
	x = int(v.x);
	y = int(v.y);
}

bool TestEqual(const Vec2 &a, const Vec2 &b, float e) { return Abs(b.x - a.x) < e && Abs(b.y - a.y) < e; }

template <> tVec2<int> operator*(const tVec2<int> &v, const Mat3 &m) {
	return {int(float(v.x) * m.m[0][0] + float(v.y) * m.m[0][1] + m.m[0][2]), int(float(v.x) * m.m[1][0] + float(v.y) * m.m[1][1] + m.m[1][2])};
}

template <> tVec2<float> operator*(const tVec2<float> &v, const Mat3 &m) {
	return {v.x * m.m[0][0] + v.y * m.m[0][1] + m.m[0][2], v.x * m.m[1][0] + v.y * m.m[1][1] + m.m[1][2]};
}

template <> float Len(const tVec2<float> &v) { return Sqrt(Len2(v)); }
template <> int Len(const tVec2<int> &v) { return int(Sqrt(float(Len2(v)))); }

template <> float Dist(const tVec2<float> &a, const tVec2<float> &b) { return Sqrt(Dist2(a, b)); }
template <> int Dist(const tVec2<int> &a, const tVec2<int> &b) { return int(Sqrt(float(Dist2(a, b)))); }

} // namespace hg
