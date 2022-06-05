// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/matrix44.h"
#include "foundation/assert.h"
#include "foundation/matrix4.h"
#include "foundation/vector3.h"
#include "foundation/vector4.h"

namespace hg {

const Mat44 Mat44::Zero(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
const Mat44 Mat44::Identity(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

//
bool operator==(const Mat44 &a, const Mat44 &b) {
	for (size_t j = 0; j < 4; ++j)
		for (size_t i = 0; i < 4; ++i)
			if (a.m[j][i] != b.m[j][i])
				return false;
	return true;
}

bool operator!=(const Mat44 &a, const Mat44 &b) {
	for (size_t j = 0; j < 4; ++j)
		for (size_t i = 0; i < 4; ++i)
			if (a.m[j][i] != b.m[j][i])
				return true;
	return false;
}

Mat44 operator*(const Mat44 &a, const Mat44 &b) {
#define __M44M44(__I, __J) a.m[__I][0] * b.m[0][__J] + a.m[__I][1] * b.m[1][__J] + a.m[__I][2] * b.m[2][__J] + a.m[__I][3] * b.m[3][__J]
	return Mat44(__M44M44(0, 0), __M44M44(1, 0), __M44M44(2, 0), __M44M44(3, 0), __M44M44(0, 1), __M44M44(1, 1), __M44M44(2, 1), __M44M44(3, 1), __M44M44(0, 2),
		__M44M44(1, 2), __M44M44(2, 2), __M44M44(3, 2), __M44M44(0, 3), __M44M44(1, 3), __M44M44(2, 3), __M44M44(3, 3));
}

Mat44 operator*(const Mat44 &a, const Mat4 &b) {
	return {a.m[0][0] * b.m[0][0] + a.m[0][1] * b.m[1][0] + a.m[0][2] * b.m[2][0], a.m[1][0] * b.m[0][0] + a.m[1][1] * b.m[1][0] + a.m[1][2] * b.m[2][0],
		a.m[2][0] * b.m[0][0] + a.m[2][1] * b.m[1][0] + a.m[2][2] * b.m[2][0], a.m[3][0] * b.m[0][0] + a.m[3][1] * b.m[1][0] + a.m[3][2] * b.m[2][0],

		a.m[0][0] * b.m[0][1] + a.m[0][1] * b.m[1][1] + a.m[0][2] * b.m[2][1], a.m[1][0] * b.m[0][1] + a.m[1][1] * b.m[1][1] + a.m[1][2] * b.m[2][1],
		a.m[2][0] * b.m[0][1] + a.m[2][1] * b.m[1][1] + a.m[2][2] * b.m[2][1], a.m[3][0] * b.m[0][1] + a.m[3][1] * b.m[1][1] + a.m[3][2] * b.m[2][1],

		a.m[0][0] * b.m[0][2] + a.m[0][1] * b.m[1][2] + a.m[0][2] * b.m[2][2], a.m[1][0] * b.m[0][2] + a.m[1][1] * b.m[1][2] + a.m[1][2] * b.m[2][2],
		a.m[2][0] * b.m[0][2] + a.m[2][1] * b.m[1][2] + a.m[2][2] * b.m[2][2], a.m[3][0] * b.m[0][2] + a.m[3][1] * b.m[1][2] + a.m[3][2] * b.m[2][2],

		a.m[0][0] * b.m[0][3] + a.m[0][1] * b.m[1][3] + a.m[0][2] * b.m[2][3] + a.m[0][3],
		a.m[1][0] * b.m[0][3] + a.m[1][1] * b.m[1][3] + a.m[1][2] * b.m[2][3] + a.m[1][3],
		a.m[2][0] * b.m[0][3] + a.m[2][1] * b.m[1][3] + a.m[2][2] * b.m[2][3] + a.m[2][3],
		a.m[3][0] * b.m[0][3] + a.m[3][1] * b.m[1][3] + a.m[3][2] * b.m[2][3] + a.m[3][3]};
}

Mat44 operator*(const Mat4 &a, const Mat44 &b) {
	return {a.m[0][0] * b.m[0][0] + a.m[0][1] * b.m[1][0] + a.m[0][2] * b.m[2][0] + a.m[0][3] * b.m[3][0],
		a.m[1][0] * b.m[0][0] + a.m[1][1] * b.m[1][0] + a.m[1][2] * b.m[2][0] + a.m[1][3] * b.m[3][0],
		a.m[2][0] * b.m[0][0] + a.m[2][1] * b.m[1][0] + a.m[2][2] * b.m[2][0] + a.m[2][3] * b.m[3][0], b.m[3][0],

		a.m[0][0] * b.m[0][1] + a.m[0][1] * b.m[1][1] + a.m[0][2] * b.m[2][1] + a.m[0][3] * b.m[3][1],
		a.m[1][0] * b.m[0][1] + a.m[1][1] * b.m[1][1] + a.m[1][2] * b.m[2][1] + a.m[1][3] * b.m[3][1],
		a.m[2][0] * b.m[0][1] + a.m[2][1] * b.m[1][1] + a.m[2][2] * b.m[2][1] + a.m[2][3] * b.m[3][1], b.m[3][1],

		a.m[0][0] * b.m[0][2] + a.m[0][1] * b.m[1][2] + a.m[0][2] * b.m[2][2] + a.m[0][3] * b.m[3][2],
		a.m[1][0] * b.m[0][2] + a.m[1][1] * b.m[1][2] + a.m[1][2] * b.m[2][2] + a.m[1][3] * b.m[3][2],
		a.m[2][0] * b.m[0][2] + a.m[2][1] * b.m[1][2] + a.m[2][2] * b.m[2][2] + a.m[2][3] * b.m[3][2], b.m[3][2],

		a.m[0][0] * b.m[0][3] + a.m[0][1] * b.m[1][3] + a.m[0][2] * b.m[2][3] + a.m[0][3] * b.m[3][3],
		a.m[1][0] * b.m[0][3] + a.m[1][1] * b.m[1][3] + a.m[1][2] * b.m[2][3] + a.m[1][3] * b.m[3][3],
		a.m[2][0] * b.m[0][3] + a.m[2][1] * b.m[1][3] + a.m[2][2] * b.m[2][3] + a.m[2][3] * b.m[3][3], b.m[3][3]};
}

Vec3 operator*(const Mat44 &m, const Vec3 &v) {
	return {v.x * m.m[0][0] + v.y * m.m[0][1] + v.z * m.m[0][2] + m.m[0][3], v.x * m.m[1][0] + v.y * m.m[1][1] + v.z * m.m[1][2] + m.m[1][3],
		v.x * m.m[2][0] + v.y * m.m[2][1] + v.z * m.m[2][2] + m.m[2][3]};
}

Vec4 operator*(const Mat44 &m, const Vec4 &v) {
	return {v.x * m.m[0][0] + v.y * m.m[0][1] + v.z * m.m[0][2] + v.w * m.m[0][3], v.x * m.m[1][0] + v.y * m.m[1][1] + v.z * m.m[1][2] + v.w * m.m[1][3],
		v.x * m.m[2][0] + v.y * m.m[2][1] + v.z * m.m[2][2] + v.w * m.m[2][3], v.x * m.m[3][0] + v.y * m.m[3][1] + v.z * m.m[3][2] + v.w * m.m[3][3]};
}

void TransformVec3(const Mat44 &__restrict m, Vec4 *__restrict out, const Vec3 *__restrict in, unsigned int count) {
	for (unsigned int i = 0; i < count; ++i) {
		const float x = in[i].x, y = in[i].y, z = in[i].z;
		out[i].x = x * m.m[0][0] + y * m.m[0][1] + z * m.m[0][2] + m.m[0][3];
		out[i].y = x * m.m[1][0] + y * m.m[1][1] + z * m.m[1][2] + m.m[1][3];
		out[i].z = x * m.m[2][0] + y * m.m[2][1] + z * m.m[2][2] + m.m[2][3];
		out[i].w = x * m.m[3][0] + y * m.m[3][1] + z * m.m[3][2] + m.m[3][3];
	}
}

void TransformVec4(const Mat44 &__restrict m, Vec4 *__restrict out, const Vec4 *__restrict in, unsigned int count) {
	for (unsigned int i = 0; i < count; ++i) {
		const float x = in[i].x, y = in[i].y, z = in[i].z, w = in[i].w;
		out[i].x = x * m.m[0][0] + y * m.m[0][1] + z * m.m[0][2] + w * m.m[0][3];
		out[i].y = x * m.m[1][0] + y * m.m[1][1] + z * m.m[1][2] + w * m.m[1][3];
		out[i].z = x * m.m[2][0] + y * m.m[2][1] + z * m.m[2][2] + w * m.m[2][3];
		out[i].w = x * m.m[3][0] + y * m.m[3][1] + z * m.m[3][2] + w * m.m[3][3];
	}
}

Mat44 Transpose(const Mat44 &m) {
	return {m.m[0][0], m.m[0][1], m.m[0][2], m.m[0][3], m.m[1][0], m.m[1][1], m.m[1][2], m.m[1][3], m.m[2][0], m.m[2][1], m.m[2][2], m.m[2][3], m.m[3][0],
		m.m[3][1], m.m[3][2], m.m[3][3]};
}

Mat44 Inverse(const Mat44 &m, bool &result) {
	float inv[16], det;

	inv[0] = m.m[1][1] * m.m[2][2] * m.m[3][3] - m.m[1][1] * m.m[2][3] * m.m[3][2] - m.m[2][1] * m.m[1][2] * m.m[3][3] + m.m[2][1] * m.m[1][3] * m.m[3][2] +
			 m.m[3][1] * m.m[1][2] * m.m[2][3] - m.m[3][1] * m.m[1][3] * m.m[2][2];
	inv[4] = -m.m[1][0] * m.m[2][2] * m.m[3][3] + m.m[1][0] * m.m[2][3] * m.m[3][2] + m.m[2][0] * m.m[1][2] * m.m[3][3] - m.m[2][0] * m.m[1][3] * m.m[3][2] -
			 m.m[3][0] * m.m[1][2] * m.m[2][3] + m.m[3][0] * m.m[1][3] * m.m[2][2];
	inv[8] = m.m[1][0] * m.m[2][1] * m.m[3][3] - m.m[1][0] * m.m[2][3] * m.m[3][1] - m.m[2][0] * m.m[1][1] * m.m[3][3] + m.m[2][0] * m.m[1][3] * m.m[3][1] +
			 m.m[3][0] * m.m[1][1] * m.m[2][3] - m.m[3][0] * m.m[1][3] * m.m[2][1];
	inv[12] = -m.m[1][0] * m.m[2][1] * m.m[3][2] + m.m[1][0] * m.m[2][2] * m.m[3][1] + m.m[2][0] * m.m[1][1] * m.m[3][2] - m.m[2][0] * m.m[1][2] * m.m[3][1] -
			  m.m[3][0] * m.m[1][1] * m.m[2][2] + m.m[3][0] * m.m[1][2] * m.m[2][1];
	inv[1] = -m.m[0][1] * m.m[2][2] * m.m[3][3] + m.m[0][1] * m.m[2][3] * m.m[3][2] + m.m[2][1] * m.m[0][2] * m.m[3][3] - m.m[2][1] * m.m[0][3] * m.m[3][2] -
			 m.m[3][1] * m.m[0][2] * m.m[2][3] + m.m[3][1] * m.m[0][3] * m.m[2][2];
	inv[5] = m.m[0][0] * m.m[2][2] * m.m[3][3] - m.m[0][0] * m.m[2][3] * m.m[3][2] - m.m[2][0] * m.m[0][2] * m.m[3][3] + m.m[2][0] * m.m[0][3] * m.m[3][2] +
			 m.m[3][0] * m.m[0][2] * m.m[2][3] - m.m[3][0] * m.m[0][3] * m.m[2][2];
	inv[9] = -m.m[0][0] * m.m[2][1] * m.m[3][3] + m.m[0][0] * m.m[2][3] * m.m[3][1] + m.m[2][0] * m.m[0][1] * m.m[3][3] - m.m[2][0] * m.m[0][3] * m.m[3][1] -
			 m.m[3][0] * m.m[0][1] * m.m[2][3] + m.m[3][0] * m.m[0][3] * m.m[2][1];
	inv[13] = m.m[0][0] * m.m[2][1] * m.m[3][2] - m.m[0][0] * m.m[2][2] * m.m[3][1] - m.m[2][0] * m.m[0][1] * m.m[3][2] + m.m[2][0] * m.m[0][2] * m.m[3][1] +
			  m.m[3][0] * m.m[0][1] * m.m[2][2] - m.m[3][0] * m.m[0][2] * m.m[2][1];
	inv[2] = m.m[0][1] * m.m[1][2] * m.m[3][3] - m.m[0][1] * m.m[1][3] * m.m[3][2] - m.m[1][1] * m.m[0][2] * m.m[3][3] + m.m[1][1] * m.m[0][3] * m.m[3][2] +
			 m.m[3][1] * m.m[0][2] * m.m[1][3] - m.m[3][1] * m.m[0][3] * m.m[1][2];
	inv[6] = -m.m[0][0] * m.m[1][2] * m.m[3][3] + m.m[0][0] * m.m[1][3] * m.m[3][2] + m.m[1][0] * m.m[0][2] * m.m[3][3] - m.m[1][0] * m.m[0][3] * m.m[3][2] -
			 m.m[3][0] * m.m[0][2] * m.m[1][3] + m.m[3][0] * m.m[0][3] * m.m[1][2];
	inv[10] = m.m[0][0] * m.m[1][1] * m.m[3][3] - m.m[0][0] * m.m[1][3] * m.m[3][1] - m.m[1][0] * m.m[0][1] * m.m[3][3] + m.m[1][0] * m.m[0][3] * m.m[3][1] +
			  m.m[3][0] * m.m[0][1] * m.m[1][3] - m.m[3][0] * m.m[0][3] * m.m[1][1];
	inv[14] = -m.m[0][0] * m.m[1][1] * m.m[3][2] + m.m[0][0] * m.m[1][2] * m.m[3][1] + m.m[1][0] * m.m[0][1] * m.m[3][2] - m.m[1][0] * m.m[0][2] * m.m[3][1] -
			  m.m[3][0] * m.m[0][1] * m.m[1][2] + m.m[3][0] * m.m[0][2] * m.m[1][1];
	inv[3] = -m.m[0][1] * m.m[1][2] * m.m[2][3] + m.m[0][1] * m.m[1][3] * m.m[2][2] + m.m[1][1] * m.m[0][2] * m.m[2][3] - m.m[1][1] * m.m[0][3] * m.m[2][2] -
			 m.m[2][1] * m.m[0][2] * m.m[1][3] + m.m[2][1] * m.m[0][3] * m.m[1][2];
	inv[7] = m.m[0][0] * m.m[1][2] * m.m[2][3] - m.m[0][0] * m.m[1][3] * m.m[2][2] - m.m[1][0] * m.m[0][2] * m.m[2][3] + m.m[1][0] * m.m[0][3] * m.m[2][2] +
			 m.m[2][0] * m.m[0][2] * m.m[1][3] - m.m[2][0] * m.m[0][3] * m.m[1][2];
	inv[11] = -m.m[0][0] * m.m[1][1] * m.m[2][3] + m.m[0][0] * m.m[1][3] * m.m[2][1] + m.m[1][0] * m.m[0][1] * m.m[2][3] - m.m[1][0] * m.m[0][3] * m.m[2][1] -
			  m.m[2][0] * m.m[0][1] * m.m[1][3] + m.m[2][0] * m.m[0][3] * m.m[1][1];
	inv[15] = m.m[0][0] * m.m[1][1] * m.m[2][2] - m.m[0][0] * m.m[1][2] * m.m[2][1] - m.m[1][0] * m.m[0][1] * m.m[2][2] + m.m[1][0] * m.m[0][2] * m.m[2][1] +
			  m.m[2][0] * m.m[0][1] * m.m[1][2] - m.m[2][0] * m.m[0][2] * m.m[1][1];
	det = m.m[0][0] * inv[0] + m.m[0][1] * inv[4] + m.m[0][2] * inv[8] + m.m[0][3] * inv[12];

	Mat44 out;

	if (det == 0.f) {
		out = Mat44::Identity;

		result = false;
	} else {
		det = 1.f / det;

		for (int i = 0; i < 16; i++)
			reinterpret_cast<float *>(out.m)[i] = inv[i] * det;

		result = true;
	}

	return out;
}

Mat44 Inverse(const Mat44 &m) {
	bool result;
	auto out = Inverse(m, result);
	__ASSERT__(result);
	return out;
}

Vec4 GetRow(const Mat44 &m, unsigned int n) { return {m.m[n][0], m.m[n][1], m.m[n][2], m.m[n][3]}; }
Vec4 GetColumn(const Mat44 &m, unsigned int n) { return {m.m[0][n], m.m[1][n], m.m[2][n], m.m[3][n]}; }

void SetRow(Mat44 &m, unsigned int n, const Vec4 &v) {
	m.m[n][0] = v.x;
	m.m[n][1] = v.y;
	m.m[n][2] = v.z;
	m.m[n][3] = v.w;
}

void SetColumn(Mat44 &m, unsigned int n, const Vec4 &v) {
	m.m[0][n] = v.x;
	m.m[1][n] = v.y;
	m.m[2][n] = v.z;
	m.m[3][n] = v.w;
}

//
Mat44::Mat44(float m00, float m10, float m20, float m30, float m01, float m11, float m21, float m31, float m02, float m12, float m22, float m32, float m03,
	float m13, float m23, float m33) {
	m[0][0] = m00;
	m[1][0] = m10;
	m[2][0] = m20;
	m[3][0] = m30;
	m[0][1] = m01;
	m[1][1] = m11;
	m[2][1] = m21;
	m[3][1] = m31;
	m[0][2] = m02;
	m[1][2] = m12;
	m[2][2] = m22;
	m[3][2] = m32;
	m[0][3] = m03;
	m[1][3] = m13;
	m[2][3] = m23;
	m[3][3] = m33;
}

Mat44::Mat44(const Mat4 &m_) {
	m[0][0] = m_.m[0][0];
	m[0][1] = m_.m[0][1];
	m[0][2] = m_.m[0][2];
	m[0][3] = m_.m[0][3];

	m[1][0] = m_.m[1][0];
	m[1][1] = m_.m[1][1];
	m[1][2] = m_.m[1][2];
	m[1][3] = m_.m[1][3];

	m[2][0] = m_.m[2][0];
	m[2][1] = m_.m[2][1];
	m[2][2] = m_.m[2][2];
	m[2][3] = m_.m[2][3];

	m[3][0] = 0.f;
	m[3][1] = 0.f;
	m[3][2] = 0.f;
	m[3][3] = 1.f;
}

} // namespace hg
