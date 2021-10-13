// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/bit.h"
#include <cstdint>

namespace hg {

int get_bit_count(int v) {
	int n;
	for (n = 0; v; ++n)
		v &= (v - 1);
	return n;
}

int get_shift_count(int v) {
	int n;
	for (n = 0; !(v & 1) && (n < 32); ++n)
		v >>= 1;
	return n;
}

int count_set_bit(int v) {
	int count = 0;
	for (int n = 0; n < 32; ++n) {
		count += v & 1;
		v >>= 1;
	}
	return count;
}

void write_bit(void *ptr_, unsigned bitoffset, unsigned bitcount, unsigned v) {
	auto ptr = reinterpret_cast<uint8_t *>(ptr_);

	ptr += bitoffset >> 3;
	bitoffset &= 7;

	while (bitcount--) {
		ptr[0] |= ((v >> bitcount) & 1) << bitoffset;

		if (bitoffset == 7) {
			bitoffset = 0;
			ptr++;
		} else
			++bitoffset;
	}
}

unsigned read_bit(const void *ptr_, unsigned offsetbit, unsigned nbit) {
	auto ptr = reinterpret_cast<const uint8_t *>(ptr_);

	ptr += offsetbit >> 3;
	offsetbit &= 7;

	unsigned v = 0;
	while (nbit--) {
		v += v;

		if (ptr[0] & (1 << offsetbit))
			v |= 1;

		if (offsetbit == 7) {
			offsetbit = 0;
			++ptr;
		} else {
			++offsetbit;
		}
	}
	return v;
}

} // namespace hg
