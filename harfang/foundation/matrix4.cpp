// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/matrix4.h"
#include "foundation/matrix3.h"
#include "foundation/quaternion.h"
#include "foundation/vector3.h"
#include "foundation/vector4.h"

namespace hg {

const Mat4 Mat4::Zero(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
const Mat4 Mat4::Identity(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0);

//
bool operator==(const Mat4 &a, const Mat4 &b) {
	for (size_t j = 0; j < 3; ++j)
		for (size_t i = 0; i < 4; ++i)
			if (a.m[j][i] != b.m[j][i])
				return false;
	return true;
}

bool operator!=(const Mat4 &a, const Mat4 &b) {
	for (size_t j = 0; j < 3; ++j)
		for (size_t i = 0; i < 4; ++i)
			if (a.m[j][i] != b.m[j][i])
				return true;
	return false;
}

Mat4 operator*(const Mat4 &a, const Mat4 &b) {
	return {a.m[0][0] * b.m[0][0] + a.m[0][1] * b.m[1][0] + a.m[0][2] * b.m[2][0], a.m[1][0] * b.m[0][0] + a.m[1][1] * b.m[1][0] + a.m[1][2] * b.m[2][0],
		a.m[2][0] * b.m[0][0] + a.m[2][1] * b.m[1][0] + a.m[2][2] * b.m[2][0],

		a.m[0][0] * b.m[0][1] + a.m[0][1] * b.m[1][1] + a.m[0][2] * b.m[2][1], a.m[1][0] * b.m[0][1] + a.m[1][1] * b.m[1][1] + a.m[1][2] * b.m[2][1],
		a.m[2][0] * b.m[0][1] + a.m[2][1] * b.m[1][1] + a.m[2][2] * b.m[2][1],

		a.m[0][0] * b.m[0][2] + a.m[0][1] * b.m[1][2] + a.m[0][2] * b.m[2][2], a.m[1][0] * b.m[0][2] + a.m[1][1] * b.m[1][2] + a.m[1][2] * b.m[2][2],
		a.m[2][0] * b.m[0][2] + a.m[2][1] * b.m[1][2] + a.m[2][2] * b.m[2][2],

		a.m[0][0] * b.m[0][3] + a.m[0][1] * b.m[1][3] + a.m[0][2] * b.m[2][3] + a.m[0][3],
		a.m[1][0] * b.m[0][3] + a.m[1][1] * b.m[1][3] + a.m[1][2] * b.m[2][3] + a.m[1][3],
		a.m[2][0] * b.m[0][3] + a.m[2][1] * b.m[1][3] + a.m[2][2] * b.m[2][3] + a.m[2][3]};
}

Mat4 operator*(const Mat4 &m, float v) {
	Mat4 o;
	for (auto j = 0; j < 3; ++j)
		for (auto i = 0; i < 4; ++i)
			o.m[j][i] = m.m[j][i] * v;
	return o;
}

Mat4 operator/(const Mat4 &m, float v) {
	Mat4 o;
	for (auto j = 0; j < 3; ++j)
		for (auto i = 0; i < 4; ++i)
			o.m[j][i] = m.m[j][i] / v;
	return o;
}

Mat4 operator+(const Mat4 &a, const Mat4 &b) {
	Mat4 o;
	for (auto j = 0; j < 3; ++j)
		for (auto i = 0; i < 4; ++i)
			o.m[j][i] = a.m[j][i] + b.m[j][i];
	return o;
}

Mat4 operator-(const Mat4 &a, const Mat4 &b) {
	Mat4 o;
	for (auto j = 0; j < 3; ++j)
		for (auto i = 0; i < 4; ++i)
			o.m[j][i] = a.m[j][i] - b.m[j][i];
	return o;
}

Vec3 operator*(const Mat4 &m, const Vec3 &v) {
	return {v.x * m.m[0][0] + v.y * m.m[0][1] + v.z * m.m[0][2] + m.m[0][3], v.x * m.m[1][0] + v.y * m.m[1][1] + v.z * m.m[1][2] + m.m[1][3],
		v.x * m.m[2][0] + v.y * m.m[2][1] + v.z * m.m[2][2] + m.m[2][3]};
}

Vec4 operator*(const Mat4 &m, const Vec4 &v) {
	return {v.x * m.m[0][0] + v.y * m.m[0][1] + v.z * m.m[0][2] + v.w * m.m[0][3], v.x * m.m[1][0] + v.y * m.m[1][1] + v.z * m.m[1][2] + v.w * m.m[1][3],
		v.x * m.m[2][0] + v.y * m.m[2][1] + v.z * m.m[2][2] + v.w * m.m[2][3], v.w};
}

//
void TransformVec3(const Mat4 &__restrict m, Vec3 *__restrict out, const Vec3 *__restrict in, unsigned int count) {
	for (unsigned int i = 0; i < count; ++i) {
		const float x = in[i].x, y = in[i].y, z = in[i].z;
		out[i].x = x * m.m[0][0] + y * m.m[0][1] + z * m.m[0][2] + m.m[0][3];
		out[i].y = x * m.m[1][0] + y * m.m[1][1] + z * m.m[1][2] + m.m[1][3];
		out[i].z = x * m.m[2][0] + y * m.m[2][1] + z * m.m[2][2] + m.m[2][3];
	}
}

void TransformVec3(const Mat4 &__restrict m, Vec4 *__restrict out, const Vec3 *__restrict in, unsigned int count) {
	for (unsigned int i = 0; i < count; ++i) {
		const float x = in[i].x, y = in[i].y, z = in[i].z;
		out[i].x = x * m.m[0][0] + y * m.m[0][1] + z * m.m[0][2] + m.m[0][3];
		out[i].y = x * m.m[1][0] + y * m.m[1][1] + z * m.m[1][2] + m.m[1][3];
		out[i].z = x * m.m[2][0] + y * m.m[2][1] + z * m.m[2][2] + m.m[2][3];
		out[i].w = 1.f;
	}
}

void RotateVec3(const Mat4 &__restrict m, Vec3 *__restrict out, const Vec3 *__restrict in, unsigned int count) {
	for (unsigned int i = 0; i < count; ++i) {
		const float x = in[i].x, y = in[i].y, z = in[i].z;
		out[i].x = x * m.m[0][0] + y * m.m[0][1] + z * m.m[0][2];
		out[i].y = x * m.m[1][0] + y * m.m[1][1] + z * m.m[1][2];
		out[i].z = x * m.m[2][0] + y * m.m[2][1] + z * m.m[2][2];
	}
}

//
Vec4 GetRow(const Mat4 &m, unsigned int n) { return {m.m[n][0], m.m[n][1], m.m[n][2], m.m[n][3]}; }
Vec3 GetColumn(const Mat4 &m, unsigned int n) { return {m.m[0][n], m.m[1][n], m.m[2][n]}; }

void SetRow(Mat4 &m, unsigned int n, const Vec4 &v) {
	m.m[n][0] = v.x;
	m.m[n][1] = v.y;
	m.m[n][2] = v.z;
	m.m[n][3] = v.w;
}

void SetColumn(Mat4 &m, unsigned int n, const Vec3 &v) {
	m.m[0][n] = v.x;
	m.m[1][n] = v.y;
	m.m[2][n] = v.z;
}

Vec3 GetX(const Mat4 &m) { return GetColumn(m, 0); }
Vec3 GetY(const Mat4 &m) { return GetColumn(m, 1); }
Vec3 GetZ(const Mat4 &m) { return GetColumn(m, 2); }

Vec3 GetT(const Mat4 &m) { return GetColumn(m, 3); }
Vec3 GetTranslation(const Mat4 &m) { return GetT(m); }

Vec3 GetR(const Mat4 &m, RotationOrder order) {
	Vec3 rotation;
	Decompose(m, nullptr, &rotation, nullptr, order);
	return rotation;
}

Vec3 GetRotation(const Mat4 &m, RotationOrder order) { return GetR(m, order); }

Vec3 GetS(const Mat4 &m) { return {Len(GetX(m)), Len(GetY(m)), Len(GetZ(m))}; }
Vec3 GetScale(const Mat4 &m) { return GetS(m); }

Mat3 GetRMatrix(const Mat4 &m) {
	Mat3 rotation;
	Decompose(m, nullptr, &rotation, nullptr);
	return rotation;
}

Mat3 GetRotationMatrix(const Mat4 &m) { return GetRMatrix(m); }

void SetX(Mat4 &m, const Vec3 &v) { SetColumn(m, 0, v); }
void SetY(Mat4 &m, const Vec3 &v) { SetColumn(m, 1, v); }
void SetZ(Mat4 &m, const Vec3 &v) { SetColumn(m, 2, v); }

void SetT(Mat4 &m, const Vec3 &v) { SetTranslation(m, v); }
void SetTranslation(Mat4 &m, const Vec3 &v) { SetColumn(m, 3, v); }

void SetS(Mat4 &m, const Vec3 &v) {
	SetX(m, Normalize(GetX(m)) * v.x);
	SetY(m, Normalize(GetY(m)) * v.y);
	SetZ(m, Normalize(GetZ(m)) * v.z);
}

void SetScale(Mat4 &m, const Vec3 &v) { SetS(m, v); }

//
Mat4 TransformationMat4(const Vec3 &p, const Mat3 &r) { return TransformationMat4(p, r, Vec3::One); }
Mat4 TransformationMat4(const Vec3 &p, const Mat3 &r, const Vec3 &s) {
	return {r.m[0][0] * s.x, r.m[1][0] * s.x, r.m[2][0] * s.x, r.m[0][1] * s.y, r.m[1][1] * s.y, r.m[2][1] * s.y, r.m[0][2] * s.z, r.m[1][2] * s.z,
		r.m[2][2] * s.z, p.x, p.y, p.z};
}

Mat4 TransformationMat4(const Vec3 &p, const Vec3 &e) { return TransformationMat4(p, e, Vec3::One); }
Mat4 TransformationMat4(const Vec3 &p, const Vec3 &e, const Vec3 &s) { return TransformationMat4(p, RotationMat3(e), s); }

//
Mat4 LerpAsOrthonormalBase(const Mat4 &a, const Mat4 &b, float k, bool fast) {
	if (fast) {
		Mat4 o;
		for (auto c = 0; c < 3; ++c)
			for (auto r = 0; r < 4; ++r)
				o.m[c][r] = (b.m[c][r] - a.m[c][r]) * k + a.m[c][r];
		return o;
	}

	Mat3 a_matrix3, b_matrix3;
	Vec3 a_position, b_position, a_scale, b_scale;

	Decompose(a, &a_position, &a_matrix3, &a_scale);
	Decompose(b, &b_position, &b_matrix3, &b_scale);

	Quaternion a_orientation = QuaternionFromMatrix3(a_matrix3);
	Quaternion b_orientation = QuaternionFromMatrix3(b_matrix3);

	return TranslationMat4((b_position - a_position) * k + a_position) * Mat4(ToMatrix3(Slerp(a_orientation, b_orientation, k))) *
		   ScaleMat4((b_scale - a_scale) * k + a_scale);
}

void Decompose(const Mat4 &m, Vec3 *position, Vec3 *rotation, Vec3 *scale, RotationOrder order) {
	Mat3 m3;
	Decompose(m, position, &m3, scale);
	if (rotation)
		*rotation = ToEuler(m3, order);
}

void Decompose(const Mat4 &m, Vec3 *position, Mat3 *rotation, Vec3 *scale) {
	// extract position
	if (position)
		*position = GetT(m);

	const Vec3 &vx = GetX(m);
	const Vec3 &vy = GetY(m);
	const Vec3 &vz = GetZ(m);

	// extract scale
	Vec3 scl(Len(vx), Len(vy), Len(vz));

	// handle negative scale (permute X to preserve left-handedness)
	auto left = Cross(vy, vz);
	if (Dot(left, vx) < 0.f)
		scl.x = -scl.x;

	if (scale)
		*scale = scl;

	// rotation 3x3 (renormalized)
	if (rotation) {
		if (scl.x)
			SetX(*rotation, vx/scl.x);
		else
			SetX(*rotation, {1.f, 0.f, 0.f});

		if (scl.y)
			SetY(*rotation, vy/scl.y);
		else
			SetY(*rotation, {0.f, 1.f, 0.f});

		if (scl.z)
			SetZ(*rotation, vz/scl.z);
		else
			SetZ(*rotation, {0.f, 0.f, 1.f});
	}
}

//
bool Inverse(const Mat4 &m, Mat4 &I) {
	float inv[12], det;

	inv[0] = m.m[1][1] * m.m[2][2] - m.m[1][1] - m.m[2][1] * m.m[1][2];
	inv[4] = -m.m[1][0] * m.m[2][2] + m.m[2][0] * m.m[1][2];
	inv[8] = m.m[1][0] * m.m[2][1] - m.m[2][0] * m.m[1][1];
	inv[1] = -m.m[0][1] * m.m[2][2] + m.m[2][1] * m.m[0][2];
	inv[5] = m.m[0][0] * m.m[2][2] - m.m[2][0] * m.m[0][2];
	inv[9] = -m.m[0][0] * m.m[2][1] + m.m[2][0] * m.m[0][1];
	inv[2] = m.m[0][1] * m.m[1][2] - m.m[1][1] * m.m[0][2];
	inv[6] = -m.m[0][0] * m.m[1][2] + m.m[1][0] * m.m[0][2];
	inv[10] = m.m[0][0] * m.m[1][1] - m.m[1][0] * m.m[0][1];
	inv[3] = -m.m[0][1] * m.m[1][2] * m.m[2][3] + m.m[0][1] * m.m[1][3] * m.m[2][2] + m.m[1][1] * m.m[0][2] * m.m[2][3] - m.m[1][1] * m.m[0][3] * m.m[2][2] -
			 m.m[2][1] * m.m[0][2] * m.m[1][3] + m.m[2][1] * m.m[0][3] * m.m[1][2];
	inv[7] = m.m[0][0] * m.m[1][2] * m.m[2][3] - m.m[0][0] * m.m[1][3] * m.m[2][2] - m.m[1][0] * m.m[0][2] * m.m[2][3] + m.m[1][0] * m.m[0][3] * m.m[2][2] +
			 m.m[2][0] * m.m[0][2] * m.m[1][3] - m.m[2][0] * m.m[0][3] * m.m[1][2];
	inv[11] = -m.m[0][0] * m.m[1][1] * m.m[2][3] + m.m[0][0] * m.m[1][3] * m.m[2][1] + m.m[1][0] * m.m[0][1] * m.m[2][3] - m.m[1][0] * m.m[0][3] * m.m[2][1] -
			  m.m[2][0] * m.m[0][1] * m.m[1][3] + m.m[2][0] * m.m[0][3] * m.m[1][1];
	det = m.m[0][0] * inv[0] + m.m[0][1] * inv[4] + m.m[0][2] * inv[8];

	if (!det)
		return false;

	det = 1.f / det;

	for (auto i = 0; i < 12; i++)
		reinterpret_cast<float *>(I.m)[i] = inv[i] * det;

	return true;
}

Mat4 InverseFast(const Mat4 &m) {
	const float Sx = 1.f / Len2(GetX(m)), Sy = 1.f / Len2(GetY(m)), Sz = 1.f / Len2(GetZ(m));
	Mat4 o(m.m[0][0] * Sx, m.m[0][1] * Sy, m.m[0][2] * Sz, m.m[1][0] * Sx, m.m[1][1] * Sy, m.m[1][2] * Sz, m.m[2][0] * Sx, m.m[2][1] * Sy, m.m[2][2] * Sz, 0, 0,
		0);

	const float Tx = -m.m[0][3], Ty = -m.m[1][3], Tz = -m.m[2][3];
	o.m[0][3] = o.m[0][0] * Tx + o.m[0][1] * Ty + o.m[0][2] * Tz;
	o.m[1][3] = o.m[1][0] * Tx + o.m[1][1] * Ty + o.m[1][2] * Tz;
	o.m[2][3] = o.m[2][0] * Tx + o.m[2][1] * Ty + o.m[2][2] * Tz;
	return o;
}

//
Mat4 Orthonormalize(const Mat4 &m) {
	const auto T = GetT(m);
	return TransformationMat4(T, Orthonormalize(Mat3(m)));
}

Mat4 Normalize(const Mat4 &m) {
	const auto T = GetT(m);
	return TransformationMat4(T, Normalize(Mat3(m)));
}

//
Mat4 TranslationMat4(const Vec3 &t) { return {1, 0, 0, 0, 1, 0, 0, 0, 1, t.x, t.y, t.z}; }
Mat4 RotationMat4(const Vec3 &e, RotationOrder r) { return Mat4(RotationMat3(e, r)); }
Mat4 ScaleMat4(const Vec3 &s) { return {s.x, 0, 0, 0, s.y, 0, 0, 0, s.z, 0, 0, 0}; }
Mat4 ScaleMat4(float s) { return {s, 0, 0, 0, s, 0, 0, 0, s, 0, 0, 0}; }

//
Mat4 Mat4LookAt(const Vec3 &p, const Vec3 &at, const Vec3 &s) { return TransformationMat4(p, Mat3LookAt(at - p), s); }
Mat4 Mat4LookAtUp(const Vec3 &p, const Vec3 &at, const Vec3 &up, const Vec3 &s) { return TransformationMat4(p, Mat3LookAt(at - p, up), s); }

Mat4 Mat4LookToward(const Vec3 &p, const Vec3 &d, const Vec3 &s) { return TransformationMat4(p, Mat3LookAt(d), s); }
Mat4 Mat4LookTowardUp(const Vec3 &p, const Vec3 &d, const Vec3 &up, const Vec3 &s) { return TransformationMat4(p, Mat3LookAt(d, up), s); }

//
void Set(Mat4 &m, const float *_m) {
	m.m[0][0] = _m[0];
	m.m[1][0] = _m[1];
	m.m[2][0] = _m[2];
	m.m[0][1] = _m[3];
	m.m[1][1] = _m[4];
	m.m[2][1] = _m[5];
	m.m[0][2] = _m[6];
	m.m[1][2] = _m[7];
	m.m[2][2] = _m[8];
	m.m[0][3] = _m[9];
	m.m[1][3] = _m[10];
	m.m[2][3] = _m[11];
}

void Set(Mat4 &m, float m00, float m10, float m20, float m01, float m11, float m21, float m02, float m12, float m22, float m03, float m13, float m23) {
	m.m[0][0] = m00;
	m.m[1][0] = m10;
	m.m[2][0] = m20;
	m.m[0][1] = m01;
	m.m[1][1] = m11;
	m.m[2][1] = m21;
	m.m[0][2] = m02;
	m.m[1][2] = m12;
	m.m[2][2] = m22;
	m.m[0][3] = m03;
	m.m[1][3] = m13;
	m.m[2][3] = m23;
}

//
Mat4::Mat4(float m00, float m10, float m20, float m01, float m11, float m21, float m02, float m12, float m22, float m03, float m13, float m23) {
	Set(*this, m00, m10, m20, m01, m11, m21, m02, m12, m22, m03, m13, m23);
}

Mat4::Mat4(const Mat3 &o) {
	m[0][0] = o.m[0][0];
	m[1][0] = o.m[1][0];
	m[2][0] = o.m[2][0];
	m[0][1] = o.m[0][1];
	m[1][1] = o.m[1][1];
	m[2][1] = o.m[2][1];
	m[0][2] = o.m[0][2];
	m[1][2] = o.m[1][2];
	m[2][2] = o.m[2][2];
	m[0][3] = 0;
	m[1][3] = 0;
	m[2][3] = 0;
}

Mat4::Mat4(const float *v) { Set(*this, v); }

Mat4 Mat4FromFloat16Transposed(const float m[16]) { return {m[0], m[1], m[2], m[4], m[5], m[6], m[8], m[9], m[10], m[12], m[13], m[14]}; }

void Mat4ToFloat16Transposed(const Mat4 &m, float t[16]) {
	t[0] = m.m[0][0];
	t[1] = m.m[1][0];
	t[2] = m.m[2][0];
	t[3] = 0.f;
	t[4] = m.m[0][1];
	t[5] = m.m[1][1];
	t[6] = m.m[2][1];
	t[7] = 0.f;
	t[8] = m.m[0][2];
	t[9] = m.m[1][2];
	t[10] = m.m[2][2];
	t[11] = 0.f;
	t[12] = m.m[0][3];
	t[13] = m.m[1][3];
	t[14] = m.m[2][3];
	t[15] = 1.f;
}

//
Mat4 ComputeBillboardMat4(const Vec3 &pos, const Mat3 &camera, const Vec3 &scale) { return TransformationMat4(pos, camera, scale); }

} // namespace hg
