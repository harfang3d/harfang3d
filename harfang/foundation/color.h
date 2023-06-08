// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <cstdint>

#include "foundation/math.h"
#include "foundation/assert.h"

#ifdef max
#	undef max
#endif

namespace hg {

/// RGBA floating point color
struct Color {
	static const Color Zero;
	static const Color One;
	static const Color White;
	static const Color Grey;
	static const Color Black;
	static const Color Red;
	static const Color Green;
	static const Color Blue;
	static const Color Yellow;
	static const Color Orange;
	static const Color Purple;
	static const Color Transparent;

	Color() : r(0.F), g(0.F), b(0.F), a(0.F) {}
	Color(float r_, float g_, float b_, float a_ = 1.F) : r(r_), g(g_), b(b_), a(a_) {}

	inline Color &operator+=(const Color &c) {
		r += c.r;
		g += c.g;
		b += c.b;
		a += c.a;
		return *this;
	}

	inline Color &operator+=(const float k) {
		r += k;
		g += k;
		b += k;
		a += k;
		return *this;
	}

	inline Color &operator-=(const Color &c) {
		r -= c.r;
		g -= c.g;
		b -= c.b;
		a -= c.a;
		return *this;
	}

	inline Color &operator-=(const float k) {
		r -= k;
		g -= k;
		b -= k;
		a -= k;
		return *this;
	}

	inline Color &operator*=(const Color &c) {
		r *= c.r;
		g *= c.g;
		b *= c.b;
		a *= c.a;
		return *this;
	}

	inline Color &operator*=(const float k) {
		r *= k;
		g *= k;
		b *= k;
		a *= k;
		return *this;
	}

	inline Color &operator/=(const Color &c) {
		r /= c.r;
		g /= c.g;
		b /= c.b;
		a /= c.a;
		return *this;
	}

	inline Color &operator/=(const float k) {
		float i = 1.0f / k;
		r *= i;
		g *= i;
		b *= i;
		a *= i;
		return *this;
	}

	inline float operator[](int n) const {
		__ASSERT__(n >= 0 && n <= 3);
		float res;

		if (n == 0) {
			res = r;
		} else if (n == 1) {
			res = g;
		} else if (n == 2) {
			res = b;
		} else if (n == 3) {
			res = a;
		} else {
			res = std::numeric_limits<float>::max();
		}

		return res;
	}

	inline float &operator[](int n) {
		__ASSERT__(n >= 0 && n <= 3);
		float *res;

		if (n == 0) {
			res = &r;
		} else if (n == 1) {
			res = &g;
		} else if (n == 2) {
			res = &b;
		} else if (n == 3) {
			res = &a;
		} else {
			res = nullptr;
		}

		return *res;
	}

	float r, g, b, a;
};

inline bool operator==(const Color &a, const Color &b) {
	return Equal(a.r, b.r) && Equal(a.g, b.g) && Equal(a.b, b.b) && Equal(a.a, b.a);
}

inline bool operator!=(const Color &a, const Color &b) { return NotEqual(a.r, b.r) || NotEqual(a.g, b.g) || NotEqual(a.b, b.b) || NotEqual(a.a, b.a); }

Color operator+(const Color &a, const Color &b);
Color operator+(const Color &a, const float v);
Color operator-(const Color &a, const Color &b);
Color operator-(const Color &a, const float v);
Color operator*(const Color &a, const Color &b);
Color operator*(const Color &a, const float v);
Color operator*(const float v, const Color &a);
Color operator/(const Color &a, const Color &b);
Color operator/(const Color &a, const float v);

/**
	@short Return a grayscale value representing this color
	The grayscale value is computed accounting for the human eye color intensity perception.
*/
float ColorToGrayscale(const Color &c);

/// Return the color object as an RGBA value.
uint32_t ColorToRGBA32(const Color &c);
/// Return a 32 bit ABGR integer from a color.
uint32_t ColorToABGR32(const Color &c);
/// Create a color from a 32 bit RGBA integer.
Color ColorFromRGBA32(unsigned int rgba32);
/// Create a color from a 32 bit ABGR integer.
Color ColorFromABGR32(unsigned int abgr32);

/// Convert from ARGB to RGBA.
uint32_t ARGB32ToRGBA32(unsigned int argb);

/// Create a color from separate components.
uint32_t RGBA32(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
uint32_t ARGB32(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

/// Vector squared distance.
float Dist2(const Color &i, const Color &j);
/// Vector distance.
float Dist(const Color &i, const Color &j);

/// Compare two colors.
bool AlmostEqual(const Color &a, const Color &b, const float epsilon = 0.00001F);

/// Scale the chroma component of a color, return the result as a new color.
Color ChromaScale(const Color &c, float k);
/// Scale the alpha component of a color, return the result as a new color.
Color AlphaScale(const Color &c, float k);

/// Clamp color components to [min;max].
Color Clamp(const Color &c, float min, float max);
/// Clamp color components to [min;max].
Color Clamp(const Color &c, const Color &min, const Color &max);
/// Clamp color magnitude to [min;max].
Color ClampLen(const Color &c, float min, float max);

struct Vec3;
Color ColorFromVector3(const Vec3 &);

struct Vec4;
Color ColorFromVector4(const Vec4 &);

/// Create a color from integer values in the [0;255] range.
inline Color ColorI(int r, int g, int b, int a = 255) { return {float(r) / 255.f, float(g) / 255.f, float(b) / 255.f, float(a) / 255.f}; }

/// Convert input RGBA color to hue/luminance/saturation, alpha channel is left unmodified.
Color ToHLS(const Color &);
/// Convert input hue/luminance/saturation color to RGBA, alpha channel is left unmodified.
Color FromHLS(const Color &);

Color SetHue(const Color &c, float h);
Color SetSaturation(const Color &c, float s);
Color SetLuminance(const Color &c, float l);
Color ScaleHue(const Color &c, float k);
Color ScaleSaturation(const Color &c, float k);
Color ScaleLuminance(const Color &c, float k);

} // namespace hg
