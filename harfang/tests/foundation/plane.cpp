// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <math.h>

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/plane.h"

#include "foundation/math.h"
#include "foundation/matrix4.h"

using namespace hg;

void test_plane() {
	const Vec3 o0(2.f, -1.4f, 10.f);
	const Vec3 n0 = Normalize(Vec3::One);
	Plane p0 = MakePlane(o0, n0);
	TEST_CHECK(Equal(p0.x, n0.x));
	TEST_CHECK(Equal(p0.y, n0.y));
	TEST_CHECK(Equal(p0.z, n0.z));
	TEST_CHECK(Equal(p0.w, -Dot(o0,n0)));

	float u = asin(1.f / sqrt(3.f));
	float v = Pi / 4.f;
	Plane p1 = MakePlane(Vec3::Zero, Vec3(0.f, 0.f, 1.f), TransformationMat4(o0, Vec3(-u, v, 0)));
	TEST_CHECK(AlmostEqual(p0, p1, 0.000001f));

	TEST_CHECK(Equal(DistanceToPlane(p0, Vec3(4.f, -3.4f, 10.f)), 0.f));
	TEST_CHECK(AlmostEqual(DistanceToPlane(p0, Vec3(6.f, -1.4f, 12.f)), 2.f * sqrt(3.f)));
	TEST_CHECK(AlmostEqual(DistanceToPlane(p0, Vec3(2.f, -5.4f, 8.f)), -2.f * sqrt(3.f)));
}