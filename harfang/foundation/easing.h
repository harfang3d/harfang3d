// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <cstdint>

namespace hg {

enum Easing : uint8_t {
	E_Linear,
	E_Step,
	E_SmoothStep,
	E_InQuad,
	E_OutQuad,
	E_InOutQuad,
	E_OutInQuad,
	E_InCubic,
	E_OutCubic,
	E_InOutCubic,
	E_OutInCubic,
	E_InQuart,
	E_OutQuart,
	E_InOutQuart,
	E_OutInQuart,
	E_InQuint,
	E_OutQuint,
	E_InOutQuint,
	E_OutInQuint,
	E_InSine,
	E_OutSine,
	E_InOutSine,
	E_OutInSine,
	E_InExpo,
	E_OutExpo,
	E_InOutExpo,
	E_OutInExpo,
	E_InCirc,
	E_OutCirc,
	E_InOutCirc,
	E_OutInCirc,
	E_InElastic,
	E_OutElastic,
	E_InOutElastic,
	E_OutInElastic,
	E_InBack,
	E_OutBack,
	E_InOutBack,
	E_OutInBack,
	E_InBounce,
	E_OutBounce,
	E_InOutBounce,
	E_OutInBounce,
	E_Count
};

using EaseFunc = float (*)(float t);
EaseFunc GetEaseFunc(Easing easing);

float EaseLinear(float t);
float EaseStep(float t);
float EaseSmoothStep(float t);
float EaseInQuad(float t);
float EaseOutQuad(float t);
float EaseInOutQuad(float t);
float EaseOutInQuad(float t);
float EaseInCubic(float t);
float EaseOutCubic(float t);
float EaseInOutCubic(float t);
float EaseOutInCubic(float t);
float EaseInQuart(float t);
float EaseOutQuart(float t);
float EaseInOutQuart(float t);
float EaseOutInQuart(float t);
float EaseInQuint(float t);
float EaseOutQuint(float t);
float EaseInOutQuint(float t);
float EaseOutInQuint(float t);
float EaseInSine(float t);
float EaseOutSine(float t);
float EaseInOutSine(float t);
float EaseOutInSine(float t);
float EaseInExpo(float t);
float EaseOutExpo(float t);
float EaseInOutExpo(float t);
float EaseOutInExpo(float t);
float EaseInCirc(float t);
float EaseOutCirc(float t);
float EaseInOutCirc(float t);
float EaseOutInCirc(float t);
float EaseOutElastic(float t);
float EaseInElastic(float t);
float EaseInOutElastic(float t);
float EaseOutInElastic(float t);
float EaseInBack(float t);
float EaseOutBack(float t);
float EaseInOutBack(float t);
float EaseOutInBack(float t);
float EaseOutBounce(float t);
float EaseInBounce(float t);
float EaseInOutBounce(float t);
float EaseOutInBounce(float t);

} // namespace hg
