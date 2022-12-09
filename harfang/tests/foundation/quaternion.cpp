// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <math.h>

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/quaternion.h"

#include "foundation/math.h"
#include "foundation/unit.h"
#include "foundation/vector3.h"
#include "foundation/vector4.h"
#include "foundation/matrix3.h"
#include "foundation/matrix4.h"

using namespace hg;

void test_quaternion() {
	{
		Quaternion q0;
		TEST_CHECK(Equal(q0.x, 0.0f));
		TEST_CHECK(Equal(q0.y, 0.0f));
		TEST_CHECK(Equal(q0.z, 0.0f));
		TEST_CHECK(Equal(q0.w, 1.0f));

		Quaternion q1(0.1f, 0.2f, 0.3f, -1.f);
		TEST_CHECK(Equal(q1.x, 0.1f));
		TEST_CHECK(Equal(q1.y, 0.2f));
		TEST_CHECK(Equal(q1.z, 0.3f));
		TEST_CHECK(Equal(q1.w,-1.0f));

		Quaternion q2(q1);
		TEST_CHECK(Equal(q2.x, q1.x));
		TEST_CHECK(Equal(q2.y, q1.y));
		TEST_CHECK(Equal(q2.z, q1.z));
		TEST_CHECK(Equal(q2.w, q1.w));
	}
	{
		Quaternion q0(0.22f, 1.4f, -2.75f, -0.3503f);
		q0 += Quaternion(0.042f, -0.2f, 5.25f, 0.9017f);
		TEST_CHECK(Equal(q0.x, 0.262f));
		TEST_CHECK(Equal(q0.y, 1.2f));
		TEST_CHECK(Equal(q0.z, 2.5f));
		TEST_CHECK(Equal(q0.w, 0.5514f));
	}
	{
		Quaternion q0(20.97f, -3.1f, 10.02f, -1.402f);
		q0 += 0.1f;
		TEST_CHECK(Equal(q0.x, 21.07f));
		TEST_CHECK(Equal(q0.y, -3.f));
		TEST_CHECK(AlmostEqual(q0.z, 10.12f));
		TEST_CHECK(Equal(q0.w, -1.302f));
	}
	{
		Quaternion q0(55.555f, 0.336f, 5.5555f, 7.9191f);
		q0 -= Quaternion(2.002f, 1.01f, 2.2222f, 0.054f);
		TEST_CHECK(Equal(q0.x, 53.553f));
		TEST_CHECK(Equal(q0.y, -0.674f));
		TEST_CHECK(Equal(q0.z, 3.3333f));
		TEST_CHECK(Equal(q0.w, 7.8651f));
	}
	{
		Quaternion q0(-1.025f, 2.3235f, 7.81194f, -0.40441f);
		q0 -= 0.004f;
		TEST_CHECK(Equal(q0.x, -1.029f));
		TEST_CHECK(Equal(q0.y, 2.3195f));
		TEST_CHECK(Equal(q0.z, 7.80794f));
		TEST_CHECK(Equal(q0.w, -0.40841f));
	}
	{
		{
			const Vec3 axis = Normalize(Vec3(1.f, 2.f, 1.f));
			const float theta0 = Deg(40.f);
			const float theta1 = Deg(120.f);
			const float theta2 = theta0 + theta1;
			Quaternion q0 = QuaternionFromAxisAngle(theta0, axis);
			Quaternion q1 = QuaternionFromAxisAngle(theta1, axis);
			Quaternion q2 = QuaternionFromAxisAngle(theta2, axis);
			q0 *= q1;
			TEST_CHECK(Equal(q0.x, q2.x));
			TEST_CHECK(Equal(q0.y, q2.y));
			TEST_CHECK(Equal(q0.z, q2.z));
			TEST_CHECK(Equal(q0.w, q2.w));

			q1 *= Conjugate(q1);
			TEST_CHECK(Equal(q1.x, 0.0f));
			TEST_CHECK(Equal(q1.y, 0.0f));
			TEST_CHECK(Equal(q1.z, 0.0f));
			TEST_CHECK(Equal(q1.w, 1.0f));
		}
		{
			float s = 1.f / Sqrt(2.f);
			Quaternion q0 = Quaternion(s, 0.f, 0.f, s);
			Quaternion q1 = Quaternion(0.f, s, 0.f, s);
			q0 *= q1;
			TEST_CHECK(Equal(q0.x, 0.5f));
			TEST_CHECK(Equal(q0.y, 0.5f));
			TEST_CHECK(Equal(q0.z, 0.5f));
			TEST_CHECK(Equal(q0.w, 0.5f));			
		}
	}
	{
		Quaternion q0(-2.022f, 11.11f, 0.075f, -101.3f);
		q0 *= 0.3f;
		TEST_CHECK(Equal(q0.x, -0.6066f));
		TEST_CHECK(Equal(q0.y, 3.333f));
		TEST_CHECK(Equal(q0.z, 0.0225f));
		TEST_CHECK(AlmostEqual(q0.w, -30.39f, 0.00001f));
	}
	{
		Quaternion q0(-2.015f, 0.03f, 27.6f, -0.975f);
		q0 /= 0.75f;
		TEST_CHECK(AlmostEqual(q0.x, -2.686666666f));
		TEST_CHECK(Equal(q0.y, 0.04f));
		TEST_CHECK(AlmostEqual(q0.z, 36.8f, 0.00001f));
		TEST_CHECK(Equal(q0.w, -1.3f));
	}
	{
		Quaternion q0(-0.1f, 0.2f, -0.3f, 1.f);
		Quaternion q1(q0);
		Quaternion q2(1.f, 0.f, 0.1f, 1.f);
		TEST_CHECK(q0 == q1);
		TEST_CHECK((q0 == q2) == false);
		TEST_CHECK((q1 == q2) == false);
	}
	{
		Quaternion q0(-0.1f, 0.2f, -0.3f, 1.f);
		Quaternion q1(q0);
		Quaternion q2(1.f, 0.f, 0.1f, 1.f);
		TEST_CHECK(q0 != q2);
		TEST_CHECK(q1 != q2);
		TEST_CHECK((q0 != q1) == false);
	}
	{
		TEST_CHECK(Equal(Dot(Quaternion(-1.0f, 1.0f,-1.0f, 1.0f), Quaternion(1.0f, 1.0f, 1.0f, 1.0f)), 0.f));
		TEST_CHECK(Equal(Dot(Quaternion( 2.0f, 0.5f,-4.0f, 2.0f), Quaternion(0.5f, 2.0f,-2.0f, 3.0f)), 16.f));
	}
	{
		Quaternion q0(2.020f, 0.5f, -0.5f, 0.3104f);
		Quaternion q1(1.010f, 1.f, -1.f, 0.0306f);
		Quaternion q2 = q0 + q1;
		TEST_CHECK(Equal(q2.x, 3.030f));
		TEST_CHECK(Equal(q2.y, 1.5f));
		TEST_CHECK(Equal(q2.z, -1.5f));
		TEST_CHECK(Equal(q2.w, 0.341f));
	}
	{
		Quaternion q0(47.3473f, 0.5f, -0.5f, 0.754f);
		Quaternion q1 = q0 + 0.25f;
		TEST_CHECK(Equal(q1.x, 47.5973f));
		TEST_CHECK(Equal(q1.y, 0.75f));
		TEST_CHECK(Equal(q1.z, -0.25f));
		TEST_CHECK(Equal(q1.w, 1.004f));
	}
	{
		Quaternion q0(0.5f, -0.5f, 1.25909f, -0.30303f);
		Quaternion q1(1.f, -1.f, 1.70707f, 0.0101f);
		Quaternion q2 = q0 - q1;
		TEST_CHECK(Equal(q2.x, -0.5f));
		TEST_CHECK(Equal(q2.y, 0.5f));
		TEST_CHECK(Equal(q2.z, -0.44798f));
		TEST_CHECK(Equal(q2.w, -0.31313f));
	}
	{
		Quaternion q0(0.5f, -0.5f, 3.333f, 7.05f);
		Quaternion q1 = q0 - 0.25f;
		TEST_CHECK(Equal(q1.x, 0.25f));
		TEST_CHECK(Equal(q1.y, -0.75f));
		TEST_CHECK(Equal(q1.z, 3.083f));
		TEST_CHECK(Equal(q1.w, 6.8f));
	}
	{
		{
			const Vec3 axis = Normalize(Vec3(1.f, 2.f, 1.f));
			const float theta0 = Deg(40.f);
			const float theta1 = Deg(120.f);
			const float theta2 = theta0 + theta1;
			Quaternion q0 = QuaternionFromAxisAngle(theta0, axis);
			Quaternion q1 = QuaternionFromAxisAngle(theta1, axis);
			Quaternion q2 = QuaternionFromAxisAngle(theta2, axis);
			Quaternion q3 = q0 * q1;
			TEST_CHECK(Equal(q2.x, q3.x));
			TEST_CHECK(Equal(q2.y, q3.y));
			TEST_CHECK(Equal(q2.z, q3.z));
			TEST_CHECK(Equal(q2.w, q3.w));

			Quaternion q4 = q0 * Conjugate(q0);
			TEST_CHECK(Equal(q4.x, 0.0f));
			TEST_CHECK(Equal(q4.y, 0.0f));
			TEST_CHECK(Equal(q4.z, 0.0f));
			TEST_CHECK(Equal(q4.w, 1.0f));
		}
		{
			float s = 1.f / Sqrt(2.f);
			Quaternion q0 = Quaternion(s, 0.f, 0.f, s);
			Quaternion q1 = Quaternion(0.f, s, 0.f, s);
			Quaternion q2 = q0 * q1;
			TEST_CHECK(Equal(q2.x, 0.5f));
			TEST_CHECK(Equal(q2.y, 0.5f));
			TEST_CHECK(Equal(q2.z, 0.5f));
			TEST_CHECK(Equal(q2.w, 0.5f));			
		}
	}
	{
		Quaternion q0(-5.06f, 0.75f, 2.72645f, 0.1717f);
		Quaternion q1 = q0 * 4.25f;
		Quaternion q2 = 4.25f * q0;
		TEST_CHECK(Equal(q1.x, -21.505f));
		TEST_CHECK(Equal(q1.y, 3.1875f));
		TEST_CHECK(AlmostEqual(q1.z, 11.5874125f));
		TEST_CHECK(Equal(q1.w, 0.729725f));
		TEST_CHECK(Equal(q2.x, q1.x));
		TEST_CHECK(Equal(q2.y, q1.y));
		TEST_CHECK(Equal(q2.z, q1.z));
		TEST_CHECK(Equal(q2.w, q1.w));
	}
	{
		Quaternion q0(9.008f, 0.75f, -57.1002f, 3.7f);
		Quaternion q1 = q0 / 0.1f;
		TEST_CHECK(Equal(q1.x, 90.08f));
		TEST_CHECK(Equal(q1.y, 7.5f));
		TEST_CHECK(Equal(q1.z, -571.002));
		TEST_CHECK(Equal(q1.w, 37.f));
	}
	{
		Quaternion q0 = Normalize(Quaternion(3.0f, -3.0f, -3.0f, 3.0f));
		Quaternion q1 = Normalize(QuaternionFromAxisAngle(Deg(-30.f), Normalize(Vec3(2.f, -1.f, 0.5f))));
		Quaternion q2 = Normalize(Quaternion(1.f, 0.f, 0.f,-1.f));
		TEST_CHECK(AlmostEqual(Dot(q0, q0), 1.f));
		TEST_CHECK(AlmostEqual(Dot(q1, q1), 1.f));
		TEST_CHECK(AlmostEqual(Dot(q2, q2), 1.f));
	}
	{
		Quaternion q0(9.008f, 0.75f, -57.1002f, 3.7f);
		Quaternion q1 = Conjugate(q0);
		TEST_CHECK(Equal(q1.x, -q0.x));
		TEST_CHECK(Equal(q1.y, -q0.y));
		TEST_CHECK(Equal(q1.z, -q0.z));
		TEST_CHECK(Equal(q1.w, q0.w));
	}
	{
		const Vec3 axis = Normalize(Vec3(1.f, -2.f, -1.f));
		Quaternion q0 = QuaternionFromAxisAngle(Deg(160.f), axis);
		Quaternion q1 = q0 * Inverse(q0);
		Quaternion q2 = QuaternionFromAxisAngle(0.f, axis);
		TEST_CHECK(Equal(q1.x, q2.x));
		TEST_CHECK(Equal(q1.y, q2.y));
		TEST_CHECK(Equal(q1.z, q2.z));
		TEST_CHECK(Equal(q1.w, q2.w));
	}
	{
		Quaternion q0 = Quaternion(4.0f, -4.0f, -4.0f, 4.0f);
		Quaternion q1 = Quaternion(1.f, 0.f, 0.f,-1.f);
		TEST_CHECK(Equal(Len(q0), 8.f));
		TEST_CHECK(Equal(Len(q1), Sqrt(2.f)));
	}
	{
		Quaternion q0 = Quaternion(4.0f, -4.0f, -4.0f, 4.0f);
		Quaternion q1 = Quaternion(1.f, 0.f, 0.f,-1.f);
		TEST_CHECK(Equal(Len2(q0), 64.f));
		TEST_CHECK(Equal(Len2(q1), 2.f));
	}
	{
		Quaternion q0(1.0f,-2.0f,-3.0f, 4.0f);
		Quaternion q1(0.1f, 0.2f, 0.3f,-0.4f);
		TEST_CHECK(Equal(Dist(q0, q1), Sqrt(35.9f)));
	}
	{ 
		{
			Quaternion q0 = QuaternionFromAxisAngle(Deg(90.f), Vec3::Up);
			Quaternion q1 = QuaternionFromAxisAngle(Deg(180.f), Vec3::Up);
			Quaternion q2 = QuaternionFromAxisAngle(Deg(90.f + 45.f), Vec3::Up);
			Quaternion q3 = Slerp(q0, q1, 0.f);
			TEST_CHECK(Equal(q3.x, q0.x));
			TEST_CHECK(Equal(q3.y, q0.y));
			TEST_CHECK(Equal(q3.z, q0.z));
			TEST_CHECK(Equal(q3.w, q0.w));

			Quaternion q4 = Slerp(q0, q1, 1.f);
			TEST_CHECK(Equal(q4.x, q1.x));
			TEST_CHECK(Equal(q4.y, q1.y));
			TEST_CHECK(Equal(q4.z, q1.z));
			TEST_CHECK(Equal(q4.w, q1.w));

			Quaternion q5 = Slerp(q0, q1, 0.5f);
			TEST_CHECK(Equal(q5.x, q2.x));
			TEST_CHECK(Equal(q5.y, q2.y));
			TEST_CHECK(Equal(q5.z, q2.z));
			TEST_CHECK(Equal(q5.w, q2.w));
		}
		{ 
			float t = 0.4f;
			Quaternion q0 = QuaternionFromAxisAngle(Deg(160.f), Normalize(Vec3(1.f, -2.f, -1.f)));
			Quaternion q1 = QuaternionFromAxisAngle(Deg(45.f), Normalize(Vec3(1.f, 0.f, -2.f)));
			Quaternion q2 = Slerp(q0, q1, t);
			Quaternion q3 = Slerp(q1, q0, 1.f-t);
			TEST_CHECK(Equal(q3.x, q2.x));
			TEST_CHECK(Equal(q3.y, q2.y));
			TEST_CHECK(Equal(q3.z, q2.z));
			TEST_CHECK(Equal(q3.w, q2.w));
		}
		{
			float t = 0.75f;
			Quaternion q0 = QuaternionFromAxisAngle(Deg(30.f), Vec3::Up);
			Quaternion q1 = QuaternionFromAxisAngle(Deg(-30.f), -Vec3::Up);
			Quaternion q2 = Lerp(q0, q1, t);
			Quaternion q3 = Slerp(q0, q1, t);
			TEST_CHECK(Equal(q3.x, q2.x));
			TEST_CHECK(Equal(q3.y, q2.y));
			TEST_CHECK(Equal(q3.z, q2.z));
			TEST_CHECK(Equal(q3.w, q2.w));
		}
		{
			const Vec3 axis = Normalize(Vec3::One);
			const float t = 0.25f;
			const float a0 = 30.f, a1 = 180.f + a0;
			Quaternion q0 = QuaternionFromAxisAngle(Deg(a1), axis);
			Quaternion q1 = QuaternionFromAxisAngle(Deg(a0), -axis);
			Quaternion q2 = QuaternionFromAxisAngle(Deg(240.f), axis);
			Quaternion q3 = Slerp(q0, q1, t);
			TEST_CHECK(AlmostEqual(q3.x, q2.x));
			TEST_CHECK(AlmostEqual(q3.y, q2.y));
			TEST_CHECK(AlmostEqual(q3.z, q2.z));
			TEST_CHECK(AlmostEqual(q3.w, q2.w));
		}
	}
	{
		Vec3 angle = Deg3(-60.f, 30.f, 45.f);
		{
			Mat3 rot = RotationMat3(angle, RO_ZYX);
			Quaternion q0 = QuaternionFromEuler(angle, RO_ZYX);
			Mat3 m = ToMatrix3(q0);
			TEST_CHECK(AlmostEqual(GetColumn(m, 0), GetColumn(rot, 0), 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 1), GetColumn(rot, 1), 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 2), GetColumn(rot, 2), 0.000001f));
		}
		{
			Mat3 rot = RotationMat3(angle, RO_YZX);
			Quaternion q0 = QuaternionFromEuler(angle, RO_YZX);
			Mat3 m = ToMatrix3(q0);
			TEST_CHECK(AlmostEqual(GetColumn(m, 0), GetColumn(rot, 0), 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 1), GetColumn(rot, 1), 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 2), GetColumn(rot, 2), 0.000001f));
		}
		{
			Mat3 rot = RotationMat3(angle, RO_ZXY);
			Quaternion q0 = QuaternionFromEuler(angle, RO_ZXY);
			Mat3 m = ToMatrix3(q0);
			TEST_CHECK(AlmostEqual(GetColumn(m, 0), GetColumn(rot, 0), 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 1), GetColumn(rot, 1), 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 2), GetColumn(rot, 2), 0.000001f));
		}
		{
			Mat3 rot = RotationMat3(angle, RO_XZY);
			Quaternion q0 = QuaternionFromEuler(angle, RO_XZY);
			Mat3 m = ToMatrix3(q0);
			TEST_CHECK(AlmostEqual(GetColumn(m, 0), GetColumn(rot, 0), 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 1), GetColumn(rot, 1), 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 2), GetColumn(rot, 2), 0.000001f));
		}
		{
			Mat3 rot = RotationMat3(angle, RO_YXZ);
			Quaternion q0 = QuaternionFromEuler(angle, RO_YXZ);
			Mat3 m = ToMatrix3(q0);
			TEST_CHECK(AlmostEqual(GetColumn(m, 0), GetColumn(rot, 0), 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 1), GetColumn(rot, 1), 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 2), GetColumn(rot, 2), 0.000001f));
		}
		{
			Mat3 rot = RotationMat3(angle, RO_XYZ);
			Quaternion q0 = QuaternionFromEuler(angle, RO_XYZ);
			Mat3 m = ToMatrix3(q0);
			TEST_CHECK(AlmostEqual(GetColumn(m, 0), GetColumn(rot, 0), 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 1), GetColumn(rot, 1), 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 2), GetColumn(rot, 2), 0.000001f));
		}
		{
			Mat3 rot = RotationMat3(angle, RO_XY);
			Quaternion q0 = QuaternionFromEuler(angle, RO_XY);
			Vec3 theta = ToEuler(q0, RO_XY);
			TEST_CHECK(Equal(theta.x, angle.x));
			TEST_CHECK(Equal(theta.y, angle.y));
			TEST_CHECK(Equal(theta.z, 0.f));
		}
	}
	{
		{
			Vec3 angle = Deg3(-30.f, 45.f,-60.f);
			Mat3 rot = RotationMat3(angle);
			Quaternion q0 = QuaternionFromMatrix3(rot);
			TEST_CHECK(AlmostEqual(ToEuler(q0), angle, 0.000001f));
		}
		{
			Vec3 angle = Deg3(180.f, 0.f, 0.f);
			Mat3 rot = RotationMat3(angle);
			Quaternion q0 = QuaternionFromMatrix3(rot);
			Mat3 m = ToMatrix3(q0);
			TEST_CHECK(AlmostEqual(GetColumn(m, 0), GetColumn(rot, 0), 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 1), GetColumn(rot, 1), 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 2), GetColumn(rot, 2), 0.000001f));
		}
		{
			Vec3 angle = Deg3(0.f, 180.f, 0.f);
			Mat3 rot = RotationMat3(angle);
			Quaternion q0 = QuaternionFromMatrix3(rot);
			Mat3 m = ToMatrix3(q0);
			TEST_CHECK(AlmostEqual(GetColumn(m, 0), GetColumn(rot, 0), 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 1), GetColumn(rot, 1), 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 2), GetColumn(rot, 2), 0.000001f));
		}
		{
			Vec3 angle = Deg3(0.f, 0.f, 180.f);
			Mat3 rot = RotationMat3(angle);
			Quaternion q0 = QuaternionFromMatrix3(rot);
			Mat3 m = ToMatrix3(q0);
			TEST_CHECK(AlmostEqual(GetColumn(m, 0), GetColumn(rot, 0), 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 1), GetColumn(rot, 1), 0.000001f));
			TEST_CHECK(AlmostEqual(GetColumn(m, 2), GetColumn(rot, 2), 0.000001f));
		}
	}
	{
		const Vec3 p = Vec3(1.5f, 2.3f, 10.f);
		const Mat3 m = Mat3LookAt(p);
		Quaternion q = QuaternionLookAt(p);
		TEST_CHECK(AlmostEqual(ToEuler(m), ToEuler(q), 1.e-5f));
	}
	{ 
		const Vec3 angle = Deg3(-60.f, 45.f, 90.f);
		const Quaternion q = QuaternionFromEuler(angle);
		const Mat3 m = RotationMat3(angle);
		Vec3 v0(-2.f, 3.f, -4.f);
		TEST_CHECK(AlmostEqual(m * v0, Rotate(q, v0), 1.e-5f));
	}
	{
		const Vec3 angle = Deg3(-60.f, 45.f, 90.f);
		const Quaternion q = QuaternionFromEuler(angle);
		const Mat4 m = RotationMat4(angle);
		Vec4 v0(-2.f, 3.f, -4.f, 11.f);
		TEST_CHECK(AlmostEqual(m * v0, Rotate(q, v0), 1.e-5f));

		TEST_CHECK(AlmostEqual(Rotate(QuaternionFromEuler(Deg3(90.f, 0.f, 0.f)), Vec3::Up), Vec3::Front, 1.e-5f));
		TEST_CHECK(AlmostEqual(Rotate(QuaternionFromEuler(Deg3(0.f, 0.f, 90.f)), Vec3::Up), Vec3::Left, 1.e-5f));
		TEST_CHECK(AlmostEqual(Rotate(QuaternionFromEuler(Deg3(0.f, 90.f, 0.f)), Vec3::Left), Vec3::Front, 1.e-5f));
	}
}