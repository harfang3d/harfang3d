// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <math.h>

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/vector4.h"

#include "foundation/color.h"
#include "foundation/matrix4.h"
#include "foundation/vector2.h"
#include "foundation/vector3.h"

#include "foundation/math.h"

using namespace hg;

void test_vec4() {
	{
		Vec2 v2f(0.111f, 2.222f);
		Vec4 vf(v2f);
		TEST_CHECK(Equal(vf.x, 0.111f));
		TEST_CHECK(Equal(vf.y, 2.222f));
		TEST_CHECK(Equal(vf.z, 0.f));
		TEST_CHECK(Equal(vf.w, 1.f));
	}
	{
		iVec2 v2i(3, -17);
		Vec4 vf(v2i);
		TEST_CHECK(Equal(vf.x, 3.f));
		TEST_CHECK(Equal(vf.y, -17.f));
		TEST_CHECK(Equal(vf.z, 0.f));
		TEST_CHECK(Equal(vf.w, 1.f));
	}
	{
		Vec4 v(3.333f);
		TEST_CHECK(Equal(v.x, 3.333f));
		TEST_CHECK(Equal(v.y, 3.333f));
		TEST_CHECK(Equal(v.z, 3.333f));
		TEST_CHECK(Equal(v.w, 3.333f));
	}
	{
		Color c(1.f, 2.f, 3.f, 4.f);
		Vec4 v(c);
		TEST_CHECK(Equal(v.x, 1.f));
		TEST_CHECK(Equal(v.y, 2.f));
		TEST_CHECK(Equal(v.z, 3.f));
		TEST_CHECK(Equal(v.w, 4.f));
	}
	{
		Vec4 v = -Vec4(1.f, 2.f, 3.f, -5.f);
		TEST_CHECK(Equal(v.x, -1.f));
		TEST_CHECK(Equal(v.y, -2.f));
		TEST_CHECK(Equal(v.z, -3.f));
		TEST_CHECK(Equal(v.w, 5.f));
	}
	{
		Vec4 u(0.22f, 1.4f, -2.75f, -0.3503f);
		u += Vec4(0.042f, -0.2f, 5.25f, 0.9017f);
		TEST_CHECK(Equal(u.x, 0.262f));
		TEST_CHECK(Equal(u.y, 1.2f));
		TEST_CHECK(Equal(u.z, 2.5f));
		TEST_CHECK(Equal(u.w, 0.5514f));
	}
	{
		Vec4 u(20.97f, -3.1f, 10.02f, -1.402f);
		u += 0.1f;
		TEST_CHECK(Equal(u.x, 21.07f));
		TEST_CHECK(Equal(u.y, -3.f));
		TEST_CHECK(AlmostEqual(u.z, 10.12f));
		TEST_CHECK(Equal(u.w, -1.302f));
	}
	{
		Vec4 u(55.555f, 0.336f, 5.5555f, 7.9191f);
		u -= Vec4(2.002f, 1.01f, 2.2222f, 0.054f);
		TEST_CHECK(Equal(u.x, 53.553f));
		TEST_CHECK(Equal(u.y, -0.674f));
		TEST_CHECK(Equal(u.z, 3.3333f));
		TEST_CHECK(Equal(u.w, 7.8651f));
	}
	{
		Vec4 u(-1.025f, 2.3235f, 7.81194f, -0.40441f);
		u -= 0.004f;
		TEST_CHECK(Equal(u.x, -1.029));
		TEST_CHECK(Equal(u.y, 2.3195f));
		TEST_CHECK(Equal(u.z, 7.80794f));
		TEST_CHECK(Equal(u.w, -0.40841f));
	}
	{
		Vec4 u(7.011f, 4.0f, -11.f, 1.202f);
		u *= Vec4(0.33f, 0.5f, 0.4f, -5.01f);
		TEST_CHECK(Equal(u.x, 2.31363f));
		TEST_CHECK(Equal(u.y, 2.f));
		TEST_CHECK(Equal(u.z, -4.4f));
		TEST_CHECK(AlmostEqual(u.w, -6.02202f));
	}
	{
		Vec4 u(-2.022f, 11.11f, 0.075f, -101.3f);
		u *= 0.3f;
		TEST_CHECK(Equal(u.x, -0.6066f));
		TEST_CHECK(Equal(u.y, 3.333f));
		TEST_CHECK(Equal(u.z, 0.0225f));
		TEST_CHECK(AlmostEqual(u.w, -30.39f, 0.00001f));
	}
	{
		Vec4 u(0.4f, 4.8f, -26.f, 1.4f);
		u /= Vec4(-0.25f, 0.3f, -0.104f, 0.07f);
		TEST_CHECK(Equal(u.x, -1.6f));
		TEST_CHECK(Equal(u.y, 16.f));
		TEST_CHECK(Equal(u.z, 250.f));
		TEST_CHECK(Equal(u.w, 20.f));
	}
	{
		Vec4 u(-2.015f, 0.03f, 27.6f, -0.975f);
		u /= 0.75f;
		TEST_CHECK(AlmostEqual(u.x, -2.686666666f));
		TEST_CHECK(Equal(u.y, 0.04f));
		TEST_CHECK(AlmostEqual(u.z, 36.8f, 0.00001f));
		TEST_CHECK(Equal(u.w, -1.3f));
	}
	{
		Vec4 u(1.207f, -44.01f, 0.34034f, -54.0127f);
		TEST_CHECK(Equal(u[0], u.x));
		TEST_CHECK(Equal(u[1], u.y));
		TEST_CHECK(Equal(u[2], u.z));
		TEST_CHECK(Equal(u[3], u.w));

		u[1] = 9.125f;
		TEST_CHECK(Equal(u.y, 9.125f));
	}
	{ 
		const Vec4 u(-2.015f, 0.03f, 27.6f, -0.975f); 
		TEST_CHECK(Equal(u[0], u.x));
		TEST_CHECK(Equal(u[1], u.y));
		TEST_CHECK(Equal(u[2], u.z));
		TEST_CHECK(Equal(u[3], u.w));
		TEST_CHECK(u[4] == std::numeric_limits<float>::max());
	}
	{
		Vec4 u(75.757575f, 1.207f, -44.01f, 0.192f);
		Vec4 v(75.757575f, 1.207f, -44.01f, 0.192f);
		Vec4 w(70.0101f, 4.4444f, 1.0001f, 0.4f);
		TEST_CHECK(u == v);
		TEST_CHECK((u == w) == false);
	}
	{
		Vec4 u(75.757575f, 1.207f, -44.01f, 0.192f);
		Vec4 v(75.757575f, 1.207f, -44.01f, 0.192f);
		Vec4 w(70.0101f, 4.4444f, 1.0001f, 0.4f);
		TEST_CHECK(u != w);
		TEST_CHECK((u != v) == false);
	}
	{
		Vec4 u(2.020f, 0.5f, -0.5f, 0.3104f);
		Vec4 v(1.010f, 1.f, -1.f, 0.0306f);
		Vec4 w = u + v;
		TEST_CHECK(Equal(w.x, 3.030f));
		TEST_CHECK(Equal(w.y, 1.5f));
		TEST_CHECK(Equal(w.z, -1.5f));
		TEST_CHECK(Equal(w.w, 0.341f));
	}
	{
		Vec4 u(47.3473f, 0.5f, -0.5f, 0.754f);
		Vec4 v = u + 0.25f;
		TEST_CHECK(Equal(v.x, 47.5973f));
		TEST_CHECK(Equal(v.y, 0.75f));
		TEST_CHECK(Equal(v.z, -0.25f));
		TEST_CHECK(Equal(v.w, 1.004f));
	}
	{
		Vec4 u(0.5f, -0.5f, 1.25909f, -0.30303f);
		Vec4 v(1.f, -1.f, 1.70707f, 0.0101f);
		Vec4 w = u - v;
		TEST_CHECK(Equal(w.x, -0.5f));
		TEST_CHECK(Equal(w.y, 0.5f));
		TEST_CHECK(Equal(w.z, -0.44798f));
		TEST_CHECK(Equal(w.w, -0.31313f));
	}
	{
		Vec4 u(0.5f, -0.5f, 3.333f, 7.05f);
		Vec4 v = u - 0.25f;
		TEST_CHECK(Equal(v.x, 0.25f));
		TEST_CHECK(Equal(v.y, -0.75f));
		TEST_CHECK(Equal(v.z, 3.083f));
		TEST_CHECK(Equal(v.w, 6.8f));
	}
	{
		Vec4 u(0.15f, -2.5f, 1.505f, 0.08);
		Vec4 v(1.1f, 0.3f, 0.76f, 2.04f);
		Vec4 w = u * v;
		TEST_CHECK(Equal(w.x, 0.165f));
		TEST_CHECK(Equal(w.y, -0.75f));
		TEST_CHECK(Equal(w.z, 1.1438f));
		TEST_CHECK(Equal(w.w, 0.1632f));
	}
	{
		Vec4 u(-5.06f, 0.75f, 2.72645f, 0.1717f);
		Vec4 v = u * 4.25f;
		Vec4 w = 4.25f * u;
		TEST_CHECK(Equal(v.x, -21.505f));
		TEST_CHECK(Equal(v.y, 3.1875f));
		TEST_CHECK(AlmostEqual(v.z, 11.5874125f));
		TEST_CHECK(Equal(v.w, 0.729725f));
		TEST_CHECK(Equal(w.x, v.x));
		TEST_CHECK(Equal(w.y, v.y));
		TEST_CHECK(Equal(w.z, v.z));
		TEST_CHECK(Equal(w.w, v.w));
	}
	{
		Vec4 u(0.48f, -2.79f, -1.3334f, 0.0794f);
		Vec4 v(1.5f, 0.03f, -0.401401f, 2.2f);
		Vec4 w = u / v;
		TEST_CHECK(Equal(w.x, 0.32));
		TEST_CHECK(Equal(w.y, -93.f));
		TEST_CHECK(Equal(w.z, 3.321865167f));
		TEST_CHECK(Equal(w.w, 0.0360909f));
	}
	{
		Vec4 u(9.008f, 0.75f, -57.1002f, 3.7f);
		Vec4 v = u / 0.1f;
		TEST_CHECK(Equal(v.x, 90.08f));
		TEST_CHECK(Equal(v.y, 7.5f));
		TEST_CHECK(Equal(v.z, -571.002));
		TEST_CHECK(Equal(v.w, 37.f));
	}
	{
		Vec4 u = RandomVec4();
		TEST_CHECK(Abs(u.x) <= 1.f);
		TEST_CHECK(Abs(u.y) <= 1.f);
		TEST_CHECK(Abs(u.z) <= 1.f);
		TEST_CHECK(Abs(u.w) <= 1.f);
		Vec4 v = RandomVec4();
		TEST_CHECK(Equal(u.x, v.x) == false);
		TEST_CHECK(Equal(u.y, v.y) == false);
		TEST_CHECK(Equal(u.z, v.z) == false);
		TEST_CHECK(Equal(u.w, v.w) == false);
		Vec4 w = RandomVec4(0.f, 10.f);
		TEST_CHECK((w.x >= 0.f) && (w.x <= 10.f));
		TEST_CHECK((w.y >= 0.f) && (w.y <= 10.f));
		TEST_CHECK((w.z >= 0.f) && (w.z <= 10.f));
		TEST_CHECK((w.w >= 0.f) && (w.w <= 10.f));
	}
	{
		Vec4 u = RandomVec4(Vec4(-1.f, 0.f, -0.5f, 2.f), Vec4(0.f, 1.f, 0.5f, 4.f));
		TEST_CHECK((u.x >= -1.f) && (u.x <= 0.f));
		TEST_CHECK((u.y >= 0.f) && (u.y <= 1.f));
		TEST_CHECK((u.z >= -0.5f) && (u.z <= 0.5f));
		TEST_CHECK((u.w >= 2.0f) && (u.w <= 4.f));

		Vec4 v = RandomVec4(-Vec4::One, Vec4::One);
		Vec4 w = RandomVec4(-Vec4::One, Vec4::One);
		TEST_CHECK(Equal(v.x, w.x) == false);
		TEST_CHECK(Equal(v.y, w.y) == false);
		TEST_CHECK(Equal(v.z, w.z) == false);
		TEST_CHECK(Equal(v.w, w.w) == false);
	}
	{
		Vec4 u(-3.0444f, 102.001f, -0.0001f, 2.2012f);
		Vec4 v(-3.0443f, 102.00105f, -0.00005f, 2.20115f);
		Vec4 w(-3.04f, 102.0015f, 0.003f, 2.2014f);
		TEST_CHECK(AlmostEqual(u, v, 0.0001f));
		TEST_CHECK(AlmostEqual(u, w, 0.0001f) == false);
		TEST_CHECK(AlmostEqual(u, w, 0.005f));
	}
	{
		Vec4 v = Abs(Vec4(-1.f, 1.f, -0.5f, -0.5f));
		TEST_CHECK(Equal(v.x, 1.f));
		TEST_CHECK(Equal(v.y, 1.f));
		TEST_CHECK(Equal(v.z, 0.5f));
		TEST_CHECK(Equal(v.z, 0.5f));
	}
	{
		Vec4 v = Reverse(Vec4(1.f, 2.f, 3.f, 4.f));
		TEST_CHECK(Equal(v.x, -1.f));
		TEST_CHECK(Equal(v.y, -2.f));
		TEST_CHECK(Equal(v.z, -3.f));
		TEST_CHECK(Equal(v.w, -4.f));
	}
	{
		Vec4 v = Normalize(Vec4(4.701f, -4.701f, -4.701f, 4.701f));
		float l = Sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
		TEST_CHECK(Equal(l, 1.f));
	}
	{
		Vec4 v = Vec4::One;
		v *= Mat4(0.1f, 1.f, 0.1f, 0.2f, 2.f, 10.0f, 0.3f, 3.f, 0.01f, 0.4f, 4.f, 1.0f);
		TEST_CHECK(Equal(v.x, 1.f));
		TEST_CHECK(Equal(v.y, 10.f));
		TEST_CHECK(AlmostEqual(v.z, 11.11f));
		TEST_CHECK(Equal(v.w, 1.f));
	}
	{
		Vec4 u = Vec4I(255, 0, 128);
		TEST_CHECK(AlmostEqual(u.x, 1.f, 1.f / 255.f));
		TEST_CHECK(AlmostEqual(u.y, 0.f, 1.f / 255.f));
		TEST_CHECK(AlmostEqual(u.z, 0.5f, 1.f / 255.f));
		TEST_CHECK(AlmostEqual(u.w, 1.f, 1.f / 255.f));
	}
}
