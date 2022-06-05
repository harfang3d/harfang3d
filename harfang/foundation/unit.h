// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <iomanip>
#include <sstream>
#include <string>
#include <type_traits>

#include "foundation/time.h"

namespace hg {

/// Convert an angle in degrees to the engine unit system.
template <typename T> constexpr T Deg(T v) {
	static_assert(std::is_floating_point<T>::value, "Expected floating point type");
	return v / T(180) * T(3.1415926535);
}
/// Convert an angle in radians to the engine unit system.
template <typename T> constexpr T Rad(T v) {
	static_assert(std::is_floating_point<T>::value, "Expected floating point type");
	return v;
}

/// Convert an angle in degrees to radians.
template <typename T> constexpr T DegreeToRadian(T v) { return v * (3.1415926535f / 180.f); }
/// Convert an angle in radians to degrees.
template <typename T> constexpr T RadianToDegree(T v) { return v * (180.f / 3.1415926535f); }

template <typename T> constexpr T Sec(T v) { return v; }

template <typename T> constexpr T Csec(T v) {
	static_assert(std::is_floating_point<T>::value, "Expected floating point type");
	return v * T(0.01);
}
template <typename T> constexpr T Ms(T v) {
	static_assert(std::is_floating_point<T>::value, "Expected floating point type");
	return v * T(0.001);
}

template <typename T> constexpr T Ton(T v) { return v * T(1000); }
template <typename T> constexpr T Kg(T v) { return v; }

template <typename T> constexpr T G(T v) {
	static_assert(std::is_floating_point<T>::value, "Expected floating point type");
	return v * T(0.001);
}

template <typename T> constexpr T Km(T v) { return v * T(1000); }
template <typename T> constexpr T Mtr(T v) { return v; }

template <typename T> constexpr T Cm(T v) {
	static_assert(std::is_floating_point<T>::value, "Expected floating point type");
	return v * T(0.01);
}
template <typename T> constexpr T Mm(T v) {
	static_assert(std::is_floating_point<T>::value, "Expected floating point type");
	return v * T(0.001);
}
template <typename T> constexpr T Inch(T v) {
	static_assert(std::is_floating_point<T>::value, "Expected floating point type");
	return v * T(0.0254);
}

inline constexpr size_t KB(const size_t size) { return size * 1024; }
inline constexpr size_t MB(const size_t size) { return size * 1024 * 1024; }

template <typename T> std::string FormatMemorySize(T v_) {
	std::ostringstream str;

	int64_t v = int64_t(v_);

	if (v < 0) {
		str << "-";
		v = -v;
	}

	if (v < 1024LL)
		str << std::fixed << std::setprecision(0) << v << "B";
	else if (v < 1024LL * 1024LL)
		str << std::fixed << std::setfill('0') << std::setprecision(1) << float((v * 10LL) / (1024LL)) / 10.f << "KB";
	else if (v < 1024LL * 1024LL * 1024LL)
		str << std::fixed << std::setfill('0') << std::setprecision(1) << float((v * 10LL) / (1024LL * 1024LL)) / 10.f << "MB";
	else if (v < 1024LL * 1024LL * 1024LL * 1024LL)
		str << std::fixed << std::setfill('0') << std::setprecision(1) << float((v * 10LL) / (1024LL * 1024LL * 1024LL)) / 10.f << "GB";
	else
		str << std::fixed << std::setfill('0') << std::setprecision(1) << float((v * 10LL) / (1024LL * 1024LL * 1024LL * 1024LL)) / 10.f << "TB";

	return str.str();
}

template <typename T> std::string FormatCount(T v_) {
	std::ostringstream str;

	int64_t v = int64_t(v_);

	if (v < 0) {
		str << "-";
		v = -v;
	}

	if (v < 1000LL)
		str << v;
	else if (v < 1000LL * 1000LL)
		str << std::fixed << std::setfill('0') << std::setprecision(1) << float((v * 10LL) / (1000LL)) / 10.f << "K";
	else if (v < 1000LL * 1000LL * 1000LL)
		str << std::fixed << std::setfill('0') << std::setprecision(1) << float((v * 10LL) / (1000LL * 1000LL)) / 10.f << "M";
	else
		str << std::fixed << std::setfill('0') << std::setprecision(1) << float((v * 10LL) / (1000LL * 1000LL * 1000LL)) / 10.f << "G";

	return str.str();
}

template <typename T> std::string FormatDistance(T v_) {
	std::ostringstream str;

	auto v = float(v_);

	if (v < 0) {
		str << "-";
		v = -v;
	}

	if (v < 0.1)
		str << std::fixed << std::setfill('0') << std::setprecision(1) << float(v) * 1000.f << "mm";
	else if (v < 1)
		str << std::fixed << std::setfill('0') << std::setprecision(1) << float(v) * 100.f << "cm";
	else if (v >= 1000)
		str << std::fixed << std::setfill('0') << std::setprecision(1) << float(v) / 1000.f << "km";
	else
		str << std::fixed << std::setfill('0') << std::setprecision(1) << float(v) << "m";

	return str.str();
}

std::string FormatTime(time_ns t);

} // namespace hg
