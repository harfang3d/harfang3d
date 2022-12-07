// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <math.h>

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/math.h"

#include "foundation/rotation_order.h"

using namespace hg;

void test_math() {
	TEST_CHECK(Abs(0) == 0);
	TEST_CHECK(Abs( 1000) == 1000);
	TEST_CHECK(Abs(-1000) == 1000);
	TEST_CHECK(Abs( 2.f) == 2.f);
	TEST_CHECK(Abs(-2.f) == 2.f);

	TEST_CHECK(Min(0, 0) == 0);
	TEST_CHECK(Min(10, 40) == 10);
	TEST_CHECK(Min(60, 3) == 3);
	TEST_CHECK(Min(-1, 4) == -1);
	TEST_CHECK(Min(-999, -3) == -999);
	TEST_CHECK(Min(-12, -34) == -34);
	TEST_CHECK(Min(1.0/3.0, 1.0/4.0) == 1.0/4.0);
	TEST_CHECK(Min(55.5555f, 48.33333f) == 48.33333f);
	TEST_CHECK(Min(-55.5555f,-48.33333f) == -55.5555f);
	TEST_CHECK(Min(10.0051, 10.005) == 10.005);

	TEST_CHECK(Min(22.22224, 22.22226, 22.222265) == 22.22224);
	TEST_CHECK(Min(0.00003, -0.00001, 0.0004) == -0.00001);
	TEST_CHECK(Min(5.55755, 5.550015, 5.54975) == 5.54975);

	TEST_CHECK(Max(0, 0) == 0);
	TEST_CHECK(Max(10, 40) == 40);
	TEST_CHECK(Max(60, 3) == 60);
	TEST_CHECK(Max(-1, 4) == 4);
	TEST_CHECK(Max(-999, -3) == -3);
	TEST_CHECK(Max(-12, -34) == -12);
	TEST_CHECK(Max(1.0 / 3.0, 1.0 / 4.0) == 1.0 / 3.0);
	TEST_CHECK(Max(55.5555f, 48.33333f) == 55.5555f);
	TEST_CHECK(Max(-55.5555f, -48.33333f) == -48.33333f);
	TEST_CHECK(Max(10.0051, 10.005) == 10.0051);

	TEST_CHECK(Max(22.22224, 22.22226, 22.222265) == 22.222265);
	TEST_CHECK(Max(0.00003, 0.0004, -0.00001) == 0.0004);
	TEST_CHECK(Max(5.55755, 5.550015, 5.54975) == 5.55755);

	TEST_CHECK(Clamp(0, -1, 1) == 0);
	TEST_CHECK(Clamp(1, -2, 2) == 1);
	TEST_CHECK(Clamp(2, -1, 1) == 1);
	TEST_CHECK(Clamp(-2, -1, 1) == -1);
	TEST_CHECK(Clamp(0.0, -1.02, 1.415) == 0.0);
	TEST_CHECK(Clamp(-1.2, -1.02, 1.415) == -1.02);
	TEST_CHECK(Clamp(1.4152, -1.02, 1.415) == 1.415);
	TEST_CHECK(Clamp(-33.3333, -9.4, 13.5) == -9.4);
	TEST_CHECK(Clamp(999.9999, -77.002, 2.715) == 2.715);
	TEST_CHECK(Clamp(1.10333, -2.11111, 2.323232) == 1.10333);

	TEST_CHECK(Lerp(-4.f, 5.f, 0.f) == -4.f);
	TEST_CHECK(Lerp(-4.f, 5.f, 1.f) == 5.f);
	TEST_CHECK(Lerp(9.f, 3.f, 0.f) == 9.f);
	TEST_CHECK(Lerp(9.f, 3.f, 1.f) == 3.f);
	TEST_CHECK(Lerp(-11.f, 11.f, 0.5f) == 0.f);

	TEST_CHECK(Sqrt(0.f) == 0.f);
	TEST_CHECK(Sqrt(1.f) == 1.f);
	TEST_CHECK(Sqrt(25.f) == 5.f);
	TEST_CHECK(Sqrt(9.f) == 3.f);
	TEST_CHECK(Sqrt(2.f) == 1.41421356237309504880f);

	TEST_CHECK(Equal(0.f, 0.f));
	TEST_CHECK(Equal(0.f, 1.f) == false);
	TEST_CHECK(Equal(99.999998f, 99.999998f));
	TEST_CHECK(Equal(17.353535f, 17.353535f));
	TEST_CHECK(Equal(17.3535352f, 17.3535351f));
	TEST_CHECK(AlmostEqual(0.12547f, 0.125721f, 0.001));
	TEST_CHECK(AlmostEqual(0.1259999f, 0.1281111f, 0.001) == false);

	TEST_CHECK(EqualZero(0.f));
	TEST_CHECK(AlmostEqualZero(0.00001f, 0.00001f));
	TEST_CHECK(AlmostEqualZero(-0.00001f, 0.00001f));
	TEST_CHECK(EqualZero(0.000002f) == false);
	TEST_CHECK(AlmostEqualZero(0.0001678f, 0.0002f));

	TEST_CHECK(Ceil(1.f) == 1.f);
	TEST_CHECK(Ceil(-1.f) == -1.f);
	TEST_CHECK(Ceil(1.5125f) == 2.f);
	TEST_CHECK(Ceil(1.99999f) == 2.f);
	TEST_CHECK(Ceil(1.49999f) == 2.f);
	TEST_CHECK(Ceil(1.11111f) == 2.f);
	TEST_CHECK(Ceil(-2.11111f) == -2.f);
	TEST_CHECK(Ceil(-0.5f) == 0.f);

	TEST_CHECK(Floor(1.f) == 1.f);
	TEST_CHECK(Floor(-1.f) == -1.f);
	TEST_CHECK(Floor(1.5125f) == 1.f);
	TEST_CHECK(Floor(1.99999f) == 1.f);
	TEST_CHECK(Floor(1.49999f) == 1.f);
	TEST_CHECK(Floor(1.11111f) == 1.f);
	TEST_CHECK(Floor(-2.11111f) == -3.f);
	TEST_CHECK(Floor(-0.5f) == -1.f);

	TEST_CHECK(Round(1.f) == 1.f);
	TEST_CHECK(Round(-1.f) == -1.f);
	TEST_CHECK(Round(1.5125f) == 2.f);
	TEST_CHECK(Round(1.99999f) == 2.f);
	TEST_CHECK(Round(1.49999f) == 1.f);
	TEST_CHECK(Round(1.11111f) == 1.f);
	TEST_CHECK(Round(-2.11111f) == -2.f);
	TEST_CHECK(Round(-4.75f) == -5.f);
	TEST_CHECK(Round(-0.5f) == -1.f);

	TEST_CHECK(Mod(301.f) == 0.f);
	TEST_CHECK(Equal(Mod(0.5555f), 0.5555f));
	TEST_CHECK(Equal(Mod(-99.125f), -0.125f));
	
	TEST_CHECK(Frac(4.f) == 0.f);
	TEST_CHECK(Equal(Frac(11.5555f), 0.5555f));
	TEST_CHECK(Equal(Frac(-8.125f), -0.125f));

	TEST_CHECK(RangeAdjust(0.f, -1.f, 1.f, -2.f, 1.f) == -0.5f);
	TEST_CHECK(RangeAdjust(0.5f, 0.f, 1.f, -1.f, 1.f) == 0.f);

	TEST_CHECK(Equal(Quantize(5.f, 4.f), 4.f));
	TEST_CHECK(Equal(Quantize(6.f, 9.f), 0.f));
	TEST_CHECK(Equal(Quantize(32.561f, 3.f), 30.f));

	TEST_CHECK(Equal(Wrap(2.5f, 0.f, 1.f), 0.5f));
	TEST_CHECK(Equal(Wrap(4.5, 2.5, 5.25), 4.5f));
	TEST_CHECK(Equal(Wrap(1.7, 0.5, 0.725), 0.575f));
	TEST_CHECK(Equal(Wrap(HalfPi, -Pi, Pi), HalfPi));
	TEST_CHECK(Equal(Wrap(Pi, -Pi, Pi), Pi));
	TEST_CHECK(Equal(Wrap(1.0, 0.0, 1.0), 1.0));
	TEST_CHECK(Equal(Wrap(2.f, 0.f, 1.f), 1.f));
	TEST_CHECK(Equal(Wrap(-7, -6, -3), -4));
	TEST_CHECK(Equal(Wrap(28,  6,  3), 4));
	
	TEST_CHECK(IsFinite(32755.f) == true);
	TEST_CHECK(IsFinite(-9999.999f) == true);
	TEST_CHECK(IsFinite(log(0.0)) == false);
	
	TEST_CHECK(getPOT(256) == 256);
	TEST_CHECK(getPOT(220) == 256);
	TEST_CHECK(getPOT(1960) == 2048);
	TEST_CHECK(getPOT(-140) == 1);

	TEST_CHECK(isPOT(512));
	TEST_CHECK(isPOT(1080) == false);
	TEST_CHECK(isPOT(-256) == false);

	TEST_CHECK(Equal(Sin(0.f), 0.f));
	TEST_CHECK(Equal(Sin(Pi / 6.f), 1.f / 2.f));
	TEST_CHECK(Equal(Sin(Pi / 4.f), float(sqrt(2.0) / 2.0)));
	TEST_CHECK(Equal(Sin(Pi / 3.f), float(sqrt(3.0) / 2.0)));
	TEST_CHECK(Equal(Sin(HalfPi), 1.f));
	TEST_CHECK(Equal(Sin(Pi), 0.f));
	TEST_CHECK(AlmostEqual(Sin(TwoPi), 0.f));
	TEST_CHECK(Equal(Sin(3.f * Pi / 2.f), -1.f));
	TEST_CHECK(Equal(Sin(-HalfPi), -1.f));
	TEST_CHECK(Equal(Sin(-1.5f), -Sin(1.5f)));
	TEST_CHECK(Equal(Sin(1.5f - Pi), -Sin(1.5f)));
	TEST_CHECK(AlmostEqual(Sin(0.4f + Pi), -Sin(0.4f)));

	TEST_CHECK(Equal(ASin(0.f), 0.f));
	TEST_CHECK(Equal(ASin(0.5f), Pi / 6.f));
	TEST_CHECK(Equal(ASin(float(sqrt(2.0) / 2.0)), Pi / 4.f));
	TEST_CHECK(Equal(ASin(float(sqrt(3.0) / 2.0)), Pi / 3.f));
	TEST_CHECK(Equal(ASin(1.f), HalfPi));
	TEST_CHECK(Equal(ASin(-1.f), -HalfPi));
	TEST_CHECK(Equal(ASin(-0.7f), -ASin(0.7f)));

	TEST_CHECK(Equal(Cos(0.f), 1.f));
	TEST_CHECK(Equal(Cos(Pi / 6.f), float(sqrt(3.0) / 2.f)));
	TEST_CHECK(Equal(Cos(Pi / 4.f), float(sqrt(2.0) / 2.0)));
	TEST_CHECK(Equal(Cos(Pi / 3.f), 1.0 / 2.0));
	TEST_CHECK(Equal(Cos(HalfPi), 0.f));
	TEST_CHECK(Equal(Cos(Pi),-1.f));
	TEST_CHECK(Equal(Cos(TwoPi), 1.f));
	TEST_CHECK(Equal(Cos(3.f * Pi / 2.f), 0.f));
	TEST_CHECK(Equal(Cos(-1.5f), Cos(1.5f)));
	TEST_CHECK(Equal(Cos(1.5f - Pi), -Cos(1.5f)));
	TEST_CHECK(Equal(Cos(2.6f - Pi), -Cos(2.6f)));

	TEST_CHECK(Equal(Cos(0.7f + HalfPi),-Sin(0.7f)));
	TEST_CHECK(Equal(Cos(2.5f - HalfPi), Sin(2.5f)));
	TEST_CHECK(Equal(Sin(0.3f + HalfPi), Cos(0.3f)));
	TEST_CHECK(Equal(Sin(1.8f - HalfPi),-Cos(1.8f)));

	const float math_pi = 
#if defined(M_PI)
		static_cast<float>(M_PI);
#else
		static_cast<float>(3.14159265358979323846);
#endif
	
	float pi_e = Abs(Pi - acos(-1.f));
	if(Abs(pi_e) == 0.f) {
		pi_e = 1.e-5f;
	}

	TEST_CHECK(Equal(ACos(1.f), 0.f));
	TEST_CHECK(Equal(ACos(float(sqrt(3.0) / 2.f)), Pi / 6.f));
	TEST_CHECK(Equal(ACos(float(sqrt(2.0) / 2.0)), Pi / 4.f));
	TEST_CHECK(Equal(ACos(1.0 / 2.0), Pi / 3.f));
	TEST_CHECK(Equal(ACos(0.f), HalfPi));
	TEST_CHECK(AlmostEqual(ACos(-1.f), Pi, pi_e));
	TEST_CHECK(Equal(ACos(-0.7f), HalfPi + ASin(0.7f)));

	TEST_CHECK(Equal(Tan(0.f), 0.f));
	TEST_CHECK(Equal(Tan(Pi / 6.f), float(sqrt(3.0) / 3.0)));
	TEST_CHECK(Equal(Tan(Pi / 4.f), 1.f));
	TEST_CHECK(Equal(Tan(Pi / 3.f), float(sqrt(3.0))));
	TEST_CHECK(Equal(Tan(-1.4f), -Tan(1.4f)));
	TEST_CHECK(AlmostEqual(Tan(0.6f + Pi), Tan(0.6f)));

	TEST_CHECK(Equal(ATan(0.f), 0.f));
	TEST_CHECK(Equal(ATan(float(sqrt(3.0) / 3.0)), Pi / 6.f));
	TEST_CHECK(Equal(ATan(1.f), Pi / 4.f));
	TEST_CHECK(Equal(ATan(float(sqrt(3.0))), Pi / 3.f));
	TEST_CHECK(Equal(ATan(-1.4f), -ATan(1.4f)));
	TEST_CHECK(Equal(ATan(1.f/0.4f) + ATan(0.4f), HalfPi));

	TEST_CHECK(Equal(LinearInterpolate(-1.f, 1.f, 0.5f), 0.f));
	TEST_CHECK(Equal(LinearInterpolate(102, 204, 0), 102));
	TEST_CHECK(Equal(LinearInterpolate(102, 204, 1), 204));
	TEST_CHECK(Equal(LinearInterpolate(16, 32, 0.5), 24));
	TEST_CHECK(Equal(LinearInterpolate(-5.f, 5.f, 0.1f), -4.f));
	TEST_CHECK(Equal(LinearInterpolate(-5.f, 5.f, 0.9f), 4.f));
	TEST_CHECK(Equal(LinearInterpolate( 8.f, 9.f, 1.1f), 9.1f));
	TEST_CHECK(Equal(LinearInterpolate( 6.f, 8.f,-0.1f), 5.8f));

	TEST_CHECK(Equal(CosineInterpolate(-1.f, 1.f, 0.0f),-1.f));
	TEST_CHECK(Equal(CosineInterpolate(-1.f, 1.f, 1.0f), 1.f));
	TEST_CHECK(Equal(CosineInterpolate( 1.f, 3.f, 0.5f), 2.f));
	TEST_CHECK(Equal(CosineInterpolate( 2.f, 4.f, 2.0f), 2.f));
	TEST_CHECK(Equal(CosineInterpolate( 2.f, 4.f, 3.0f), 4.f));
	TEST_CHECK(AlmostEqual(CosineInterpolate(-2.f, 2.f, 2.5f), 0.f));
	TEST_CHECK(Equal(CosineInterpolate(-2.f, 2.f, 1.f / 3.f), -1.f));
	TEST_CHECK(Equal(CosineInterpolate(-2.f, 2.f, 7.f / 3.f), -1.f));
	TEST_CHECK(AlmostEqual(CosineInterpolate(-2.f, 2.f, 4.f / 3.f), 1.f));
	
	const float tab[5] = {-2.f,-1.f,0.f,1.f,2.f};
	TEST_CHECK(Equal(LinearInterpolateArray(5, tab, 0.0f),-2.0f));
	TEST_CHECK(Equal(LinearInterpolateArray(5, tab, 1.0f), 2.0f));
	TEST_CHECK(Equal(LinearInterpolateArray(5, tab, 0.5f), 0.0f));
	TEST_CHECK(Equal(LinearInterpolateArray(5, tab, 0.25f),-1.0f));
	TEST_CHECK(Equal(LinearInterpolateArray(5, tab, 0.75f), 1.0f));
	TEST_CHECK(Equal(LinearInterpolateArray(5, tab, 0.125f),-1.5f));
	TEST_CHECK(Equal(LinearInterpolateArray(5, tab, 0.875f), 1.5f));

    TEST_CHECK(ReverseRotationOrder(RO_ZYX) == RO_XYZ);
    TEST_CHECK(ReverseRotationOrder(RO_YZX) == RO_XZY);
    TEST_CHECK(ReverseRotationOrder(RO_ZXY) == RO_YXZ);
    TEST_CHECK(ReverseRotationOrder(RO_XZY) == RO_YZX);
    TEST_CHECK(ReverseRotationOrder(RO_YXZ) == RO_ZXY);
    TEST_CHECK(ReverseRotationOrder(RO_XYZ) == RO_ZYX);
    TEST_CHECK(ReverseRotationOrder(RO_XY) == RO_Default);

    TEST_CHECK(Equal(Pow(2.f, -1.f), 0.5f));
    TEST_CHECK(Equal(Pow(3.f, 2.f), 9.f));
    TEST_CHECK(Equal(Pow(25.f, 0.5f), 5.f));
}
