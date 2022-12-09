// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/math.h"
#include "foundation/rotation_order.h"

namespace hg {

struct Vec3;
struct Vec4;
struct Mat3;

/// Quaternion
struct Quaternion {
	static const Quaternion Identity;

	Quaternion() : x(0.f), y(0.f), z(0.f), w(1.f) {};
	Quaternion(const Quaternion &q) = default;
	Quaternion(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}

	float x, y, z, w;

	Quaternion &operator+=(const Quaternion &b) {
		x += b.x;
		y += b.y;
		z += b.z;
		w += b.w;
		return *this;
	}

	Quaternion &operator+=(float k) {
		x += k;
		y += k;
		z += k;
		w += k;
		return *this;
	}

	Quaternion &operator-=(const Quaternion &b) {
		x -= b.x;
		y -= b.y;
		z -= b.z;
		w -= b.w;
		return *this;
	}

	Quaternion &operator-=(float k) {
		x -= k;
		y -= k;
		z -= k;
		w -= k;
		return *this;
	}

	Quaternion &operator*=(const Quaternion &b) {
		Quaternion a = *this;
		w = a.w * b.w - (a.x * b.x + a.y * b.y + a.z * b.z);
		x = a.w * b.x + b.w * a.x + a.y * b.z - a.z * b.y;
		y = a.w * b.y + b.w * a.y + a.z * b.x - a.x * b.z;
		z = a.w * b.z + b.w * a.z + a.x * b.y - a.y * b.x;
		return *this;
	}

	Quaternion &operator*=(float k) {
		x *= k;
		y *= k;
		z *= k;
		w *= k;
		return *this;
	}

	Quaternion &operator/=(float k) {
		k = 1.f / k;
		x *= k;
		y *= k;
		z *= k;
		w *= k;
		return *this;
	}
};

inline bool operator==(const Quaternion &a, const Quaternion &b) { return Equal(a.x, b.x) && Equal(a.y, b.y) && Equal(a.z, b.z) && Equal(a.w, b.w); }
inline bool operator!=(const Quaternion &a, const Quaternion &b) { return NotEqual(a.x, b.x) || NotEqual(a.y, b.y) || NotEqual(a.z, b.z) || NotEqual(a.w, b.w); }

inline float Dot(const Quaternion &a, const Quaternion &b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

inline Quaternion operator+(const Quaternion &a, const Quaternion &b) { return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w}; }
inline Quaternion operator+(const Quaternion &q, float v) { return {q.x + v, q.y + v, q.z + v, q.w + v}; }
inline Quaternion operator-(const Quaternion &a, const Quaternion &b) { return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w}; }
inline Quaternion operator-(const Quaternion &q, float v) { return {q.x - v, q.y - v, q.z - v, q.w - v}; }

inline Quaternion operator*(const Quaternion &a, const Quaternion &b) {
	return {
		a.w * b.x + b.w * a.x + a.y * b.z - a.z * b.y,
		a.w * b.y + b.w * a.y + a.z * b.x - a.x * b.z,
		a.w * b.z + b.w * a.z + a.x * b.y - a.y * b.x,
		a.w * b.w - (a.x * b.x + a.y * b.y + a.z * b.z)};
}

inline Quaternion operator*(const Quaternion &q, float v) { return {q.x * v, q.y * v, q.z * v, q.w * v}; }
inline Quaternion operator*(float v, const Quaternion &q) { return q * v; }
inline Quaternion operator/(const Quaternion &q, float v) { return {q.x / v, q.y / v, q.z / v, q.w / v}; }

/// Normalize quaternion.
Quaternion Normalize(const Quaternion &q);
/// Conjugate
Quaternion Conjugate(const Quaternion &q);
/// Inverse quaternion.
Quaternion Inverse(const Quaternion &q);

float Len(const Quaternion &q);
float Len2(const Quaternion &q);

/// Distance to quaternion.
float Dist(const Quaternion &a, const Quaternion &b);
/// Spherical linear interpolation.
Quaternion Slerp(const Quaternion &a, const Quaternion &b, float t);

/// From Euler angle triplet.
Quaternion QuaternionFromEuler(float x, float y, float z, RotationOrder = RO_Default);
Quaternion QuaternionFromEuler(const Vec3 &euler, RotationOrder = RO_Default);
/// Get an orientation from a 'look at' vector (look_at = to - from).
Quaternion QuaternionLookAt(const Vec3 &at);
/// From matrix3.
Quaternion QuaternionFromMatrix3(const Mat3 &m);
/// From axis-angle.
Quaternion QuaternionFromAxisAngle(float angle, const Vec3 &axis);

/// To rotation matrix.
Mat3 ToMatrix3(const Quaternion &q);
/// To Euler angle triplet.
Vec3 ToEuler(const Quaternion &q, RotationOrder = RO_Default);

/// Rotate a vector
Vec3 Rotate(const Quaternion &q, const Vec3 &v);
Vec4 Rotate(const Quaternion &q, const Vec4 &v);

} // namespace hg
