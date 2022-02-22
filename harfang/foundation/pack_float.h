// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <limits>
#include <type_traits>

#include "foundation/math.h"

namespace hg {

template <typename T> inline constexpr T pack_float(float v, float scale = 1.f) {
	return T(Clamp(v / scale, (std::is_signed<T>::value) ? -1.f : 0.f, 1.f) * std::numeric_limits<T>::max());
}
template <typename T> inline constexpr float unpack_float(T v, float scale = 1.f) { return (float(v) / std::numeric_limits<T>::max()) * scale; }

//
template <> inline constexpr float pack_float<float>(float v, float) { return v; }
template <> inline constexpr float unpack_float<float>(float v, float) { return v; }

} // namespace hg
