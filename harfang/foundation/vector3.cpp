// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/vector3.h"
#include "foundation/assert.h"
#include "foundation/math.h"
#include "foundation/matrix3.h"
#include "foundation/matrix4.h"
#include "foundation/rand.h"
#include "foundation/unit.h"
#include "foundation/vector2.h"
#include "foundation/vector4.h"

#include <limits>

namespace hg {

const Vec3 Vec3::Zero(0, 0, 0);
const Vec3 Vec3::One(1, 1, 1);
const Vec3 Vec3::Min(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());
const Vec3 Vec3::Max(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
const Vec3 Vec3::Left(-1, 0, 0), Vec3::Right(1, 0, 0), Vec3::Up(0, 1, 0), Vec3::Down(0, -1, 0), Vec3::Front(0, 0, 1), Vec3::Back(0, 0, -1);

//
Vec3::Vec3(const tVec2<int> &v) : x(float(v.x)), y(float(v.y)), z(0.f) {}
Vec3::Vec3(const tVec2<float> &v) : x(v.x), y(v.y), z(0.f) {}
Vec3::Vec3(const float &v) : x(v), y(v), z(v) {}
Vec3::Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
Vec3::Vec3(const Vec4 &v) : x(v.x), y(v.y), z(v.z) {}

//
int Hash(const Vec3 &v) {
	const auto a = static_cast<int>(v.x * 10.f), b = static_cast<int>(v.y * 10.f), c = static_cast<int>(v.z * 10.f);
	// From Christer Ericson's Realtime Collision Detection.
	return a * 0x8da6b343 + b * 0xd8163841 + c * 0xcb1ab31f;
}

//
Vec3 MakeVec3(const Vec4 &v) { return {v.x, v.y, v.z}; }

Vec3 RandomVec3(float min, float max) { return {FRRand(min, max), FRRand(min, max), FRRand(min, max)}; }
Vec3 RandomVec3(const Vec3 &min, const Vec3 &max) { return {FRRand(min.x, max.x), FRRand(min.y, max.y), FRRand(min.z, max.z)}; }

bool AlmostEqual(const Vec3 &a, const Vec3 &b, float epsilon) { return Abs(a.x - b.x) < epsilon && Abs(a.y - b.y) < epsilon && Abs(a.z - b.z) < epsilon; }

//
float Dist2(const Vec3 &a, const Vec3 &b) { return (b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y) + (b.z - a.z) * (b.z - a.z); }
float Dist(const Vec3 &a, const Vec3 &b) { return Sqrt(Dist2(a, b)); }

float Len2(const Vec3 &v) { return v.x * v.x + v.y * v.y + v.z * v.z; }
float Len(const Vec3 &v) { return Sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }

Vec3 Min(const Vec3 &a, const Vec3 &b) { return {Min(a.x, b.x), Min(a.y, b.y), Min(a.z, b.z)}; }
Vec3 Max(const Vec3 &a, const Vec3 &b) { return {Max(a.x, b.x), Max(a.y, b.y), Max(a.z, b.z)}; }

Vec3 Normalize(const Vec3 &v) {
	const auto l = Len(v);
	return l > 0.f ? v / l : v;
}

Vec3 Clamp(const Vec3 &v, float min, float max) { return {Clamp(v.x, min, max), Clamp(v.y, min, max), Clamp(v.z, min, max)}; }

Vec3 Clamp(const Vec3 &v, const Vec3 &min, const Vec3 &max) { return {Clamp(v.x, min.x, max.x), Clamp(v.y, min.y, max.y), Clamp(v.z, min.z, max.z)}; }

Vec3 ClampLen(const Vec3 &v, float min, float max) {
	const auto l = Len(v);
	const auto k = Clamp(l, min, max) / l;
	return v * k;
}

//
Vec3 Abs(const Vec3 &v) { return {Abs(v.x), Abs(v.y), Abs(v.z)}; }
Vec3 Sign(const Vec3 &v) { return {v.x < 0.f ? -1.f : 1.f, v.y < 0.f ? -1.f : 1.f, v.z < 0.f ? -1.f : 1.f}; }

Vec3 Reflect(const Vec3 &v, const Vec3 &n) {
	const auto rv = Reverse(v);
	return n * (2.f * Dot(rv, n)) - rv;
}

Vec3 Refract(const Vec3 &v, const Vec3 &n, float k_in, float k_out) {
	const auto k = k_in / k_out;
	return v * k + n * (k - 1.f);
}

Vec3 Floor(const Vec3 &v) { return {Floor(v.x), Floor(v.y), Floor(v.z)}; }
Vec3 Ceil(const Vec3 &v) { return {Ceil(v.x), Ceil(v.y), Ceil(v.z)}; }

Vec3 FaceForward(const Vec3 &v, const Vec3 &d) { return Dot(v, d) >= 0.f ? Reverse(v) : v; }

//
Vec3 BaseToEuler(const Vec3 &u) {
	Vec3 euler;

	auto _v = Sqrt(u.x * u.x + u.y * u.y + u.z * u.z);
	euler.x = -ASin(u.y / _v);

	_v = Sqrt(u.x * u.x + u.z * u.z);
	euler.y = _v > 0.00001f ? ASin(u.x / _v) : 0;

	if (u.z < 0.f) {
		if (euler.y < 0.f)
			euler.y = -(Pi + euler.y);
		else
			euler.y = Pi - euler.y;
	}
	euler.z = 0;

	return euler;
}

Vec3 BaseToEuler(const Vec3 &u, const Vec3 &v) {
	auto euler = BaseToEuler(u);

	const auto mx = RotationMatX(Rad(euler.x));
	const auto my = RotationMatY(Rad(euler.y));

	const auto bv = mx * (my * Vec3(1, 0, 0));
	const auto vn = Normalize(v);

	const auto vc = Dot(vn, bv);

	if (vc >= 1.f)
		euler.z = 0.f;
	else if (vc <= -1.f)
		euler.z = Pi;
	else
		euler.z = ACos(vc);

	if (Dot(Cross(bv, vn), u) <= 0.f)
		euler.z = (Pi + Pi) - euler.z;

	return euler;
}

Vec3 Quantize(const Vec3 &v, float qx, float qy, float qz) {
	return {qx ? float(int(v.x / qx) * qx) : v.x, qy ? float(int(v.y / qy) * qy) : v.y, qz ? float(int(v.z / qz) * qz) : v.z};
}

Vec3 Quantize(const Vec3 &v, float q) { return Quantize(v, q, q, q); }

Vec3 Deg3(float x, float y, float z) { return {Deg(x), Deg(y), Deg(z)}; }
Vec3 Rad3(float x, float y, float z) { return {Rad(x), Rad(y), Rad(z)}; }

} // namespace hg
