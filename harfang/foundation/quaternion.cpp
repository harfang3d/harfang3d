// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/quaternion.h"
#include "foundation/math.h"
#include "foundation/matrix3.h"
#include "foundation/vector3.h"

namespace hg {

const Quaternion Quaternion::Identity(0, 0, 0, 1);

Quaternion Slerp(const Quaternion &a, const Quaternion &b, float t) {
	float d = Dot(a, b);

	bool bFlip = false;

	if (d < 0.f) {
		d = -d;
		bFlip = true;
	}

	float inv_d;
	if (1.f - d < 0.000001f) {
		inv_d = 1.f - t;
	} else {
		float theta = ACos(d);
		float s = 1.f / Sin(theta);

		inv_d = Sin((1.f - t) * theta) * s;
		t = Sin(t * theta) * s;
	}

	if (bFlip)
		t = -t;

	return Quaternion(inv_d * a.x + t * b.x, inv_d * a.y + t * b.y, inv_d * a.z + t * b.z, inv_d * a.w + t * b.w);
}

//
float Dist(const Quaternion &a, const Quaternion &b) {
	const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z, dw = a.w - b.w;
	return Sqrt((dx * dx) + (dy * dy) + (dz * dz) + (dw * dw));
}

Quaternion Conjugate(const Quaternion &q) {
	return Quaternion(-q.x, -q.y, -q.z, q.w);
}

Quaternion Inverse(const Quaternion &q) {
	const float l = Len2(q);
	return l ? Quaternion(q.x / -l, q.y / -l, q.z / -l, q.w / l) : Quaternion::Identity;
}

float Len(const Quaternion &q) { return Sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w); }
float Len2(const Quaternion &q) { return q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w; }

Quaternion Normalize(const Quaternion &q) {
	float l = Len(q);
	return l ? Quaternion(q.x / l, q.y / l, q.z / l, q.w / l) : Quaternion::Identity;
}

//
Quaternion QuaternionLookAt(const Vec3 &at) { return QuaternionFromMatrix3(Mat3LookAt(at)); }

//
static size_t quaternion_inext[3] = {1, 2, 0};

Quaternion QuaternionFromMatrix3(const Mat3 &m) {
	// from "Quaternion Calculus and Fast Animation"
	float _x = 0, _y = 0, _z = 0, _w = 1;
	float trace = m.m[0][0] + m.m[1][1] + m.m[2][2];

	if (trace > 0.0) {
		// |w| > 1/2, may as well choose w > 1/2
		float root = Sqrt(trace + 1.0f); // 2w
		_w = 0.5f * root;
		root = 0.5f / root; // 1/(4w)
		_x = (m.m[2][1] - m.m[1][2]) * root;
		_y = (m.m[0][2] - m.m[2][0]) * root;
		_z = (m.m[1][0] - m.m[0][1]) * root;
	} else {
		// |w| <= 1/2
		size_t i = 0;
		if (m.m[1][1] > m.m[0][0])
			i = 1;
		if (m.m[2][2] > m.m[i][i])
			i = 2;
		size_t j = quaternion_inext[i];
		size_t k = quaternion_inext[j];

		float root = Sqrt(m.m[i][i] - m.m[j][j] - m.m[k][k] + 1.0f);
		float *quat[3] = {&_x, &_y, &_z};
		*quat[i] = 0.5f * root;
		root = 0.5f / root;
		_w = (m.m[k][j] - m.m[j][k]) * root;
		*quat[j] = (m.m[j][i] + m.m[i][j]) * root;
		*quat[k] = (m.m[k][i] + m.m[i][k]) * root;
	}
	return {_x, _y, _z, _w};
}

//
Quaternion QuaternionFromAxisAngle(float a, const Vec3 &v) {
	float sn = Sin(a * 0.5f), cs = Cos(a * 0.5f);
	return Normalize(Quaternion(v.x * sn, v.y * sn, v.z * sn, cs));
}

Quaternion QuaternionFromEuler(float x, float y, float z, RotationOrder rorder) {
	Quaternion r,
		qx = QuaternionFromAxisAngle(x, Vec3::Right),
		qy = QuaternionFromAxisAngle(y, Vec3::Up),
		qz = QuaternionFromAxisAngle(z, Vec3::Front);

	switch (rorder) {
		case RO_ZYX:
			r = qz * qy * qx;
			break;
		case RO_YZX:
			r = qy * qz * qx;
			break;
		case RO_ZXY:
			r = qz * qx * qy;
			break;
		case RO_XZY:
			r = qx * qz * qy;
			break;
		default:
		case RO_YXZ:
			r = qy * qx * qz;
			break;
		case RO_XYZ:
			r = qx * qy * qz;
			break;
		case RO_XY:
			r = qx * qy;
			break;
	}
	return Normalize(r);
}

//
Mat3 ToMatrix3(const Quaternion &q) {
	float sqw = q.w * q.w, sqx = q.x * q.x, sqy = q.y * q.y, sqz = q.z * q.z;

	Mat3 m;

	float invs = 1.f / (sqx + sqy + sqz + sqw);
	m.m[0][0] = (sqx - sqy - sqz + sqw) * invs; // Since sqw + sqx + sqy + sqz = 1 / invs * invs.
	m.m[1][1] = (-sqx + sqy - sqz + sqw) * invs;
	m.m[2][2] = (-sqx - sqy + sqz + sqw) * invs;

	float tmp1 = q.x * q.y;
	float tmp2 = q.z * q.w;
	m.m[1][0] = 2.f * (tmp1 + tmp2) * invs;
	m.m[0][1] = 2.f * (tmp1 - tmp2) * invs;

	tmp1 = q.x * q.z;
	tmp2 = q.y * q.w;
	m.m[2][0] = 2.f * (tmp1 - tmp2) * invs;
	m.m[0][2] = 2.f * (tmp1 + tmp2) * invs;
	tmp1 = q.y * q.z;
	tmp2 = q.x * q.w;
	m.m[2][1] = 2.f * (tmp1 + tmp2) * invs;
	m.m[1][2] = 2.f * (tmp1 - tmp2) * invs;

	return m;
}

Vec3 ToEuler(const Quaternion &q, RotationOrder rorder) { return ToEuler(ToMatrix3(q), rorder); }
Quaternion QuaternionFromEuler(const Vec3 &v, RotationOrder order) { return QuaternionFromEuler(v.x, v.y, v.z, order); }

} // namespace hg
