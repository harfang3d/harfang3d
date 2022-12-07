// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <math.h>

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/easing.h"

#include "foundation/math.h"

using namespace hg;

void test_easing() {
	{
		TEST_CHECK(GetEaseFunc(E_Linear) == EaseLinear);
		TEST_CHECK(GetEaseFunc(E_Step) == EaseStep);
		TEST_CHECK(GetEaseFunc(E_SmoothStep) == EaseSmoothStep);
		TEST_CHECK(GetEaseFunc(E_InQuad) == EaseInQuad);
		TEST_CHECK(GetEaseFunc(E_OutQuad) == EaseOutQuad);
		TEST_CHECK(GetEaseFunc(E_InOutQuad) == EaseInOutQuad);
		TEST_CHECK(GetEaseFunc(E_OutInQuad) == EaseOutInQuad);
		TEST_CHECK(GetEaseFunc(E_InCubic) == EaseInCubic);
		TEST_CHECK(GetEaseFunc(E_OutCubic) == EaseOutCubic);
		TEST_CHECK(GetEaseFunc(E_InOutCubic) == EaseInOutCubic);
		TEST_CHECK(GetEaseFunc(E_OutInCubic) == EaseOutInCubic);
		TEST_CHECK(GetEaseFunc(E_InQuart) == EaseInQuart);
		TEST_CHECK(GetEaseFunc(E_OutQuart) == EaseOutQuart);
		TEST_CHECK(GetEaseFunc(E_InOutQuart) == EaseInOutQuart);
		TEST_CHECK(GetEaseFunc(E_OutInQuart) == EaseOutInQuart);
		TEST_CHECK(GetEaseFunc(E_InQuint) == EaseInQuint);
		TEST_CHECK(GetEaseFunc(E_OutQuint) == EaseOutQuint);
		TEST_CHECK(GetEaseFunc(E_InOutQuint) == EaseInOutQuint);
		TEST_CHECK(GetEaseFunc(E_OutInQuint) == EaseOutInQuint);
		TEST_CHECK(GetEaseFunc(E_InSine) == EaseInSine);
		TEST_CHECK(GetEaseFunc(E_OutSine) == EaseOutSine);
		TEST_CHECK(GetEaseFunc(E_InOutSine) == EaseInOutSine);
		TEST_CHECK(GetEaseFunc(E_OutInSine) == EaseOutInSine);
		TEST_CHECK(GetEaseFunc(E_InExpo) == EaseInExpo);
		TEST_CHECK(GetEaseFunc(E_OutExpo) == EaseOutExpo);
		TEST_CHECK(GetEaseFunc(E_InOutExpo) == EaseInOutExpo);
		TEST_CHECK(GetEaseFunc(E_OutInExpo) == EaseOutInExpo);
		TEST_CHECK(GetEaseFunc(E_InCirc) == EaseInCirc);
		TEST_CHECK(GetEaseFunc(E_OutCirc) == EaseOutCirc);
		TEST_CHECK(GetEaseFunc(E_InOutCirc) == EaseInOutCirc);
		TEST_CHECK(GetEaseFunc(E_OutInCirc) == EaseOutInCirc);
		TEST_CHECK(GetEaseFunc(E_InElastic) == EaseInElastic);
		TEST_CHECK(GetEaseFunc(E_OutElastic) == EaseOutElastic);
		TEST_CHECK(GetEaseFunc(E_InOutElastic) == EaseInOutElastic);
		TEST_CHECK(GetEaseFunc(E_OutInElastic) == EaseOutInElastic);
		TEST_CHECK(GetEaseFunc(E_InBack) == EaseInBack);
		TEST_CHECK(GetEaseFunc(E_OutBack) == EaseOutBack);
		TEST_CHECK(GetEaseFunc(E_InOutBack) == EaseInOutBack);
		TEST_CHECK(GetEaseFunc(E_OutInBack) == EaseOutInBack);
		TEST_CHECK(GetEaseFunc(E_InBounce) == EaseInBounce);
		TEST_CHECK(GetEaseFunc(E_OutBounce) == EaseOutBounce);
		TEST_CHECK(GetEaseFunc(E_InOutBounce) == EaseInOutBounce);
		TEST_CHECK(GetEaseFunc(E_OutInBounce) == EaseOutInBounce);
		TEST_CHECK(GetEaseFunc(E_Count) == EaseLinear);
	}
	{ 
		TEST_CHECK(EaseLinear(0.0f) == 0.0f); 
		TEST_CHECK(EaseLinear(0.5f) == 0.5f);
		TEST_CHECK(EaseLinear(1.0f) == 1.0f); 
	}
	{
		TEST_CHECK(EaseStep(0.0f) == 0.0f);
		TEST_CHECK(EaseStep(0.25f) == 0.0f);
		TEST_CHECK(EaseStep(0.5f) == 1.0f);
		TEST_CHECK(EaseStep(1.0f) == 1.0f);
	}
	{ 
		TEST_CHECK(EaseSmoothStep(0.0f) == 0.0f);
		TEST_CHECK(AlmostEqual(EaseSmoothStep(0.25f), 0.15625f));
		TEST_CHECK(AlmostEqual(EaseSmoothStep(0.5f), 0.5f));
		TEST_CHECK(AlmostEqual(EaseSmoothStep(0.75f), 0.84375f));
		TEST_CHECK(EaseSmoothStep(1.0f) == 1.0f);
	}
	{
		TEST_CHECK(EaseInQuad(0.0f) == 0.0f);
		TEST_CHECK(AlmostEqual(EaseInQuad(0.25f), 0.0625f));
		TEST_CHECK(AlmostEqual(EaseInQuad(0.5f), 0.25f));
		TEST_CHECK(AlmostEqual(EaseInQuad(0.75f), 0.5625f));
		TEST_CHECK(EaseInQuad(1.0f) == 1.0f);
	}
	{
		TEST_CHECK(EaseOutQuad(0.0f) == 0.0f);
		TEST_CHECK(AlmostEqual(EaseOutQuad(0.25f), 0.4375));
		TEST_CHECK(AlmostEqual(EaseOutQuad(0.5f), 0.75f));
		TEST_CHECK(AlmostEqual(EaseOutQuad(0.75f), 0.9375));
		TEST_CHECK(EaseOutQuad(1.0f) == 1.0f);
	}
	{
		TEST_CHECK(AlmostEqual(EaseInOutQuad(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseInOutQuad(0.25f), 0.125f));
		TEST_CHECK(AlmostEqual(EaseInOutQuad(0.5f), 0.5f));
		TEST_CHECK(AlmostEqual(EaseInOutQuad(0.75f), 0.875f));
		TEST_CHECK(AlmostEqual(EaseInOutQuad(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseOutInQuad(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseOutInQuad(0.25f), 0.375f));
		TEST_CHECK(AlmostEqual(EaseOutInQuad(0.5f), 0.5f));
		TEST_CHECK(AlmostEqual(EaseOutInQuad(0.75f), 0.625f));
		TEST_CHECK(AlmostEqual(EaseOutInQuad(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseInCubic(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseInCubic(0.25f), 0.015625f));
		TEST_CHECK(AlmostEqual(EaseInCubic(0.5f), 0.125f));
		TEST_CHECK(AlmostEqual(EaseInCubic(0.75f), 0.421875f));
		TEST_CHECK(AlmostEqual(EaseInCubic(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseOutCubic(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseOutCubic(0.25f), 0.578125f));
		TEST_CHECK(AlmostEqual(EaseOutCubic(0.5f), 0.875f));
		TEST_CHECK(AlmostEqual(EaseOutCubic(0.75f), 0.984375f));
		TEST_CHECK(AlmostEqual(EaseOutCubic(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseInOutCubic(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseInOutCubic(0.25f), 0.0625f));
		TEST_CHECK(AlmostEqual(EaseInOutCubic(0.5f), 0.5f));
		TEST_CHECK(AlmostEqual(EaseInOutCubic(0.75f), 0.9375f));
		TEST_CHECK(AlmostEqual(EaseInOutCubic(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseOutInCubic(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseOutInCubic(0.25f), 0.4375f));
		TEST_CHECK(AlmostEqual(EaseOutInCubic(0.5f), 0.5f));
		TEST_CHECK(AlmostEqual(EaseOutInCubic(0.75f), 0.5625f));
		TEST_CHECK(AlmostEqual(EaseOutInCubic(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseInQuart(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseInQuart(0.25f), 0.003906f));
		TEST_CHECK(AlmostEqual(EaseInQuart(0.5f), 0.0625f));
		TEST_CHECK(AlmostEqual(EaseInQuart(0.75f), 0.316406f));
		TEST_CHECK(AlmostEqual(EaseInQuart(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseOutQuart(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseOutQuart(0.25f), 0.683594f));
		TEST_CHECK(AlmostEqual(EaseOutQuart(0.5f), 0.9375f));
		TEST_CHECK(AlmostEqual(EaseOutQuart(0.75f), 0.996094f));
		TEST_CHECK(AlmostEqual(EaseOutQuart(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseInOutQuart(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseInOutQuart(0.25f), 0.03125f));
		TEST_CHECK(AlmostEqual(EaseInOutQuart(0.5f), 0.5f));
		TEST_CHECK(AlmostEqual(EaseInOutQuart(0.75f), 0.96875f));
		TEST_CHECK(AlmostEqual(EaseInOutQuart(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseOutInQuart(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseOutInQuart(0.25f), 0.46875f));
		TEST_CHECK(AlmostEqual(EaseOutInQuart(0.5f), 0.5f));
		TEST_CHECK(AlmostEqual(EaseOutInQuart(0.75f), 0.53125f));
		TEST_CHECK(AlmostEqual(EaseOutInQuart(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseInQuint(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseInQuint(0.25f), 0.000977f));
		TEST_CHECK(AlmostEqual(EaseInQuint(0.5f), 0.03125f));
		TEST_CHECK(AlmostEqual(EaseInQuint(0.75f), 0.237305f));
		TEST_CHECK(AlmostEqual(EaseInQuint(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseOutQuint(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseOutQuint(0.25f), 0.762695f));
		TEST_CHECK(AlmostEqual(EaseOutQuint(0.5f), 0.96875f));
		TEST_CHECK(AlmostEqual(EaseOutQuint(0.75f), 0.999023f));
		TEST_CHECK(AlmostEqual(EaseOutQuint(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseInOutQuint(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseInOutQuint(0.25f), 0.015625f));
		TEST_CHECK(AlmostEqual(EaseInOutQuint(0.5f), 0.5f));
		TEST_CHECK(AlmostEqual(EaseInOutQuint(0.75f), 0.984375f));
		TEST_CHECK(AlmostEqual(EaseInOutQuint(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseOutInQuint(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseOutInQuint(0.25f), 0.484375f));
		TEST_CHECK(AlmostEqual(EaseOutInQuint(0.5f), 0.5f));
		TEST_CHECK(AlmostEqual(EaseOutInQuint(0.75f), 0.515625f));
		TEST_CHECK(AlmostEqual(EaseOutInQuint(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseInSine(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseInSine(0.25f), 0.07612f));
		TEST_CHECK(AlmostEqual(EaseInSine(0.5f), 0.292893f));
		TEST_CHECK(AlmostEqual(EaseInSine(0.75f), 0.617317f));
		TEST_CHECK(AlmostEqual(EaseInSine(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseOutSine(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseOutSine(0.25f), 0.382683f));
		TEST_CHECK(AlmostEqual(EaseOutSine(0.5f), 0.707107f));
		TEST_CHECK(AlmostEqual(EaseOutSine(0.75f), 0.92388f));
		TEST_CHECK(AlmostEqual(EaseOutSine(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseInOutSine(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseInOutSine(0.25f), 0.146447f));
		TEST_CHECK(AlmostEqual(EaseInOutSine(0.5f), 0.5f));
		TEST_CHECK(AlmostEqual(EaseInOutSine(0.75f), 0.853553f));
		TEST_CHECK(AlmostEqual(EaseInOutSine(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseOutInSine(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseOutInSine(0.25f), 0.353553f));
		TEST_CHECK(AlmostEqual(EaseOutInSine(0.5f), 0.5f));
		TEST_CHECK(AlmostEqual(EaseOutInSine(0.75f), 0.646447f));
		TEST_CHECK(AlmostEqual(EaseOutInSine(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseInExpo(0.0f), -0.000023f));
		TEST_CHECK(AlmostEqual(EaseInExpo(0.25f), 0.004524f));
		TEST_CHECK(AlmostEqual(EaseInExpo(0.5f), 0.03025f));
		TEST_CHECK(AlmostEqual(EaseInExpo(0.75f), 0.175777f));
		TEST_CHECK(AlmostEqual(EaseInExpo(1.0f), 0.999f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseOutExpo(0.0f), 0.001f));
		TEST_CHECK(AlmostEqual(EaseOutExpo(0.25f), 0.824223f));
		TEST_CHECK(AlmostEqual(EaseOutExpo(0.5f), 0.96975f));
		TEST_CHECK(AlmostEqual(EaseOutExpo(0.75f), 0.995476f));
		TEST_CHECK(AlmostEqual(EaseOutExpo(1.0f), 1.000023f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseInOutExpo(0.0f), -0.000012f));
		TEST_CHECK(AlmostEqual(EaseInOutExpo(0.25f), 0.015125f));
		TEST_CHECK(AlmostEqual(EaseInOutExpo(0.5f), 0.5005f));
		TEST_CHECK(AlmostEqual(EaseInOutExpo(0.75f), 0.984875f));
		TEST_CHECK(AlmostEqual(EaseInOutExpo(1.0f), 1.000012f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseOutInExpo(0.0f), 0.0005f));
		TEST_CHECK(AlmostEqual(EaseOutInExpo(0.25f), 0.484875f));
		TEST_CHECK(AlmostEqual(EaseOutInExpo(0.5f), 0.499988f));
		TEST_CHECK(AlmostEqual(EaseOutInExpo(0.75f), 0.515125f));
		TEST_CHECK(AlmostEqual(EaseOutInExpo(1.0f), 0.9995f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseInCirc(0.0f), -0.0f));
		TEST_CHECK(AlmostEqual(EaseInCirc(0.25f), 0.031754f));
		TEST_CHECK(AlmostEqual(EaseInCirc(0.5f), 0.133975f));
		TEST_CHECK(AlmostEqual(EaseInCirc(0.75f), 0.338562f));
		TEST_CHECK(AlmostEqual(EaseInCirc(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseOutCirc(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseOutCirc(0.25f), 0.661438f));
		TEST_CHECK(AlmostEqual(EaseOutCirc(0.5f), 0.866025f));
		TEST_CHECK(AlmostEqual(EaseOutCirc(0.75f), 0.968246f));
		TEST_CHECK(AlmostEqual(EaseOutCirc(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseInOutCirc(0.0f), -0.0f));
		TEST_CHECK(AlmostEqual(EaseInOutCirc(0.25f), 0.066987f));
		TEST_CHECK(AlmostEqual(EaseInOutCirc(0.5f), 0.5f));
		TEST_CHECK(AlmostEqual(EaseInOutCirc(0.75f), 0.933013f));
		TEST_CHECK(AlmostEqual(EaseInOutCirc(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseOutInCirc(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseOutInCirc(0.25f), 0.433013f));
		TEST_CHECK(AlmostEqual(EaseOutInCirc(0.5f), 0.5f));
		TEST_CHECK(AlmostEqual(EaseOutInCirc(0.75f), 0.566987f));
		TEST_CHECK(AlmostEqual(EaseOutInCirc(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseInElastic(0.0f), -0.000488f));
		TEST_CHECK(AlmostEqual(EaseInElastic(0.25f), -0.005524f));
		TEST_CHECK(AlmostEqual(EaseInElastic(0.5f), -0.015625f));
		TEST_CHECK(AlmostEqual(EaseInElastic(0.75f), 0.088388f));
		TEST_CHECK(AlmostEqual(EaseInElastic(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseOutElastic(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseOutElastic(0.25f), 0.911612f));
		TEST_CHECK(AlmostEqual(EaseOutElastic(0.5f), 1.015625f));
		TEST_CHECK(AlmostEqual(EaseOutElastic(0.75f), 1.005524f));
		TEST_CHECK(AlmostEqual(EaseOutElastic(1.0f), 1.000488f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseInOutElastic(0.0f), -0.000244f));
		TEST_CHECK(AlmostEqual(EaseInOutElastic(0.25f), -0.007813f));
		TEST_CHECK(AlmostEqual(EaseInOutElastic(0.5f), 0.5f));
		TEST_CHECK(AlmostEqual(EaseInOutElastic(0.75f), 1.007813f));
		TEST_CHECK(AlmostEqual(EaseInOutElastic(1.0f), 1.000244f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseOutInElastic(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseOutInElastic(0.25f), 0.507813f));
		TEST_CHECK(AlmostEqual(EaseOutInElastic(0.5f), 0.499756f));
		TEST_CHECK(AlmostEqual(EaseOutInElastic(0.75f), 0.492188f));
		TEST_CHECK(AlmostEqual(EaseOutInElastic(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseInBack(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseInBack(0.25f), -0.161152f));
		TEST_CHECK(AlmostEqual(EaseInBack(0.5f), -0.375f));
		TEST_CHECK(AlmostEqual(EaseInBack(0.75f), -0.108455f));
		TEST_CHECK(AlmostEqual(EaseInBack(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseOutBack(0.0f), -0.0f));
		TEST_CHECK(AlmostEqual(EaseOutBack(0.25f), 1.108455f));
		TEST_CHECK(AlmostEqual(EaseOutBack(0.5f), 1.375f));
		TEST_CHECK(AlmostEqual(EaseOutBack(0.75f), 1.161152f));
		TEST_CHECK(AlmostEqual(EaseOutBack(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseInOutBack(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseInOutBack(0.25f), -0.1875f));
		TEST_CHECK(AlmostEqual(EaseInOutBack(0.5f), 0.5f));
		TEST_CHECK(AlmostEqual(EaseInOutBack(0.75f), 1.1875f));
		TEST_CHECK(AlmostEqual(EaseInOutBack(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseOutInBack(0.0f), -0.0f));
		TEST_CHECK(AlmostEqual(EaseOutInBack(0.25f), 0.6875f));
		TEST_CHECK(AlmostEqual(EaseOutInBack(0.5f), 0.5f));
		TEST_CHECK(AlmostEqual(EaseOutInBack(0.75f), 0.3125f));
		TEST_CHECK(AlmostEqual(EaseOutInBack(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseInBounce(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseInBounce(0.25f), 0.041137f));
		TEST_CHECK(AlmostEqual(EaseInBounce(0.5f), 0.28125f));
		TEST_CHECK(AlmostEqual(EaseInBounce(0.75f), 0.527344f));
		TEST_CHECK(AlmostEqual(EaseInBounce(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseOutBounce(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseOutBounce(0.25f), 0.472656f));
		TEST_CHECK(AlmostEqual(EaseOutBounce(0.5f), 0.71875f));
		TEST_CHECK(AlmostEqual(EaseOutBounce(0.75f), 0.958863f));
		TEST_CHECK(AlmostEqual(EaseOutBounce(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseInOutBounce(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseInOutBounce(0.25f), 0.140625f));
		TEST_CHECK(AlmostEqual(EaseInOutBounce(0.5f), 0.5f));
		TEST_CHECK(AlmostEqual(EaseInOutBounce(0.75f), 0.859375f));
		TEST_CHECK(AlmostEqual(EaseInOutBounce(1.0f), 1.0f));
	}
	{
		TEST_CHECK(AlmostEqual(EaseOutInBounce(0.0f), 0.0f));
		TEST_CHECK(AlmostEqual(EaseOutInBounce(0.25f), 0.359375f));
		TEST_CHECK(AlmostEqual(EaseOutInBounce(0.5f), 0.5f));
		TEST_CHECK(AlmostEqual(EaseOutInBounce(0.75f), 0.640625f));
		TEST_CHECK(AlmostEqual(EaseOutInBounce(1.0f), 1.0f));
	}
}
