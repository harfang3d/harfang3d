// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/easing.h"
#include "foundation/math.h"

#include <math.h>

// adapted from BX Copyright 2011-2019 Branimir Karadzic. All rights reserved.
// which is much more logic and practical than the original code

// Reference(s):
// - https://web.archive.org/web/20181126040153/https://easings.net/
// - https://web.archive.org/web/20181126040212/http://robertpenner.com/easing/
//

namespace hg {

template <EaseFunc Ease> float EaseOut(float t) { return 1.f - Ease(1.f - t); }

template <EaseFunc EaseFrom0toH, EaseFunc EaseFromHto1> float EaseMix(float t) {
	return t < 0.5f ? EaseFrom0toH(2.f * t) * 0.5f : EaseFromHto1(2.f * t - 1.f) * 0.5f + 0.5f;
}

float EaseLinear(float t) { return t; }
float EaseStep(float t) { return t < 0.5f ? 0.f : 1.f; }
float EaseSmoothStep(float t) { return (t * t) * (3.f - 2.f * t); }
float EaseInQuad(float t) { return (t * t); }
float EaseOutQuad(float t) { return EaseOut<EaseInQuad>(t); }
float EaseInOutQuad(float t) { return EaseMix<EaseInQuad, EaseOutQuad>(t); }
float EaseOutInQuad(float t) { return EaseMix<EaseOutQuad, EaseInQuad>(t); }
float EaseInCubic(float t) { return t * t * t; }
float EaseOutCubic(float t) { return EaseOut<EaseInCubic>(t); }
float EaseInOutCubic(float t) { return EaseMix<EaseInCubic, EaseOutCubic>(t); }
float EaseOutInCubic(float t) { return EaseMix<EaseOutCubic, EaseInCubic>(t); }
float EaseInQuart(float t) { return t * t * t * t; }
float EaseOutQuart(float t) { return EaseOut<EaseInQuart>(t); }
float EaseInOutQuart(float t) { return EaseMix<EaseInQuart, EaseOutQuart>(t); }
float EaseOutInQuart(float t) { return EaseMix<EaseOutQuart, EaseInQuart>(t); }
float EaseInQuint(float t) { return t * t * t * t * t; }
float EaseOutQuint(float t) { return EaseOut<EaseInQuint>(t); }
float EaseInOutQuint(float t) { return EaseMix<EaseInQuint, EaseOutQuint>(t); }
float EaseOutInQuint(float t) { return EaseMix<EaseOutQuint, EaseInQuint>(t); }
float EaseInSine(float t) { return 1.f - cosf(t * HalfPi); }
float EaseOutSine(float t) { return EaseOut<EaseInSine>(t); }
float EaseInOutSine(float t) { return EaseMix<EaseInSine, EaseOutSine>(t); }
float EaseOutInSine(float t) { return EaseMix<EaseOutSine, EaseInSine>(t); }
float EaseInExpo(float t) { return powf(2.f, 10.f * (t - 1.f)) - 0.001f; }
float EaseOutExpo(float t) { return EaseOut<EaseInExpo>(t); }
float EaseInOutExpo(float t) { return EaseMix<EaseInExpo, EaseOutExpo>(t); }
float EaseOutInExpo(float t) { return EaseMix<EaseOutExpo, EaseInExpo>(t); }
float EaseInCirc(float t) { return -(sqrtf(1.f - t * t) - 1.f); }
float EaseOutCirc(float t) { return EaseOut<EaseInCirc>(t); }
float EaseInOutCirc(float t) { return EaseMix<EaseInCirc, EaseOutCirc>(t); }
float EaseOutInCirc(float t) { return EaseMix<EaseOutCirc, EaseInCirc>(t); }
float EaseOutElastic(float t) { return powf(2.f, -10.f * t) * sinf((t - 0.3f / 4.f) * (2.f * Pi) / 0.3f) + 1.f; }
float EaseInElastic(float t) { return EaseOut<EaseOutElastic>(t); }
float EaseInOutElastic(float t) { return EaseMix<EaseInElastic, EaseOutElastic>(t); }
float EaseOutInElastic(float t) { return EaseMix<EaseOutElastic, EaseInElastic>(t); }
float EaseInBack(float t) { return EaseInCubic(t) - t * sinf(t * Pi); }
float EaseOutBack(float t) { return EaseOut<EaseInBack>(t); }
float EaseInOutBack(float t) { return EaseMix<EaseInBack, EaseOutBack>(t); }
float EaseOutInBack(float t) { return EaseMix<EaseOutBack, EaseInBack>(t); }

float EaseOutBounce(float t) {
	if (4.f / 11.f > t)
		return 121.f / 16.f * t * t;

	if (8.f / 11.f > t)
		return 363.f / 40.f * t * t - 99.f / 10.f * t + 17.f / 5.f;

	if (9.f / 10.f > t)
		return 4356.f / 361.f * t * t - 35442.f / 1805.f * t + 16061.f / 1805.f;

	return 54.f / 5.f * t * t - 513.f / 25.f * t + 268.f / 25.f;
}

float EaseInBounce(float t) { return EaseOut<EaseOutBounce>(t); }
float EaseInOutBounce(float t) { return EaseMix<EaseInBounce, EaseOutBounce>(t); }
float EaseOutInBounce(float t) { return EaseMix<EaseOutBounce, EaseInBounce>(t); }

//
static const EaseFunc Ease_func[E_Count] = {EaseLinear, EaseStep, EaseSmoothStep, EaseInQuad, EaseOutQuad, EaseInOutQuad, EaseOutInQuad, EaseInCubic,
	EaseOutCubic, EaseInOutCubic, EaseOutInCubic, EaseInQuart, EaseOutQuart, EaseInOutQuart, EaseOutInQuart, EaseInQuint, EaseOutQuint, EaseInOutQuint,
	EaseOutInQuint, EaseInSine, EaseOutSine, EaseInOutSine, EaseOutInSine, EaseInExpo, EaseOutExpo, EaseInOutExpo, EaseOutInExpo, EaseInCirc, EaseOutCirc,
	EaseInOutCirc, EaseOutInCirc, EaseInElastic, EaseOutElastic, EaseInOutElastic, EaseOutInElastic, EaseInBack, EaseOutBack, EaseInOutBack, EaseOutInBack,
	EaseInBounce, EaseOutBounce, EaseInOutBounce, EaseOutInBounce};

EaseFunc GetEaseFunc(Easing easing) { return Ease_func[easing]; }

} // namespace hg
