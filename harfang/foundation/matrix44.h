// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

namespace hg {

struct Vec3;
struct Vec4;
struct Mat4;

/// Matrix 4x4
struct Mat44 {
	static const Mat44 Zero;
	static const Mat44 Identity;

	Mat44() = default;
	explicit Mat44(const Mat4 &m);
	Mat44(float m00, float m10, float m20, float m30, float m01, float m11, float m21, float m31, float m02, float m12, float m22, float m32, float m03,
		float m13, float m23, float m33);

	float m[4][4];
};

bool operator==(const Mat44 &a, const Mat44 &b);
bool operator!=(const Mat44 &a, const Mat44 &b);

Mat44 operator*(const Mat44 &a, const Mat44 &b);

Mat44 operator*(const Mat44 &a, const Mat4 &b);
Mat44 operator*(const Mat4 &a, const Mat44 &b);

Vec3 operator*(const Mat44 &m, const Vec3 &v);
Vec4 operator*(const Mat44 &m, const Vec4 &v);

void TransformVec3(const Mat44 &__restrict m, Vec4 *__restrict out, const Vec3 *__restrict in, unsigned int count = 1);
void TransformVec4(const Mat44 &__restrict m, Vec4 *__restrict out, const Vec4 *__restrict in, unsigned int count = 1);

Mat44 Transpose(const Mat44 &m);

/// Inverse matrix 4x4.
Mat44 Inverse(const Mat44 &m, bool &result);
/// Inverse matrix 4x4, this variant does not report error and might silently fail.
Mat44 Inverse(const Mat44 &m);

Vec4 GetRow(const Mat44 &m, unsigned int n);
Vec4 GetColumn(const Mat44 &m, unsigned int n);

void SetRow(Mat44 &m, unsigned int n, const Vec4 &v);
void SetColumn(Mat44 &m, unsigned int n, const Vec4 &v);

} // namespace hg
