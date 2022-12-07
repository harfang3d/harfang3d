// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <math.h>

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/math.h"
#include "foundation/vector2.h"
#include "foundation/vector3.h"
#include "foundation/vector4.h"

using namespace hg;

void test_vec2() {
	{
		Vec3 v3(0.f, 1.f, -1.f);
		Vec2 vf(v3);
		TEST_CHECK(Equal(vf.x, 0.f));
		TEST_CHECK(Equal(vf.y, 1.f));
		iVec2 vi(v3);
		TEST_CHECK(vi.x == 0);
		TEST_CHECK(vi.y == 1);
	}
	{
		Vec4 v4(1.f, 2.f, 3.f, 4.f);
		Vec2 vf(v4);
		TEST_CHECK(Equal(vf.x, 1.f));
		TEST_CHECK(Equal(vf.y, 2.f));
		iVec2 vi(v4);
		TEST_CHECK(vi.x == 1);
		TEST_CHECK(vi.y == 2);
	}
	{
		Vec2 u(0.5f, -0.5f);
		Vec2 v(1.f, -1.f);
		Vec2 w = u + v;
		TEST_CHECK(Equal(w.x, 1.5f));
		TEST_CHECK(Equal(w.y, -1.5f));
	}
	{
		iVec2 u(1, -1);
		iVec2 v(2, 6);
		iVec2 w = u + v;
		TEST_CHECK(w.x == 3);
		TEST_CHECK(w.y == 5);
	}
	{
		Vec2 u(0.5f, -0.5f);
		Vec2 v = u + 0.25f;
		TEST_CHECK(Equal(v.x, 0.75f));
		TEST_CHECK(Equal(v.y, -0.25f));
	}
	{
		iVec2 u(-2, -1);
		iVec2 v = u + 2;
		TEST_CHECK(v.x == 0);
		TEST_CHECK(v.y == 1);
	}
	{
		Vec2 u(0.5f, -0.5f);
		Vec2 v(1.f, -1.f);
		Vec2 w = u - v;
		TEST_CHECK(Equal(w.x, -0.5f));
		TEST_CHECK(Equal(w.y, 0.5f));
	}
	{
		iVec2 u(3, 6);
		iVec2 v(4, 8);
		iVec2 w = u - v;
		TEST_CHECK(w.x == -1);
		TEST_CHECK(w.y == -2);
	}
	{
		Vec2 u(0.5f, -0.5f);
		Vec2 v = u - 0.25f;
		TEST_CHECK(Equal(v.x, 0.25f));
		TEST_CHECK(Equal(v.y, -0.75f));
	}
	{
		iVec2 u(-2, -1);
		iVec2 v = u - 2;
		TEST_CHECK(v.x == -4);
		TEST_CHECK(v.y == -3);
	}
	{
		Vec2 u(0.15f, -2.5f);
		Vec2 v(1.1f, 0.3f);
		Vec2 w = u * v;
		TEST_CHECK(Equal(w.x, 0.165f));
		TEST_CHECK(Equal(w.y, -0.75f));
	}
	{
		iVec2 u(5, 9);
		iVec2 v(3, -7);
		iVec2 w = u * v;
		TEST_CHECK(w.x == 15);
		TEST_CHECK(w.y == -63);
	}
	{
		Vec2 u(-5.06f, 0.75f);
		Vec2 v = u * 4.25f;
		Vec2 w = 4.25f * u;
		TEST_CHECK(Equal(v.x, -21.505f));
		TEST_CHECK(Equal(v.y, 3.1875f));
		TEST_CHECK(Equal(w.x, v.x));
		TEST_CHECK(Equal(w.y, v.y));
	}
	{
		iVec2 u(-77, 109);
		iVec2 v = u * 3;
		iVec2 w = 3 * u;
		TEST_CHECK(v.x == -231);
		TEST_CHECK(v.y == 327);
		TEST_CHECK(w.x == v.x);
		TEST_CHECK(w.y == v.y);
	}
	{
		Vec2 u(0.48f, -2.79f);
		Vec2 v(1.5f, 0.03f);
		Vec2 w = u / v;
		TEST_CHECK(AlmostEqual(w.x, 0.32, 1e-5f));
		TEST_CHECK(AlmostEqual(w.y, -93.f, 1e-5f));
	}
	{
		iVec2 u(104, 54);
		iVec2 v(3, -3);
		iVec2 w = u / v;
		TEST_CHECK(w.x == 34);
		TEST_CHECK(w.y == -18);
	}
	{
		Vec2 u(9.008f, 0.75f);
		Vec2 v = u / 0.1f;
		TEST_CHECK(Equal(v.x, 90.08f));
		TEST_CHECK(Equal(v.y, 7.5f));
	}
	{
		iVec2 u(200, 18);
		iVec2 v = u / 4;
		TEST_CHECK(v.x == 50);
		TEST_CHECK(v.y == 4);
	}
	{
		Vec2 u(1.4f, -2.75f);
		u += Vec2(-0.2f, 5.25);
		TEST_CHECK(Equal(u.x, 1.2f));
		TEST_CHECK(Equal(u.y, 2.5f));
	}
	{
		iVec2 u(-8, 5);
		u += iVec2(-2, 1);
		TEST_CHECK(u.x == -10);
		TEST_CHECK(u.y == 6);
	}
	{
		Vec2 u(-3.1f, 10.02f);
		u += 0.1f;
		TEST_CHECK(Equal(u.x, -3.f));
		TEST_CHECK(AlmostEqual(u.y, 10.12f));
	}
	{
		iVec2 u(7, 21);
		u += 5;
		TEST_CHECK(u.x == 12);
		TEST_CHECK(u.y == 26);
	}
	{
		Vec2 u(0.336f, 5.5555f);
		u -= Vec2(1.01f, 2.2222f);
		TEST_CHECK(Equal(u.x, -0.674f));
		TEST_CHECK(Equal(u.y, 3.3333f));
	}
	{
		iVec2 u(7, -4);
		u -= iVec2(5, -2);
		TEST_CHECK(u.x == 2);
		TEST_CHECK(u.y == -2);
	}
	{
		Vec2 u(2.3235f, 7.81194f);
		u -= 0.004f;
		TEST_CHECK(Equal(u.x, 2.3195f));
		TEST_CHECK(Equal(u.y, 7.80794f));
	}
	{
		iVec2 u(100, 5);
		u -= 10;
		TEST_CHECK(u.x == 90);
		TEST_CHECK(u.y == -5);
	}
	{
		Vec2 u(4.0f, -11.f);
		u *= Vec2(0.5f, 0.4f);
		TEST_CHECK(Equal(u.x, 2.f));
		TEST_CHECK(Equal(u.y, -4.4f));
	}
	{
		iVec2 u(4, 7);
		u *= iVec2(4, -10);
		TEST_CHECK(u.x == 16);
		TEST_CHECK(u.y == -70);
	}
	{
		Vec2 u(11.11f, 0.075f);
		u *= 0.3f;
		TEST_CHECK(Equal(u.x, 3.333f));
		TEST_CHECK(Equal(u.y, 0.0225f));
	}
	{
		iVec2 u(5, 101);
		u *= 3;
		TEST_CHECK(u.x == 15);
		TEST_CHECK(u.y == 303);
	}
	{
		Vec2 u(4.8f, -26.f);
		u /= Vec2(0.3f, -0.104f);
		TEST_CHECK(AlmostEqual(u.x, 16.f, 1e-4f));
		TEST_CHECK(AlmostEqual(u.y, 250.f, 1e-4f));
	}
	{
		iVec2 u(8, 365);
		u /= iVec2(2, 97);
		TEST_CHECK(u.x == 4);
		TEST_CHECK(u.y == 3);
	}
	{
		Vec2 u(0.03f, 27.6f);
		u /= 0.75f;
		TEST_CHECK(AlmostEqual(u.x, 0.04f, 1e-5f));
		TEST_CHECK(AlmostEqual(u.y, 36.8f, 1e-5f));
	}
	{
		iVec2 u(121, -909);
		u /= 11;
		TEST_CHECK(u.x == 11);
		TEST_CHECK(u.y == -82);
	}
	{
		Vec2 u(1.207f, -44.01f);
		TEST_CHECK(Equal(u[0], u.x));
		TEST_CHECK(Equal(u[1], u.y));

		u[0] = -47.f;
		TEST_CHECK(Equal(u.x, -47.f));
	}
	{
		const Vec2 u(1.207f, -44.01f);
		TEST_CHECK(Equal(u[0], u.x));
		TEST_CHECK(Equal(u[1], u.y));
		TEST_CHECK(Equal(u[8], std::numeric_limits<float>::max()));
	}
	{
		iVec2 u(121, -909);
		TEST_CHECK(u[0] == u.x);
		TEST_CHECK(u[1] == u.y);

		u[1] = 28;
		TEST_CHECK(u.y == 28);
	}
	{
		const iVec2 u(121, -909);
		TEST_CHECK(u[0] == u.x);
		TEST_CHECK(u[1] == u.y);
		TEST_CHECK(Equal(u[2], std::numeric_limits<int>::max()));
	}
	{
		Vec2 u(1.207f, -44.01f);
		Vec2 v(1.207f, -44.01f);
		Vec2 w(4.4444f, 1.0001f);
		TEST_CHECK(u == v);
		TEST_CHECK((u == w) == false);
	}
	{
		iVec2 u(121, -909);
		iVec2 v(121, -909);
		iVec2 w(4, -908);
		TEST_CHECK(u == v);
		TEST_CHECK((u == w) == false);
	}
	{
		Vec2 u(1.207f, -44.01f);
		Vec2 v(1.207f, -44.01f);
		Vec2 w(4.4444f, 1.0001f);
		TEST_CHECK(u != w);
		TEST_CHECK((u != v) == false);
	}
	{
		iVec2 u(121, -909);
		iVec2 v(121, -909);
		iVec2 w(4, -908);
		TEST_CHECK(u != w);
		TEST_CHECK((u != v) == false);
	}
	{
		Vec2 u(-3.0444f, 102.001f);
		Vec2 v(-3.0443f, 102.00105f);
		Vec2 w(-3.04f, 102.0015f);
		TEST_CHECK(AlmostEqual(u, v, 0.0001f));
		TEST_CHECK(AlmostEqual(u, w, 0.0001f) == false);
		TEST_CHECK(AlmostEqual(u, w, 0.005f));
	}
	{
		iVec2 u(10001, 10003);
		iVec2 v(10000, 10004);
		iVec2 w(10011, 9999);
		TEST_CHECK(AlmostEqual(u, v, 1));
		TEST_CHECK(AlmostEqual(u, w, 1) == false);
		TEST_CHECK(AlmostEqual(u, w, 10));
	}
	{
		TEST_CHECK(Min(Vec2(0.336f, 5.5555f), Vec2(1.01f, 2.2222f)) == Vec2(0.336f, 2.2222f));
		TEST_CHECK(Min(iVec2(7, -4), iVec2(5, -2)) == iVec2(5, -4));
	}
	{
		TEST_CHECK(Max(Vec2(0.336f, 5.5555f), Vec2(1.01f, 2.2222f)) == Vec2(1.01f, 5.5555f));
		TEST_CHECK(Max(iVec2(7, -4), iVec2(5, -2)) == iVec2(7, -2));
	}
	{
		TEST_CHECK(Equal(Len2(Vec2(1.f, -1.f)), 2.f));
		TEST_CHECK(Len2(iVec2(-1, -1)) == 2);
	}
	{
		TEST_CHECK(Equal(Len(Vec2(3.f, -4.f)), 5.f));
		TEST_CHECK(Len(iVec2(-4, -3)) == 5);
	}
	{
		TEST_CHECK(Equal(Dot(Vec2(1.f, -1.f), Vec2(-1.f, -1.f)), 0.f));
		TEST_CHECK(Equal(Dot(Vec2(2.f, 3.f), Vec2(4.f, 5.f)), 23.f));
		TEST_CHECK(Dot(iVec2(1, 1), iVec2(-1, -1)) == -2);
		TEST_CHECK(Dot(iVec2(2, -3), iVec2(4, 5)) == -7);
	}
	{
		TEST_CHECK(Normalize(Vec2(0.3f, -0.4f)) == Vec2(0.6, -0.8));
		TEST_CHECK(Equal(Len(Normalize(Vec2(0.3f, -0.4f))), 1.f));
		TEST_CHECK(Normalize(iVec2(-1, 1)) == iVec2(-1, 1));
		TEST_CHECK(Len(Normalize(iVec2(-1, 1))) == 1);
	}
	{
		TEST_CHECK(Reverse(Vec2(-1.1f, 0.5f)) == Vec2(1.1, -0.5f));
		TEST_CHECK(Reverse(iVec2(78, -99)) == iVec2(-78, 99));
	}
	{
		TEST_CHECK(AlmostEqual(Dist2(Vec2(0.3f, -1.2f), Vec2(1.2f, 3.1f)), 19.3f, 0.00001f));
		TEST_CHECK(Equal(Dist2(iVec2(4, 12), iVec2(7, 8)), 25));
	}
	{
		TEST_CHECK(Equal(Dist(Vec2(1.4f, 2.3f), Vec2(-0.2f, -0.1f)), sqrt(8.32f)));
		TEST_CHECK(Equal(Dist(iVec2(-2, -2), iVec2(1, 2)), 5));
	}
	{
		Vec2 u = -Vec2(0.03f, 27.6f);
		TEST_CHECK(Equal(u.x, -0.03f));
		TEST_CHECK(Equal(u.y, -27.6f));
	}
	{
		iVec2 u = -iVec2(121, -909);
		TEST_CHECK(u.x == -121);
		TEST_CHECK(u.y == 909);
	}
	{
		Vec2 u(-3.0444f, 102.001f);
		Vec2 v(-3.0443f, 102.00105f);
		Vec2 w(-3.04f, 102.0015f);
		TEST_CHECK(AlmostEqual(u, v, 0.0001f));
		TEST_CHECK(AlmostEqual(u, w, 0.0001f) == false);
		TEST_CHECK(AlmostEqual(u, w, 0.005f));
	}
}
