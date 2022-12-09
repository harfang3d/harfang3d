// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <math.h>

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/math.h"
#include "foundation/vector2.h"
#include "foundation/vector3.h"
#include "foundation/vector4.h"

using namespace hg;

void test_vec3() {
	{
		Vec2 v2f(0.111f, 2.222f);
		Vec3 vf(v2f);
		TEST_CHECK(Equal(vf.x, 0.111f));
		TEST_CHECK(Equal(vf.y, 2.222f));
		TEST_CHECK(Equal(vf.z, 0.f));
	}
	{
		iVec2 v2i(-2, 55);
		Vec3 vi(v2i);
		TEST_CHECK(Equal(vi.x, -2.f));
		TEST_CHECK(Equal(vi.y, 55.f));
		TEST_CHECK(Equal(vi.z, 0.f));
	}
	{
		Vec3 v(3.333f);
		TEST_CHECK(Equal(v.x, 3.333f));
		TEST_CHECK(Equal(v.y, 3.333f));
		TEST_CHECK(Equal(v.z, 3.333f));
	}
	{
		Vec4 v4(1.f, 2.f, 3.f, 4.f);
		Vec3 v(v4);
		TEST_CHECK(Equal(v.x, 1.f));
		TEST_CHECK(Equal(v.y, 2.f));
		TEST_CHECK(Equal(v.z, 3.f));
	}
	{
		Vec3 v = -Vec3(1.f, 2.f, 3.f);
		TEST_CHECK(Equal(v.x, -1.f));
		TEST_CHECK(Equal(v.y, -2.f));
		TEST_CHECK(Equal(v.z, -3.f));
	}
	{
		Vec3 u(0.22f, 1.4f, -2.75f);
		u += Vec3(0.042f, -0.2f, 5.25f);
		TEST_CHECK(Equal(u.x, 0.262f));
		TEST_CHECK(Equal(u.y, 1.2f));
		TEST_CHECK(Equal(u.z, 2.5f));
	}
	{
		Vec3 u(20.97f, -3.1f, 10.02f);
		u += 0.1f;
		TEST_CHECK(Equal(u.x, 21.07f));
		TEST_CHECK(Equal(u.y, -3.f));
		TEST_CHECK(AlmostEqual(u.z, 10.12f));
	}
	{
		Vec3 u(55.555f, 0.336f, 5.5555f);
		u -= Vec3(2.002f, 1.01f, 2.2222f);
		TEST_CHECK(Equal(u.x, 53.553f));
		TEST_CHECK(Equal(u.y, -0.674f));
		TEST_CHECK(Equal(u.z, 3.3333f));
	}
	{
		Vec3 u(-1.025f, 2.3235f, 7.81194f);
		u -= 0.004f;
		TEST_CHECK(Equal(u.x, -1.029));
		TEST_CHECK(Equal(u.y, 2.3195f));
		TEST_CHECK(Equal(u.z, 7.80794f));
	}
	{
		Vec3 u(7.011f, 4.0f, -11.f);
		u *= Vec3(0.33f, 0.5f, 0.4f);
		TEST_CHECK(Equal(u.x, 2.31363f));
		TEST_CHECK(Equal(u.y, 2.f));
		TEST_CHECK(Equal(u.z, -4.4f));
	}
	{
		Vec3 u(-2.022f, 11.11f, 0.075f);
		u *= 0.3f;
		TEST_CHECK(Equal(u.x, -0.6066f));
		TEST_CHECK(Equal(u.y, 3.333f));
		TEST_CHECK(Equal(u.z, 0.0225f));
	}
	{
		Vec3 u(0.4f, 4.8f, -26.f);
		u /= Vec3(-0.25f, 0.3f, -0.104f);
		TEST_CHECK(Equal(u.x, -1.6f));
		TEST_CHECK(Equal(u.y, 16.f));
		TEST_CHECK(Equal(u.z, 250.f));
	}
	{
		Vec3 u(-2.015f, 0.03f, 27.6f);
		u /= 0.75f;
		TEST_CHECK(AlmostEqual(u.x, -2.686666666f));
		TEST_CHECK(Equal(u.y, 0.04f));
		TEST_CHECK(AlmostEqual(u.z, 36.8f, 0.00001f));
	}
	{
		Vec3 u(1.207f, -44.01f, 0.34034f);
		TEST_CHECK(Equal(u[0], u.x));
		TEST_CHECK(Equal(u[1], u.y));
		TEST_CHECK(Equal(u[2], u.z));
		
		u[1] = 8.125f;
		TEST_CHECK(Equal(u.y, 8.125f));
	}
	{
		const Vec3 u(1.207f, -44.01f, 0.34034f);
		TEST_CHECK(Equal(u[0], u.x));
		TEST_CHECK(Equal(u[1], u.y));
		TEST_CHECK(Equal(u[2], u.z));
		TEST_CHECK(u[3] == std::numeric_limits<float>::max());
	}
	{
		Vec3 u(75.757575f, 1.207f, -44.01f);
		Vec3 v(75.757575f, 1.207f, -44.01f);
		Vec3 w(70.0101f, 4.4444f, 1.0001f);
		TEST_CHECK(u == v);
		TEST_CHECK((u == w) == false);
	}
	{
		Vec3 u(75.757575f, 1.207f, -44.01f);
		Vec3 v(75.757575f, 1.207f, -44.01f);
		Vec3 w(70.0101f, 4.4444f, 1.0001f);
		TEST_CHECK(u != w);
		TEST_CHECK((u != v) == false);
	}
	{
		Vec3 u(2.020f, 0.5f, -0.5f);
		Vec3 v(1.010f, 1.f, -1.f);
		Vec3 w = u + v;
		TEST_CHECK(Equal(w.x, 3.030f));
		TEST_CHECK(Equal(w.y, 1.5f));
		TEST_CHECK(Equal(w.z, -1.5f));
	}
	{
		Vec3 u(47.3473f, 0.5f, -0.5f);
		Vec3 v = u + 0.25f;
		TEST_CHECK(Equal(v.x, 47.5973f));
		TEST_CHECK(Equal(v.y, 0.75f));
		TEST_CHECK(Equal(v.z, -0.25f));
	}
	{
		Vec3 u(0.5f, -0.5f, 1.25909f);
		Vec3 v(1.f, -1.f, 1.70707f);
		Vec3 w = u - v;
		TEST_CHECK(Equal(w.x, -0.5f));
		TEST_CHECK(Equal(w.y, 0.5f));
		TEST_CHECK(Equal(w.z, -0.44798f));
	}
	{
		Vec3 u(0.5f, -0.5f, 3.333f);
		Vec3 v = u - 0.25f;
		TEST_CHECK(Equal(v.x, 0.25f));
		TEST_CHECK(Equal(v.y, -0.75f));
		TEST_CHECK(Equal(v.z, 3.083f));
	}
	{
		Vec3 u(0.15f, -2.5f, 1.505f);
		Vec3 v(1.1f, 0.3f, 0.76f);
		Vec3 w = u * v;
		TEST_CHECK(Equal(w.x, 0.165f));
		TEST_CHECK(Equal(w.y, -0.75f));
		TEST_CHECK(Equal(w.z, 1.1438f));
	}
	{
		Vec3 u(-5.06f, 0.75f, 2.72645f);
		Vec3 v = u * 4.25f;
		Vec3 w = 4.25f * u;
		TEST_CHECK(Equal(v.x, -21.505f));
		TEST_CHECK(Equal(v.y, 3.1875f));
		TEST_CHECK(AlmostEqual(v.z, 11.5874125f));
		TEST_CHECK(Equal(w.x, v.x));
		TEST_CHECK(Equal(w.y, v.y));
		TEST_CHECK(Equal(w.z, v.z));
	}
	{
		Vec3 u(0.48f, -2.79f, -1.3334f);
		Vec3 v(1.5f, 0.03f, -0.401401f);
		Vec3 w = u / v;
		TEST_CHECK(Equal(w.x, 0.32));
		TEST_CHECK(Equal(w.y, -93.f));
		TEST_CHECK(Equal(w.z, 3.321865167f));
	}
	{
		Vec3 u(9.008f, 0.75f, -57.1002f);
		Vec3 v = u / 0.1f;
		TEST_CHECK(Equal(v.x, 90.08f));
		TEST_CHECK(Equal(v.y, 7.5f));
		TEST_CHECK(Equal(v.z, -571.002f));
	}
	{
		Vec3 u = RandomVec3();
		TEST_CHECK(Abs(u.x) <= 1.f);
		TEST_CHECK(Abs(u.y) <= 1.f);
		TEST_CHECK(Abs(u.z) <= 1.f);
		Vec3 v = RandomVec3();
		TEST_CHECK(Equal(u.x, v.x) == false);
		TEST_CHECK(Equal(u.y, v.y) == false);
		TEST_CHECK(Equal(u.z, v.z) == false);
		Vec3 w = RandomVec3(0.f, 10.f);
		TEST_CHECK((w.x >= 0.f) && (w.x <= 10.f));
		TEST_CHECK((w.y >= 0.f) && (w.y <= 10.f));
		TEST_CHECK((w.z >= 0.f) && (w.z <= 10.f));
	}
	{
		Vec3 u = RandomVec3(Vec3(-1.f, 0.f, -0.5f), Vec3(0.f, 1.f, 0.5f));
		TEST_CHECK((u.x >= -1.f) && (u.x <= 0.f));
		TEST_CHECK((u.y >= 0.f) && (u.y <= 1.f));
		TEST_CHECK((u.z >= -0.5f) && (u.z <= 0.5f));

		Vec3 v = RandomVec3(-Vec3::One, Vec3::One);
		Vec3 w = RandomVec3(-Vec3::One, Vec3::One);
		TEST_CHECK(Equal(v.x, w.x) == false);
		TEST_CHECK(Equal(v.y, w.y) == false);
		TEST_CHECK(Equal(v.z, w.z) == false);
	}
	{
		Vec3 u(-3.0444f, 102.001f, -0.0001f);
		Vec3 v(-3.0443f, 102.00105f, -0.00005f);
		Vec3 w(-3.04f, 102.0015f, 0.003f);
		TEST_CHECK(AlmostEqual(u, v, 0.0001f));
		TEST_CHECK(AlmostEqual(u, w, 0.0001f) == false);
		TEST_CHECK(AlmostEqual(u, w, 0.005f));
	}
	TEST_CHECK(Equal(Dist2(Vec3(0.3f, -1.2f, 0.45f), Vec3(1.2f, 3.1f, 0.6f)), 19.3225f));
	TEST_CHECK(Equal(Dist(Vec3(1.4f, 2.3f, -5.43f), Vec3(-0.2f, -0.1f, -3.77f)), sqrt(11.0756f)));
	TEST_CHECK(Equal(Len2(Vec3(1.f, -1.f, 0.3f)), 2.09f));
	TEST_CHECK(Equal(Len(Vec3(3.f, -4.f, 12.f)), 13.f));
	TEST_CHECK(Min(Vec3(0.336f, 5.5555f, -0.0101f), Vec3(1.01f, 2.2222f, -0.0102f)) == Vec3(0.336f, 2.2222f, -0.0102f));
	TEST_CHECK(Max(Vec3(0.336f, 5.5555f, -0.0101f), Vec3(1.01f, 2.2222f, -0.0102f)) == Vec3(1.01f, 5.5555f, -0.0101f));
	{
		TEST_CHECK(Equal(Dot(Vec3(1.f, -1.f, 1.f), Vec3(-1.f, 0.f, 1.f)), 0.f));
		TEST_CHECK(Equal(Dot(Vec3(2.f, 3.f, 4.f), Vec3(5.f, 6.f, 7.f)), 56.f));
	}
	{
		Vec3 u = Cross(Vec3(-1.f, 1.f, 1.f), Vec3(1.f, -1.f, 1.f));
		TEST_CHECK(Equal(u.x, 2.0f));
		TEST_CHECK(Equal(u.y, 2.0f));
		TEST_CHECK(Equal(u.z, 0.0f));
		Vec3 v = Cross(Vec3(-1.f, 1.f, 1.f), Vec3(1.f, -1.f, -1.f));
		TEST_CHECK(Equal(v.x, 0.0f));
		TEST_CHECK(Equal(v.y, 0.0f));
		TEST_CHECK(Equal(v.z, 0.0f));
		const Vec3 w0(2.02f, -1.5151f, 0.997f);
		const Vec3 w1(2.4042f, -0.67f, 0.789f);
		Vec3 w = Cross(w0, w1);
		TEST_CHECK(AlmostEqual(w.x, -0.527424f));
		TEST_CHECK(AlmostEqual(w.y, 0.803207f));
		TEST_CHECK(AlmostEqual(w.z, 2.2892f));
		TEST_CHECK(AlmostEqual(Dot(w, w0), 0.0f));
		TEST_CHECK(AlmostEqual(Dot(w, w1), 0.0f));
	}
	{
		Vec3 v = Reverse(Vec3(1.f, 2.f, 3.f));
		TEST_CHECK(Equal(v.x, -1.f));
		TEST_CHECK(Equal(v.y, -2.f));
		TEST_CHECK(Equal(v.z, -3.f));
	}
	{
		Vec3 v = Inverse(Vec3(2.f, -4.f, 0.1f));
		TEST_CHECK(Equal(v.x, 0.5f));
		TEST_CHECK(Equal(v.y, -0.25f));
		TEST_CHECK(Equal(v.z, 10.f));
	}
	{
		float inv_sqrt3 = 1.f / sqrtf(3.f);
		TEST_CHECK(Normalize(Vec3(4.701f, -4.701f, 4.701f)) == Vec3(inv_sqrt3, -inv_sqrt3, inv_sqrt3));
		TEST_CHECK(Equal(Len(Normalize(Vec3(0.3f, -0.4f, 0.5f))), 1.f));
	}
	{
		Vec3 v = Deg3(60.f, 120.f, 315.f);
		TEST_CHECK(AlmostEqual(v.x, Pi / 3.f, 1.e-5f));
		TEST_CHECK(AlmostEqual(v.y, TwoPi / 3.f, 1.e-5f));
		TEST_CHECK(AlmostEqual(v.z, 7.f * Pi / 4.f, 1.e-5f));
	}
	{
		Vec3 v = Rad3(Pi / 3.f, TwoPi / 3.f, Pi);
		TEST_CHECK(Equal(v.x, Pi / 3.f));
		TEST_CHECK(Equal(v.y, TwoPi / 3.f));
		TEST_CHECK(Equal(v.z, Pi));
	}
	{
		Vec3 u(4.f, -2.f, 0.5f);
		Vec3 v = Clamp(u, -1.f, 1.f);
		TEST_CHECK(Equal(v.x, 1.0f));
		TEST_CHECK(Equal(v.y, -1.0f));
		TEST_CHECK(Equal(v.z, 0.5f));
	}
	{
		Vec3 u = Clamp(Vec3(-0.75f, 0.5f, 2.1f), Vec3(-1.f, -1.f, 1.5f), Vec3(-0.5f, 1.f, 3.f));
		TEST_CHECK(Equal(u.x, -0.75f));
		TEST_CHECK(Equal(u.y, 0.5f));
		TEST_CHECK(Equal(u.z, 2.1f));

		Vec3 v = Clamp(Vec3(-3.f, 4.f, 1.5f), Vec3(-1.f, -1.f, 2.f), Vec3(-0.5f, 1.f, 3.f));
		TEST_CHECK(Equal(v.x, -1.0f));
		TEST_CHECK(Equal(v.y, 1.0f));
		TEST_CHECK(Equal(v.z, 2.0f));
	}
	{
		TEST_CHECK(Equal(Len(ClampLen(Vec3(1.f, -1.f, 1.f), 0.5f, 1.f)), 1.f));
		TEST_CHECK(Equal(Len(ClampLen(Vec3(0.2f, -0.1f, 0.2f), 1.2f, 2.f)), 1.2f));
		TEST_CHECK(Equal(Len(ClampLen(Vec3(-3.0f, 4.0f, -12.0f), 2.0f, 20.0f)), 13.f));
	}
	TEST_CHECK(Abs(Vec3(-0.1f, 0.4f, -2.1f)) == Vec3(0.1f, 0.4f, 2.1f));
	{
		TEST_CHECK(Sign(Vec3(-0.1f, 0.4f, -2.1f)) == Vec3(-1.0f, 1.0f, -1.0f));
		TEST_CHECK(Sign(Vec3(0.0f, -0.0f, 1.0f)) == Vec3(1.0f, -1.0f, 1.0f));
	}
	{
		{
			const Vec3 u = Vec3(1.0f, -2.0f, 0.5f);
			const Vec3 v = Vec3(1.0f, 1.0f, 0.0f);
			Vec3 w = Reflect(u, v);
			TEST_CHECK(Equal(w.x, 2.0f));
			TEST_CHECK(Equal(w.y, -1.0f));
			TEST_CHECK(Equal(w.z, 0.5));
		}
		{
			const Vec3 u = Vec3(1.0f, -2.f, -1.0f);
			const Vec3 v = Vec3(1.0f, 1.0f, 1.0f);
			Vec3 w = Reflect(u, v);
			TEST_CHECK(AlmostEqual(w.x, 7.0f / 3.0f));
			TEST_CHECK(Equal(w.y, -2.0f / 3.f));
			TEST_CHECK(Equal(w.z, 1.0f / 3.0f));
		}
	}
	TEST_CHECK(Floor(Vec3(-0.1f, 1.7f, 2.0f)) == Vec3(-1.0f, 1.0f, 2.0f));
	TEST_CHECK(Ceil(Vec3(-3.2f, 1.0f, 4.1f)) == Vec3(-3.0f, 1.0f, 5.0f));
	{
		Vec3 u(2.0f, 1.0f, -0.5f);
		Vec3 v = FaceForward(u, Vec3(1.0f, -1.0f, 1.0f));
		TEST_CHECK(v.x == -u.x);
		TEST_CHECK(v.y == -u.y);
		TEST_CHECK(v.z == -u.z);

		v = FaceForward(u, Vec3(-1.0f, -1.0f, -1.0f));
		TEST_CHECK(v.x == u.x);
		TEST_CHECK(v.y == u.y);
		TEST_CHECK(v.z == u.z);
	}
	TEST_CHECK(MakeVec3(Vec4(1.0f, -1.0f, 2.0f, 3.0f)) == Vec3(1.0f, -1.0f, 2.0f));

	{
		const Vec3 u(1.f, -1.f, 0.f);
		const Vec3 n(0.f, 1.f, 0.f);
		TEST_CHECK(AlmostEqual(Refract(u, n, 1.f, 1.f), u, 0.000001f));
		TEST_CHECK(AlmostEqual(Refract(u, n, 1.f, 1.5f), Vec3(1.f / 1.5, -1.f, 0.f), 0.000001f));
	}

	{
		TEST_CHECK(AlmostEqual(Quantize(Vec3(25.5f, 7.2f, -44.3f), 10.f), Vec3(20.f, 0.f, -40.f), 0.000001f));
		TEST_CHECK(AlmostEqual(Quantize(Vec3(1.8f, 0.6f, 2.345f), 0.5f, 0, 0.1f), Vec3(1.5f, 0.6f, 2.3f), 0.000001f));
		TEST_CHECK(AlmostEqual(Quantize(Vec3(1.1f, 0.6f, 2.3f), 0, 1.f, 2.f), Vec3(1.1f, 0.f, 2.f), 0.000001f));
		TEST_CHECK(AlmostEqual(Quantize(Vec3(-3.33333f, 41.125f, 0.004f), 0), Vec3(-3.33333f, 41.125f, 0.004f), 0.000001f));
	}

	{
		TEST_CHECK(Hash(Vec3::Up) != Hash(Vec3::Down));
		TEST_CHECK(Hash(Vec3::Left) != Hash(Vec3::Right));
		TEST_CHECK(Hash(Vec3::Front) != Hash(Vec3::Back));
	}

	{
		TEST_CHECK(AlmostEqual(BaseToEuler(Vec3::Front), Vec3::Zero, 0.000001f));
		TEST_CHECK(AlmostEqual(BaseToEuler(Vec3::Back), Vec3(0, Pi, 0), 0.000001f));
		TEST_CHECK(AlmostEqual(BaseToEuler(Vec3::Up), Vec3(-HalfPi, 0, 0), 0.000001f));
		TEST_CHECK(AlmostEqual(BaseToEuler(Vec3::Right), Vec3(0, HalfPi, 0), 0.000001f));
		TEST_CHECK(AlmostEqual(BaseToEuler(Vec3(-1.f, 0.f, -1.f)), Vec3(0, -3.f * Pi / 4.f, 0), 0.000001f));
	}

	{
		TEST_CHECK(AlmostEqual(BaseToEuler(Vec3::Front, Vec3::Right), Vec3::Zero, 0.00001f));
		TEST_CHECK(AlmostEqual(BaseToEuler(Vec3::Front, Vec3::Down), Vec3(0, 0, 3.f * HalfPi), 0.000001f));
		TEST_CHECK(AlmostEqual(BaseToEuler(Vec3::Front, Vec3::Up), Vec3(0, 0, HalfPi), 0.000001f));
		TEST_CHECK(AlmostEqual(BaseToEuler(Vec3::Front, Vec3::Down), Vec3(0, 0, 3.f * HalfPi), 0.000001f));
		TEST_CHECK(AlmostEqual(BaseToEuler(Vec3::Front, Vec3::Left), Vec3(0, 0, Pi), 0.000001f));
		TEST_CHECK(AlmostEqual(BaseToEuler(Vec3::Left, Vec3::Back), Vec3(0, -HalfPi, Pi), 0.000001f));
		TEST_CHECK(AlmostEqual(BaseToEuler(Vec3::Right, Vec3::Back), Vec3(0, HalfPi, 0), 0.000001f));
	}
}
