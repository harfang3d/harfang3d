// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <math.h>
#include <float.h>

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/math.h"
#include "foundation/minmax.h"

#include "foundation/axis.h"
#include "foundation/math.h"
#include "foundation/matrix4.h"
#include "foundation/vector3.h"
#include "foundation/vector4.h"

using namespace hg;

void test_minmax() {
	{ 
		MinMax box;
		TEST_CHECK(box.mn == Vec3::Max);
		TEST_CHECK(box.mx == Vec3::Min);
	}
	{
		MinMax box(Vec3(-1.f, -1.f, -1.f), Vec3(1.f, 1.f, 1.f));
		TEST_CHECK(box.mn == Vec3(-1.f, -1.f, -1.f));
		TEST_CHECK(box.mx == Vec3(1.f, 1.f, 1.f));
	}
	TEST_CHECK(Equal(GetArea(MinMax(Vec3(-1.f, 1.f, 0.f), Vec3(0.f, 2.f, 1.f))), 1.f));
	TEST_CHECK(AlmostEqual(GetCenter(MinMax(Vec3(-1.f, 1.f, 0.f), Vec3(0.f, 2.f, 1.f))), Vec3(-0.5f, 1.5f, 0.5f), 0.000001f));
	TEST_CHECK(AlmostEqual(GetSize(MinMax(Vec3(-1.f, 1.f, 0.f), Vec3(0.f, 2.f, 1.f))), Vec3::One, 0.000001f));
	{ 
		Vec3 v[8];
		MinMax box(Vec3(-1.f, 1.f, 0.f), Vec3(0.f, 2.f, 1.f));
		GetMinMaxVertices(box, v);
		TEST_CHECK(AlmostEqual(v[0], Vec3(-1.f, 1.f, 0.f), 0.000001f));
		TEST_CHECK(AlmostEqual(v[1], Vec3(0.f, 1.f, 0.f), 0.000001f));
		TEST_CHECK(AlmostEqual(v[2], Vec3(0.f, 2.f, 0.f), 0.000001f));
		TEST_CHECK(AlmostEqual(v[3], Vec3(-1.f, 2.f, 0.f), 0.000001f));
		TEST_CHECK(AlmostEqual(v[4], Vec3(-1.f, 1.f, 1.f), 0.000001f));
		TEST_CHECK(AlmostEqual(v[5], Vec3(0.f, 1.f, 1.f), 0.000001f));
		TEST_CHECK(AlmostEqual(v[6], Vec3(0.f, 2.f, 1.f), 0.000001f));
		TEST_CHECK(AlmostEqual(v[7], Vec3(-1.f, 2.f, 1.f), 0.000001f));
	}
	{ 
		MinMax box(Vec3(-1.f, 1.f, 0.f), Vec3(0.f, 2.f, 1.f));
		Vec3 origin;
		float radius;
		ComputeMinMaxBoundingSphere(box, origin, radius);
		TEST_CHECK(AlmostEqual(origin, Vec3(-0.5f, 1.5f, 0.5f), 0.000001f));
		TEST_CHECK(Equal(radius, Dist(Vec3(-1.f, 1.f, 0.f), Vec3(0.f, 2.f, 1.f)) / 2.f));
	}
	{ 
		TEST_CHECK(Overlap(MinMax(Vec3(-2.f, -1.f, -1.f), Vec3(-1.0f, 0.f, 0.f)), MinMax(Vec3(-1.5f, -0.5f, -0.5f), Vec3(-0.5f, 0.5f, 0.5f)), A_X));
		TEST_CHECK(Overlap(MinMax(Vec3(-2.f, -1.f, -1.f), Vec3(-1.f, 0.f, 0.f)), MinMax(Vec3(-0.5f, -0.5f, -0.5f), Vec3(0.5f, 0.5f, 0.5f)), A_X) == false);
		TEST_CHECK(Overlap(MinMax(Vec3(-2.f, -1.f, -1.f), Vec3(-1.f, 0.f, 0.f)), MinMax(Vec3(-0.5f, -0.5f, -0.5f), Vec3(0.5f, 0.5f, 0.5f)), A_Y));
		TEST_CHECK(Overlap(MinMax(Vec3(-2.f, -1.f, -1.f), Vec3(-1.f, 0.f, 0.f)), MinMax(Vec3(-0.2f, -3.5f, -0.5f), Vec3(0.8f, -2.5f, 0.5f)), A_Y) == false);
		TEST_CHECK(Overlap(MinMax(Vec3(-2.f, -1.f, -1.f), Vec3(-1.f, 0.f, 0.f)), MinMax(Vec3(-0.2f, -0.5f, -0.5f), Vec3(0.8f, 0.5f, 0.5f)), A_Z));
		TEST_CHECK(Overlap(MinMax(Vec3(-2.f, -1.f, -1.f), Vec3(-1.f, 0.f, 0.f)), MinMax(Vec3(-0.2f, -0.5f, 3.f), Vec3(0.8f, 0.5f, 4.f)), A_Z) == false);
	}
	{
		TEST_CHECK(Overlap(MinMax(Vec3(-1.f, -1.f, -1.f), Vec3(1.0f, 1.f, 1.f)), MinMax(Vec3(0.2f, -0.5f, -0.2f), Vec3(0.7f, 0.5f, 0.8f))));
		TEST_CHECK(Overlap(MinMax(Vec3(0.2f, -0.5f, -0.2f), Vec3(0.7f, 0.5f, 0.8f)), MinMax(Vec3(-1.f, -1.f, -1.f), Vec3(1.0f, 1.f, 1.f))));
		TEST_CHECK(Overlap(MinMax(Vec3(-1.f, -1.f, -1.f), Vec3(1.0f, 1.f, 1.f)), MinMax(Vec3(1.2f, -0.5f, -0.2f), Vec3(1.7f, 0.5f, 0.8f))) == false);
		TEST_CHECK(Overlap(MinMax(Vec3(-1.f, -1.f, -1.f), Vec3(1.0f, 1.f, 1.f)), MinMax(Vec3(0.2f, -3.f, -2.2f), Vec3(0.7f, -2.f, -1.2f))) == false);
	}
	{
		MinMax m0(Vec3(-1.f, -1.f, -1.f), Vec3(1.f, 1.f, 1.f));
		MinMax m1(Vec3(-1.f, -1.f, -1.f), Vec3(1.f, 1.f, 1.f));
		MinMax m2(Vec3(0.2f, -0.5f, -0.2f), Vec3(0.7f, 0.5f, 0.8f));
		TEST_CHECK(m0 == m1);
		TEST_CHECK((m0 == m2) == false);
		TEST_CHECK((m1 == m2) == false);
	}	
	{
		MinMax m0(Vec3(-1.f, -1.f, -1.f), Vec3(1.f, 1.f, 1.f));
		MinMax m1(Vec3(-1.f, -1.f, -1.f), Vec3(1.f, 1.f, 1.f));
		MinMax m2(Vec3(0.2f, -0.5f, -0.2f), Vec3(0.7f, 0.5f, 0.8f));
		TEST_CHECK(m0 != m2);
		TEST_CHECK(m1 != m2);
		TEST_CHECK((m0 != m1) == false);
		
	}
	{
		MinMax box(Vec3(-1.f, -1.f, -1.f), Vec3(1.f, 1.f, 1.f));
		TEST_CHECK(Contains(box, Vec3(-0.5f, 0.75f, -0.1f)));
		TEST_CHECK(Contains(box, Vec3(0.4f, -1.75f, 0.3f)) == false);
		TEST_CHECK(Contains(box, Vec3(1.4f, 0.5f, 0.3f)) == false);
		TEST_CHECK(Contains(box, Vec3(-0.2f, 0.5f, 6.3f)) == false);
		TEST_CHECK(Contains(box, Vec3(0.8f, 0.5f, -2.3f)) == false);
	}
	{
		MinMax m0(Vec3(-1.f, -1.f, -1.f), Vec3(1.0f, 1.f, 1.f));
		MinMax m1(Vec3(0.2f, -0.5f, -0.2f), Vec3(0.7f, 0.5f, 0.8f));
		MinMax m2(Vec3(0.5f, 0.5f, 0.5f), Vec3(1.5f, 1.5f, 1.5f));
		MinMax m3(Vec3(-3.5f, -2.0f, 0.5f), Vec3(-3.f, -1.f, 1.5f));
		TEST_CHECK(Union(m0, m1) == m0);
		TEST_CHECK(Union(m1, m0) == m0);
		TEST_CHECK(Union(m0, m2) == MinMax(Vec3(-1.f, -1.f, -1.f), Vec3(1.5f, 1.5f, 1.5f)));
		TEST_CHECK(Union(m0, m3) == MinMax(Vec3(-3.5f, -2.f, -1.f), Vec3(1.f, 1.f, 1.5f)));
	}
	{
		MinMax m0(Vec3(-1.f, -1.f, -1.f), Vec3(1.0f, 1.f, 1.f));
		TEST_CHECK(Union(m0, Vec3(0.7f, -0.2f, 0.1f)) == m0);
		{
			MinMax m1 = Union(m0, Vec3(1.7f, -0.2f, 0.1f)); 
			TEST_CHECK(AlmostEqual(m1.mn, m0.mn, 0.000001f));
			TEST_CHECK(AlmostEqual(m1.mx, Vec3(1.7f, 1.f, 1.f), 0.000001f));
		}
		{
			MinMax m1 = Union(m0, Vec3(0.1f, -2.6f, 1.2f));
			TEST_CHECK(AlmostEqual(m1.mn, Vec3(-1.f,-2.6f,-1.f), 0.000001f));
			TEST_CHECK(AlmostEqual(m1.mx, Vec3(1.f, 1.f, 1.2f), 0.000001f));
		}
		{
			MinMax m1 = Union(m0, Vec3(-3.1f, -2.6f,-1.2f));
			TEST_CHECK(AlmostEqual(m1.mn, Vec3(-3.1f, -2.6f, -1.2f), 0.000001f));
			TEST_CHECK(AlmostEqual(m1.mx, Vec3(1.f, 1.f, 1.f), 0.000001f));
		}
		{
			MinMax m1 = Union(m0, Vec3(4.1f, 1.5f, 2.5f));
			TEST_CHECK(AlmostEqual(m1.mn, Vec3(-1.f, -1.f, -1.f), 0.000001f));
			TEST_CHECK(AlmostEqual(m1.mx, Vec3(4.1f, 1.5f, 2.5f), 0.000001f));
		}
	}
	{ 
		MinMax m0(-Vec3::One, Vec3::One); 
		Mat4 trs = TransformationMat4(Vec3(-0.5f, 0.5f, 0.8f), Deg3(45.f, -30.f, 60.f), Vec3(2.f, 0.8f, 1.f));
		MinMax m1 = trs * m0;
		Vec3 p0 = trs * m0.mn;
		Vec3 p1 = trs * m0.mx;
		TEST_CHECK(AlmostEqual(m1.mn, Min(p0, p1), 0.000001f));
		TEST_CHECK(AlmostEqual(m1.mx, Max(p0, p1), 0.000001f));
	}
	{
		MinMax m0(-Vec3::One, Vec3::One);
		bool ret = false;
		float t0 = 0.f, t1 = 0.f;
		TEST_CHECK(IntersectRay(m0, Vec3(0.5f, -0.5f, 0.5f), Vec3(0.f, 0.f, 1.f), t0, t1));
		TEST_CHECK(Equal(t0, 0.0f));
		TEST_CHECK(Equal(t1, 0.5f));

		TEST_CHECK(IntersectRay(m0, Vec3(2.0f, 1.0f, -1.0f), Vec3(0.f, 0.f, 1.f), t0, t1) == false);
		TEST_CHECK(Equal(t0, 0.0f));
		TEST_CHECK(Equal(t1, FLT_MAX));

		TEST_CHECK(IntersectRay(m0, Vec3(2.0f, 0.1f, 0.1f), Vec3(0.f, 0.f,-1.f), t0, t1) == false);
		TEST_CHECK(Equal(t0, 0.0f));
		TEST_CHECK(Equal(t1, FLT_MAX));

		TEST_CHECK(IntersectRay(m0, Vec3(2.0f, 2.0f, 2.0f), Vec3(0.f, 1.f, 0.f)) == false);

		TEST_CHECK(IntersectRay(m0, Vec3(1.0f, 2.0f, -2.0f), Normalize(Vec3(-1.f,-1.f, 1.f))));
	}
	{
		MinMax m0(-Vec3::One, Vec3::One);
		bool ret = false;
		Vec3 i, n;

		TEST_CHECK(ClassifyLine(m0, Vec3(0.5f, -0.5f, 0.5f), Vec3(0.f, 0.f, 1.f), i, &n));
		TEST_CHECK(AlmostEqual(i, Vec3(0.5f, -0.5f, 0.5), 0.000001f));
		TEST_CHECK(AlmostEqual(n, Vec3::Zero, 0.000001f));

		TEST_CHECK(ClassifyLine(m0, Vec3(-2.f, 1.0f, 0.f), Normalize(Vec3(1.f, -1.f, 0.f)), i, &n));
		TEST_CHECK(AlmostEqual(i, Vec3(-1.f, 0.f, 0.f), 0.000001f));
		TEST_CHECK(AlmostEqual(n, Vec3(-1.f, 0.f, 0.f), 0.000001f));

		TEST_CHECK(ClassifyLine(m0, Vec3(-4.f, 1.0f, 0.f), Normalize(Vec3(1.f, -1.f, 0.f)), i, &n) == false);

		TEST_CHECK(ClassifyLine(m0, Vec3(0.5f, 2.0f, 0.5f), Vec3(0.f, -1.f, 0.f), i, &n));
		TEST_CHECK(AlmostEqual(i, Vec3(0.5f, 1.f, 0.5f), 0.000001f));
		TEST_CHECK(AlmostEqual(n, Vec3(0.f, 1.f, 0.f), 0.000001f));

		TEST_CHECK(ClassifyLine(m0, Vec3(1.5f, 2.0f, 0.5f), Vec3(0.f, -1.f, 0.f), i, &n) == false);

		TEST_CHECK(ClassifyLine(m0, Vec3(-0.5f, -2.0f, -0.5f), Vec3(0.f, 1.f, 0.f), i, &n));
		TEST_CHECK(AlmostEqual(i, Vec3(-0.5f, -1.f, -0.5f), 0.000001f));
		TEST_CHECK(AlmostEqual(n, Vec3(0.f, -1.f, 0.f), 0.000001f));

		TEST_CHECK(ClassifyLine(m0, Vec3(1.5f, -2.0f, -0.5f), Vec3(0.f, 1.f, 0.f), i, &n) == false);

		TEST_CHECK(ClassifyLine(m0, Vec3(-0.5f, -0.5f, 2.0f), Vec3(0.f, 0.f, -1.f), i, &n));
		TEST_CHECK(AlmostEqual(i, Vec3(-0.5f, -0.5f, 1.f), 0.000001f));
		TEST_CHECK(AlmostEqual(n, Vec3(0.f, 0.f, 1.f), 0.000001f));

		TEST_CHECK(ClassifyLine(m0, Vec3(-0.5f,-2.0f, 2.0f), Vec3(0.f, 0.f, 1.f), i, &n) == false);

		TEST_CHECK(ClassifyLine(m0, Vec3(-0.5f, 0.5f, -2.0f), Vec3(0.f, 0.f, 1.f), i, &n));
		TEST_CHECK(AlmostEqual(i, Vec3(-0.5f, 0.5f, -1.f), 0.000001f));
		TEST_CHECK(AlmostEqual(n, Vec3(0.f, 0.f, -1.f), 0.000001f));

		TEST_CHECK(ClassifyLine(m0, Vec3(0.5f, -2.0f, -2.0f), Vec3(0.f, 0.f, 1.f), i, &n) == false);
	}
	{
		MinMax m0(-Vec3::One, Vec3::One);
		bool ret = false;
		Vec3 i, n;

		TEST_CHECK(ClassifySegment(m0, Vec3(0.5f, -0.5f, 0.5f), Vec3(0.5f, -0.5f, 1.5f), i, &n));
		TEST_CHECK(AlmostEqual(i, Vec3(0.5f, -0.5f, 0.5), 0.000001f));
		TEST_CHECK(AlmostEqual(n, Vec3::Zero, 0.000001f));

		TEST_CHECK(ClassifySegment(m0, Vec3(0.5f, -0.5f, 0.5f), Vec3(0.5f, -0.5f, 0.75f), i, &n));
		TEST_CHECK(AlmostEqual(i, Vec3(0.5f, -0.5f, 0.5f), 0.000001f));
		TEST_CHECK(AlmostEqual(n, Vec3::Zero, 0.000001f));

		TEST_CHECK(ClassifySegment(m0, Vec3(2.f, 2.0f, 2.f), Vec3(0.f, 0.f, 0.f), i, &n));
		TEST_CHECK(AlmostEqual(i, Vec3(0.f, 0.f, 0.f), 0.000001f));
		TEST_CHECK(AlmostEqual(n, Vec3::Zero, 0.000001f));

		TEST_CHECK(ClassifySegment(m0, Vec3(-2.f, 1.0f, 0.f), Vec3(1.f, -2.f, 0.f), i, &n));
		TEST_CHECK(AlmostEqual(i, Vec3(-1.f, 0.f, 0.f), 0.000001f));
		TEST_CHECK(AlmostEqual(n, Vec3(-1.f, 0.f, 0.f), 0.000001f));

		TEST_CHECK(ClassifySegment(m0, Vec3(2.f, 1.0f, 0.f), Vec3(-1.f, -2.f, 0.f), i, &n));
		TEST_CHECK(AlmostEqual(i, Vec3(1.f, 0.f, 0.f), 0.000001f));
		TEST_CHECK(AlmostEqual(n, Vec3(1.f, 0.f, 0.f), 0.000001f));

		TEST_CHECK(ClassifySegment(m0, Vec3(-2.f, 1.0f, 0.f), Vec3(-1.5f, 0.5f, 0.f), i, &n) == false);
		TEST_CHECK(ClassifySegment(m0, Vec3(-4.f, 1.0f, 0.f), Vec3(-1.f, -2.f, 0.f), i, &n) == false);

		TEST_CHECK(ClassifySegment(m0, Vec3(0.5f, 2.0f, 0.5f), Vec3(0.5f, -1.2f, 0.5f), i, &n));
		TEST_CHECK(AlmostEqual(i, Vec3(0.5f, 1.f, 0.5f), 0.000001f));
		TEST_CHECK(AlmostEqual(n, Vec3(0.f, 1.f, 0.f), 0.000001f));

		TEST_CHECK(ClassifySegment(m0, Vec3(0.5f, 2.0f, 0.5f), Vec3(0.5f, 1.1f, 0.5f), i, &n) == false);
		TEST_CHECK(ClassifySegment(m0, Vec3(1.5f, 2.0f, 0.5f), Vec3(1.5f, -1.f, 0.5f), i, &n) == false);
		
		TEST_CHECK(ClassifySegment(m0, Vec3(-0.5f, -2.0f, -0.5f), Vec3(-0.5f, 2.f, -0.5f), i, &n));
		TEST_CHECK(AlmostEqual(i, Vec3(-0.5f, -1.f, -0.5f), 0.000001f));
		TEST_CHECK(AlmostEqual(n, Vec3(0.f, -1.f, 0.f), 0.000001f));

		TEST_CHECK(ClassifySegment(m0, Vec3(-0.5f, -2.0f, -0.5f), Vec3(0.5f, -1.5f, -0.5f), i, &n) == false);
		TEST_CHECK(ClassifySegment(m0, Vec3(1.5f, -2.0f, -0.5f), Vec3(1.5f, 2.f, -0.5f), i, &n) == false);

		TEST_CHECK(ClassifySegment(m0, Vec3(-0.5f, -0.5f, 2.0f), Vec3(-0.5f,-0.5f,-5.f), i, &n));
		TEST_CHECK(AlmostEqual(i, Vec3(-0.5f, -0.5f, 1.f), 0.000001f));
		TEST_CHECK(AlmostEqual(n, Vec3(0.f, 0.f, 1.f), 0.000001f));

		TEST_CHECK(ClassifySegment(m0, Vec3(-0.5f, -2.0f, 2.0f), Vec3(-0.5f, -2.f, -4.f), i, &n) == false);
		TEST_CHECK(ClassifySegment(m0, Vec3(-0.5f, -0.5f, 2.0f), Vec3(-0.5f, -0.5f, 1.5f), i, &n) == false);

		TEST_CHECK(ClassifySegment(m0, Vec3(-0.5f, 0.5f, -2.0f), Vec3(-0.5f, 0.5f, 3.f), i, &n));
		TEST_CHECK(AlmostEqual(i, Vec3(-0.5f, 0.5f, -1.f), 0.000001f));
		TEST_CHECK(AlmostEqual(n, Vec3(0.f, 0.f, -1.f), 0.000001f));

		TEST_CHECK(ClassifySegment(m0, Vec3(-0.5f, 0.5f, -2.0f), Vec3(-0.5f, 0.5f, -1.3f), i, &n) == false);
		TEST_CHECK(ClassifySegment(m0, Vec3(0.5f, -2.0f, -2.0f), Vec3(0.5f, -4.f, 4.f), i, &n) == false);
	}
	{
		MinMax m0 = MinMaxFromPositionSize(Vec3::Zero, Vec3::One);
		TEST_CHECK(AlmostEqual(m0.mn, Vec3(-0.5f), 0.000001f));
		TEST_CHECK(AlmostEqual(m0.mx, Vec3( 0.5f), 0.000001f));
	}
}
