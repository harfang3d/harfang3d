// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <math.h>

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/frustum.h"

#include "foundation/math.h"
#include "foundation/unit.h"
#include "foundation/matrix44.h"
#include "foundation/minmax.h"
#include "foundation/projection.h"

using namespace hg;

void test_frustum() {
	const float znear = 0.1f;
	const float zfar = 10.f;
	const Vec2 res(2560.f, 1440.f);
	const Vec2 ar = ComputeAspectRatioX(res.x, res.y);
	const float fov = Deg(60.f);
	const float zoom = FovToZoomFactor(fov);

	const Mat44 proj = ComputePerspectiveProjectionMatrix(znear, zfar, zoom, ar);
	const Mat4 view = Mat4LookAtUp(Vec3(0.f, 5.f, 5.f), Vec3(0.f, 0.f, 5.f), Vec3(0.f, 0.f, 1.f));

	Frustum frustum0 = MakeFrustum(proj);
	Frustum frustum1 = MakeFrustum(proj, view);
	Frustum frustum2 = TransformFrustum(frustum0, TransformationMat4(Vec3(0.f, 5.f, 5.f), Deg3(90.f, 0.f, 0.f)));

	TEST_CHECK(AlmostEqual(frustum1[FP_Top], frustum2[FP_Top], 1.e-5f));
	TEST_CHECK(AlmostEqual(frustum1[FP_Bottom], frustum2[FP_Bottom], 1.e-5f));
	TEST_CHECK(AlmostEqual(frustum1[FP_Left], frustum2[FP_Left], 1.e-5f));
	TEST_CHECK(AlmostEqual(frustum1[FP_Right], frustum2[FP_Right], 1.e-5f));
	TEST_CHECK(AlmostEqual(frustum1[FP_Near], frustum2[FP_Near], 1.e-5f));
	TEST_CHECK(AlmostEqual(frustum1[FP_Far], frustum2[FP_Far], 1.e-5f));

	MinMax m[4] = { 
		MinMax(Vec3(-0.1f, -0.1f, 4.9f), Vec3(0.1f, 0.1f, 5.1f)),
		MinMax(Vec3(-0.1f, -0.1f, 9.9f), Vec3(0.1f, 0.1f, 10.1f)),
		MinMax(Vec3(-2.f, -2.f, 2.f), Vec3(2.f, 2.f, 6.f)),
		MinMax(Vec3(2.f, -5.1f, 4.9f), Vec3(2.2f, -4.8f, 5.1f))
	};

	TEST_CHECK(TestVisibility(frustum0, m[0]) == V_Inside);
	TEST_CHECK(TestVisibility(frustum0, m[1]) == V_Clipped);
	TEST_CHECK(TestVisibility(frustum0, m[2]) == V_Clipped);
	TEST_CHECK(TestVisibility(frustum0, m[3]) == V_Outside);

	TEST_CHECK(TestVisibility(frustum1, m[0]) == V_Inside);
	TEST_CHECK(TestVisibility(frustum1, m[1]) == V_Outside);
	TEST_CHECK(TestVisibility(frustum1, m[2]) == V_Clipped);
	TEST_CHECK(TestVisibility(frustum1, m[3]) == V_Clipped);


	struct {
		Vec3 origin;
		float radius;
	} spheres[4];
	
	Vec3 cubes[4][8];

	for (int i = 0; i < 4; i++) {
		ComputeMinMaxBoundingSphere(m[i], spheres[i].origin, spheres[i].radius);
		GetMinMaxVertices(m[i], cubes[i]);
	}

	TEST_CHECK(TestVisibility(frustum0, spheres[0].origin, spheres[0].radius) == V_Inside);
	TEST_CHECK(TestVisibility(frustum0, spheres[1].origin, spheres[1].radius) == V_Clipped);
	TEST_CHECK(TestVisibility(frustum0, spheres[2].origin, spheres[2].radius) == V_Clipped);
	TEST_CHECK(TestVisibility(frustum0, spheres[3].origin, spheres[3].radius) == V_Outside);

	TEST_CHECK(TestVisibility(frustum1, spheres[0].origin, spheres[0].radius) == V_Inside);
	TEST_CHECK(TestVisibility(frustum1, spheres[1].origin, spheres[1].radius) == V_Outside);
	TEST_CHECK(TestVisibility(frustum1, spheres[2].origin, spheres[2].radius) == V_Clipped);
	TEST_CHECK(TestVisibility(frustum1, spheres[3].origin, spheres[3].radius) == V_Clipped);

	TEST_CHECK(TestVisibility(frustum0, 8, cubes[0]) == V_Inside);
	TEST_CHECK(TestVisibility(frustum0, 8, cubes[1]) == V_Clipped);
	TEST_CHECK(TestVisibility(frustum0, 8, cubes[2]) == V_Clipped);
	TEST_CHECK(TestVisibility(frustum0, 8, cubes[3]) == V_Outside);

	TEST_CHECK(TestVisibility(frustum1, 8, cubes[0]) == V_Inside);
	TEST_CHECK(TestVisibility(frustum1, 8, cubes[1]) == V_Outside);
	TEST_CHECK(TestVisibility(frustum1, 8, cubes[2]) == V_Clipped);
	TEST_CHECK(TestVisibility(frustum1, 8, cubes[3]) == V_Clipped);

	/*

	Frustum TransformFrustum(const Frustum &frustum, const Mat4 &mtx);
*/
}