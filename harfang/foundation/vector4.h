// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/math.h"
#include "foundation/assert.h"

#include <cstddef>

namespace hg {

template <typename T> struct tVec2;

struct Vec3;
struct Mat4;
struct Color;

struct Vec4 {
	static const Vec4 Zero;
	static const Vec4 One;

	Vec4() = default;
	Vec4(float x, float y, float z, float w = 1.f);

	explicit Vec4(const tVec2<int> &v);
	explicit Vec4(const tVec2<float> &v);
	explicit Vec4(const Vec3 &v, float w = 1.f);
	explicit Vec4(const Color &c);
	explicit Vec4(float v);

	inline Vec4 &operator+=(const Vec4 &b) {
		x += b.x;
		y += b.y;
		z += b.z;
		w += b.w;
		return *this;
	}

	inline Vec4 &operator+=(const float k) {
		x += k;
		y += k;
		z += k;
		w += k;
		return *this;
	}

	inline Vec4 &operator-=(const Vec4 &b) {
		x -= b.x;
		y -= b.y;
		z -= b.z;
		w -= b.w;
		return *this;
	}

	inline Vec4 &operator-=(const float k) {
		x -= k;
		y -= k;
		z -= k;
		w -= k;
		return *this;
	}

	inline Vec4 &operator*=(const Vec4 &b) {
		x *= b.x;
		y *= b.y;
		z *= b.z;
		w *= b.w;
		return *this;
	}

	inline Vec4 &operator*=(const float k) {
		x *= k;
		y *= k;
		z *= k;
		w *= k;
		return *this;
	}

	inline Vec4 &operator/=(const Vec4 &b) {
		x /= b.x;
		y /= b.y;
		z /= b.z;
		w /= b.w;
		return *this;
	}

	inline Vec4 &operator/=(const float k) {
		const float k_ = NotEqualZero(k) ? 1.F / k : 0.F;
		x *= k_;
		y *= k_;
		z *= k_;
		w *= k_;
		return *this;
	}

	void operator*=(const Mat4 &);

	inline Vec4 operator-() const {
		return Vec4(-x, -y, -z, -w);
	}

	inline float operator[](size_t n) const {
		__ASSERT__(n >= 0 && n <= 3);
		float res;

		if (n == 0) {
			res = x;
		} else if (n == 1) {
			res = y;
		} else if (n == 2) {
			res = z;
		} else if (n == 3) {
			res = w;
		} else {
			res = std::numeric_limits<float>::max();
		}

		return res;
	}

	inline float &operator[](size_t n) {
		__ASSERT__(n >= 0 && n <= 3);
		float *res;

		if (n == 0) {
			res = &x;
		} else if (n == 1) {
			res = &y;
		} else if (n == 2) {
			res = &z;
		} else if (n == 3) {
			res = &w;
		} else {
			res = nullptr;
		}

		return *res;
	}

	float x, y, z, w;
};

inline bool operator==(const Vec4 &a, const Vec4 &b) { return Equal(a.x, b.x) && Equal(a.y, b.y) && Equal(a.z, b.z) && Equal(a.w, b.w); }
inline bool operator!=(const Vec4 &a, const Vec4 &b) { return NotEqual(a.x, b.x) || NotEqual(a.y, b.y) || NotEqual(a.z, b.z) || NotEqual(a.w, b.w); }

inline Vec4 operator+(const Vec4 &a, const Vec4 &b) { return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w}; }
inline Vec4 operator+(const Vec4 &a, const float v) { return {a.x + v, a.y + v, a.z + v, a.w + v}; }
inline Vec4 operator-(const Vec4 &a, const Vec4 &b) { return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w}; }
inline Vec4 operator-(const Vec4 &a, const float v) { return {a.x - v, a.y - v, a.z - v, a.w - v}; }
inline Vec4 operator*(const Vec4 &a, const Vec4 &b) { return {a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w}; }
inline Vec4 operator*(const Vec4 &a, const float v) { return {a.x * v, a.y * v, a.z * v, a.w * v}; }
inline Vec4 operator*(const float v, const Vec4 &a) { return a * v; }
inline Vec4 operator/(const Vec4 &a, const Vec4 &b) { return {a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w}; }
inline Vec4 operator/(const Vec4 &v, const float k) { return {v.x / k, v.y / k, v.z / k, v.w / k}; }

Vec4 Abs(const Vec4 &v);

Vec4 Normalize(const Vec4 &v);

inline Vec4 Reverse(const Vec4 &v) {
	return Vec4(-v.x, -v.y, -v.z, -v.w);
}

/// Return a vector from integer value in the [0;255] range.
inline Vec4 Vec4I(int x, int y, int z, int w = 255) { return {float(x) / 255.f, float(y) / 255.f, float(z) / 255.f, float(w) / 255.f}; }

/// Return a random vector.
Vec4 RandomVec4(float min = -1.f, float max = 1.f);
Vec4 RandomVec4(const Vec4 &min, const Vec4 &max);

bool AlmostEqual(const Vec4 &a, const Vec4 &b, float epsilon);

} // namespace hg
