// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/rand.h"

namespace hg {

static uint32_t x = 0x75bcd15, y = 0x159a55e5, z = 0x1f123bb5;

static uint32_t xorshf96() { // XORSHIFT period 2^96-1
	uint32_t t;

	x ^= x << 16;
	x ^= x >> 5;
	x ^= x << 1;

	t = x;
	x = y;
	y = z;
	z = t ^ x ^ y;

	return z;
}

void Seed(uint32_t) { /* TODO */
}

uint32_t Rand(uint32_t r) {
	if (!r)
		return 0;
	return xorshf96() % r;
}

float FRand(float r) { return float(Rand(65536)) * r / 65536.f; }

float FRRand(float lo, float hi) {
	const float v = float(Rand(65536)) / 65536.f;
	return v * (hi - lo) + lo;
}

} // namespace hg
