// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/half_float.h"

namespace hg {

// -15 stored using a single precision bias of 127
static const unsigned int HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP = 0x38000000;
// max exponent value in single precision that will be converted
// to Inf or Nan when stored as a half-float
static const unsigned int HALF_FLOAT_MAX_BIASED_EXP_AS_SINGLE_FP_EXP = 0x47800000;
// 255 is the max exponent biased value
static const unsigned int FLOAT_MAX_BIASED_EXP = (0xFF << 23);
static const unsigned int HALF_FLOAT_MAX_BIASED_EXP = (0x1F << 10);

hfloat float_to_hfloat(float v) {
	unsigned int x = *(unsigned int *)&v;
	unsigned int sign = (unsigned short)(x >> 31);
	unsigned int mantissa;
	unsigned int exp;

	hfloat hf;

	// get mantissa
	mantissa = x & ((1 << 23) - 1);

	// get exponent bits
	exp = x & FLOAT_MAX_BIASED_EXP;

	if (exp >= HALF_FLOAT_MAX_BIASED_EXP_AS_SINGLE_FP_EXP) {
		// check if the original single precision float number is a NaN
		if (mantissa && (exp == FLOAT_MAX_BIASED_EXP)) {
			// we have a single precision NaN
			mantissa = (1 << 23) - 1;
		} else {
			// 16-bit half-float representation stores number as Inf
			mantissa = 0;
		}

		hf = (hfloat(sign) << 15) | hfloat(HALF_FLOAT_MAX_BIASED_EXP) | hfloat(mantissa >> 13);
	}
	// check if exponent is <= -15
	else if (exp <= HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP) {
		// store a denorm half-float value or zero.
		exp = (HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP - exp) >> 23;
		mantissa >>= (14 + exp);
		hf = (hfloat(sign) << 15) | hfloat(mantissa);
	} else {
		hf = (hfloat(sign) << 15) | hfloat((exp - HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP) >> 13) | hfloat(mantissa >> 13);
	}

	return hf;
}

float hfloat_to_float(hfloat v) {
	unsigned int sign = (unsigned int)(v >> 15);
	unsigned int mantissa = (unsigned int)(v & ((1 << 10) - 1));
	unsigned int exp = (unsigned int)(v & HALF_FLOAT_MAX_BIASED_EXP);
	unsigned int f;

	if (exp == HALF_FLOAT_MAX_BIASED_EXP) {
		// we have a half-float NaN or Inf
		// half-float NaNs will be converted to a single precision NaN
		// half-float Infs will be converted to a single precision Inf
		exp = FLOAT_MAX_BIASED_EXP;

		if (mantissa)
			mantissa = (1 << 23) - 1; // set all bits to indicate a NaN
	} else if (exp == 0x0) {
		// convert half-float zero/denorm to single precision value
		if (mantissa) {
			mantissa <<= 1;
			exp = HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP;

			// check for leading 1 in denorm mantissa
			while ((mantissa & (1 << 10)) == 0) {
				// for every leading 0, decrement single precision exponent by 1
				// and shift half-float mantissa value to the left
				mantissa <<= 1;
				exp -= (1 << 23);
			}

			// clamp the mantissa to 10-bits
			mantissa &= ((1 << 10) - 1);
			// shift left to generate single-precision mantissa of 23-bits
			mantissa <<= 13;
		}
	} else {
		// shift left to generate single-precision mantissa of 23-bits
		mantissa <<= 13;

		// generate single precision biased exponent value
		exp = (exp << 13) + HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP;
	}

	f = (sign << 31) | exp | mantissa;
	return *reinterpret_cast<float *>(&f);
}

} // namespace hg
