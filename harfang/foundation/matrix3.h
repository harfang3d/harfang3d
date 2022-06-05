// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/rotation_order.h"

namespace hg {

template <typename T> struct tVec2;
struct Vec3;
struct Vec4;
struct Mat4;

/**
	@short Matrix 3x3
	This matrix class is column major.
*/
struct Mat3 {
	static const Mat3 Zero;
	static const Mat3 Identity;

	Mat3() = default;
	Mat3(float m00, float m10, float m20, float m01, float m11, float m21, float m02, float m12, float m22);
	explicit Mat3(const Mat4 &m);
	explicit Mat3(const float *v);
	Mat3(const Vec3 &x, const Vec3 &y, const Vec3 &z);

	Mat3 &operator+=(const Mat3 &b) {
		for (int j = 0; j < 3; ++j)
			for (int i = 0; i < 3; ++i)
				m[i][j] += b.m[i][j];
		return *this;
	}

	Mat3 &operator-=(const Mat3 &b) {
		for (int j = 0; j < 3; ++j)
			for (int i = 0; i < 3; ++i)
				m[i][j] -= b.m[i][j];
		return *this;
	}

	Mat3 &operator*=(const float k) {
		for (int j = 0; j < 3; ++j)
			for (int i = 0; i < 3; ++i)
				m[i][j] *= k;
		return *this;
	}

	Mat3 &operator*=(const Mat3 &b) {
		for (int j = 0; j < 3; ++j)
			for (int i = 0; i < 3; ++i)
				m[i][j] *= b.m[i][j];
		return *this;
	}

	float m[3][3];
};

//
bool operator==(const Mat3 &a, const Mat3 &b);
bool operator!=(const Mat3 &a, const Mat3 &b);

Mat3 operator+(const Mat3 &a, const Mat3 &b);
Mat3 operator-(const Mat3 &a, const Mat3 &b);

Mat3 operator*(const Mat3 &a, const float v);

tVec2<float> operator*(const Mat3 &m, const tVec2<float> &v);
Vec3 operator*(const Mat3 &m, const Vec3 &v);
Vec4 operator*(const Mat3 &m, const Vec4 &v);
Mat3 operator*(const Mat3 &m, const Mat3 &b);

/// Transform an array of vector objects.
void TransformVec2(const Mat3 &__restrict m, tVec2<float> *__restrict out, const tVec2<float> *__restrict in, int count = 1);
void TransformVec3(const Mat3 &__restrict m, Vec3 *__restrict out, const Vec3 *__restrict in, int count = 1);
void TransformVec4(const Mat3 &__restrict m, Vec4 *__restrict out, const Vec4 *__restrict in, int count = 1);

/// Compute the determinant of the matrix.
float Det(const Mat3 &m);
/// Compute inverse matrix.
bool Inverse(const Mat3 &m, Mat3 &i);
/// Return the transposed matrix.
Mat3 Transpose(const Mat3 &m);

/// Rotation matrix around X axis.
Mat3 RotationMatX(float angle);
/// Rotation matrix around Y axis.
Mat3 RotationMatY(float angle);
/// Rotation matrix around Z axis.
Mat3 RotationMatZ(float angle);

Mat3 RotationMatXZY(float x, float y, float z);
Mat3 RotationMatZYX(float x, float y, float z);
Mat3 RotationMatXYZ(float x, float y, float z);
Mat3 RotationMatZXY(float x, float y, float z);
Mat3 RotationMatYZX(float x, float y, float z);
Mat3 RotationMatYXZ(float x, float y, float z);
Mat3 RotationMatXY(float x, float y);

/// Vector matrix.
Mat3 VectorMat3(const Vec3 &v);
/// Translation matrix.
Mat3 TranslationMat3(const tVec2<float> &t);
Mat3 TranslationMat3(const Vec3 &t);
/// Scale matrix.
Mat3 ScaleMat3(const tVec2<float> &s);
Mat3 ScaleMat3(const Vec3 &s);
/// Creates a matrix __M__ so that __Mv = p⨯v__.
/// Simply put, multiplying this matrix to any vector __v__ is equivalent to compute the cross product between __p__ and __v__.
Mat3 CrossProductMat3(const Vec3 &v);

/// Rotation matrix in 2D around a pivot point.
Mat3 RotationMat2D(float angle, const tVec2<float> &pivot);

/// From Euler triplet.
Mat3 RotationMat3(float x = 0, float y = 0, float z = 0, RotationOrder order = RO_Default);
/// From Euler vector.
Mat3 RotationMat3(const Vec3 &euler, RotationOrder order = RO_Default);

/**
	@short Look at.
	Create a rotation matrix pointing in the same direction as a vector.
	The up vector in world space of the final matrix can be specified.
*/
Mat3 Mat3LookAt(const Vec3 &front);
Mat3 Mat3LookAt(const Vec3 &front, const Vec3 &up);

/// Return the nth row.
Vec3 GetRow(const Mat3 &m, int n);
/// Return the nth column.
Vec3 GetColumn(const Mat3 &m, int n);

/// Set the nth row.
void SetRow(Mat3 &m, int n, const Vec3 &row);
/// Set the nth column.
void SetColumn(Mat3 &m, int n, const Vec3 &col);

Vec3 GetX(const Mat3 &m);
Vec3 GetY(const Mat3 &m);
Vec3 GetZ(const Mat3 &m);
Vec3 GetTranslation(const Mat3 &m);
Vec3 GetScale(const Mat3 &m);

void SetX(Mat3 &m, const Vec3 &X);
void SetY(Mat3 &m, const Vec3 &Y);
void SetZ(Mat3 &m, const Vec3 &Z);
void SetTranslation(Mat3 &m, const tVec2<float> &T);
void SetTranslation(Mat3 &m, const Vec3 &T);
void SetScale(Mat3 &m, const Vec3 &S);

void SetAxises(Mat3 &m, const Vec3 &x, const Vec3 &y, const Vec3 &z);

/// Orthonormalize matrix.
Mat3 Orthonormalize(const Mat3 &m);
/// Normalize matrix.
Mat3 Normalize(const Mat3 &m);
/// Return an Euler orientation equivalent to this matrix.
Vec3 ToEuler(const Mat3 &m, RotationOrder order = RO_Default);

} // namespace hg
