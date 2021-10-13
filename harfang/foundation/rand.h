// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <cstdint>

namespace hg {

#ifndef RAND_MAX
#define RAND_MAX 0x7fff
#endif

/// Set the starting seed of the random number generator.
void Seed(uint32_t seed);
/// Return an integer random number in the range [0;r] (default [0;RAND_MAX].
uint32_t Rand(uint32_t range = RAND_MAX);

/// Return a float random value in the range [0;r] (default [0;1]).
float FRand(float range = 1.f);
/// Return a float random value in the range [lo, hi] (default [-1, 1]).
float FRRand(float range_start = -1.f, float range_end = 1.f);

} // namespace hg
