// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <cstddef>

namespace hg {

template <typename T>
struct tVec2;
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
		const float k_ = k != 0.f ? 1.f / k : 0.f;
		x *= k_;
		y *= k_;
		z *= k_;
		w *= k_;
		return *this;
	}

	void operator*=(const Mat4 &);

	inline float operator[](size_t n) const { return (&x)[n]; }
	inline float &operator[](size_t n) { return (&x)[n]; }

	float x, y, z, w;
};

inline bool operator==(const Vec4 &a, const Vec4 &b) { return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w; }
inline bool operator!=(const Vec4 &a, const Vec4 &b) { return a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.w; }

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

inline Vec4 Opposite(const Vec4 &v) { return {-v.x, -v.y, -v.z, -v.w}; }

/// Return a vector from integer value in the [0;255] range.
inline Vec4 Vec4I(int x, int y, int z, int w = 255) { return {float(x) / 255.f, float(y) / 255.f, float(z) / 255.f, float(w) / 255.f}; }

/// Return a random vector.
Vec4 RandomVec4(float min = -1.f, float max = 1.f);
Vec4 RandomVec4(const Vec4 &min, const Vec4 &max);

} // namespace hg
