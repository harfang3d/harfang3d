// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <math.h>

#define TEST_NO_MAIN 
#include "acutest.h"

#include "foundation/matrix4.h"

#include "foundation/math.h"
#include "foundation/matrix3.h"
#include "foundation/vector3.h"
#include "foundation/vector4.h"

using namespace hg;

void test_mat4() {
#if 0
	{
		Mat4 m;
		TEST_CHECK(m.m[0][0] == 1.f);
		TEST_CHECK(m.m[1][0] == 0.f);
		TEST_CHECK(m.m[2][0] == 0.f);

		TEST_CHECK(m.m[0][1] == 0.f);
		TEST_CHECK(m.m[1][1] == 1.f);
		TEST_CHECK(m.m[2][1] == 0.f);

		TEST_CHECK(m.m[0][2] == 0.f);
		TEST_CHECK(m.m[1][2] == 0.f);
		TEST_CHECK(m.m[2][2] == 1.f);

		TEST_CHECK(m.m[0][3] == 0.f);
		TEST_CHECK(m.m[1][3] == 0.f);
		TEST_CHECK(m.m[2][3] == 0.f);
	}
#endif
	{
		Mat4 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
		TEST_CHECK(m.m[0][0] == 1.f);
		TEST_CHECK(m.m[1][0] == 2.f);
		TEST_CHECK(m.m[2][0] == 3.f);

		TEST_CHECK(m.m[0][1] == 4.f);
		TEST_CHECK(m.m[1][1] == 5.f);
		TEST_CHECK(m.m[2][1] == 6.f);

		TEST_CHECK(m.m[0][2] == 7.f);
		TEST_CHECK(m.m[1][2] == 8.f);
		TEST_CHECK(m.m[2][2] == 9.f);

		TEST_CHECK(m.m[0][3] == 10.f);
		TEST_CHECK(m.m[1][3] == 11.f);
		TEST_CHECK(m.m[2][3] == 12.f);
	}
	{
		Mat4 n(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
		Mat4 m(n);
		TEST_CHECK(m.m[0][0] == n.m[0][0]);
		TEST_CHECK(m.m[1][0] == n.m[1][0]);
		TEST_CHECK(m.m[2][0] == n.m[2][0]);

		TEST_CHECK(m.m[0][1] == n.m[0][1]);
		TEST_CHECK(m.m[1][1] == n.m[1][1]);
		TEST_CHECK(m.m[2][1] == n.m[2][1]);

		TEST_CHECK(m.m[0][2] == n.m[0][2]);
		TEST_CHECK(m.m[1][2] == n.m[1][2]);
		TEST_CHECK(m.m[2][2] == n.m[2][2]);

		TEST_CHECK(m.m[0][3] == n.m[0][3]);
		TEST_CHECK(m.m[1][3] == n.m[1][3]);
		TEST_CHECK(m.m[2][3] == n.m[2][3]);
	}
	{
		const float n[] = {1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f};
		Mat4 m(n);
		TEST_CHECK(m.m[0][0] == 1.f);
		TEST_CHECK(m.m[1][0] == 2.f);
		TEST_CHECK(m.m[2][0] == 3.f);

		TEST_CHECK(m.m[0][1] == 4.f);
		TEST_CHECK(m.m[1][1] == 5.f);
		TEST_CHECK(m.m[2][1] == 6.f);

		TEST_CHECK(m.m[0][2] == 7.f);
		TEST_CHECK(m.m[1][2] == 8.f);
		TEST_CHECK(m.m[2][2] == 9.f);

		TEST_CHECK(m.m[0][3] == 10.f);
		TEST_CHECK(m.m[1][3] == 11.f);
		TEST_CHECK(m.m[2][3] == 12.f);
	}
	{
		const Mat3 n(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
		Mat4 m(n);
		TEST_CHECK(m.m[0][0] == 1.f);
		TEST_CHECK(m.m[1][0] == 2.f);
		TEST_CHECK(m.m[2][0] == 3.f);

		TEST_CHECK(m.m[0][1] == 4.f);
		TEST_CHECK(m.m[1][1] == 5.f);
		TEST_CHECK(m.m[2][1] == 6.f);

		TEST_CHECK(m.m[0][2] == 7.f);
		TEST_CHECK(m.m[1][2] == 8.f);
		TEST_CHECK(m.m[2][2] == 9.f);

		TEST_CHECK(m.m[0][3] == 0.f);
		TEST_CHECK(m.m[1][3] == 0.f);
		TEST_CHECK(m.m[2][3] == 0.f);
	}
	{ 
		Mat4 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f); 
		Mat4 n(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f); 
		Mat4 p(0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f);
		TEST_CHECK(m == n);
		TEST_CHECK((m == p) == false);
	}
	{
		Mat4 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
		Mat4 n(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
		Mat4 p(0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f);
		TEST_CHECK(m != p);
		TEST_CHECK((m != n) == false);
	}
	{
		const Mat4 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
		const Mat4 n(0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f);
		Mat4 p = m * n;
		TEST_CHECK(AlmostEqual(GetColumn(p, 0), Vec3(3.f, 3.6f, 4.2f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(p, 1), Vec3(6.6f, 8.1f, 9.6f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(p, 2), Vec3(10.2f, 12.6f, 15.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(p, 3), Vec3(23.8f, 28.1f, 32.4f), 0.00001f));
	}
	{
		Mat4 m = Mat4(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f) * 2.f;
		TEST_CHECK(AlmostEqual(GetColumn(m, 0), Vec3(2.f, 4.f, 6.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(m, 1), Vec3(8.f, 10.f, 12.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(m, 2), Vec3(14.f, 16.f, 18.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(m, 3), Vec3(20.f, 22.f, 24.f), 0.00001f));
	}
	{
		Mat4 m = Mat4(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f) / -10.f;
		TEST_CHECK(AlmostEqual(GetColumn(m, 0), Vec3(-0.1f,-0.2f,-0.3f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(m, 1), Vec3(-0.4f,-0.5f,-0.6f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(m, 2), Vec3(-0.7f,-0.8f,-0.9f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(m, 3), Vec3(-1.0f,-1.1f,-1.2f), 0.00001f));
	}
	{
		const Mat4 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
		const Mat4 n(0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f);
		Mat4 p = m + n;
		TEST_CHECK(AlmostEqual(GetColumn(p, 0), Vec3(1.1f, 2.2f, 3.3f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(p, 1), Vec3(4.4f, 5.5f, 6.6f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(p, 2), Vec3(7.7f, 8.8f, 9.9f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(p, 3), Vec3(11.0f, 12.1f, 13.2f), 0.00001f));
	}
	{
		const Mat4 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
		const Mat4 n(0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f);
		Mat4 p = m - n;
		TEST_CHECK(AlmostEqual(GetColumn(p, 0), Vec3(0.9f, 1.8f, 2.7f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(p, 1), Vec3(3.6f, 4.5f, 5.4f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(p, 2), Vec3(6.3f, 7.2f, 8.1f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(p, 3), Vec3(9.0f, 9.9f, 10.8f), 0.00001f));
	}
	{
		Vec3 v = Mat4(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f) * Vec3(0.1f, 0.2f, 0.3f);
		TEST_CHECK(AlmostEqual(v.x, 13.f, 1.e-5f));
		TEST_CHECK(AlmostEqual(v.y, 14.6f, 1.e-5f));
		TEST_CHECK(AlmostEqual(v.z, 16.2f, 1.e-5f));
	}
	{
		Vec4 v = Mat4(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f) * Vec4(0.1f,-0.2f, 0.3f,-0.4f);
		TEST_CHECK(Equal(v.x, -2.6f));
		TEST_CHECK(Equal(v.y, -2.8f));
		TEST_CHECK(AlmostEqual(v.z, -3.f));
		TEST_CHECK(Equal(v.w, -0.4f));
	}
	{
		const Mat4 m(-1.f, 2.f, -3.f, 4.f, -5.f, 6.f, -7.f, 8.f, 9.f, 10.f, -11.f, -12.f);
		Mat4 n;
		bool res = Inverse(m, n);
		TEST_CHECK(AlmostEqual(GetColumn(n, 0), Vec3(31.f, 14.f, 1.f) / 18.f, 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(n, 1), Vec3(26.f, 10.f, 2.f) / 18.f, 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(n, 2), Vec3(1.f, 2.f, 1.f) / 18.f, 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(n, 3), Vec3(-12.f, -6.f, 24.f) / 18.f, 0.00001f));
	
		Mat4 p = m * n;
		TEST_CHECK(AlmostEqual(GetColumn(p, 0), Vec3(1.f, 0.f, 0.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(p, 1), Vec3(0.f, 1.f, 0.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(p, 2), Vec3(0.f, 0.f, 1.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(p, 3), Vec3(0.f, 0.f, 0.f), 0.00001f));
	}
	{
		const Mat4 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
		Mat4 n;
		bool res = Inverse(m, n);
		TEST_CHECK(res == false);
	}
	{
		const float u = 2.f * sqrt(2.f);
		const Mat4 m(u, 0.f, -u, 2.f, u, 2.f, 2.f, -u, 2.f, -1.f, 0.5f, 2.f);
		Mat4 n = m * InverseFast(m);
		TEST_CHECK(AlmostEqual(GetColumn(n, 0), Vec3(1.f, 0.f, 0.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(n, 1), Vec3(0.f, 1.f, 0.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(n, 2), Vec3(0.f, 0.f, 1.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(n, 3), Vec3(0.f, 0.f, 0.f), 0.00001f));
	}
	{ 
		Mat4 m = Orthonormalize(Mat4(-1.f, 2.f, -3.f, 4.f, -5.f, 6.f, -7.f, 8.f, 9.f, 10.f, -11.f, -12.f));
		Vec3 i = GetX(m), j = GetY(m), k = GetZ(m), t = GetT(m);
		TEST_CHECK(Equal(Dot(i, j), 0.f));
		TEST_CHECK(Equal(Dot(j, k), 0.f));
		TEST_CHECK(Equal(Dot(k, i), 0.f));
		TEST_CHECK(Equal(Len2(i), 1.f));
		TEST_CHECK(Equal(Len2(j), 1.f));
		TEST_CHECK(Equal(Len2(k), 1.f));
		TEST_CHECK(AlmostEqual(t, Vec3(10.f, -11.f, -12.f), 0.000001f));
	}
	{ 
		Mat4 m = Normalize(Mat4(-1.f, 2.f, -3.f, 4.f, -5.f, 6.f, -7.f, 8.f, 9.f, 10.f, -11.f, -12.f));
		Vec3 i = GetX(m), j = GetY(m), k = GetZ(m), t = GetT(m);
		TEST_CHECK(Equal(Len2(i), 1.f));
		TEST_CHECK(Equal(Len2(j), 1.f));
		TEST_CHECK(Equal(Len2(k), 1.f));
		TEST_CHECK(AlmostEqual(t, Vec3(10.f, -11.f, -12.f), 0.000001f));
	}
    {
        const Vec3 u0 = Deg3(30.f, 45.f, 60.f);
        const Vec3 s0(2.f, 3.f, 4.f);
        const Vec3 t0(-4.f, 5.f, -3.f);
        Mat3 r0 = RotationMat3(u0, RO_ZYX);
        Mat4 m = TranslationMat4(t0) * Mat4(r0 * ScaleMat3(s0));
        Vec3 s, t;
        Mat3 r;
        Decompose(m, &t, &r, &s);
		TEST_CHECK(AlmostEqual(t, t0, 0.000001f));
		TEST_CHECK(AlmostEqual(s, s0, 0.000001f));
		TEST_CHECK(AlmostEqual(GetColumn(r, 0), GetColumn(r0, 0), 0.000001f));
		TEST_CHECK(AlmostEqual(GetColumn(r, 1), GetColumn(r0, 1), 0.000001f));
		TEST_CHECK(AlmostEqual(GetColumn(r, 2), GetColumn(r0, 2), 0.000001f));
    }
	{
		Mat4 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
		Vec3 in[8] = {Vec3(0.1f, 0.2f, 0.3f), Vec3(0.1f, -0.2f, 0.3f), Vec3(-0.1f, -0.2f, 0.3f), Vec3(-0.1f, 0.2f, 0.3f), Vec3(0.1f, 0.2f, -0.3f),
			Vec3(0.1f, -0.2f, -0.3f), Vec3(-0.1f, -0.2f, -0.3f), Vec3(-0.1f, 0.2f, -0.3f)};
		Vec3 out[8];
		TransformVec3(m, out, in, 8);
		TEST_CHECK(AlmostEqual(out[0], Vec3(13.f, 14.6f, 16.2f), 0.000001f));
		TEST_CHECK(AlmostEqual(out[1], Vec3(11.4f, 12.6f, 13.8f), 0.000001f));
		TEST_CHECK(AlmostEqual(out[2], Vec3(11.2f, 12.2f, 13.2f), 0.000001f));
		TEST_CHECK(AlmostEqual(out[3], Vec3(12.8f, 14.2f, 15.6f), 0.000001f));
		TEST_CHECK(AlmostEqual(out[4], Vec3(8.8f, 9.8f, 10.8f), 0.000001f));
		TEST_CHECK(AlmostEqual(out[5], Vec3(7.2f, 7.8f, 8.4f), 0.000001f));
		TEST_CHECK(AlmostEqual(out[6], Vec3(7.f, 7.4f, 7.8f), 0.000001f));
		TEST_CHECK(AlmostEqual(out[7], Vec3(8.6f, 9.4f, 10.2f), 0.000001f));
	}
	{
		Mat4 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
		Vec3 in[8] = {Vec3(0.1f, 0.2f, 0.3f), Vec3(0.1f, -0.2f, 0.3f), Vec3(-0.1f, -0.2f, 0.3f), Vec3(-0.1f, 0.2f, 0.3f), Vec3(0.1f, 0.2f, -0.3f),
			Vec3(0.1f, -0.2f, -0.3f), Vec3(-0.1f, -0.2f, -0.3f), Vec3(-0.1f, 0.2f, -0.3f)};
		Vec4 out[8];
		TransformVec3(m, out, in, 8);
		TEST_CHECK(AlmostEqual(out[0], Vec4(13.f, 14.6f, 16.2f, 1.f), 0.000001f));
		TEST_CHECK(AlmostEqual(out[1], Vec4(11.4f, 12.6f, 13.8f, 1.f), 0.000001f));
		TEST_CHECK(AlmostEqual(out[2], Vec4(11.2f, 12.2f, 13.2f, 1.f), 0.000001f));
		TEST_CHECK(AlmostEqual(out[3], Vec4(12.8f, 14.2f, 15.6f, 1.f), 0.000001f));
		TEST_CHECK(AlmostEqual(out[4], Vec4(8.8f, 9.8f, 10.8f, 1.f), 0.000001f));
		TEST_CHECK(AlmostEqual(out[5], Vec4(7.2f, 7.8f, 8.4f, 1.f), 0.000001f));
		TEST_CHECK(AlmostEqual(out[6], Vec4(7.f, 7.4f, 7.8f, 1.f), 0.000001f));
		TEST_CHECK(AlmostEqual(out[7], Vec4(8.6f, 9.4f, 10.2f, 1.f), 0.000001f));
	}
	{ 
		Mat4 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
		Vec3 in[8] = {Vec3(0.1f, 0.2f, 0.3f), Vec3(0.1f, -0.2f, 0.3f), Vec3(-0.1f, -0.2f, 0.3f), Vec3(-0.1f, 0.2f, 0.3f), Vec3(0.1f, 0.2f, -0.3f),
			Vec3(0.1f, -0.2f, -0.3f), Vec3(-0.1f, -0.2f, -0.3f), Vec3(-0.1f, 0.2f, -0.3f)};
		Vec3 out[8];
		RotateVec3(m, out, in, 8);
		TEST_CHECK(AlmostEqual(out[0], Vec3(3.f, 3.6f, 4.2f), 0.000001f));
		TEST_CHECK(AlmostEqual(out[1], Vec3(1.4f, 1.6f, 1.8f), 0.000001f));
		TEST_CHECK(AlmostEqual(out[2], Vec3(1.2f, 1.2f, 1.2f), 0.000001f));
		TEST_CHECK(AlmostEqual(out[3], Vec3(2.8f, 3.2f, 3.6f), 0.000001f));
		TEST_CHECK(AlmostEqual(out[4], Vec3(-1.2f, -1.2f, -1.2f), 0.000001f));
		TEST_CHECK(AlmostEqual(out[5], Vec3(-2.8f, -3.2f, -3.6f), 0.000001f));
		TEST_CHECK(AlmostEqual(out[6], Vec3(-3.f, -3.6f, -4.2f), 0.000001f));
		TEST_CHECK(AlmostEqual(out[7], Vec3(-1.4f, -1.6f, -1.8f), 0.000001f));

	}
	{
		Mat4 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
		TEST_CHECK(AlmostEqual(GetX(m), Vec3(1.f, 2.f, 3.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetY(m), Vec3(4.f, 5.f, 6.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetZ(m), Vec3(7.f, 8.f, 9.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetT(m), Vec3(10.f, 11.f, 12.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetTranslation(m), Vec3(10.f, 11.f, 12.f), 0.00001f));
	}
	{
		const Vec3 u = Deg3(30.f, 45.f, 60.f);
        TEST_CHECK(AlmostEqual(GetR(RotationMat4(u, RO_ZYX), RO_ZYX), u, 0.000001f));
        TEST_CHECK(AlmostEqual(GetR(RotationMat4(u, RO_YZX), RO_YZX), u, 0.000001f));
        TEST_CHECK(AlmostEqual(GetR(RotationMat4(u, RO_ZXY), RO_ZXY), u, 0.000001f));
        TEST_CHECK(AlmostEqual(GetR(RotationMat4(u, RO_XZY), RO_XZY), u, 0.000001f));
        TEST_CHECK(AlmostEqual(GetR(RotationMat4(u, RO_YXZ), RO_YXZ), u, 0.000001f));
        TEST_CHECK(AlmostEqual(GetR(RotationMat4(u, RO_XYZ), RO_XYZ), u, 0.000001f));
        TEST_CHECK(AlmostEqual(GetR(RotationMat4(u, RO_XY), RO_XY), u, 0.000001f));

        TEST_CHECK(AlmostEqual(GetRotation(RotationMat4(u, RO_ZYX), RO_ZYX), u, 0.000001f));
        TEST_CHECK(AlmostEqual(GetRotation(RotationMat4(u, RO_YZX), RO_YZX), u, 0.000001f));
        TEST_CHECK(AlmostEqual(GetRotation(RotationMat4(u, RO_ZXY), RO_ZXY), u, 0.000001f));
        TEST_CHECK(AlmostEqual(GetRotation(RotationMat4(u, RO_XZY), RO_XZY), u, 0.000001f));
        TEST_CHECK(AlmostEqual(GetRotation(RotationMat4(u, RO_YXZ), RO_YXZ), u, 0.000001f));
        TEST_CHECK(AlmostEqual(GetRotation(RotationMat4(u, RO_XYZ), RO_XYZ), u, 0.000001f));
        TEST_CHECK(AlmostEqual(GetRotation(RotationMat4(u, RO_XY), RO_XY), u, 0.000001f));
	}
	{
		const Vec3 theta = Deg3(30.f, -60.f, 45.f);
		const float cx = cos(theta.x), sx = sin(theta.x);
		const float cy = cos(theta.y), sy = sin(theta.y);
		const float cz = cos(theta.z), sz = sin(theta.z);
		const Mat4 rx = Mat4(1.f, 0.f, 0.f, 0.f, cx, sx, 0.f, -sx, cx, 0.f, 0.f, 0.f);
		const Mat4 ry = Mat4(cy, 0.f, -sy, 0.f, 1.f, 0.f, sy, 0.f, cy, 0.f, 0.f, 0.f);
		const Mat4 rz = Mat4(cz, sz, 0.f, -sz, cz, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f);
		const Mat4 ryxz = ry * rx * rz;
		const Mat4 rzyx = rz * ry * rx;
		const Mat4 t = Mat4(1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, -4.f, 3.f, 1.f);
		{
			Mat3 r = GetRMatrix(ryxz * t);
			TEST_CHECK(AlmostEqual(GetColumn(r, 0), GetColumn(ryxz, 0), 0.00001f));
			TEST_CHECK(AlmostEqual(GetColumn(r, 1), GetColumn(ryxz, 1), 0.00001f));
			TEST_CHECK(AlmostEqual(GetColumn(r, 2), GetColumn(ryxz, 2), 0.00001f));
			TEST_CHECK(AlmostEqual(ToEuler(r, RO_YXZ), theta, 0.00001f));
		}
		{
			Mat3 r = GetRotationMatrix(rzyx * t);
			TEST_CHECK(AlmostEqual(GetColumn(r, 0), GetColumn(rzyx, 0), 0.00001f));
			TEST_CHECK(AlmostEqual(GetColumn(r, 1), GetColumn(rzyx, 1), 0.00001f));
			TEST_CHECK(AlmostEqual(GetColumn(r, 2), GetColumn(rzyx, 2), 0.00001f));
			TEST_CHECK(AlmostEqual(ToEuler(r, RO_ZYX), theta, 0.00001f));
		}
	}
	{
		const Vec3 scale(2.f, 1.f, 4.f);
		const Mat4 ms = Mat4(scale.x, 0.f, 0.f, 0.f, scale.y, 0.f, 0.f, 0.f, scale.z, 0.f, 0.f, 0.f);
		const Mat4 mrt = RotationMat4(Deg3(-60.f, 45.f, 30.f)) * Mat4(1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, -4.f, 3.f, 1.f);
		TEST_CHECK(AlmostEqual(GetS(mrt * ms), scale, 0.00001f));
		TEST_CHECK(AlmostEqual(GetScale(ms), scale, 0.00001f));
	}
	{
		Mat4 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
		SetX(m, Vec3(-0.1f,-0.2f,-0.3f));
		TEST_CHECK(AlmostEqual(GetX(m), Vec3(-0.1f,-0.2f,-0.3f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetY(m), Vec3(4.f, 5.f, 6.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetZ(m), Vec3(7.f, 8.f, 9.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetT(m), Vec3(10.f, 11.f, 12.f), 0.00001f));
	}
	{
		Mat4 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
		SetY(m, Vec3(-0.1f,-0.2f,-0.3f));
		TEST_CHECK(AlmostEqual(GetX(m), Vec3(1.f, 2.f, 3.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetY(m), Vec3(-0.1f,-0.2f,-0.3f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetZ(m), Vec3(7.f, 8.f, 9.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetT(m), Vec3(10.f, 11.f, 12.f), 0.00001f));
	}
	{
		Mat4 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
		SetZ(m, Vec3(-0.1f,-0.2f,-0.3f));
		TEST_CHECK(AlmostEqual(GetX(m), Vec3(1.f, 2.f, 3.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetY(m), Vec3(4.f, 5.f, 6.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetZ(m), Vec3(-0.1f,-0.2f,-0.3f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetT(m), Vec3(10.f, 11.f, 12.f), 0.00001f));
	}
	{
		Mat4 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
		SetT(m, Vec3(-0.1f,-0.2f,-0.3f));
		TEST_CHECK(AlmostEqual(GetX(m), Vec3(1.f, 2.f, 3.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetY(m), Vec3(4.f, 5.f, 6.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetZ(m), Vec3(7.f, 8.f, 9.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetT(m), Vec3(-0.1f,-0.2f,-0.3f), 0.00001f));
	}
	{
		Mat4 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
		SetTranslation(m, Vec3(-0.1f,-0.2f,-0.3f));
		TEST_CHECK(AlmostEqual(GetX(m), Vec3(1.f, 2.f, 3.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetY(m), Vec3(4.f, 5.f, 6.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetZ(m), Vec3(7.f, 8.f, 9.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetT(m), Vec3(-0.1f,-0.2f,-0.3f), 0.00001f));
	}
	{
		Mat4 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
		SetS(m, Vec3(0.1f, 10.0f, 2.0f));
		TEST_CHECK(Equal(Len(GetX(m)), 0.1f));
		TEST_CHECK(Equal(Len(GetY(m)), 10.0f));
		TEST_CHECK(Equal(Len(GetZ(m)), 2.0f));
		TEST_CHECK(AlmostEqual(GetT(m), Vec3(10.f, 11.f, 12.f), 0.00001f));
	}
	{
		Mat4 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
		SetScale(m, Vec3(3.0f, 0.5f, 11.0f));
		TEST_CHECK(Equal(Len(GetX(m)), 3.0f));
		TEST_CHECK(Equal(Len(GetY(m)), 0.5f));
		TEST_CHECK(Equal(Len(GetZ(m)), 11.0f));
		TEST_CHECK(AlmostEqual(GetT(m), Vec3(10.f, 11.f, 12.f), 0.00001f));
	}
	{
		const float n[] = {1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f};
		Mat4 m(0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.f, 1.1f, 1.2f);
		Set(m, n);
		TEST_CHECK(m.m[0][0] == 1.f);
		TEST_CHECK(m.m[1][0] == 2.f);
		TEST_CHECK(m.m[2][0] == 3.f);

		TEST_CHECK(m.m[0][1] == 4.f);
		TEST_CHECK(m.m[1][1] == 5.f);
		TEST_CHECK(m.m[2][1] == 6.f);

		TEST_CHECK(m.m[0][2] == 7.f);
		TEST_CHECK(m.m[1][2] == 8.f);
		TEST_CHECK(m.m[2][2] == 9.f);

		TEST_CHECK(m.m[0][3] == 10.f);
		TEST_CHECK(m.m[1][3] == 11.f);
		TEST_CHECK(m.m[2][3] == 12.f);
	}
	{
		Mat4 m(0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.f, 1.1f, 1.2f);
		Set(m, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
		TEST_CHECK(m.m[0][0] == 1.f);
		TEST_CHECK(m.m[1][0] == 2.f);
		TEST_CHECK(m.m[2][0] == 3.f);

		TEST_CHECK(m.m[0][1] == 4.f);
		TEST_CHECK(m.m[1][1] == 5.f);
		TEST_CHECK(m.m[2][1] == 6.f);

		TEST_CHECK(m.m[0][2] == 7.f);
		TEST_CHECK(m.m[1][2] == 8.f);
		TEST_CHECK(m.m[2][2] == 9.f);

		TEST_CHECK(m.m[0][3] == 10.f);
		TEST_CHECK(m.m[1][3] == 11.f);
		TEST_CHECK(m.m[2][3] == 12.f);
	}
	{
		Mat4 m = Mat4(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
		TEST_CHECK(AlmostEqual(GetRow(m, 0), Vec4(1.f, 4.f, 7.f, 10.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetRow(m, 1), Vec4(2.f, 5.f, 8.f, 11.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetRow(m, 2), Vec4(3.f, 6.f, 9.f, 12.f), 0.00001f));
	}
	{
		Mat4 m = Mat4(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
		TEST_CHECK(AlmostEqual(GetColumn(m, 0), Vec3(1.f, 2.f, 3.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(m, 1), Vec3(4.f, 5.f, 6.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(m, 2), Vec3(7.f, 8.f, 9.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(m, 3), Vec3(10.f, 11.f, 12.f), 0.00001f));
	}
	{
		{
			Mat4 m = Mat4(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
			SetRow(m, 0, Vec4(0.1f, 0.2f, 0.3f, 0.4f));
			TEST_CHECK(AlmostEqual(GetRow(m, 0), Vec4(0.1f, 0.2f, 0.3f, 0.4f), 0.00001f));
			TEST_CHECK(AlmostEqual(GetRow(m, 1), Vec4(2.f, 5.f, 8.f, 11.f), 0.00001f));
			TEST_CHECK(AlmostEqual(GetRow(m, 2), Vec4(3.f, 6.f, 9.f, 12.f), 0.00001f));
		}
		{
			Mat4 m = Mat4(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
			SetRow(m, 1, Vec4(0.1f, 0.2f, 0.3f, 0.4f));
			TEST_CHECK(AlmostEqual(GetRow(m, 0), Vec4(1.f, 4.f, 7.f, 10.f), 0.00001f));
			TEST_CHECK(AlmostEqual(GetRow(m, 1), Vec4(0.1f, 0.2f, 0.3f, 0.4f), 0.00001f));
			TEST_CHECK(AlmostEqual(GetRow(m, 2), Vec4(3.f, 6.f, 9.f, 12.f), 0.00001f));
		}
		{
			Mat4 m = Mat4(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
			SetRow(m, 2, Vec4(0.1f, 0.2f, 0.3f, 0.4f));
			TEST_CHECK(AlmostEqual(GetRow(m, 0), Vec4(1.f, 4.f, 7.f, 10.f), 0.00001f));
			TEST_CHECK(AlmostEqual(GetRow(m, 1), Vec4(2.f, 5.f, 8.f, 11.f), 0.00001f));
			TEST_CHECK(AlmostEqual(GetRow(m, 2), Vec4(0.1f, 0.2f, 0.3f, 0.4f), 0.00001f));
		}
	}
	{
		{
			Mat4 m = Mat4(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
			SetColumn(m, 0, Vec3(-0.1f, -0.2f, -0.3f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 0), Vec3(-0.1f, -0.2f, -0.3f), 0.00001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 1), Vec3(4.f, 5.f, 6.f), 0.00001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 2), Vec3(7.f, 8.f, 9.f), 0.00001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 3), Vec3(10.f, 11.f, 12.f), 0.00001f));
		}
		{
			Mat4 m = Mat4(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
			SetColumn(m, 1, Vec3(-0.1f, -0.2f, -0.3f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 0), Vec3(1.f, 2.f, 3.f), 0.00001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 1), Vec3(-0.1f, -0.2f, -0.3f), 0.00001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 2), Vec3(7.f, 8.f, 9.f), 0.00001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 3), Vec3(10.f, 11.f, 12.f), 0.00001f));
		}
		{
			Mat4 m = Mat4(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
			SetColumn(m, 2, Vec3(-0.1f, -0.2f, -0.3f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 0), Vec3(1.f, 2.f, 3.f), 0.00001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 1), Vec3(4.f, 5.f, 6.f), 0.00001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 2), Vec3(-0.1f, -0.2f, -0.3f), 0.00001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 3), Vec3(10.f, 11.f, 12.f), 0.00001f));
		}
		{
			Mat4 m = Mat4(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
			SetColumn(m, 3, Vec3(-0.1f, -0.2f, -0.3f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 0), Vec3(1.f, 2.f, 3.f), 0.00001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 1), Vec3(4.f, 5.f, 6.f), 0.00001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 2), Vec3(7.f, 8.f, 9.f), 0.00001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 3), Vec3(-0.1f, -0.2f, -0.3f), 0.00001f));
		}
	}
    {
        const Vec3 eye(-1.f, 2.f, -5.f);
        const Vec3 target(0.5f, 0.6f, 4.f);
        const Vec3 scale(2.f, 3.f, 4.f);

        {
            Mat4 m = Mat4LookAt(eye, target);
            Vec3 i = GetX(m), j = GetY(m), k = GetZ(m), t = GetT(m);
            TEST_CHECK(Equal(Dot(i,j), 0.f));
            TEST_CHECK(Equal(Dot(j,k), 0.f));
            TEST_CHECK(Equal(Dot(k,i), 0.f));
            TEST_CHECK(AlmostEqual(Dot(k, Normalize(target - eye)), 1.f, 0.000001f));
            TEST_CHECK(AlmostEqual(eye, t, 0.000001f));
        }

        {
            Mat4 m = Mat4LookAt(eye, target, scale);
            Vec3 i = GetX(m), j = GetY(m), k = GetZ(m), t = GetT(m);
            TEST_CHECK(Equal(Dot(i,j), 0.f));
            TEST_CHECK(Equal(Dot(j,k), 0.f));
            TEST_CHECK(Equal(Dot(k,i), 0.f));
            TEST_CHECK(AlmostEqual(Dot(Normalize(k), Normalize(target - eye)), 1.f, 0.000001f));
            TEST_CHECK(AlmostEqual(GetScale(m), scale, 0.000001f));
        }
    }
    {
        const Vec3 eye(-1.f, 2.f, -5.f);
        const Vec3 target(0.5f, 0.6f, 4.f);
        const Vec3 scale(2.f, 3.f, 4.f);
        {
            Mat4 m = Mat4LookAtUp(eye, target, Vec3(0.f, 1.f, 0.f));
            Vec3 i = GetX(m), j = GetY(m), k = GetZ(m), t = GetT(m);
            TEST_CHECK(Equal(Dot(i,j), 0.f));
            TEST_CHECK(Equal(Dot(j,k), 0.f));
            TEST_CHECK(Equal(Dot(k,i), 0.f));
            TEST_CHECK(AlmostEqual(Dot(k, Normalize(target - eye)), 1.f, 0.000001f));
            TEST_CHECK(AlmostEqual(eye, t, 0.000001f));
        }
        {
            Vec3 up(0.f, -1.f, 0.f);
            Mat4 m = Mat4LookAtUp(eye, target, up);
            Vec3 i = GetX(m), j = GetY(m), k = GetZ(m), t = GetT(m);
            TEST_CHECK(Equal(Dot(i,j), 0.f));
            TEST_CHECK(Equal(Dot(j,k), 0.f));
            TEST_CHECK(Equal(Dot(k,i), 0.f));
            TEST_CHECK(AlmostEqual(Dot(k, Normalize(target - eye)), 1.f, 0.000001f));
            TEST_CHECK(AlmostEqual(eye, t, 0.000001f));
            TEST_CHECK(Dot(j, up) > 0.f);
            TEST_CHECK(Dot(i, Vec3(-1.f, 0.f, 0.f)) > 0.f);
        }
        {
            Vec3 up(1.f, 0.f, 0.f);
            Mat4 m = Mat4LookAtUp(eye, target, up);
            Vec3 i = GetX(m), j = GetY(m), k = GetZ(m), t = GetT(m);
            TEST_CHECK(Equal(Dot(i,j), 0.f));
            TEST_CHECK(Equal(Dot(j,k), 0.f));
            TEST_CHECK(Equal(Dot(k,i), 0.f));
            TEST_CHECK(AlmostEqual(Dot(Normalize(k), Normalize(target - eye)), 1.f, 0.000001f));
            TEST_CHECK(Dot(j, up) > 0.f);
            TEST_CHECK(Dot(i, Vec3(0.f, -1.f, 0.f)) > 0.f);
        }
    }
    {
        const Vec3 eye(0.f, 0.f, 0.f);
        const Vec3 direction = Normalize(Vec3(1.f, 1.f, 1.f));
        {
            Mat4 m = Mat4LookToward(eye, direction);
            Vec3 i = GetX(m), j = GetY(m), k = GetZ(m), t = GetT(m);
            TEST_CHECK(Equal(Dot(i,j), 0.f));
            TEST_CHECK(Equal(Dot(j,k), 0.f));
            TEST_CHECK(Equal(Dot(k,i), 0.f));
            TEST_CHECK(Equal(Dot(k,direction), 1.f));
            TEST_CHECK(AlmostEqual(eye, t, 0.000001f));
        }
        {
            Vec3 up(0.f, -1.f, 0.f);
            Mat4 m = Mat4LookTowardUp(eye, direction, up);
            Vec3 i = GetX(m), j = GetY(m), k = GetZ(m), t = GetT(m);
            TEST_CHECK(Equal(Dot(i,j), 0.f));
            TEST_CHECK(Equal(Dot(j,k), 0.f));
            TEST_CHECK(Equal(Dot(k,i), 0.f));
            TEST_CHECK(Equal(Dot(k,direction), 1.f));
            TEST_CHECK(AlmostEqual(eye, t, 0.000001f));
            TEST_CHECK(Dot(j, up) > 0.f);
        }
    }
    {
        const Vec3 v(-0.1f, -0.2f, -0.3f);
        Mat4 m = TranslationMat4(v);
        TEST_CHECK(AlmostEqual(GetColumn(m, 0), Vec3(1.f, 0.f, 0.f), 0.00001f));
        TEST_CHECK(AlmostEqual(GetColumn(m, 1), Vec3(0.f, 1.f, 0.f), 0.00001f));
        TEST_CHECK(AlmostEqual(GetColumn(m, 2), Vec3(0.f, 0.f, 1.f), 0.00001f));
        TEST_CHECK(AlmostEqual(GetColumn(m, 3), v, 0.00001f));
        TEST_CHECK(AlmostEqual(m * Vec3::Zero, v, 0.00001f));
    }
    {
        const Vec3 u = Deg3(30.f, 45.f, 60.f);
        {
            const Mat3 r = RotationMat3(u, RO_ZYX);
            Mat4 m = RotationMat4(u, RO_ZYX);
            TEST_CHECK(AlmostEqual(GetColumn(m, 0), GetColumn(r, 0), 0.00001f));
            TEST_CHECK(AlmostEqual(GetColumn(m, 1), GetColumn(r, 1), 0.00001f));
            TEST_CHECK(AlmostEqual(GetColumn(m, 2), GetColumn(r, 2), 0.00001f));
            TEST_CHECK(AlmostEqual(GetColumn(m, 3), Vec3(0.f, 0.f, 0.f), 0.00001f));
        }
        {
            const Mat3 r = RotationMat3(u, RO_YZX);
            Mat4 m = RotationMat4(u, RO_YZX);
            TEST_CHECK(AlmostEqual(GetColumn(m, 0), GetColumn(r, 0), 0.00001f));
            TEST_CHECK(AlmostEqual(GetColumn(m, 1), GetColumn(r, 1), 0.00001f));
            TEST_CHECK(AlmostEqual(GetColumn(m, 2), GetColumn(r, 2), 0.00001f));
            TEST_CHECK(AlmostEqual(GetColumn(m, 3), Vec3(0.f, 0.f, 0.f), 0.00001f));
        }
        {
            const Mat3 r = RotationMat3(u, RO_ZXY);
            Mat4 m = RotationMat4(u, RO_ZXY);
            TEST_CHECK(AlmostEqual(GetColumn(m, 0), GetColumn(r, 0), 0.00001f));
            TEST_CHECK(AlmostEqual(GetColumn(m, 1), GetColumn(r, 1), 0.00001f));
            TEST_CHECK(AlmostEqual(GetColumn(m, 2), GetColumn(r, 2), 0.00001f));
            TEST_CHECK(AlmostEqual(GetColumn(m, 3), Vec3(0.f, 0.f, 0.f), 0.00001f));
        }
        {
            const Mat3 r = RotationMat3(u, RO_XZY);
            Mat4 m = RotationMat4(u, RO_XZY);
            TEST_CHECK(AlmostEqual(GetColumn(m, 0), GetColumn(r, 0), 0.00001f));
            TEST_CHECK(AlmostEqual(GetColumn(m, 1), GetColumn(r, 1), 0.00001f));
            TEST_CHECK(AlmostEqual(GetColumn(m, 2), GetColumn(r, 2), 0.00001f));
            TEST_CHECK(AlmostEqual(GetColumn(m, 3), Vec3(0.f, 0.f, 0.f), 0.00001f));
        }
        {
            const Mat3 r = RotationMat3(u, RO_YXZ);
            Mat4 m = RotationMat4(u, RO_YXZ);
            TEST_CHECK(AlmostEqual(GetColumn(m, 0), GetColumn(r, 0), 0.00001f));
            TEST_CHECK(AlmostEqual(GetColumn(m, 1), GetColumn(r, 1), 0.00001f));
            TEST_CHECK(AlmostEqual(GetColumn(m, 2), GetColumn(r, 2), 0.00001f));
            TEST_CHECK(AlmostEqual(GetColumn(m, 3), Vec3(0.f, 0.f, 0.f), 0.00001f));
        }
        {
            const Mat3 r = RotationMat3(u, RO_XYZ);
            Mat4 m = RotationMat4(u, RO_XYZ);
            TEST_CHECK(AlmostEqual(GetColumn(m, 0), GetColumn(r, 0), 0.00001f));
            TEST_CHECK(AlmostEqual(GetColumn(m, 1), GetColumn(r, 1), 0.00001f));
            TEST_CHECK(AlmostEqual(GetColumn(m, 2), GetColumn(r, 2), 0.00001f));
            TEST_CHECK(AlmostEqual(GetColumn(m, 3), Vec3(0.f, 0.f, 0.f), 0.00001f));
        }
        {
            const Mat3 r = RotationMat3(u, RO_XY);
            Mat4 m = RotationMat4(u, RO_XY);
            TEST_CHECK(AlmostEqual(GetColumn(m, 0), GetColumn(r, 0), 0.00001f));
            TEST_CHECK(AlmostEqual(GetColumn(m, 1), GetColumn(r, 1), 0.00001f));
            TEST_CHECK(AlmostEqual(GetColumn(m, 2), GetColumn(r, 2), 0.00001f));
            TEST_CHECK(AlmostEqual(GetColumn(m, 3), Vec3(0.f, 0.f, 0.f), 0.00001f));
        }
    }
    {
        const Vec3 v(-0.1f, -0.2f, -0.3f);
        Mat4 m = ScaleMat4(v);
        TEST_CHECK(AlmostEqual(GetColumn(m, 0), Vec3(-0.1f, 0.f, 0.f), 0.00001f));
        TEST_CHECK(AlmostEqual(GetColumn(m, 1), Vec3(0.f,-0.2f, 0.f), 0.00001f));
        TEST_CHECK(AlmostEqual(GetColumn(m, 2), Vec3(0.f, 0.f,-0.3f), 0.00001f));
        TEST_CHECK(AlmostEqual(GetColumn(m, 3), Vec3::Zero, 0.00001f));
    }
    {
        Mat4 m = ScaleMat4(3.1);
        TEST_CHECK(AlmostEqual(GetColumn(m, 0), Vec3(3.1f, 0.f, 0.f), 0.00001f));
        TEST_CHECK(AlmostEqual(GetColumn(m, 1), Vec3(0.f,3.1f, 0.f), 0.00001f));
        TEST_CHECK(AlmostEqual(GetColumn(m, 2), Vec3(0.f, 0.f,3.1f), 0.00001f));
        TEST_CHECK(AlmostEqual(GetColumn(m, 3), Vec3::Zero, 0.00001f));
    }
    {
        const Vec3 t(2.f, -1.f, 5.f);
        const Vec3 r = Deg3(30.f, 45.f, -60.f);
        Mat4 m = TransformationMat4(t, r);
        TEST_CHECK(AlmostEqual(GetR(m), r, 0.00001f));
        TEST_CHECK(AlmostEqual(GetT(m), t, 0.00001f));
        TEST_CHECK(AlmostEqual(m * Vec3::Zero, t, 0.00001f));
        Mat4 n = Mat4(RotationMat3(-r, ReverseRotationOrder(RO_Default))) * TranslationMat4(-t) * m;
        TEST_CHECK(AlmostEqual(GetColumn(n, 0), Vec3(1.f, 0.f, 0.f), 0.000001f));
        TEST_CHECK(AlmostEqual(GetColumn(n, 1), Vec3(0.f, 1.f, 0.f), 0.000001f));
        TEST_CHECK(AlmostEqual(GetColumn(n, 2), Vec3(0.f, 0.f, 1.f), 0.000001f));
        TEST_CHECK(AlmostEqual(GetColumn(n, 3), Vec3(0.f, 0.f, 0.f), 0.000001f));
        
    }
    {
        const Vec3 t(2.f, -1.f, 5.f);
        const Vec3 r = Deg3(30.f, 45.f, -60.f);
        const Vec3 s = Deg3(2.f, 5.f, 8.f);
        Mat4 m = TransformationMat4(t, r, s);
        TEST_CHECK(AlmostEqual(GetR(m), r, 0.00001f));
        TEST_CHECK(AlmostEqual(GetT(m), t, 0.00001f));
        TEST_CHECK(AlmostEqual(GetS(m), s, 0.00001f));
        Mat4 n = Mat4(ScaleMat3(Inverse(s))) * Mat4(RotationMat3(-r, ReverseRotationOrder(RO_Default))) * TranslationMat4(-t) * m;
		TEST_CHECK(AlmostEqual(GetColumn(n, 0), Vec3(1.f, 0.f, 0.f), 1.e-5f));
		TEST_CHECK(AlmostEqual(GetColumn(n, 1), Vec3(0.f, 1.f, 0.f), 1.e-5f));
		TEST_CHECK(AlmostEqual(GetColumn(n, 2), Vec3(0.f, 0.f, 1.f), 1.e-5f));
        TEST_CHECK(AlmostEqual(GetColumn(n, 3), Vec3(0.f, 0.f, 0.f), 1.e-5f));
    }
    {
        const Vec3 t(2.f, -1.f, 5.f);
        const RotationOrder ro = RO_XZY;
        const Vec3 euler = Deg3(30.f, 45.f, -60.f);
        const Mat3 r = RotationMat3(euler, ro);
        const Vec3 s = Deg3(2.f, 5.f, 8.f);
        Mat4 m = TransformationMat4(t, r, s);
        TEST_CHECK(AlmostEqual(GetR(m, ro), euler, 0.00001f));
        TEST_CHECK(AlmostEqual(GetT(m), t, 0.00001f));
        TEST_CHECK(AlmostEqual(GetS(m), s, 0.00001f));
        Mat4 n = Mat4(ScaleMat3(Inverse(s))) * Mat4(Transpose(r)) * TranslationMat4(-t) * m;
		TEST_CHECK(AlmostEqual(GetColumn(n, 0), Vec3(1.f, 0.f, 0.f), 1.e-5f));
		TEST_CHECK(AlmostEqual(GetColumn(n, 1), Vec3(0.f, 1.f, 0.f), 1.e-5f));
		TEST_CHECK(AlmostEqual(GetColumn(n, 2), Vec3(0.f, 0.f, 1.f), 1.e-5f));
		TEST_CHECK(AlmostEqual(GetColumn(n, 3), Vec3(0.f, 0.f, 0.f), 1.e-5f));
    }
	{ 
		const float transposed[] = { 
			1.f, 2.f, 3.f, 0.f,
			4.f, 5.f, 6.f, 0.f, 
			7.f, 8.f, 9.f, 0.f, 
			10.f, 11.f, 12.f, 1.f
		};
		Mat4 m = Mat4FromFloat16Transposed(transposed);
		TEST_CHECK(AlmostEqual(GetColumn(m, 0), Vec3(1.f, 2.f, 3.f), 0.000001f));
		TEST_CHECK(AlmostEqual(GetColumn(m, 1), Vec3(4.f, 5.f, 6.f), 0.000001f));
		TEST_CHECK(AlmostEqual(GetColumn(m, 2), Vec3(7.f, 8.f, 9.f), 0.000001f));
		TEST_CHECK(AlmostEqual(GetColumn(m, 3), Vec3(10.f,11.f,12.f), 0.000001f));
	}
	{
		const Mat4 m = Mat4(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
		float u[16];
		Mat4ToFloat16Transposed(m, u);
		TEST_CHECK(Equal(u[0], 1.f));
		TEST_CHECK(Equal(u[1], 2.f));
		TEST_CHECK(Equal(u[2], 3.f));
		TEST_CHECK(Equal(u[3], 0.f));
		TEST_CHECK(Equal(u[4], 4.f));
		TEST_CHECK(Equal(u[5], 5.f));
		TEST_CHECK(Equal(u[6], 6.f));
		TEST_CHECK(Equal(u[7], 0.f));
		TEST_CHECK(Equal(u[8], 7.f));
		TEST_CHECK(Equal(u[9], 8.f));
		TEST_CHECK(Equal(u[10], 9.f));
		TEST_CHECK(Equal(u[11], 0.f));
		TEST_CHECK(Equal(u[12], 10.f));
		TEST_CHECK(Equal(u[13], 11.f));
		TEST_CHECK(Equal(u[14], 12.f));
		TEST_CHECK(Equal(u[15], 1.f));
	}
// Mat4 LerpAsOrthonormalBase(const Mat4 &a, const Mat4 &b, float k, bool fast = false);
// Mat4 ComputeBillboardMat4(const Vec3 &pos, const Mat3 &camera, const Vec3 &scale = Vec3::One);
}
