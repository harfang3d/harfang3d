// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/qmc.h"
#include "foundation/math.h"
#include "foundation/vector2.h"
#include "foundation/vector3.h"

namespace hg {

Vec2 planeHalton(int i, int p) {
	Vec2 out;
	float inv_base = 0.5f;
	out.x = 0.f;
	for (auto j = i; j; j >>= 1, inv_base /= 2.f)
		if (j & 1)
			out.x += inv_base;

	inv_base = 1.f / p;
	out.y = 0.f;
	for (int j = i; j; j /= p, inv_base /= p)
		out.y += static_cast<float>(j % p) * inv_base;

	return out;
}

void planeHalton(std::vector<Vec2> &out, int p, int n) {
	out.resize(n);
	for (auto i = 0; i < n; i++)
		out[i] = planeHalton(i, p);
}

Vec3 sphereHalton(int i, int p) {
	float inv_base = 0.5f;
	float u = 0.f;
	for (int j = i; j; j >>= 1, inv_base /= 2.f)
		u += (j & 1) * inv_base;

	inv_base = 1.f / p;
	float v = 0.f;
	for (int j = i; j; j /= p, inv_base /= p)
		v += static_cast<float>(j % p) * inv_base;

	const float phi = 4.f * Pi * v;
	const float cos_theta = 2.f * u - 1.f;
	const float sin_theta = sqrt(1.f - cos_theta * cos_theta);

	return {sin_theta * cosf(phi), sin_theta * sinf(phi), cos_theta};
}

void sphereHalton(std::vector<Vec3> &out, int p, int n) {
	out.resize(n);
	for (int i = 0; i < n; ++i)
		out[i] = sphereHalton(i, p);
}

} // namespace hg
