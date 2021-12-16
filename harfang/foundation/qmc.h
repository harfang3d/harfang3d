// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <cmath>
#include <type_traits>
#include <vector>

#include "foundation/assert.h"

namespace hg {

template <typename T>
struct tVec2;
struct Vec3;

/// Computes an element of the van der Corput sequence.
template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, T>::type vdC(int index, int base) {
	T r = 0;
	for (int j = std::abs(index), d = base; j != 0; j /= base, d *= base)
		r += static_cast<T>(j % base) / d;
	return std::signbit(index) ? -r : r;
}

/// Computes generalized halton sequence.
template <typename T>
void halton(T *out, int dimension, int index, int n) {
	static const int prime[10] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29};

	__ASSERT__(n >= 0);

	if (dimension <= 2)
		dimension = 2;
	else if (dimension > 9)
		dimension = 9;

	const int k = prime[dimension];
	for (int i = 0; i < n; ++i)
		for (int j = 0; j < dimension; ++j)
			*out++ = vdC<T>(i, k);
}

/// Computes a single element of the halton (2, p) sequence on a plane.
tVec2<float> planeHalton(int i, int p);

/// Generates halton (2, p) sequence on a plane.
void planeHalton(std::vector<tVec2<float>> &out, int p, int n);

/// Computes a single element of the halton (2, p) sequence on a sphere.
Vec3 sphereHalton(int i, int p);

/// Generates halton (2, p) sequence on a sphere.
void sphereHalton(std::vector<Vec3> &out, int p, int n);

} // namespace hg
