// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <math.h>

#define TEST_NO_MAIN 
#include "acutest.h"

#include "foundation/matrix44.h"

#include "foundation/math.h"
#include "foundation/matrix4.h"
#include "foundation/vector3.h"
#include "foundation/vector4.h"

using namespace hg;

void test_mat44() {
#if 0
	{ 
		Mat44 m;
		TEST_CHECK(m.m[0][0] == 1.f);
		TEST_CHECK(m.m[0][1] == 0.f);
		TEST_CHECK(m.m[0][2] == 0.f);
		TEST_CHECK(m.m[0][3] == 0.f);

		TEST_CHECK(m.m[1][0] == 0.f);
		TEST_CHECK(m.m[1][1] == 1.f);
		TEST_CHECK(m.m[1][2] == 0.f);
		TEST_CHECK(m.m[1][3] == 0.f);

		TEST_CHECK(m.m[2][0] == 0.f);
		TEST_CHECK(m.m[2][1] == 0.f);
		TEST_CHECK(m.m[2][2] == 1.f);
		TEST_CHECK(m.m[2][3] == 0.f);

		TEST_CHECK(m.m[3][0] == 0.f);
		TEST_CHECK(m.m[3][1] == 0.f);
		TEST_CHECK(m.m[3][2] == 0.f);
		TEST_CHECK(m.m[3][3] == 1.f);
	}
#endif
	{
		Mat44 m(0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f);
		TEST_CHECK(m.m[0][0] == 0.f);
		TEST_CHECK(m.m[1][0] == 1.f);
		TEST_CHECK(m.m[2][0] == 2.f);
		TEST_CHECK(m.m[3][0] == 3.f);

		TEST_CHECK(m.m[0][1] == 4.f);
		TEST_CHECK(m.m[1][1] == 5.f);
		TEST_CHECK(m.m[2][1] == 6.f);
		TEST_CHECK(m.m[3][1] == 7.f);

		TEST_CHECK(m.m[0][2] == 8.f);
		TEST_CHECK(m.m[1][2] == 9.f);
		TEST_CHECK(m.m[2][2] == 10.f);
		TEST_CHECK(m.m[3][2] == 11.f);

		TEST_CHECK(m.m[0][3] == 12.f);
		TEST_CHECK(m.m[1][3] == 13.f);
		TEST_CHECK(m.m[2][3] == 14.f);
		TEST_CHECK(m.m[3][3] == 15.f);
	}
	{
		Mat44 n(0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f);
		Mat44 m(n);
		TEST_CHECK(m.m[0][0] == n.m[0][0]);
		TEST_CHECK(m.m[0][1] == n.m[0][1]);
		TEST_CHECK(m.m[0][2] == n.m[0][2]);
		TEST_CHECK(m.m[0][3] == n.m[0][3]);

		TEST_CHECK(m.m[1][0] == n.m[1][0]);
		TEST_CHECK(m.m[1][1] == n.m[1][1]);
		TEST_CHECK(m.m[1][2] == n.m[1][2]);
		TEST_CHECK(m.m[1][3] == n.m[1][3]);

		TEST_CHECK(m.m[2][0] == n.m[2][0]);
		TEST_CHECK(m.m[2][1] == n.m[2][1]);
		TEST_CHECK(m.m[2][2] == n.m[2][2]);
		TEST_CHECK(m.m[2][3] == n.m[2][3]);

		TEST_CHECK(m.m[3][0] == n.m[3][0]);
		TEST_CHECK(m.m[3][1] == n.m[3][1]);
		TEST_CHECK(m.m[3][2] == n.m[3][2]);
		TEST_CHECK(m.m[3][3] == n.m[3][3]);
	}
	{
		Mat4 n(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
		Mat44 m(n);
		TEST_CHECK(m.m[0][0] == 1.f);
		TEST_CHECK(m.m[1][0] == 2.f);
		TEST_CHECK(m.m[2][0] == 3.f);
		TEST_CHECK(m.m[3][0] == 0.f);

		TEST_CHECK(m.m[0][1] == 4.f);
		TEST_CHECK(m.m[1][1] == 5.f);
		TEST_CHECK(m.m[2][1] == 6.f);
		TEST_CHECK(m.m[3][1] == 0.f);

		TEST_CHECK(m.m[0][2] == 7.f);
		TEST_CHECK(m.m[1][2] == 8.f);
		TEST_CHECK(m.m[2][2] == 9.f);
		TEST_CHECK(m.m[3][2] == 0.f);

		TEST_CHECK(m.m[0][3] == 10.f);
		TEST_CHECK(m.m[1][3] == 11.f);
		TEST_CHECK(m.m[2][3] == 12.f);
		TEST_CHECK(m.m[3][3] == 1.f);
	}
	{ 
		Mat44 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f); 
		Mat44 n(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f); 
		Mat44 p(0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f);
		TEST_CHECK(m == n);
		TEST_CHECK((m == p) == false);
	}
	{
		Mat44 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f);
		Mat44 n(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f);
		Mat44 p(0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f);
		TEST_CHECK(m != p);
		TEST_CHECK((m != n) == false);
	}
	{
		const Mat44 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f);
		const Mat44 n(0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f);
		Mat44 p = m * n;
		TEST_CHECK(AlmostEqual(GetColumn(p, 0), Vec4(9.f, 10.f, 11.f, 12.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(p, 1), Vec4(20.2f, 22.8f, 25.4f, 28.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(p, 2), Vec4(31.4f, 35.6f, 39.8f, 44.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(p, 3), Vec4(42.6f, 48.4f, 54.2f, 60.f), 0.00001f));
	}
	{
		const Mat44 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f);
		const Mat4 n(0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f);
		Mat44 p = m * n;
		TEST_CHECK(AlmostEqual(GetColumn(p, 0), Vec4(3.8f, 4.4f, 5.f, 5.6f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(p, 1), Vec4(8.3f, 9.8f, 11.3f, 12.8f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(p, 2), Vec4(12.8f, 15.2f, 17.6f, 20.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(p, 3), Vec4(30.3f, 34.6f, 38.9f, 43.2f), 0.00001f));
	}
	{
		const Mat44 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f);
		const Mat4 n(0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f);
		Mat44 p = n * m;
		TEST_CHECK(AlmostEqual(GetColumn(p, 0), Vec4(7.f, 8.f, 9.f, 4.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(p, 1), Vec4(15.8f, 18.4f, 21.f, 8.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(p, 2), Vec4(24.6f, 28.8f, 33.f, 12.f), 0.00001f));
		TEST_CHECK(AlmostEqual(GetColumn(p, 3), Vec4(33.4f, 39.2f, 45.f, 16.f), 0.00001f));
	}
	{
		const Mat44 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f);
		const Vec3 u(0.1f, 0.2f, 0.3f);
		Vec3 v = m * u;
		TEST_CHECK(Equal(v.x, 16.8f));
		TEST_CHECK(Equal(v.y, 18.4f));
		TEST_CHECK(Equal(v.z, 20.0f));
	}
	{
		const Mat44 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f);
		const Vec4 u(0.1f, 0.2f, 0.3f, 0.4f);
		Vec4 v = m * u;
		TEST_CHECK(AlmostEqual(v.x, 9.f, 1.e-5f));
		TEST_CHECK(AlmostEqual(v.y, 10.f, 1.e-5f));
		TEST_CHECK(AlmostEqual(v.z, 11.f, 1.e-5f));
		TEST_CHECK(AlmostEqual(v.w, 12.f, 1.e-5f));
	}
	{
		const Mat44 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f);
		const Vec3 u[4] = {Vec3(0.1f, 0.2f, 0.3f), Vec3(0.4f, 0.5f, 0.6f), Vec3(0.7f, 0.8f, 0.9f), Vec3(1.0f, 1.1f, 1.2f)};
		Vec4 v[4];
		TransformVec3(m, v, u, 4);
		TEST_CHECK(AlmostEqual(v[0], Vec4(16.8f, 18.4f, 20.f, 21.6f), 1.e-5f));
		TEST_CHECK(AlmostEqual(v[1], Vec4(21.3f, 23.8f, 26.3f, 28.8f), 1.e-5f));
		TEST_CHECK(AlmostEqual(v[2], Vec4(25.8f, 29.2f, 32.6f, 36.f), 1.e-5f));
		TEST_CHECK(AlmostEqual(v[3], Vec4(30.3f, 34.6f, 38.9f, 43.2f), 1.e-5f));
	}
	{
		const Mat44 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f);
		const Vec4 u[4] = {Vec4(0.1f, 0.2f, 0.3f, 0.4f), Vec4(0.5f, 0.6f, 0.7f, 0.8f), Vec4(0.9f, 1.0f, 1.1f, 1.2f), Vec4(1.3f, 1.4f, 1.5f, 1.6f)};
		Vec4 v[4];
		TransformVec4(m, v, u, 4);
		TEST_CHECK(AlmostEqual(v[0], Vec4(9.f, 10.f, 11.f, 12.f), 0.00001f));
		TEST_CHECK(AlmostEqual(v[1], Vec4(20.2f, 22.8f, 25.4f, 28.f), 0.00001f));
		TEST_CHECK(AlmostEqual(v[2], Vec4(31.4f, 35.6f, 39.8f, 44.f), 0.00001f));
		TEST_CHECK(AlmostEqual(v[3], Vec4(42.6f, 48.4f, 54.2f, 60.f), 0.00001f));
	}
	{ 
		const Mat44 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f);
		Mat44 n = Transpose(m);
		TEST_CHECK(AlmostEqual(GetRow(n, 0), Vec4(1.f, 2.f, 3.f, 4.f), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(n, 1), Vec4(5.f, 6.f, 7.f, 8.f), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(n, 2), Vec4(9.f, 10.f, 11.f, 12.f), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(n, 3), Vec4(13.f, 14.f, 15.f, 16.f), 0.000001f));
	} 
	{ 
		const Mat44 m(-1.f, 2.f, 3.f, -4.f, 5.f, -6.f, -7.f, 8.f, -9.f, 10.f, -11.f, 12.f, 13.f, -14.f, 15.f, -16.f);
		bool res = false;
		Mat44 n = Inverse(m, res);
		TEST_CHECK(res);
		TEST_CHECK(AlmostEqual(GetColumn(n, 0), Vec4(6.f, 2.f, 14.f, 10.f) / 8.f, 0.000001f));
		TEST_CHECK(AlmostEqual(GetColumn(n, 1), Vec4(5.f, 1.f, 13.f, 9.f) / 8.f, 0.000001f));
		TEST_CHECK(AlmostEqual(GetColumn(n, 2), Vec4(-8.f, -4.f, 16.f, 12.f) / 8.f, 0.000001f));
		TEST_CHECK(AlmostEqual(GetColumn(n, 3), Vec4(-7.f, -3.f, 15.f, 11.f) / 8.f, 0.000001f));
		TEST_CHECK((m * n) == Mat44::Identity);
	}
	{
		const Mat44 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f);
		bool res = false;
		Mat44 n = Inverse(m, res);
		TEST_CHECK(res == false);
		TEST_CHECK(n == Mat44::Identity);
	}
	{
		const Mat44 m(-1.f, 2.f, 3.f, -4.f, 5.f, -6.f, -7.f, 8.f, -9.f, 10.f, -11.f, 12.f, 13.f, -14.f, 15.f, -16.f);
		Mat44 n = Inverse(m);
		TEST_CHECK(AlmostEqual(GetColumn(n, 0), Vec4(6.f, 2.f, 14.f, 10.f) / 8.f, 0.000001f));
		TEST_CHECK(AlmostEqual(GetColumn(n, 1), Vec4(5.f, 1.f, 13.f, 9.f) / 8.f, 0.000001f));
		TEST_CHECK(AlmostEqual(GetColumn(n, 2), Vec4(-8.f, -4.f, 16.f, 12.f) / 8.f, 0.000001f));
		TEST_CHECK(AlmostEqual(GetColumn(n, 3), Vec4(-7.f, -3.f, 15.f, 11.f) / 8.f, 0.000001f));
		TEST_CHECK((m * n) == Mat44::Identity);
	}
	{
		const Mat44 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f);
		TEST_CHECK(GetRow(m, 0) == Vec4(1.f, 5.f, 9.f, 13.f));
		TEST_CHECK(GetRow(m, 1) == Vec4(2.f, 6.f, 10.f, 14.f));
		TEST_CHECK(GetRow(m, 2) == Vec4(3.f, 7.f, 11.f, 15.f));
		TEST_CHECK(GetRow(m, 3) == Vec4(4.f, 8.f, 12.f, 16.f));
	}
	{
		const Mat44 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f);
		TEST_CHECK(GetColumn(m, 0) == Vec4(1.f, 2.f, 3.f, 4.f));
		TEST_CHECK(GetColumn(m, 1) == Vec4(5.f, 6.f, 7.f, 8.f));
		TEST_CHECK(GetColumn(m, 2) == Vec4(9.f, 10.f, 11.f, 12.f));
		TEST_CHECK(GetColumn(m, 3) == Vec4(13.f, 14.f, 15.f, 16.f));
	}
	{
		{
			Mat44 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f);
			SetRow(m, 0, Vec4(0.1f, 0.2f, 0.3f, 0.4f));
			TEST_CHECK(GetRow(m, 0) == Vec4(0.1f, 0.2f, 0.3f, 0.4f));
			TEST_CHECK(GetRow(m, 1) == Vec4(2.f, 6.f, 10.f, 14.f));
			TEST_CHECK(GetRow(m, 2) == Vec4(3.f, 7.f, 11.f, 15.f));
			TEST_CHECK(GetRow(m, 3) == Vec4(4.f, 8.f, 12.f, 16.f));
		}
		{
			Mat44 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f);
			SetRow(m, 1, Vec4(0.1f, 0.2f, 0.3f, 0.4f));
			TEST_CHECK(GetRow(m, 0) == Vec4(1.f, 5.f, 9.f, 13.f));
			TEST_CHECK(GetRow(m, 1) == Vec4(0.1f, 0.2f, 0.3f, 0.4f));
			TEST_CHECK(GetRow(m, 2) == Vec4(3.f, 7.f, 11.f, 15.f));
			TEST_CHECK(GetRow(m, 3) == Vec4(4.f, 8.f, 12.f, 16.f));
		}
		{
			Mat44 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f);
			SetRow(m, 2, Vec4(0.1f, 0.2f, 0.3f, 0.4f));
			TEST_CHECK(GetRow(m, 0) == Vec4(1.f, 5.f, 9.f, 13.f));
			TEST_CHECK(GetRow(m, 1) == Vec4(2.f, 6.f, 10.f, 14.f));
			TEST_CHECK(GetRow(m, 2) == Vec4(0.1f, 0.2f, 0.3f, 0.4f));
			TEST_CHECK(GetRow(m, 3) == Vec4(4.f, 8.f, 12.f, 16.f));
		}
		{
			Mat44 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f);
			SetRow(m, 3, Vec4(0.1f, 0.2f, 0.3f, 0.4f));
			TEST_CHECK(GetRow(m, 0) == Vec4(1.f, 5.f, 9.f, 13.f));
			TEST_CHECK(GetRow(m, 1) == Vec4(2.f, 6.f, 10.f, 14.f));
			TEST_CHECK(GetRow(m, 2) == Vec4(3.f, 7.f, 11.f, 15.f));
			TEST_CHECK(GetRow(m, 3) == Vec4(0.1f, 0.2f, 0.3f, 0.4f));
		}
	}
	{
		{
			Mat44 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f);
			SetColumn(m, 0, Vec4(0.1f, 0.2f, 0.3f, 0.4f));
			TEST_CHECK(GetColumn(m, 0) == Vec4(0.1f, 0.2f, 0.3f, 0.4f));
			TEST_CHECK(GetColumn(m, 1) == Vec4(5.f, 6.f, 7.f, 8.f));
			TEST_CHECK(GetColumn(m, 2) == Vec4(9.f, 10.f, 11.f, 12.f));
			TEST_CHECK(GetColumn(m, 3) == Vec4(13.f, 14.f, 15.f, 16.f));
		}
		{
			Mat44 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f);
			SetColumn(m, 1, Vec4(0.1f, 0.2f, 0.3f, 0.4f));
			TEST_CHECK(GetColumn(m, 0) == Vec4(1.f, 2.f, 3.f, 4.f));
			TEST_CHECK(GetColumn(m, 1) == Vec4(0.1f, 0.2f, 0.3f, 0.4f));
			TEST_CHECK(GetColumn(m, 2) == Vec4(9.f, 10.f, 11.f, 12.f));
			TEST_CHECK(GetColumn(m, 3) == Vec4(13.f, 14.f, 15.f, 16.f));
		}
		{
			Mat44 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f);
			SetColumn(m, 2, Vec4(0.1f, 0.2f, 0.3f, 0.4f));
			TEST_CHECK(GetColumn(m, 0) == Vec4(1.f, 2.f, 3.f, 4.f));
			TEST_CHECK(GetColumn(m, 1) == Vec4(5.f, 6.f, 7.f, 8.f));
			TEST_CHECK(GetColumn(m, 2) == Vec4(0.1f, 0.2f, 0.3f, 0.4f));
			TEST_CHECK(GetColumn(m, 3) == Vec4(13.f, 14.f, 15.f, 16.f));
		}
		{
			Mat44 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f);
			SetColumn(m, 3, Vec4(0.1f, 0.2f, 0.3f, 0.4f));
			TEST_CHECK(GetColumn(m, 0) == Vec4(1.f, 2.f, 3.f, 4.f));
			TEST_CHECK(GetColumn(m, 1) == Vec4(5.f, 6.f, 7.f, 8.f));
			TEST_CHECK(GetColumn(m, 2) == Vec4(9.f, 10.f, 11.f, 12.f));
			TEST_CHECK(GetColumn(m, 3) == Vec4(0.1f, 0.2f, 0.3f, 0.4f));
		}
	}
	}
