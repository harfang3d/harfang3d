// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <math.h>

#define TEST_NO_MAIN 
#include "acutest.h"

#include "foundation/color.h"

#include "foundation/math.h"
#include "foundation/vector3.h"
#include "foundation/vector4.h"

using namespace hg;

static float ColLen2(Color c) { return (c.r * c.r) + (c.g * c.g) + (c.b * c.b); }

void test_color() {
	{
		Color c0(1.f, 2.f, 3.f, 4.f);
		Color c1(c0);
		TEST_CHECK(Equal(c1.r, 1.f));
		TEST_CHECK(Equal(c1.g, 2.f));
		TEST_CHECK(Equal(c1.b, 3.f));
		TEST_CHECK(Equal(c1.a, 4.f));
	}
	{
		Color c0(0.22f, 1.4f, -2.75f, -0.3503f);
		c0 += Color(0.042f, -0.2f, 5.25f, 0.9017f);
		TEST_CHECK(Equal(c0.r, 0.262f));
		TEST_CHECK(Equal(c0.g, 1.2f));
		TEST_CHECK(Equal(c0.b, 2.5f));
		TEST_CHECK(Equal(c0.a, 0.5514f));
	}
	{
		Color c0(20.97f, -3.1f, 10.02f, -1.402f);
		c0 += 0.1f;
		TEST_CHECK(Equal(c0.r, 21.07f));
		TEST_CHECK(Equal(c0.g, -3.f));
		TEST_CHECK(AlmostEqual(c0.b, 10.12f));
		TEST_CHECK(Equal(c0.a, -1.302f));
	}
	{
		Color c0(55.555f, 0.336f, 5.5555f, 7.9191f);
		c0 -= Color(2.002f, 1.01f, 2.2222f, 0.054f);
		TEST_CHECK(Equal(c0.r, 53.553f));
		TEST_CHECK(Equal(c0.g, -0.674f));
		TEST_CHECK(Equal(c0.b, 3.3333f));
		TEST_CHECK(Equal(c0.a, 7.8651f));
	}
	{
		Color c0(-1.025f, 2.3235f, 7.81194f, -0.40441f);
		c0 -= 0.004f;
		TEST_CHECK(Equal(c0.r, -1.029));
		TEST_CHECK(Equal(c0.g, 2.3195f));
		TEST_CHECK(Equal(c0.b, 7.80794f));
		TEST_CHECK(Equal(c0.a, -0.40841f));
	}
	{
		Color c0(7.011f, 4.0f, -11.f, 1.202f);
		c0 *= Color(0.33f, 0.5f, 0.4f, -5.01f);
		TEST_CHECK(Equal(c0.r, 2.31363f));
		TEST_CHECK(Equal(c0.g, 2.f));
		TEST_CHECK(Equal(c0.b, -4.4f));
		TEST_CHECK(AlmostEqual(c0.a, -6.02202f));
	}
	{
		Color c0(-2.022f, 11.11f, 0.075f, -101.3f);
		c0 *= 0.3f;
		TEST_CHECK(Equal(c0.r, -0.6066f));
		TEST_CHECK(Equal(c0.g, 3.333f));
		TEST_CHECK(Equal(c0.b, 0.0225f));
		TEST_CHECK(AlmostEqual(c0.a, -30.39f, 0.00001f));
	}
	{
		Color c0(0.4f, 4.8f, -26.f, 1.4f);
		c0 /= Color(-0.25f, 0.3f, -0.104f, 0.07f);
		TEST_CHECK(AlmostEqual(c0.r, -1.6f, 1.e-4f));
		TEST_CHECK(AlmostEqual(c0.g, 16.f, 1.e-4f));
		TEST_CHECK(AlmostEqual(c0.b, 250.f, 1.e-4f));
		TEST_CHECK(AlmostEqual(c0.a, 20.f, 1.e-4f));
	}
	{
		Color c0(-2.015f, 0.03f, 27.6f, -0.975f);
		c0 /= 0.75f;
		TEST_CHECK(AlmostEqual(c0.r, -2.686666666f));
		TEST_CHECK(Equal(c0.g, 0.04f));
		TEST_CHECK(AlmostEqual(c0.b, 36.8f, 0.00001f));
		TEST_CHECK(Equal(c0.a, -1.3f));
	}
	{
		Color c0(1.207f, -44.01f, 0.34034f, -54.0127f);
		TEST_CHECK(Equal(c0[0], c0.r));
		TEST_CHECK(Equal(c0[1], c0.g));
		TEST_CHECK(Equal(c0[2], c0.b));
		TEST_CHECK(Equal(c0[3], c0.a));

		c0[1] = 1.f;
		TEST_CHECK(Equal(c0.g, 1.f));
	}
	{
		const Color c0(1.207f, -44.01f, 0.34034f, -54.0127f);
		TEST_CHECK(Equal(c0[0], c0.r));
		TEST_CHECK(Equal(c0[1], c0.g));
		TEST_CHECK(Equal(c0[2], c0.b));
		TEST_CHECK(Equal(c0[3], c0.a));
		TEST_CHECK(c0[12] == std::numeric_limits<float>::max());
	}
	{
		Color c0(75.757575f, 1.207f, -44.01f, 0.192f);
		Color c1(75.757575f, 1.207f, -44.01f, 0.192f);
		Color c2(70.0101f, 4.4444f, 1.0001f, 0.4f);
		TEST_CHECK(c0 == c1);
		TEST_CHECK((c0 == c2) == false);
	}
	{
		Color c0(75.757575f, 1.207f, -44.01f, 0.192f);
		Color c1(75.757575f, 1.207f, -44.01f, 0.192f);
		Color c2(70.0101f, 4.4444f, 1.0001f, 0.4f);
		Color c3(70.0101f, 1.207f, -44.01f, 0.192f);
		TEST_CHECK(c0 != c2);
		TEST_CHECK(c0 != c3);
		TEST_CHECK((c0 != c1) == false);
	}
	{
		Color c0(2.020f, 0.5f, -0.5f, 0.3104f);
		Color c1(1.010f, 1.f, -1.f, 0.0306f);
		Color c2 = c0 + c1;
		TEST_CHECK(Equal(c2.r, 3.030f));
		TEST_CHECK(Equal(c2.g, 1.5f));
		TEST_CHECK(Equal(c2.b, -1.5f));
		TEST_CHECK(Equal(c2.a, 0.341f));
	}
	{
		Color c0(47.3473f, 0.5f, -0.5f, 0.754f);
		Color c1 = c0 + 0.25f;
		TEST_CHECK(Equal(c1.r, 47.5973f));
		TEST_CHECK(Equal(c1.g, 0.75f));
		TEST_CHECK(Equal(c1.b, -0.25f));
		TEST_CHECK(Equal(c1.a, 1.004f));
	}
	{
		Color c0(0.5f, -0.5f, 1.25909f, -0.30303f);
		Color c1(1.f, -1.f, 1.70707f, 0.0101f);
		Color c2 = c0 - c1;
		TEST_CHECK(Equal(c2.r, -0.5f));
		TEST_CHECK(Equal(c2.g, 0.5f));
		TEST_CHECK(Equal(c2.b, -0.44798f));
		TEST_CHECK(Equal(c2.a, -0.31313f));
	}
	{
		Color c0(0.5f, -0.5f, 3.333f, 7.05f);
		Color c1 = c0 - 0.25f;
		TEST_CHECK(Equal(c1.r, 0.25f));
		TEST_CHECK(Equal(c1.g, -0.75f));
		TEST_CHECK(Equal(c1.b, 3.083f));
		TEST_CHECK(Equal(c1.a, 6.8f));
	}
	{
		Color c0(0.15f, -2.5f, 1.505f, 0.08);
		Color c1(1.1f, 0.3f, 0.76f, 2.04f);
		Color c2 = c0 * c1;
		TEST_CHECK(Equal(c2.r, 0.165f));
		TEST_CHECK(Equal(c2.g, -0.75f));
		TEST_CHECK(Equal(c2.b, 1.1438f));
		TEST_CHECK(Equal(c2.a, 0.1632f));
	}
	{
		Color c0(-5.06f, 0.75f, 2.72645f, 0.1717f);
		Color c1 = c0 * 4.25f;
		Color c2 = 4.25f * c0;
		TEST_CHECK(Equal(c1.r, -21.505f));
		TEST_CHECK(Equal(c1.g, 3.1875f));
		TEST_CHECK(AlmostEqual(c1.b, 11.5874125f));
		TEST_CHECK(Equal(c1.a, 0.729725f));
		TEST_CHECK(Equal(c2.r, c1.r));
		TEST_CHECK(Equal(c2.g, c1.g));
		TEST_CHECK(Equal(c2.b, c1.b));
		TEST_CHECK(Equal(c2.a, c1.a));
	}
	{
		Color c0(0.48f, -2.79f, -1.3334f, 0.0794f);
		Color c1(1.5f, 0.03f, -0.401401f, 2.2f);
		Color c2 = c0 / c1;
		TEST_CHECK(AlmostEqual(c2.r, 0.32, 1.e-5f));
		TEST_CHECK(AlmostEqual(c2.g, -93.f, 1.e-5f));
		TEST_CHECK(AlmostEqual(c2.b, 3.321865167f, 1.e-5f));
		TEST_CHECK(AlmostEqual(c2.a, 0.0360909f, 1.e-5f));
	}
	{
		Color c0(9.008f, 0.75f, -57.1002f, 3.7f);
		Color c1 = c0 / 0.1f;
		TEST_CHECK(Equal(c1.r, 90.08f));
		TEST_CHECK(Equal(c1.g, 7.5f));
		TEST_CHECK(Equal(c1.b, -571.002));
		TEST_CHECK(Equal(c1.a, 37.f));
	}
	{
		Vec3 v(1.f, 2.f, 3.f);
		Color c0 = ColorFromVector3(v);
		TEST_CHECK(Equal(c0.r, 1.f));
		TEST_CHECK(Equal(c0.g, 2.f));
		TEST_CHECK(Equal(c0.b, 3.f));
		TEST_CHECK(Equal(c0.a, 1.f));
	}
	{
		Vec4 v(1.f, 2.f, 3.f, 4.f);
		Color c0 = ColorFromVector4(v);
		TEST_CHECK(Equal(c0.r, 1.f));
		TEST_CHECK(Equal(c0.g, 2.f));
		TEST_CHECK(Equal(c0.b, 3.f));
		TEST_CHECK(Equal(c0.a, 4.f));
	}
	{
		Color c0(4.f, -2.f, 0.5f, -0.5f);
		Color c1 = Clamp(c0, -1.f, 1.f);
		TEST_CHECK(Equal(c1.r, 1.0f));
		TEST_CHECK(Equal(c1.g, -1.0f));
		TEST_CHECK(Equal(c1.b, 0.5f));
		TEST_CHECK(Equal(c1.a, -0.5f));
	}
	{
		Color c0 = Clamp(Color(-0.75f, 0.5f, 2.1f, -1.f), Color(-1.f, -1.f, 1.5f, -1.2f), Color(-0.5f, 1.f, 3.f, 1.f));
		TEST_CHECK(Equal(c0.r, -0.75f));
		TEST_CHECK(Equal(c0.g, 0.5f));
		TEST_CHECK(Equal(c0.b, 2.1f));
		TEST_CHECK(Equal(c0.a, -1.f));

		Color c1 = Clamp(Color(-3.f, 4.f, 1.5f, -1.f), Color(-1.f, -1.f, 2.f, 0.f), Color(-0.5f, 1.f, 3.f, 1.f));
		TEST_CHECK(Equal(c1.r, -1.0f));
		TEST_CHECK(Equal(c1.g, 1.0f));
		TEST_CHECK(Equal(c1.b, 2.0f));
		TEST_CHECK(Equal(c1.a, 0.0f));
	}
	{
		TEST_CHECK(Equal(ColLen2(ClampLen(Color(1.f, -1.f, 1.f, 1.f), 0.5f, 1.f)), 1.f));
		TEST_CHECK(Equal(ColLen2(ClampLen(Color(0.2f, -0.1f, 0.2f, 0.3f), 1.2f, 2.f)), 1.2f * 1.2f));
		TEST_CHECK(Equal(ColLen2(ClampLen(Color(-3.0f, 4.0f, -12.0f, 0.f), 2.0f, 20.0f)), 13.f * 13.f));
	}
	{
		TEST_CHECK(Equal(ColorToGrayscale(Color(0.376f, 0.471f, 0.627f)), 0.459659964f));
		TEST_CHECK(Equal(ColorToGrayscale(Color(0.f, 0.f, 0.f)), 0.f));
		TEST_CHECK(Equal(ColorToGrayscale(Color(1.f, 1.f, 1.f)), 1.f));
	}
	{
		TEST_CHECK(ColorToRGBA32(Color(0.376f, 0.471f, 0.627f, 0.5f)) == 0x7f9f785f);
		TEST_CHECK(ColorToRGBA32(Color(0.f, 0.f, 0.f, 1.f)) == 0xff000000);
		TEST_CHECK(ColorToRGBA32(Color(1.f, 1.f, 1.f, 0.f)) == 0x00ffffff);
	}
	{
		TEST_CHECK(ColorToABGR32(Color(0.376f, 0.471f, 0.627f, 0.5f)) == 0x5f789f7f);
		TEST_CHECK(ColorToABGR32(Color(0.f, 0.f, 0.f, 1.f)) == 0x000000ff);
		TEST_CHECK(ColorToABGR32(Color(1.f, 1.f, 1.f, 0.f)) == 0xffffff00);
	}
	{
		TEST_CHECK(AlmostEqual(ColorFromRGBA32(0x7f9f785f), Color(0.376f, 0.471f, 0.627f, 0.5f), 1.f / 255.f));
		TEST_CHECK(AlmostEqual(ColorFromRGBA32(0xff000000), Color(0.f, 0.f, 0.f, 1.f), 1.f / 255.f));
		TEST_CHECK(AlmostEqual(ColorFromRGBA32(0x00ffffff), Color(1.f, 1.f, 1.f, 0.f), 1.f / 255.f));
	}
	{
		Color c = ColorFromABGR32(0x5f789f7f);
		TEST_CHECK(AlmostEqual(ColorFromABGR32(0x5f789f7f), Color(0.376f, 0.471f, 0.627f, 0.5f), 1.f/255.f));
		TEST_CHECK(AlmostEqual(ColorFromABGR32(0x000000ff), Color(0.f, 0.f, 0.f, 1.f), 1.f / 255.f));
		TEST_CHECK(AlmostEqual(ColorFromABGR32(0xffffff00), Color(1.f, 1.f, 1.f, 0.f), 1.f / 255.f));
	}
	{
		TEST_CHECK(ARGB32ToRGBA32(0x5f789f7f) == 0x7f9f785f);
		TEST_CHECK(ARGB32ToRGBA32(0x000000ff) == 0xff000000);
		TEST_CHECK(ARGB32ToRGBA32(0xffffff00) == 0x00ffffff);
	}
	{
		uint32_t v = RGBA32(96, 120, 160, 128);
		TEST_CHECK(RGBA32(96, 120, 160, 128) == 0x80a07860);
		TEST_CHECK(RGBA32(0, 0, 0, 255) == 0xff000000);
		TEST_CHECK(RGBA32(255, 255, 255, 0) == 0x00ffffff);
	}
	{
		TEST_CHECK(ARGB32(96, 120, 160, 128) == 0x6078a080);
		TEST_CHECK(ARGB32(0, 0, 0, 255) == 0x000000ff);
		TEST_CHECK(ARGB32(255, 255, 255, 0) == 0xffffff00);
	}
	{
		float l = (0.376f - 0.75f) * (0.376f - 0.75f) + (0.471f - 0.8f) * (0.471f - 0.8f) + (0.627f - 0.24f) * (0.627f - 0.24f) + (0.5f - 1.f) * (0.5f - 1.f);
		TEST_CHECK(Equal(Dist2(Color(0.376f, 0.471f, 0.627f, 0.5f), Color(0.75f, 0.8f, 0.24f, 1.f)), l));
		TEST_CHECK(Equal(Dist2(Color(-3.0f, 4.0f, -12.0f, 0.f), Color(0.f, 0.f, 0.f, 0.f)), 169.f));
		TEST_CHECK(Equal(Dist(Color(0.376f, 0.471f, 0.627f, 0.5f), Color(0.75f, 0.8f, 0.24f, 1.f)), sqrt(l)));
		TEST_CHECK(Equal(Dist(Color(-3.0f, 4.0f, -12.0f, 0.f), Color(0.f, 0.f, 0.f, 0.f)), 13.f));
	}
	{
		Color c0 = ChromaScale(Color(1.f, 2.f, 3.f, 4.f), -0.5f);
		TEST_CHECK(Equal(c0.r, -0.5f));
		TEST_CHECK(Equal(c0.g, -1.0f));
		TEST_CHECK(Equal(c0.b, -1.5f));
		TEST_CHECK(Equal(c0.a, 4.f));
	}
	{
		Color c0 = AlphaScale(Color(1.f, 2.f, 3.f, 4.f), -0.5f);
		TEST_CHECK(Equal(c0.r, 1.0f));
		TEST_CHECK(Equal(c0.g, 2.0f));
		TEST_CHECK(Equal(c0.b, 3.f));
		TEST_CHECK(Equal(c0.a, -2.f));
	}
	{
		Color c0(-3.0444f, 102.001f, -0.0001f, 2.2012f);
		Color c1(-3.0443f, 102.00105f, -0.00005f, 2.20115f);
		Color c2(-3.04f, 102.0015f, 0.003f, 2.2014f);
		TEST_CHECK(AlmostEqual(c0, c1, 0.0001f));
		TEST_CHECK(AlmostEqual(c0, c2, 0.0001f) == false);
		TEST_CHECK(AlmostEqual(c0, c2, 0.005f));
	}
	{
		Color c0 = ColorI(96, 120, 160, 128);
		TEST_CHECK(AlmostEqual(c0.r, 0.376f, 1.f / 255.f));
		TEST_CHECK(AlmostEqual(c0.g, 0.471f, 1.f / 255.f));
		TEST_CHECK(AlmostEqual(c0.b, 0.627f, 1.f / 255.f));
		TEST_CHECK(AlmostEqual(c0.a, 0.5f, 1.f/255.f));
	}
	{ 
		Color c0 = ToHLS(Color(0.376f, 0.471f, 0.627f));
		TEST_CHECK(AlmostEqual(c0.r, 217.3f, 0.1f));
		TEST_CHECK(AlmostEqual(c0.g, 0.5015f, 0.01f));
		TEST_CHECK(AlmostEqual(c0.b, 0.2517f, 0.01f));
		TEST_CHECK(AlmostEqual(c0.a, 1.f, 0.01f));

		TEST_CHECK(AlmostEqual(ToHLS(Color::Red), Color(0.f, 0.5f, 1.f), 0.0001f));
		TEST_CHECK(AlmostEqual(ToHLS(Color::Yellow), Color(60.f, 0.5f, 1.f), 0.0001f));
		TEST_CHECK(AlmostEqual(ToHLS(Color::Green), Color(120.f, 0.5f, 1.f), 0.0001f));
		TEST_CHECK(AlmostEqual(ToHLS(ColorI(0, 255, 255)), Color(180.f, 0.5f, 1.f), 0.0001f));
		TEST_CHECK(AlmostEqual(ToHLS(Color::Blue), Color(240.f, 0.5f, 1.f), 0.0001f));
		TEST_CHECK(AlmostEqual(ToHLS(ColorI(255, 0, 255)), Color(300.f, 0.5f, 1.f), 0.0001f));
	}
	{
		Color c0 = FromHLS(Color(217.3f, 0.5015f, 0.2517f));
		TEST_CHECK(AlmostEqual(c0.r, 0.376f, 0.1f));
		TEST_CHECK(AlmostEqual(c0.g, 0.471f, 0.01f));
		TEST_CHECK(AlmostEqual(c0.b, 0.627f, 0.01f));
		TEST_CHECK(AlmostEqual(c0.a, 1.f, 0.01f));
	
		Color c1 = FromHLS(Color(0.f, 0.75f, 0.f));
		TEST_CHECK(Equal(c1.r, 0.75f));
		TEST_CHECK(Equal(c1.g, 0.75));
		TEST_CHECK(Equal(c1.b, 0.75f));
		TEST_CHECK(Equal(c1.a, 1.f));

		Color c2 = FromHLS(Color(30.f, 0.5f, 0.5f));
		TEST_CHECK(AlmostEqual(c2.r, 0.75f, 0.00001f));
		TEST_CHECK(AlmostEqual(c2.g, 0.5f, 0.00001f));
		TEST_CHECK(AlmostEqual(c2.b, 0.25f, 0.00001f));
		TEST_CHECK(AlmostEqual(c2.a, 1.f, 0.00001f));

		Color c3 = FromHLS(Color(390.f, 0.5f, 0.5f));
		TEST_CHECK(c2 == c3);
	}
	{
		Color c0 = SetHue(FromHLS(Color(217.3f, 0.5015f, 0.2517f)), 60.f);
		Color c1 = ToHLS(c0);
		TEST_CHECK(AlmostEqual(c1.r, 60.f, 0.0001f));
		TEST_CHECK(AlmostEqual(c1.g, 0.5015f, 0.0001f));
		TEST_CHECK(AlmostEqual(c1.b, 0.2517f, 0.0001f));
		TEST_CHECK(AlmostEqual(c1.a, 1.f, 0.0001f));
	}
	{
		Color c0 = SetSaturation(FromHLS(Color(217.3f, 0.5015f, 0.2517f)), 0.8f);
		Color c1 = ToHLS(c0);
		TEST_CHECK(AlmostEqual(c1.r, 217.3f, 0.0001f));
		TEST_CHECK(AlmostEqual(c1.g, 0.5015f, 0.0001f));
		TEST_CHECK(AlmostEqual(c1.b, 0.8f, 0.0001f));
		TEST_CHECK(AlmostEqual(c1.a, 1.f, 0.0001f));
	}
	{
		Color c0 = SetLuminance(FromHLS(Color(217.3f, 0.5015f, 0.2517f)), 0.1f);
		Color c1 = ToHLS(c0);
		TEST_CHECK(AlmostEqual(c1.r, 217.3f, 0.0001f));
		TEST_CHECK(AlmostEqual(c1.g, 0.1f, 0.0001f));
		TEST_CHECK(AlmostEqual(c1.b, 0.2517f, 0.0001f));
		TEST_CHECK(AlmostEqual(c1.a, 1.f, 0.0001f));
	}
	{
		Color c0 = ScaleHue(FromHLS(Color(217.3f, 0.5015f, 0.2517f)), 0.5f);
		Color c1 = ToHLS(c0);
		TEST_CHECK(AlmostEqual(c1.r, 108.65f, 0.0001f));
		TEST_CHECK(AlmostEqual(c1.g, 0.5015f, 0.0001f));
		TEST_CHECK(AlmostEqual(c1.b, 0.2517f, 0.0001f));
		TEST_CHECK(AlmostEqual(c1.a, 1.f, 0.0001f));
	}
	{
		Color c0 = ScaleSaturation(FromHLS(Color(217.3f, 0.5015f, 0.2517f)), 0.5f);
		Color c1 = ToHLS(c0);
		TEST_CHECK(AlmostEqual(c1.r, 217.3f, 0.0001f));
		TEST_CHECK(AlmostEqual(c1.g, 0.5015f, 0.0001f));
		TEST_CHECK(AlmostEqual(c1.b, 0.12585f, 0.0001f));
		TEST_CHECK(AlmostEqual(c1.a, 1.f, 0.0001f));
	}
	{
		Color c0 = ScaleLuminance(FromHLS(Color(217.3f, 0.5015f, 0.2517f)), 0.5f);
		Color c1 = ToHLS(c0);
		TEST_CHECK(AlmostEqual(c1.r, 217.3f, 0.0001f));
		TEST_CHECK(AlmostEqual(c1.g, 0.25075f, 0.0001f));
		TEST_CHECK(AlmostEqual(c1.b, 0.2517f, 0.0001f));
		TEST_CHECK(AlmostEqual(c1.a, 1.f, 0.0001f));
	} 
}
