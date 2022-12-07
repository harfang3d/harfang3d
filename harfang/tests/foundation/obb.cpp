// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <math.h>

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/obb.h"

#include "foundation/math.h"
#include "foundation/matrix4.h"
#include "foundation/minmax.h"

using namespace hg;

void test_obb() {
	{ 
		const Vec3 pos(-1.f, 2.f, 10.f);
		const Vec3 scl(3.f, 4.f, 5.f);
		const Mat3 rot = RotationMat3(Deg3(30.f, -60.f, 45.f));

		OBB o0(pos, scl);
		TEST_CHECK(AlmostEqual(o0.pos, pos, 0.000001f));
		TEST_CHECK(AlmostEqual(o0.scl, scl, 0.000001f));
		TEST_CHECK(o0.rot == Mat3::Identity);

		OBB o1(pos, scl, rot);
		TEST_CHECK(AlmostEqual(o1.pos, pos, 0.000001f));
		TEST_CHECK(AlmostEqual(o1.scl, scl, 0.000001f));
		TEST_CHECK(AlmostEqual(GetColumn(o1.rot, 0), GetColumn(rot, 0), 0.000001f));
		TEST_CHECK(AlmostEqual(GetColumn(o1.rot, 1), GetColumn(rot, 1), 0.000001f));
		TEST_CHECK(AlmostEqual(GetColumn(o1.rot, 2), GetColumn(rot, 2), 0.000001f));
	}
	{ 
		{
			OBB o = OBBFromMinMax(MinMax(-Vec3::One, Vec3::One));
			TEST_CHECK(AlmostEqual(o.pos, Vec3::Zero, 0.000001f));
			TEST_CHECK(AlmostEqual(o.scl, 2.f * Vec3::One, 0.000001f));
			TEST_CHECK(o.rot == Mat3::Identity);
		}
		{
			OBB o = OBBFromMinMax(MinMax(Vec3(-0.5f, 1.f, -3.f), Vec3(0.5f, 2.f, -2.f)));
			TEST_CHECK(AlmostEqual(o.pos, Vec3(0.f, 1.5f, -2.5f), 0.000001f));
			TEST_CHECK(AlmostEqual(o.scl, Vec3::One, 0.000001f));
			TEST_CHECK(o.rot == Mat3::Identity);
		}
	}
	{
		{
			float s = Sqrt(2.f) / 2.f;
			float theta = Pi / 4.f;
			MinMax mm = MinMaxFromOBB(OBB(Vec3::Zero, Vec3::One, RotationMat3(Vec3(0, theta, 0))));
			TEST_CHECK(AlmostEqual(mm.mn, -Vec3(s, 0.5f, s), 0.000001f));
			TEST_CHECK(AlmostEqual(mm.mx, Vec3(s, 0.5f, s), 0.000001f));
		}
		{
			MinMax mm = MinMaxFromOBB(OBB(Vec3(0.f, 1.5f, -2.5f), Vec3(1.f, 2.f, 3.f)));
			TEST_CHECK(AlmostEqual(mm.mn, Vec3(-0.5f, 0.5f, -4.f), 0.000001f));
			TEST_CHECK(AlmostEqual(mm.mx, Vec3(0.5f, 2.5f, -1.f), 0.000001f));
		}
	}
	{
		{ 
			const Vec3 pos(-2.f, 0.5f, 10.f);
			const Vec3 scl(2.f, 1.f, 3.f);
			const Mat3 rot = RotationMat3(Deg3(-30.f, 45.f, 60.f));
			OBB o = TransformOBB(OBB(Vec3::Zero, Vec3::One), TransformationMat4(pos, rot, scl));
			TEST_CHECK(AlmostEqual(o.pos, pos, 0.000001f));
			TEST_CHECK(AlmostEqual(o.scl, scl, 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(o.rot, 0), GetColumn(rot, 0), 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(o.rot, 1), GetColumn(rot, 1), 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(o.rot, 2), GetColumn(rot, 2), 0.000001f));
		}
		{
			const Vec3 pos(-2.f, 0.5f, 10.f);
			OBB o = TransformOBB(OBB(Vec3(-4.f, -1.f, -8.f), Vec3::One), TranslationMat4(pos));
			TEST_CHECK(AlmostEqual(o.pos, Vec3(-6.f, -0.5f, 2.f), 0.000001f));
			TEST_CHECK(AlmostEqual(o.scl, Vec3::One, 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(o.rot, 0), Vec3(1.f, 0.f, 0.f), 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(o.rot, 1), Vec3(0.f, 1.f, 0.f), 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(o.rot, 2), Vec3(0.f, 0.f, 1.f), 0.000001f));
		}
		{
			const Vec3 scl(3.f, 4.f, 5.f);
			OBB o = TransformOBB(OBB(Vec3(-4.f, -1.f, -8.f), Vec3(2.f, 0.5f, 0.3f)), ScaleMat4(scl));
			TEST_CHECK(AlmostEqual(o.pos, Vec3(-4.f, -1.f, -8.f), 0.000001f));
			TEST_CHECK(AlmostEqual(o.scl, Vec3(6.f, 2.f, 1.5f), 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(o.rot, 0), Vec3(1.f, 0.f, 0.f), 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(o.rot, 1), Vec3(0.f, 1.f, 0.f), 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(o.rot, 2), Vec3(0.f, 0.f, 1.f), 0.000001f));
		}
	}
}
