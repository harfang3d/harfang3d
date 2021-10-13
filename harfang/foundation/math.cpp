// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/math.h"
#include "foundation/rotation_order.h"
#include <cfloat>
#include <cmath>

namespace hg {

RotationOrder ReverseRotationOrder(RotationOrder r) {
	switch (r) {
		case RO_ZYX:
			return RO_XYZ;
		case RO_YZX:
			return RO_XZY;
		case RO_ZXY:
			return RO_YXZ;
		case RO_XZY:
			return RO_YZX;
		case RO_YXZ:
			return RO_ZXY;
		case RO_XYZ:
			return RO_ZYX;
		default:
			return RO_Default;
	}
}

//
float Sqrt(float v) { return sqrtf(v); }

bool TestEqual(float a, float b, float e) { return Abs(b - a) < e; }
bool EqualZero(float v, float e) { return !((v < -e) || (v > e)); }

float Pow(float v, float e) { return pow(v, e); }

float Ceil(float v) { return (v < 0) ? static_cast<float>(static_cast<int>(v)) : static_cast<float>(static_cast<int>(v + 1)); }
float Floor(float v) { return (v < 0) ? static_cast<float>(static_cast<int>(v - 1)) : static_cast<float>(static_cast<int>(v)); }

float Mod(float v) {
	double integral;
	return static_cast<float>(modf(v, &integral));
}

float RangeAdjust(float v, float old_min, float old_max, float new_min, float new_max) {
	return Clamp((v - old_min) / (old_max - old_min) * (new_max - new_min) + new_min, new_min, new_max);
}

float Quantize(float v, float q) { return Floor(v / q) * q; }

float Frac(float v) { return v - int(v); }

//
bool IsFinite(float v) { return (v <= FLT_MAX && v >= -FLT_MAX); }

//
float Sin(float v) { return sin(v); }
float ASin(float v) { return asin(Clamp(v, -1.f, 1.f)); }
float Cos(float v) { return cos(v); }
float ACos(float v) { return acos(Clamp(v, -1.f, 1.f)); }
float Tan(float v) { return tan(v); }
float ATan(float v) { return atan(v); }

} // namespace hg
