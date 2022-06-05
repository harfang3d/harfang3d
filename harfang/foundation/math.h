// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <cstddef>
#include <cstdint>

namespace hg {

static const float Pi = 3.1415926535f;

static const float HalfPi = Pi / 2.f;
static const float TwoPi = Pi * 2.f;

/// Return the absolute value of the function input.
template <class T> constexpr T inline Abs(const T &v) { return v < 0 ? -v : v; }
/// Return the minimum of two values.
template <class T> constexpr T inline Min(const T &a, const T &b) { return a < b ? a : b; }
template <class T> constexpr T inline Min(const T &a, const T &b, const T &c) { return Min(Min(a, b), c); }
/// Return the maximum of two values.
template <class T> constexpr T inline Max(const T &a, const T &b) { return a > b ? a : b; }
template <class T> constexpr T inline Max(const T &a, const T &b, const T &c) { return Max(Max(a, b), c); }
/// Clip a value to the specified inclusive interval.
template <class T> constexpr T inline Clamp(const T &v, const T &min, const T &max) { return v < min ? min : (v > max ? max : v); }
/// Linear interpolate between two vectors on the [0;1] interval.
/// @see LinearInterpolate.
template <class T> constexpr T inline Lerp(const T &a, const T &b, float k) { return T(a * (1.f - k) + b * k); }

template <class T> T inline Wrap(T v, T range_start, T range_end) {
	const T dt = range_end - range_start + 1;

	while (v < range_start)
		v += dt;
	while (v > range_end)
		v -= dt;

	return v;
}

float Sqrt(float v);

bool TestEqual(float a, float b, float e = 0.000001f);
bool EqualZero(float v, float e = 0.000001f);

float Pow(float v, float exp);

float Ceil(float);
float Floor(float);
float Mod(float);

float Frac(float);

float RangeAdjust(float v, float old_min, float old_max, float new_min, float new_max);

float Quantize(float v, float q);

float Sin(float);
float ASin(float);
float Cos(float);
float ACos(float);
float Tan(float);
float ATan(float);

bool IsFinite(float);

template <typename T> constexpr T LinearInterpolate(T y0, T y1, float t) { return y0 + (y1 - y0) * t; }
/// Compute the cosine interpolated value between `y0` and `y1` at `t`.
/// @see LinearInterpolate, CubicInterpolate and HermiteInterpolate.
template <typename T> T CosineInterpolate(T y0, T y1, float t) {
	const auto t2 = (1.f - Cos(t * Pi)) / 2.f;
	return y0 * (1.f - t2) + y1 * t2;
}

/// Perform a cubic interpolation across four values with `t` in the [0;1] range between `y1` and `y2`.
/// @see LinearInterpolate, CosineInterpolate and HermiteInterpolate.
template <typename T> T CubicInterpolate(T y0, T y1, T y2, T y3, float t) {
	const auto t2 = t * t;
	T a0 = y3 - y2 - y0 + y1, a1 = y0 - y1 - a0, a2 = y2 - y0, a3 = y1;
	return a0 * t * t2 + a1 * t2 + a2 * t + a3;
}

/*
	tension: 1 is high, 0 normal, -1 is low
	bias: 0 is even, positive is towards first segment, negative towards the other
*/
template <typename T> T HermiteInterpolate(T y0, T y1, T y2, T y3, float t, float tension, float bias) {
	const auto t2 = t * t;
	const auto t3 = t2 * t;
	T t0 = (y1 - y0) * (1.f + bias) * (1.f - tension) / 2.f;
	t0 += (y2 - y1) * (1.f - bias) * (1.f - tension) / 2.f;
	T t1 = (y2 - y1) * (1.f + bias) * (1.f - tension) / 2.f;
	t1 += (y3 - y2) * (1.f - bias) * (1.f - tension) / 2.f;
	const auto a0 = 2.f * t3 - 3 * t2 + 1.f;
	const auto a1 = t3 - 2.f * t2 + t;
	const auto a2 = t3 - t2;
	const auto a3 = -2.f * t3 + 3.f * t2;
	return y1 * a0 + t0 * a1 + t1 * a2 + y2 * a3;
}

template <class T> T LinearInterpolateArray(uint32_t count, const T *values, float t) {
	const auto s = t * (count - 1);
	auto lo = uint32_t(s), hi = lo + 1;

	lo = Clamp<uint32_t>(lo, 0, count - 1);
	hi = Clamp<uint32_t>(hi, 0, count - 1);

	return LinearInterpolate(values[lo], values[hi], s - lo);
}

template <typename T> inline T getPOT(T v) {
	T n = 1;
	for (; n < v; n *= 2)
		;
	return n;
}

template <typename T> inline bool isPOT(T v) { return v == getPOT(v); }

} // namespace hg
