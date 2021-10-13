// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <cstddef>
#include <cstdint>

namespace hg {

enum Endianness { BigEndian, LittleEndian };

/// Swap endianness on arbitrary word size.
template <size_t WordSize> void SwapEndianness(void *ptr) {
	static_assert(!(WordSize & 1), "Cannot swap endianness on odd word size");

	auto p = reinterpret_cast<int8_t *>(ptr);
	for (size_t i = 0; i < WordSize / 2; ++i) {
		auto tmp = p[i];
		p[i] = p[WordSize - i - 1];
		p[WordSize - i - 1] = tmp;
	}
}

/// Return the current host memory configuration.
Endianness GetHostEndianness();

/// Convert a memory block in place to the host configuration.
template <size_t WordSize> void *ToHostEndianness(void *ptr, size_t size, Endianness from_endianness = BigEndian) {
	if (GetHostEndianness() != from_endianness)
		SwapEndianness<WordSize>(ptr);
	return ptr;
}

/// Convert a value to the host configuration.
template <class T> T ToHostEndianness(const T &v, Endianness from_endianness = BigEndian) {
	if (GetHostEndianness() == from_endianness)
		return v;

	T host_value = v;
	SwapEndianness<sizeof(T)>(&host_value);
	return host_value;
}

} // namespace hg
