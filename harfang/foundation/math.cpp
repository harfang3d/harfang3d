// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/math.h"
#include "foundation/rotation_order.h"

#include <cfloat>
#include <cmath>

namespace hg {

RotationOrder ReverseRotationOrder(RotationOrder in) {
	RotationOrder out;

	if (in == RO_ZYX) {
		out = RO_XYZ;
	} else if (in == RO_YZX) {
		out = RO_XZY;
	} else if (in == RO_ZXY) {
		out = RO_YXZ;
	} else if (in == RO_XZY) {
		out = RO_YZX;
	} else if (in == RO_YXZ) {
		out = RO_ZXY;
	} else if (in == RO_XYZ) {
		out = RO_ZYX;
	} else {
		out = RO_Default;
	}

	return out;
}

//
float Sqrt(float v) {
	return sqrtf(v);
}

float Pow(float v, float e) {
	return pow(v, e);
}

float Ceil(float v) {
	return ceilf(v);
}

float Floor(float v) {
	return floorf(v);
}

float Round(float v) {
	return roundf(v);
}

float Mod(float v) {
	double integral;
	return static_cast<float>(modf(v, &integral));
}

float RangeAdjust(float v, float old_min, float old_max, float new_min, float new_max) {
	return Clamp((v - old_min) / (old_max - old_min) * (new_max - new_min) + new_min, new_min, new_max);
}

float Quantize(float v, float q) {
	return Floor(v / q) * q;
}

float Frac(float v) {
	return v - static_cast<float>(static_cast<int>(v));
}

//
bool IsFinite(float v) {
	return (v <= FLT_MAX) && (v >= -FLT_MAX);
}

//
float Sin(float v) {
	return sin(v);
}

float ASin(float v) {
	return asin(Clamp(v, -1.F, 1.F));
}

float Cos(float v) {
	return cos(v);
}

float ACos(float v) {
	return acos(Clamp(v, -1.F, 1.F));
}

float Tan(float v) {
	return tan(v);
}

float ATan(float v) {
	return atan(v);
}

} // namespace hg
