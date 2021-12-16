// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/color.h"
#include "foundation/math.h"
#include "foundation/vector3.h"
#include "foundation/vector4.h"

namespace hg {

const Color Color::Zero{0, 0, 0, 0}, Color::One{1, 1, 1, 1}, Color::White{1, 1, 1}, Color::Grey{0.5f, 0.5f, 0.5f}, Color::Black{0, 0, 0}, Color::Red{1, 0, 0},
	Color::Green{0, 1, 0}, Color::Blue{0, 0, 1}, Color::Yellow{1, 1, 0}, Color::Orange{1, 0.3f, 0}, Color::Purple{1, 0, 1}, Color::Transparent{0, 0, 0, 0};

Color operator+(const Color &a, const Color &b) { return {a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a}; }
Color operator+(const Color &a, const float v) { return {a.r + v, a.g + v, a.b + v, a.a + v}; }
Color operator-(const Color &a, const Color &b) { return {a.r - b.r, a.g - b.g, a.b - b.b, a.a - b.a}; }
Color operator-(const Color &a, const float v) { return {a.r - v, a.g - v, a.b - v, a.a - v}; }
Color operator*(const Color &a, const Color &b) { return {a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a}; }
Color operator*(const Color &a, const float v) { return {a.r * v, a.g * v, a.b * v, a.a * v}; }
Color operator/(const Color &a, const Color &b) { return {a.r / b.r, a.g / b.g, a.b / b.b, a.a / b.a}; }
Color operator/(const Color &a, const float v) { return {a.r / v, a.g / v, a.b / v, a.a / v}; }

float ColorToGrayscale(const Color &c) { return 0.3f * c.r + 0.59f * c.g + 0.11f * c.b; }

uint32_t ColorToRGBA32(const Color &c) {
	return static_cast<uint8_t>(Clamp(c.r, 0.f, 1.f) * 255.f) | (static_cast<uint8_t>(Clamp(c.g, 0.f, 1.f) * 255.f) << 8) |
		   (static_cast<uint8_t>(Clamp(c.b, 0.f, 1.f) * 255.f) << 16) | (static_cast<uint8_t>(Clamp(c.a, 0.f, 1.f) * 255.f) << 24);
}

uint32_t ColorToABGR32(const Color &c) {
	return static_cast<uint8_t>(Clamp(c.a, 0.f, 1.f) * 255.f) | (static_cast<uint8_t>(Clamp(c.b, 0.f, 1.f) * 255.f) << 8) |
		   (static_cast<uint8_t>(Clamp(c.g, 0.f, 1.f) * 255.f) << 16) | (static_cast<uint8_t>(Clamp(c.r, 0.f, 1.f) * 255.f) << 24);
}

Color ColorFromRGBA32(unsigned int value) {
	Color c;
	c.r = float((value)&0xff) / 255.f;
	c.g = float((value >> 8) & 0xff) / 255.f;
	c.b = float((value >> 16) & 0xff) / 255.f;
	c.a = float((value >> 24) & 0xff) / 255.f;
	return c;
}

Color ColorFromABGR32(unsigned int value) {
	Color c;
	c.a = float((value)&0xff) / 255.f;
	c.b = float((value >> 8) & 0xff) / 255.f;
	c.g = float((value >> 16) & 0xff) / 255.f;
	c.r = float((value >> 24) & 0xff) / 255.f;
	return c;
}

//
unsigned int ARGB32ToRGBA32(unsigned int argb) {
	return ((argb & 0xff) << 24) + (((argb >> 8) & 0xff) << 16) + (((argb >> 16) & 0xff) << 8) + ((argb >> 24) & 0xff);
}

//
uint32_t RGBA32(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	uint8_t u8[4] = {a, b, g, r};
	return *reinterpret_cast<uint32_t *>(u8);
}

uint32_t ARGB32(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	uint8_t u8[4] = {b, g, r, a};
	return *reinterpret_cast<uint32_t *>(u8);
}

/// Vector squared distance.
float Dist2(const Color &i, const Color &j) {
	return (j.r - i.r) * (j.r - i.r) + (j.g - i.g) * (j.g - i.g) + (j.b - i.b) * (j.b - i.b) + (j.a - i.a) * (j.a - i.a);
}
/// Vector distance.
float Dist(const Color &i, const Color &j) { return Sqrt(Dist2(i, j)); }

/// Compare two colors with a configurable threshold.
bool AlmostEqual(const Color &a, const Color &b, float epsilon) {
	return Abs(a.r - b.r) <= epsilon && Abs(a.g - b.g) <= epsilon && Abs(a.b - b.b) <= epsilon && Abs(a.a - b.a) <= epsilon;
}

/// Scale the chroma component of a color, return the result as a new color.
Color ChromaScale(const Color &c, float k) { return {c.r * k, c.g * k, c.b * k, c.a}; }
/// Scale the alpha component of a color, return the result as a new color.
Color AlphaScale(const Color &c, float k) { return {c.r, c.g, c.b, c.a * k}; }

//
Color Clamp(const Color &c, float min, float max) { return {Clamp(c.r, min, max), Clamp(c.g, min, max), Clamp(c.b, min, max), Clamp(c.a, min, max)}; }
Color Clamp(const Color &c, const Color &min, const Color &max) {
	return {Clamp(c.r, min.r, max.r), Clamp(c.g, min.g, max.g), Clamp(c.b, min.b, max.b), Clamp(c.a, min.a, max.a)};
}

//
Color ClampLen(const Color &c, float min, float max) {
	const auto l2 = float(c.r * c.r + c.g * c.g + c.b * c.b);
	if ((l2 >= (min * min) && l2 <= (max * max)) || l2 < 0.000001f)
		return c;
	const auto l = Sqrt(l2);
	return c * Clamp(l, min, max) / l;
}

Color ColorFromVector3(const Vec3 &v) { return {v.x, v.y, v.z, 1}; }
Color ColorFromVector4(const Vec4 &v) { return {v.x, v.y, v.z, v.w}; }

//
Color ToHLS(const Color &c) {
	const float min = Min(c.r, c.g, c.b);
	const float max = Max(c.r, c.g, c.b);

	const double diff = max - min;
	const float l = (max + min) / 2;

	float h = 0, s = 0;

	if (diff > 0.f) {
		if (l <= 0.5)
			s = diff / (max + min);
		else
			s = diff / (2 - max - min);

		const double r_dist = (max - c.r) / diff;
		const double g_dist = (max - c.g) / diff;
		const double b_dist = (max - c.b) / diff;

		if (c.r == max)
			h = b_dist - g_dist;
		else if (c.g == max)
			h = 2.f + r_dist - b_dist;
		else
			h = 4.f + g_dist - r_dist;

		h = h * 60.f;
		if (h < 0.f)
			h += 360.f;
	}

	return {h, l, s, c.a};
}

static float QqhToRgb(float q1, float q2, float hue) {
	if (hue > 360.f)
		hue -= 360.f;
	else if (hue < 0.f)
		hue += 360.f;

	if (hue < 60.f)
		return q1 + (q2 - q1) * hue / 60.f;
	if (hue < 180.f)
		return q2;
	if (hue < 240.f)
		return q1 + (q2 - q1) * (240.f - hue) / 60.f;

	return q1;
}

Color FromHLS(const Color &c) {
	const float p2 = c.g <= 0.5f ? c.g * (1.f + c.b) : c.g + c.b - c.g * c.b;
	const float p1 = 2.f * c.g - p2;

	float r, g, b;

	if (c.b == 0.f) {
		r = c.g;
		g = c.g;
		b = c.g;
	} else {
		r = QqhToRgb(p1, p2, c.r + 120.f);
		g = QqhToRgb(p1, p2, c.r);
		b = QqhToRgb(p1, p2, c.r - 120.f);
	}

	return {r, g, b, c.a};
}

//
Color SetHue(const Color &c, float h) {
	auto hls = ToHLS(c);
	hls.r = h;
	return FromHLS(hls);
}

Color SetSaturation(const Color &c, float s) {
	auto hls = ToHLS(c);
	hls.b = s;
	return FromHLS(hls);
}

Color SetLuminance(const Color &c, float l) {
	auto hls = ToHLS(c);
	hls.g = l;
	return FromHLS(hls);
}

Color ScaleHue(const Color &c, float k) {
	auto hls = ToHLS(c);
	hls.r *= k;
	return FromHLS(hls);
}

Color ScaleSaturation(const Color &c, float k) {
	auto hls = ToHLS(c);
	hls.b *= k;
	return FromHLS(hls);
}

Color ScaleLuminance(const Color &c, float k) {
	auto hls = ToHLS(c);
	hls.g *= k;
	return FromHLS(hls);
}

} // namespace hg
