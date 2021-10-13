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
	uint32_t value;
	const auto pl = reinterpret_cast<uint8_t *>(&value);
	pl[0] = static_cast<uint8_t>(Clamp(c.r, 0.f, 1.f) * 255.f);
	pl[1] = static_cast<uint8_t>(Clamp(c.g, 0.f, 1.f) * 255.f);
	pl[2] = static_cast<uint8_t>(Clamp(c.b, 0.f, 1.f) * 255.f);
	pl[3] = static_cast<uint8_t>(Clamp(c.a, 0.f, 1.f) * 255.f);
	return value;
}

uint32_t ColorToABGR32(const Color &c) {
	uint32_t value;
	const auto pl = reinterpret_cast<uint8_t *>(&value);
	pl[0] = static_cast<uint8_t>(Clamp(c.a, 0.f, 1.f) * 255.f);
	pl[1] = static_cast<uint8_t>(Clamp(c.b, 0.f, 1.f) * 255.f);
	pl[2] = static_cast<uint8_t>(Clamp(c.g, 0.f, 1.f) * 255.f);
	pl[3] = static_cast<uint8_t>(Clamp(c.r, 0.f, 1.f) * 255.f);
	return value;
}

Color ColorFromRGBA32(unsigned int value) {
	Color c;
	const auto i = reinterpret_cast<const uint8_t *>(&value);
	c.r = float(i[0]) / 255.f;
	c.g = float(i[1]) / 255.f;
	c.b = float(i[2]) / 255.f;
	c.a = float(i[3]) / 255.f;
	return c;
}

Color ColorFromABGR32(unsigned int value) {
	Color c;
	const auto i = reinterpret_cast<const uint8_t *>(&value);
	c.a = float(i[0]) / 255.f;
	c.b = float(i[1]) / 255.f;
	c.g = float(i[2]) / 255.f;
	c.r = float(i[3]) / 255.f;
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

} // namespace hg
