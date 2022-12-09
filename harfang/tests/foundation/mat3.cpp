// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <math.h>

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/matrix3.h"

#include "foundation/math.h"
#include "foundation/matrix4.h"
#include "foundation/unit.h"
#include "foundation/vector2.h"
#include "foundation/vector3.h"
#include "foundation/vector4.h"

using namespace hg;

void test_mat3() {
	{
		Mat3 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
		TEST_CHECK(Equal(m.m[0][0], 1.f));
		TEST_CHECK(Equal(m.m[1][0], 2.f));
		TEST_CHECK(Equal(m.m[2][0], 3.f));

		TEST_CHECK(Equal(m.m[0][1], 4.f));
		TEST_CHECK(Equal(m.m[1][1], 5.f));
		TEST_CHECK(Equal(m.m[2][1], 6.f));

		TEST_CHECK(Equal(m.m[0][2], 7.f));
		TEST_CHECK(Equal(m.m[1][2], 8.f));
		TEST_CHECK(Equal(m.m[2][2], 9.f));
	}
	{
		const float raw[] = {1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f};
		Mat3 m(raw);
		TEST_CHECK(Equal(m.m[0][0], 1.f));
		TEST_CHECK(Equal(m.m[1][0], 2.f));
		TEST_CHECK(Equal(m.m[2][0], 3.f));

		TEST_CHECK(Equal(m.m[0][1], 4.f));
		TEST_CHECK(Equal(m.m[1][1], 5.f));
		TEST_CHECK(Equal(m.m[2][1], 6.f));

		TEST_CHECK(Equal(m.m[0][2], 7.f));
		TEST_CHECK(Equal(m.m[1][2], 8.f));
		TEST_CHECK(Equal(m.m[2][2], 9.f));
	}
	{
		const Mat4 m4(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f);
		Mat3 m(m4);
		TEST_CHECK(Equal(m.m[0][0], 1.f));
		TEST_CHECK(Equal(m.m[1][0], 2.f));
		TEST_CHECK(Equal(m.m[2][0], 3.f));

		TEST_CHECK(Equal(m.m[0][1], 4.f));
		TEST_CHECK(Equal(m.m[1][1], 5.f));
		TEST_CHECK(Equal(m.m[2][1], 6.f));

		TEST_CHECK(Equal(m.m[0][2], 7.f));
		TEST_CHECK(Equal(m.m[1][2], 8.f));
		TEST_CHECK(Equal(m.m[2][2], 9.f));
	}
	{
		const Vec3 u(1.f, 2.f, 3.f);
		const Vec3 v(4.f, 5.f, 6.f);
		const Vec3 w(7.f, 8.f, 9.f);
		Mat3 m(u, v, w);
		TEST_CHECK(Equal(m.m[0][0], u.x));
		TEST_CHECK(Equal(m.m[1][0], u.y));
		TEST_CHECK(Equal(m.m[2][0], u.z));

		TEST_CHECK(Equal(m.m[0][1], v.x));
		TEST_CHECK(Equal(m.m[1][1], v.y));
		TEST_CHECK(Equal(m.m[2][1], v.z));

		TEST_CHECK(Equal(m.m[0][2], w.x));
		TEST_CHECK(Equal(m.m[1][2], w.y));
		TEST_CHECK(Equal(m.m[2][2], w.z));
	}
	{
		Mat3 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
		m += Mat3(0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f);

		TEST_CHECK(Equal(m.m[0][0], 1.1f));
		TEST_CHECK(Equal(m.m[1][0], 2.2f));
		TEST_CHECK(Equal(m.m[2][0], 3.3f));

		TEST_CHECK(Equal(m.m[0][1], 4.4f));
		TEST_CHECK(Equal(m.m[1][1], 5.5f));
		TEST_CHECK(Equal(m.m[2][1], 6.6f));

		TEST_CHECK(Equal(m.m[0][2], 7.7f));
		TEST_CHECK(Equal(m.m[1][2], 8.8f));
		TEST_CHECK(Equal(m.m[2][2], 9.9f));
	}
	{
		Mat3 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
		m -= Mat3(0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f);

		TEST_CHECK(Equal(m.m[0][0], 0.9f));
		TEST_CHECK(Equal(m.m[1][0], 1.8f));
		TEST_CHECK(Equal(m.m[2][0], 2.7f));

		TEST_CHECK(Equal(m.m[0][1], 3.6f));
		TEST_CHECK(Equal(m.m[1][1], 4.5f));
		TEST_CHECK(Equal(m.m[2][1], 5.4f));

		TEST_CHECK(Equal(m.m[0][2], 6.3f));
		TEST_CHECK(Equal(m.m[1][2], 7.2f));
		TEST_CHECK(Equal(m.m[2][2], 8.1f));
	}
	{
		Mat3 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
		m *= 0.1f;

		TEST_CHECK(Equal(m.m[0][0], 0.1f));
		TEST_CHECK(Equal(m.m[1][0], 0.2f));
		TEST_CHECK(Equal(m.m[2][0], 0.3f));

		TEST_CHECK(Equal(m.m[0][1], 0.4f));
		TEST_CHECK(Equal(m.m[1][1], 0.5f));
		TEST_CHECK(Equal(m.m[2][1], 0.6f));

		TEST_CHECK(Equal(m.m[0][2], 0.7f));
		TEST_CHECK(Equal(m.m[1][2], 0.8f));
		TEST_CHECK(Equal(m.m[2][2], 0.9f));
	}
	{
		Mat3 m(-0.1f, 0.4f, -0.7f, 0.2f, -0.5f, 0.8f, -0.3f, 0.6f, -0.9f);
		m *= Mat3(-1.0f, 2.0f, 3.0f, 4.0f, -5.0f, 6.0f, 7.0f, 8.0f, -9.0f);

		TEST_CHECK(Equal(m.m[0][0], -0.4f));
		TEST_CHECK(Equal(m.m[1][0], 0.4f));
		TEST_CHECK(AlmostEqual(m.m[2][0], -0.4f));

		TEST_CHECK(Equal(m.m[0][1], -3.2f));
		TEST_CHECK(Equal(m.m[1][1], 7.7f));
		TEST_CHECK(Equal(m.m[2][1], -12.2f));

		TEST_CHECK(AlmostEqual(m.m[0][2], 3.6f));
		TEST_CHECK(AlmostEqual(m.m[1][2], -6.6f));
		TEST_CHECK(AlmostEqual(m.m[2][2], 9.6f));
	}
	{
		const Mat3 m0 = Mat3(0.1f, 0.4f, 0.7f, 0.2f, 0.5f, 0.8f, 0.3f, 0.6f, 0.9f);
		const Mat3 m1 = m0 * Mat3::Identity;
		const Mat3 m2 = Mat3(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);

		TEST_CHECK(m0 == m1);
		TEST_CHECK((m0 == m2) == false);
		TEST_CHECK((m1 == m2) == false);
	}
	{
		const Mat3 m0 = Mat3(0.1f, 0.4f, 0.7f, 0.2f, 0.5f, 0.8f, 0.3f, 0.6f, 0.9f);
		const Mat3 m1 = Mat3(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
		const Mat3 m2 = m0 * Mat3::Identity;

		TEST_CHECK(m0 != m1);
		TEST_CHECK((m0 != m2) == false);
	}
	{
		Mat3 m = Mat3(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f) + Mat3(0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f);

		TEST_CHECK(Equal(m.m[0][0], 1.1f));
		TEST_CHECK(Equal(m.m[1][0], 2.2f));
		TEST_CHECK(Equal(m.m[2][0], 3.3f));

		TEST_CHECK(Equal(m.m[0][1], 4.4f));
		TEST_CHECK(Equal(m.m[1][1], 5.5f));
		TEST_CHECK(Equal(m.m[2][1], 6.6f));

		TEST_CHECK(Equal(m.m[0][2], 7.7f));
		TEST_CHECK(Equal(m.m[1][2], 8.8f));
		TEST_CHECK(Equal(m.m[2][2], 9.9f));
	}
	{
		Mat3 m = Mat3(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f) - Mat3(0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f);

		TEST_CHECK(Equal(m.m[0][0], 0.9f));
		TEST_CHECK(Equal(m.m[1][0], 1.8f));
		TEST_CHECK(Equal(m.m[2][0], 2.7f));

		TEST_CHECK(Equal(m.m[0][1], 3.6f));
		TEST_CHECK(Equal(m.m[1][1], 4.5f));
		TEST_CHECK(Equal(m.m[2][1], 5.4f));

		TEST_CHECK(Equal(m.m[0][2], 6.3f));
		TEST_CHECK(Equal(m.m[1][2], 7.2f));
		TEST_CHECK(Equal(m.m[2][2], 8.1f));
	}
	{
		Mat3 m = Mat3(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f) * 0.1f;

		TEST_CHECK(Equal(m.m[0][0], 0.1f));
		TEST_CHECK(Equal(m.m[1][0], 0.2f));
		TEST_CHECK(Equal(m.m[2][0], 0.3f));

		TEST_CHECK(Equal(m.m[0][1], 0.4f));
		TEST_CHECK(Equal(m.m[1][1], 0.5f));
		TEST_CHECK(Equal(m.m[2][1], 0.6f));

		TEST_CHECK(Equal(m.m[0][2], 0.7f));
		TEST_CHECK(Equal(m.m[1][2], 0.8f));
		TEST_CHECK(Equal(m.m[2][2], 0.9f));
	}
	{
		const Mat3 m0 = Mat3(0.1f, 0.4f, 0.7f, 0.2f, 0.5f, 0.8f, 0.3f, 0.6f, 0.9f);
		const Mat3 m1 = Mat3(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
		Mat3 m = m0 * m1;

		TEST_CHECK(Equal(m.m[0][0], 1.4f));
		TEST_CHECK(Equal(m.m[1][0], 3.2f));
		TEST_CHECK(Equal(m.m[2][0], 5.0f));

		TEST_CHECK(Equal(m.m[0][1], 3.2f));
		TEST_CHECK(Equal(m.m[1][1], 7.7f));
		TEST_CHECK(Equal(m.m[2][1], 12.2f));

		TEST_CHECK(Equal(m.m[0][2], 5.0f));
		TEST_CHECK(AlmostEqual(m.m[1][2], 12.2f));
		TEST_CHECK(Equal(m.m[2][2], 19.4f));
	}
	{
		Vec2 v = Mat3(0.1f, -0.4f, 0.7f, -0.2f, 0.5f, -0.8f, 0.3f, -0.6f, 0.9f) * Vec2(2.0f, 3.0f);

		TEST_CHECK(Equal(v.x, -0.1f));
		TEST_CHECK(Equal(v.y, 0.1f));
	}
	{
		Vec3 v = Mat3(0.1f, -0.4f, 0.7f, -0.2f, 0.5f, -0.8f, 0.3f, -0.6f, 0.9f) * Vec3(2.0f, 3.0f, 5.0f);

		TEST_CHECK(Equal(v.x, 1.1f));
		TEST_CHECK(Equal(v.y, -2.3f));
		TEST_CHECK(Equal(v.z, 3.5f));
	}
	{
		Vec4 v = Mat3(-0.1f, 0.4f, -0.7f, 0.2f, -0.5f, 0.8f, -0.3f, 0.6f, -0.9f) * Vec4(1.0f, -1.0f, 2.0f, 6.f);

		TEST_CHECK(Equal(v.x, -0.9f));
		TEST_CHECK(Equal(v.y, 2.1f));
		TEST_CHECK(Equal(v.z, -3.3f));
		TEST_CHECK(Equal(v.w, 6.0f));
	}
	{
		const Mat3 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
		Vec2 u[4] = {
			Vec2(1.f, 0.f),
			Vec2(0.f, 1.f),
			Vec2(1.f, -1.f),
			Vec2(0.2f, 0.3f),
		};
		Vec2 v[4];
		TransformVec2(m, v, u, 4);
		TEST_CHECK(v[0] == Vec2(8.f, 10.f));
		TEST_CHECK(v[1] == Vec2(11.f, 13.f));
		TEST_CHECK(v[2] == Vec2(4.f, 5.f));
		TEST_CHECK(v[3] == Vec2(8.4f, 9.9f));
	}
	{
		const Mat3 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
		Vec3 u[4] = {
			Vec3(1.f, 0.5f, 0.f),
			Vec3(0.f, 1.f, 1.f),
			Vec3(1.f, -1.f, -1.f),
			Vec3(0.2f, 0.3f, 0.4f),
		};
		Vec3 v[4];
		TransformVec3(m, v, u, 4);
		TEST_CHECK(v[0] == Vec3(3.f, 4.5, 6.f));
		TEST_CHECK(v[1] == Vec3(11.f, 13.f, 15.f));
		TEST_CHECK(v[2] == Vec3(-10.f, -11.f, -12.f));
		TEST_CHECK(v[3] == Vec3(4.2f, 5.1f, 6.f));
	}
	{
		const Mat3 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
		Vec4 u[4] = {
			Vec4(0.f, 1.1f, 0.5f, 1.f),
			Vec4(0.f, 1.f, -1.f, 2.f),
			Vec4(-1.f, 1.f, -1.f, 0.f),
			Vec4(4.f, 1.f, 2.f, 0.5f),
		};
		Vec4 v[4];
		TransformVec4(m, v, u, 4);
		TEST_CHECK(v[0] == Vec4(7.9f, 9.5f, 11.1f, 1.f));
		TEST_CHECK(v[1] == Vec4(-3.f, -3.f, -3.f, 2.f));
		TEST_CHECK(v[2] == Vec4(-4.f, -5.f, -6.f, 0.f));
		TEST_CHECK(v[3] == Vec4(22.f, 29.f, 36.f, 0.5f));
	}
	{
		const Mat3 m0(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
		const Mat3 m1(7.f, 5.f, 6.f, 4.f, 2.f, 9.f, 1.f, 8.f, 3.f);
		TEST_CHECK(Det(m0) == 0.f);
		TEST_CHECK(Det(m1) == -297.f);
	}
	{
		const Mat3 m0(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
		const Mat3 m1(7.f, 5.f, 6.f, 4.f, 2.f, 9.f, 1.f, 8.f, 3.f);
		const Mat3 n = Mat3(22.f, -11.f, -11.f, 1.f, -5.f, 13.f, -10.f, 17.f, 2.f) / 99.f;
		Mat3 w;
		TEST_CHECK(Inverse(m0, w) == false);
		TEST_CHECK(Inverse(m1, w) == true);
		TEST_CHECK(w == n);
	}
	TEST_CHECK(Transpose(Mat3(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f)) == Mat3(1.f, 4.f, 7.f, 2.f, 5.f, 8.f, 3.f, 6.f, 9.f));
	{
		const float cs = Cos(Pi / 3.f);
		const float sn = Sin(Pi / 3.f);
		const Mat3 rx(1.f, 0.f, 0.f, 0.f, cs, sn, 0.f, -sn, cs);
		const Mat3 ry(cs, 0.f, -sn, 0.f, 1.f, 0.f, sn, 0.f, cs);
		const Mat3 rz(cs, sn, 0.f, -sn, cs, 0.f, 0.f, 0.f, 1.f);
		TEST_CHECK(RotationMatX(Pi / 3.f) == rx);
		TEST_CHECK(RotationMatY(Pi / 3.f) == ry);
		TEST_CHECK(RotationMatZ(Pi / 3.f) == rz);
	}
	{
		const hg::Vec3 r = Deg3(30.f, 45.f, -60.f);
		{
			Mat3 m = RotationMatXZY(r.x, r.y, r.z);
			Mat3 n = RotationMatX(r.x) * RotationMatZ(r.z) * RotationMatY(r.y);
			TEST_CHECK(AlmostEqual(GetRow(m, 0), GetRow(n, 0), 0.000001f));
			TEST_CHECK(AlmostEqual(GetRow(m, 1), GetRow(n, 1), 0.000001f));
			TEST_CHECK(AlmostEqual(GetRow(m, 2), GetRow(n, 2), 0.000001f));
		}
		{
			Mat3 m = RotationMatZYX(r.x, r.y, r.z);
			Mat3 n = RotationMatZ(r.z) * RotationMatY(r.y) * RotationMatX(r.x);
			TEST_CHECK(AlmostEqual(GetRow(m, 0), GetRow(n, 0), 0.000001f));
			TEST_CHECK(AlmostEqual(GetRow(m, 1), GetRow(n, 1), 0.000001f));
			TEST_CHECK(AlmostEqual(GetRow(m, 2), GetRow(n, 2), 0.000001f));
		}
		{
			Mat3 m = RotationMatXYZ(r.x, r.y, r.z);
			Mat3 n = RotationMatX(r.x) * RotationMatY(r.y) * RotationMatZ(r.z);
			TEST_CHECK(AlmostEqual(GetRow(m, 0), GetRow(n, 0), 0.000001f));
			TEST_CHECK(AlmostEqual(GetRow(m, 1), GetRow(n, 1), 0.000001f));
			TEST_CHECK(AlmostEqual(GetRow(m, 2), GetRow(n, 2), 0.000001f));
		}
		{
			Mat3 m = RotationMatZXY(r.x, r.y, r.z);
			Mat3 n = RotationMatZ(r.z) * RotationMatX(r.x) * RotationMatY(r.y);
			TEST_CHECK(AlmostEqual(GetRow(m, 0), GetRow(n, 0), 0.000001f));
			TEST_CHECK(AlmostEqual(GetRow(m, 1), GetRow(n, 1), 0.000001f));
			TEST_CHECK(AlmostEqual(GetRow(m, 2), GetRow(n, 2), 0.000001f));
		}
		{
			Mat3 m = RotationMatYZX(r.x, r.y, r.z);
			Mat3 n = RotationMatY(r.y) * RotationMatZ(r.z) * RotationMatX(r.x);
			TEST_CHECK(AlmostEqual(GetRow(m, 0), GetRow(n, 0), 0.000001f));
			TEST_CHECK(AlmostEqual(GetRow(m, 1), GetRow(n, 1), 0.000001f));
			TEST_CHECK(AlmostEqual(GetRow(m, 2), GetRow(n, 2), 0.000001f));
		}
		{
			Mat3 m = RotationMatYZX(r.x, r.y, r.z);
			Mat3 n = RotationMatY(r.y) * RotationMatZ(r.z) * RotationMatX(r.x);
			TEST_CHECK(AlmostEqual(GetRow(m, 0), GetRow(n, 0), 0.000001f));
			TEST_CHECK(AlmostEqual(GetRow(m, 1), GetRow(n, 1), 0.000001f));
			TEST_CHECK(AlmostEqual(GetRow(m, 2), GetRow(n, 2), 0.000001f));
		}
		{
			Mat3 m = RotationMatXY(r.x, r.y);
			Mat3 n = RotationMatX(r.x) * RotationMatY(r.y);
			TEST_CHECK(AlmostEqual(GetRow(m, 0), GetRow(n, 0), 0.000001f));
			TEST_CHECK(AlmostEqual(GetRow(m, 1), GetRow(n, 1), 0.000001f));
			TEST_CHECK(AlmostEqual(GetRow(m, 2), GetRow(n, 2), 0.000001f));
		}
	}
	TEST_CHECK(VectorMat3(Vec3(1.f, -2.f, -3.f)) == Mat3(1.f, 0.f, 0.f, -2.f, 0.f, 0.f, -3.f, 0.f, 0.f));
	TEST_CHECK(TranslationMat3(Vec2(2.f, -3.f)) == Mat3(1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 2.f, -3.f, 1.f));
	TEST_CHECK(TranslationMat3(Vec3(-1.f, 2.f, -5.f)) == Mat3(1.f, 0.f, 0.f, 0.f, 1.f, 0.f, -1.f, 2.f, 1.f));
	TEST_CHECK(ScaleMat3(Vec2(-2.f, 3.f)) == Mat3(-2.f, 0.f, 0.f, 0.f, 3.f, 0.f, 0.f, 0.f, 1.f));
	TEST_CHECK(ScaleMat3(Vec3(0.5f, -1.f, -0.33f)) == Mat3(0.5f, 0.f, 0.f, 0.f, -1.f, 0.f, 0.f, 0.f, -0.33f));
	{{const Vec3 u(2.02f, -1.5151f, 0.997f);
	const Vec3 v(2.4042f, -0.67f, 0.789f);
	Vec3 w0 = Cross(u, v);
	Vec3 w1 = CrossProductMat3(u) * v;

	TEST_CHECK(Equal(w0.x, w1.x));
	TEST_CHECK(Equal(w0.y, w1.y));
	TEST_CHECK(Equal(w0.z, w1.z));
}
{
	const Vec3 u(-1.f, 1.f, 1.f);
	const Vec3 v(1.f, -1.f, -1.f);
	Vec3 w0 = Cross(u, v);
	Vec3 w1 = CrossProductMat3(u) * v;

	TEST_CHECK(Equal(w0.x, w1.x));
	TEST_CHECK(Equal(w0.y, w1.y));
	TEST_CHECK(Equal(w0.z, w1.z));
}
}
{
	TEST_CHECK(RotationMat3(-30.f, 120.f, -45.f, RO_ZYX) == RotationMatZYX(-30.f, 120.f, -45.f));
	TEST_CHECK(RotationMat3(-30.f, 120.f, -45.f, RO_YZX) == RotationMatYZX(-30.f, 120.f, -45.f));
	TEST_CHECK(RotationMat3(-30.f, 120.f, -45.f, RO_ZXY) == RotationMatZXY(-30.f, 120.f, -45.f));
	TEST_CHECK(RotationMat3(-30.f, 120.f, -45.f, RO_XZY) == RotationMatXZY(-30.f, 120.f, -45.f));
	TEST_CHECK(RotationMat3(-30.f, 120.f, -45.f, RO_YXZ) == RotationMatYXZ(-30.f, 120.f, -45.f));
	TEST_CHECK(RotationMat3(-30.f, 120.f, -45.f, RO_XYZ) == RotationMatXYZ(-30.f, 120.f, -45.f));
}
{
	const Vec3 u = Deg3(30.f, -120.f, 45.f);
	TEST_CHECK(RotationMat3(u, RO_ZYX) == RotationMatZYX(u.x, u.y, u.z));
	TEST_CHECK(RotationMat3(u, RO_YZX) == RotationMatYZX(u.x, u.y, u.z));
	TEST_CHECK(RotationMat3(u, RO_ZXY) == RotationMatZXY(u.x, u.y, u.z));
	TEST_CHECK(RotationMat3(u, RO_XZY) == RotationMatXZY(u.x, u.y, u.z));
	TEST_CHECK(RotationMat3(u, RO_YXZ) == RotationMatYXZ(u.x, u.y, u.z));
	TEST_CHECK(RotationMat3(u, RO_XYZ) == RotationMatXYZ(u.x, u.y, u.z));
}
TEST_CHECK(AlmostEqual(RotationMat2D(Pi / 2.f, Vec2(-1.f, -2.f)) * Vec3(1.f, 2.f, 1.f), Vec3(-5.f, 0.f, 1.f), 0.000001f));
{
	{
		const Vec3 p = Vec3(1.5f, 2.3f, 10.f);
		Mat3 m = Mat3LookAt(p);
		Vec3 i = GetX(m), j = GetY(m), k = GetZ(m);
		TEST_CHECK(AlmostEqual(Normalize(p), k, 0.000001f));
		TEST_CHECK(Equal(Dot(i, j), 0.f));
		TEST_CHECK(Equal(Dot(j, k), 0.f));
		TEST_CHECK(Equal(Dot(k, i), 0.f));
	}
	{
		Mat3 m = Mat3LookAt(Vec3(0.f, -12.f, 0.f));
		Vec3 i = GetX(m), j = GetY(m), k = GetZ(m);
		TEST_CHECK(AlmostEqual(i, Vec3(-1.f, 0.f, 0.f), 0.000001f));
		TEST_CHECK(Equal(Dot(i, j), 0.f));
		TEST_CHECK(Equal(Dot(j, k), 0.f));
		TEST_CHECK(Equal(Dot(k, i), 0.f));
	}
	TEST_CHECK(Mat3LookAt(Vec3::Zero) == Mat3::Identity);
}
{
	TEST_CHECK(Mat3LookAt(Vec3(0.f, 0.f, 1.f), Vec3(0.f, 1.f, 0.f)) == Mat3(1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f));
	TEST_CHECK(Mat3LookAt(Vec3(0.f, 0.f, 1.f), Vec3(1.f, 0.f, 0.f)) == Mat3(0.f, -1.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f));
	TEST_CHECK(Mat3LookAt(Vec3(1.f, 0.f, 0.f), Vec3(0.f, 0.f, 1.f)) == Mat3(0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f));

	TEST_CHECK(Mat3LookAt(Vec3(0.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f)) == Mat3::Identity);
}
{
	Mat3 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
	Vec3 u = GetRow(m, 0);
	Vec3 v = GetRow(m, 1);
	Vec3 w = GetRow(m, 2);

	TEST_CHECK(Equal(u.x, 1.f));
	TEST_CHECK(Equal(v.x, 2.f));
	TEST_CHECK(Equal(w.x, 3.f));

	TEST_CHECK(Equal(u.y, 4.f));
	TEST_CHECK(Equal(v.y, 5.f));
	TEST_CHECK(Equal(w.y, 6.f));

	TEST_CHECK(Equal(u.z, 7.f));
	TEST_CHECK(Equal(v.z, 8.f));
	TEST_CHECK(Equal(w.z, 9.f));
}
{
	Mat3 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
	Vec3 u = GetColumn(m, 0);
	Vec3 v = GetColumn(m, 1);
	Vec3 w = GetColumn(m, 2);

	TEST_CHECK(Equal(u.x, 1.f));
	TEST_CHECK(Equal(u.y, 2.f));
	TEST_CHECK(Equal(u.z, 3.f));

	TEST_CHECK(Equal(v.x, 4.f));
	TEST_CHECK(Equal(v.y, 5.f));
	TEST_CHECK(Equal(v.z, 6.f));

	TEST_CHECK(Equal(w.x, 7.f));
	TEST_CHECK(Equal(w.y, 8.f));
	TEST_CHECK(Equal(w.z, 9.f));
}
{
	Mat3 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
	Vec3 u = GetX(m);
	Vec3 v = GetY(m);
	Vec3 w = GetZ(m);

	TEST_CHECK(Equal(u.x, 1.f));
	TEST_CHECK(Equal(u.y, 2.f));
	TEST_CHECK(Equal(u.z, 3.f));

	TEST_CHECK(Equal(v.x, 4.f));
	TEST_CHECK(Equal(v.y, 5.f));
	TEST_CHECK(Equal(v.z, 6.f));

	TEST_CHECK(Equal(w.x, 7.f));
	TEST_CHECK(Equal(w.y, 8.f));
	TEST_CHECK(Equal(w.z, 9.f));
}
{{Mat3 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
SetRow(m, 0, Vec3(30.1f, 40.2f, 50.3f));
Vec3 u = GetRow(m, 0);
Vec3 v = GetRow(m, 1);
Vec3 w = GetRow(m, 2);

TEST_CHECK(Equal(u.x, 30.1f));
TEST_CHECK(Equal(v.x, 2.f));
TEST_CHECK(Equal(w.x, 3.f));

TEST_CHECK(Equal(u.y, 40.2f));
TEST_CHECK(Equal(v.y, 5.f));
TEST_CHECK(Equal(w.y, 6.f));

TEST_CHECK(Equal(u.z, 50.3f));
TEST_CHECK(Equal(v.z, 8.f));
TEST_CHECK(Equal(w.z, 9.f));
}
{
	Mat3 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
	SetRow(m, 1, Vec3(60.4f, 70.5f, 80.6f));
	Vec3 u = GetRow(m, 0);
	Vec3 v = GetRow(m, 1);
	Vec3 w = GetRow(m, 2);

	TEST_CHECK(Equal(u.x, 1.f));
	TEST_CHECK(Equal(v.x, 60.4f));
	TEST_CHECK(Equal(w.x, 3.f));

	TEST_CHECK(Equal(u.y, 4.f));
	TEST_CHECK(Equal(v.y, 70.5f));
	TEST_CHECK(Equal(w.y, 6.f));

	TEST_CHECK(Equal(u.z, 7.f));
	TEST_CHECK(Equal(v.z, 80.6f));
	TEST_CHECK(Equal(w.z, 9.f));
}
{
	Mat3 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
	SetRow(m, 2, Vec3(90.7f, 10.8f, 20.9f));
	Vec3 u = GetRow(m, 0);
	Vec3 v = GetRow(m, 1);
	Vec3 w = GetRow(m, 2);

	TEST_CHECK(Equal(u.x, 1.f));
	TEST_CHECK(Equal(v.x, 2.f));
	TEST_CHECK(Equal(w.x, 90.7f));

	TEST_CHECK(Equal(u.y, 4.f));
	TEST_CHECK(Equal(v.y, 5.f));
	TEST_CHECK(Equal(w.y, 10.8f));

	TEST_CHECK(Equal(u.z, 7.f));
	TEST_CHECK(Equal(v.z, 8.f));
	TEST_CHECK(Equal(w.z, 20.9f));
}
}
{{Mat3 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
SetColumn(m, 0, Vec3(30.1f, 40.2f, 50.3f));
Vec3 u = GetColumn(m, 0);
Vec3 v = GetColumn(m, 1);
Vec3 w = GetColumn(m, 2);

TEST_CHECK(Equal(u.x, 30.1f));
TEST_CHECK(Equal(u.y, 40.2f));
TEST_CHECK(Equal(u.z, 50.3f));

TEST_CHECK(Equal(v.x, 4.f));
TEST_CHECK(Equal(v.y, 5.f));
TEST_CHECK(Equal(v.z, 6.f));

TEST_CHECK(Equal(w.x, 7.f));
TEST_CHECK(Equal(w.y, 8.f));
TEST_CHECK(Equal(w.z, 9.f));
}
{
	Mat3 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
	SetColumn(m, 1, Vec3(60.4f, 70.5f, 80.6f));
	Vec3 u = GetColumn(m, 0);
	Vec3 v = GetColumn(m, 1);
	Vec3 w = GetColumn(m, 2);

	TEST_CHECK(Equal(u.x, 1.f));
	TEST_CHECK(Equal(u.y, 2.f));
	TEST_CHECK(Equal(u.z, 3.f));

	TEST_CHECK(Equal(v.x, 60.4f));
	TEST_CHECK(Equal(v.y, 70.5f));
	TEST_CHECK(Equal(v.z, 80.6f));

	TEST_CHECK(Equal(w.x, 7.f));
	TEST_CHECK(Equal(w.y, 8.f));
	TEST_CHECK(Equal(w.z, 9.f));
}
{
	Mat3 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
	SetColumn(m, 2, Vec3(90.7f, 10.8f, 20.9f));
	Vec3 u = GetColumn(m, 0);
	Vec3 v = GetColumn(m, 1);
	Vec3 w = GetColumn(m, 2);

	TEST_CHECK(Equal(u.x, 1.f));
	TEST_CHECK(Equal(u.y, 2.f));
	TEST_CHECK(Equal(u.z, 3.f));

	TEST_CHECK(Equal(v.x, 4.f));
	TEST_CHECK(Equal(v.y, 5.f));
	TEST_CHECK(Equal(v.z, 6.f));

	TEST_CHECK(Equal(w.x, 90.7f));
	TEST_CHECK(Equal(w.y, 10.8f));
	TEST_CHECK(Equal(w.z, 20.9f));
}
}
{
	Mat3 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
	SetX(m, Vec3(30.1f, 40.2f, 50.3f));
	Vec3 u = GetColumn(m, 0);
	Vec3 v = GetColumn(m, 1);
	Vec3 w = GetColumn(m, 2);

	TEST_CHECK(Equal(u.x, 30.1f));
	TEST_CHECK(Equal(u.y, 40.2f));
	TEST_CHECK(Equal(u.z, 50.3f));

	TEST_CHECK(Equal(v.x, 4.f));
	TEST_CHECK(Equal(v.y, 5.f));
	TEST_CHECK(Equal(v.z, 6.f));

	TEST_CHECK(Equal(w.x, 7.f));
	TEST_CHECK(Equal(w.y, 8.f));
	TEST_CHECK(Equal(w.z, 9.f));
}
{
	Mat3 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
	SetY(m, Vec3(60.4f, 70.5f, 80.6f));
	Vec3 u = GetColumn(m, 0);
	Vec3 v = GetColumn(m, 1);
	Vec3 w = GetColumn(m, 2);

	TEST_CHECK(Equal(u.x, 1.f));
	TEST_CHECK(Equal(u.y, 2.f));
	TEST_CHECK(Equal(u.z, 3.f));

	TEST_CHECK(Equal(v.x, 60.4f));
	TEST_CHECK(Equal(v.y, 70.5f));
	TEST_CHECK(Equal(v.z, 80.6f));

	TEST_CHECK(Equal(w.x, 7.f));
	TEST_CHECK(Equal(w.y, 8.f));
	TEST_CHECK(Equal(w.z, 9.f));
}
{
	Mat3 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
	SetZ(m, Vec3(90.7f, 10.8f, 20.9f));
	Vec3 u = GetColumn(m, 0);
	Vec3 v = GetColumn(m, 1);
	Vec3 w = GetColumn(m, 2);

	TEST_CHECK(Equal(u.x, 1.f));
	TEST_CHECK(Equal(u.y, 2.f));
	TEST_CHECK(Equal(u.z, 3.f));

	TEST_CHECK(Equal(v.x, 4.f));
	TEST_CHECK(Equal(v.y, 5.f));
	TEST_CHECK(Equal(v.z, 6.f));

	TEST_CHECK(Equal(w.x, 90.7f));
	TEST_CHECK(Equal(w.y, 10.8f));
	TEST_CHECK(Equal(w.z, 20.9f));
}
TEST_CHECK(GetTranslation(Mat3(1.f, 0.f, 0.f, 0.f, 1.f, 0.f, -1.f, 2.f, 3.f)) == Vec3(-1.f, 2.f, 0.f));
{
	const float u = 2.f * sqrt(2.f);
	Mat3 m(u, 0.f, -u, 2.f, u, 2.f, 2.f, -u, 2.f); // rotY(Pi/4) * rotX(Pi/4) * scale(4)
	Vec3 s = GetScale(m);
	TEST_CHECK(AlmostEqual(s, Vec3(4.f), 0.000001f));
}
{
	Mat3 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
	SetTranslation(m, Vec2(-1.f, 0.5f));
	TEST_CHECK(m.m[0][0] == 1.f);
	TEST_CHECK(m.m[1][0] == 2.f);
	TEST_CHECK(m.m[2][0] == 3.f);
	TEST_CHECK(m.m[0][1] == 4.f);
	TEST_CHECK(m.m[1][1] == 5.f);
	TEST_CHECK(m.m[2][1] == 6.f);
	TEST_CHECK(m.m[0][2] == -1.f);
	TEST_CHECK(m.m[1][2] == 0.5f);
	TEST_CHECK(m.m[2][2] == 9.f);
}
{
	Mat3 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
	SetTranslation(m, Vec3(-1.25f, 11.75f, -99.f));
	TEST_CHECK(m.m[0][0] == 1.f);
	TEST_CHECK(m.m[1][0] == 2.f);
	TEST_CHECK(m.m[2][0] == 3.f);
	TEST_CHECK(m.m[0][1] == 4.f);
	TEST_CHECK(m.m[1][1] == 5.f);
	TEST_CHECK(m.m[2][1] == 6.f);
	TEST_CHECK(m.m[0][2] == -1.25f);
	TEST_CHECK(m.m[1][2] == 11.75f);
	TEST_CHECK(m.m[2][2] == 9.f);
}
{
	const float v = sqrt(2.f);
	const float u = 2.f * v;
	const float w = v / 2.f;
	const Vec3 s0(1.f, 0.5f, 0.25f);
	const Mat3 n(w, 0.f, -w, 0.25f, w / 2.f, 0.25f, 0.125f, -w / 4.f, 0.125f); //  rotY(Pi/4) * rotX(Pi/4) * scale(s0)
	Mat3 m(u, 0.f, -u, 2.f, u, 2.f, 2.f, -u, 2.f); // rotY(Pi/4) * rotX(Pi/4) * scale(4)
	SetScale(m, s0);
	Vec3 s1 = GetScale(m);
	Vec3 s2 = GetScale(n);
	TEST_CHECK(AlmostEqual(s0, s1, 0.000001f));
	TEST_CHECK(AlmostEqual(GetRow(m, 0), GetRow(n, 0), 0.000001f));
	TEST_CHECK(AlmostEqual(GetRow(m, 1), GetRow(n, 1), 0.000001f));
	TEST_CHECK(AlmostEqual(GetRow(m, 2), GetRow(n, 2), 0.000001f));
}
{
	Mat3 m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
	SetAxises(m, Vec3(-0.1f, -0.2f, -0.3f), Vec3(-1.1f, -2.2f, -3.3f), Vec3(-11.f, -22.f, -33.f));
	TEST_CHECK(m.m[0][0] == -0.1f);
	TEST_CHECK(m.m[1][0] == -0.2f);
	TEST_CHECK(m.m[2][0] == -0.3f);
	TEST_CHECK(m.m[0][1] == -1.1f);
	TEST_CHECK(m.m[1][1] == -2.2f);
	TEST_CHECK(m.m[2][1] == -3.3f);
	TEST_CHECK(m.m[0][2] == -11.f);
	TEST_CHECK(m.m[1][2] == -22.f);
	TEST_CHECK(m.m[2][2] == -33.f);
}
{
	Mat3 m = Orthonormalize(Mat3(8.f, 8.f, 6.f, 1.f, 1.f, 0.f, 8.f, 4.f, 5.f));
	Vec3 i = GetX(m), j = GetY(m), k = GetZ(m);
	TEST_CHECK(AlmostEqual(Dot(i, j), 0.f, 1.e-5f));
	TEST_CHECK(AlmostEqual(Dot(j, k), 0.f, 1.e-5f));
	TEST_CHECK(AlmostEqual(Dot(k, i), 0.f, 1.e-5f));
	TEST_CHECK(AlmostEqual(Len2(i), 1.f, 1.e-5f));
	TEST_CHECK(AlmostEqual(Len2(j), 1.f, 1.e-5f));
	TEST_CHECK(AlmostEqual(Len2(k), 1.f, 1.e-5f));
}
{
	Mat3 m = Normalize(Mat3(8.f, 8.f, 6.f, 1.f, 1.f, 0.f, 8.f, 4.f, 5.f));
	TEST_CHECK(Equal(Len2(GetX(m)), 1.f));
	TEST_CHECK(Equal(Len2(GetY(m)), 1.f));
	TEST_CHECK(Equal(Len2(GetZ(m)), 1.f));
}
{
	const Vec3 u = Deg3(30.f, 45.f, 60.f);
	TEST_CHECK(AlmostEqual(u, ToEuler(RotationMat3(u)), 0.000001f));
	TEST_CHECK(AlmostEqual(u, ToEuler(RotationMat3(u, RO_ZYX), RO_ZYX), 0.000001f));
	TEST_CHECK(AlmostEqual(u, ToEuler(RotationMat3(u, RO_YZX), RO_YZX), 0.000001f));
	TEST_CHECK(AlmostEqual(u, ToEuler(RotationMat3(u, RO_ZXY), RO_ZXY), 0.000001f));
	TEST_CHECK(AlmostEqual(u, ToEuler(RotationMat3(u, RO_XZY), RO_XZY), 0.000001f));
	TEST_CHECK(AlmostEqual(u, ToEuler(RotationMat3(u, RO_YXZ), RO_YXZ), 0.000001f));
	TEST_CHECK(AlmostEqual(u, ToEuler(RotationMat3(u, RO_XYZ), RO_XYZ), 0.000001f));
	TEST_CHECK(AlmostEqual(u, ToEuler(RotationMat3(u, RO_XY), RO_XY), 0.000001f));
}
}
