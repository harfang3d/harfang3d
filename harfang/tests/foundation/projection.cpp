// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <math.h>

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/projection.h"

#include "foundation/math.h"
#include "foundation/unit.h"

using namespace hg;

void test_projection() {
	{
		SetNDCInfos(true, false);
		TEST_CHECK(GetNDCInfos().origin_bottom_left == true);
		TEST_CHECK(GetNDCInfos().homogeneous_depth == false);

		SetNDCInfos(false, true);
		TEST_CHECK(GetNDCInfos().origin_bottom_left == false);
		TEST_CHECK(GetNDCInfos().homogeneous_depth == true);
	}
	{
		const float fov = Deg(60.f);
		const float zoom = sqrt(3.f);

		float z = FovToZoomFactor(fov);
		TEST_CHECK(Equal(z, zoom));

		float f = ZoomFactorToFov(zoom);
		TEST_CHECK(Equal(f, fov));
	}
	{
		SetNDCInfos(true, true);
		Mat44 m0 = ComputeOrthographicProjectionMatrix(1.0f, 21.f, 40.f, Vec2(16.f / 9.f, 1.f), Vec2(-1.5f, 1.5f));
		TEST_CHECK(AlmostEqual(GetRow(m0, 0), Vec4(0.028125f, 0.0f, 0.0f, -1.5f), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(m0, 1), Vec4(0.0f, 0.05f, 0.0f, 1.5f), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(m0, 2), Vec4(0.0f, 0.0f, 0.1f, -1.1f), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(m0, 3), Vec4(0.0f, 0.0f, 0.0f, 1.0f), 0.000001f));

		SetNDCInfos(true, false);
		Mat44 m1 = ComputeOrthographicProjectionMatrix(1.0f, 21.f, 40.f, Vec2(16.f / 9.f, 1.f), Vec2(-1.5f, 1.5f));
		TEST_CHECK(AlmostEqual(GetRow(m1, 0), GetRow(m0, 0), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(m1, 1), GetRow(m0, 1), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(m1, 2), Vec4(0.0f, 0.0f, 0.05f, -0.05f), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(m1, 3), GetRow(m0, 3), 0.000001f));
	}
	{
		SetNDCInfos(true, true);
		const float zoom = FovToZoomFactor(Deg(60.f));
		const Vec2 ar = ComputeAspectRatioX(16.f, 9.f);
		const Vec2 off = Vec2(-1.5f, 1.5f);
		const float znear = 1.f;
		const float zfar = 21.f;

		Mat44 m0 = ComputePerspectiveProjectionMatrix(znear, zfar, zoom, ar, off);
		TEST_CHECK(AlmostEqual(GetRow(m0, 0), Vec4(zoom / ar.x, 0.0f, 0.0f, off.x), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(m0, 1), Vec4(0.0f, zoom / ar.y, 0.0f, off.y), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(m0, 2), Vec4(0.0f, 0.0f, 1.1f, -2.1f), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(m0, 3), Vec4(0.0f, 0.0f, 1.0f, 0.0f), 0.000001f));

		SetNDCInfos(true, false);
		Mat44 m1 = ComputePerspectiveProjectionMatrix(znear, zfar, zoom, ar, off);
		TEST_CHECK(AlmostEqual(GetRow(m1, 0), GetRow(m0, 0), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(m1, 1), GetRow(m0, 1), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(m1, 2), Vec4(0.0f, 0.0f, 1.05f, -1.05f), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(m1, 3), GetRow(m0, 3), 0.000001f));
	}
	{
		const float width = 3840.f, height = 2160.f;
		const float znear = 1.f, zfar = 21.f;
		SetNDCInfos(true, true);
		Mat44 m0 = Compute2DProjectionMatrix(znear, zfar, width, height, true);
		TEST_CHECK(AlmostEqual(GetRow(m0, 0), Vec4(2.f / width, 0.0f, 0.0f, -1.f), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(m0, 1), Vec4(0.0f, 2.f / height, 0.0f, -1.f), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(m0, 2), Vec4(0.0f, 0.0f, 0.1f, -1.1f), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(m0, 3), Vec4(0.0f, 0.0f, 0.0f, 1.0f), 0.000001f));

		Mat44 m1 = Compute2DProjectionMatrix(znear, zfar, width, height, false);
		TEST_CHECK(AlmostEqual(GetRow(m1, 0), GetRow(m0, 0), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(m1, 1), -GetRow(m0, 1), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(m1, 2), GetRow(m0, 2), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(m1, 3), GetRow(m0, 3), 0.000001f));

		SetNDCInfos(true, false);
		Mat44 m2 = Compute2DProjectionMatrix(znear, zfar, width, height, true);
		TEST_CHECK(AlmostEqual(GetRow(m2, 0), GetRow(m0, 0), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(m2, 0), GetRow(m0, 0), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(m2, 2), Vec4(0.0f, 0.0f, 0.05f, -0.05f), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(m2, 3), GetRow(m0, 3), 0.000001f));

		Mat44 m3 = Compute2DProjectionMatrix(znear, zfar, width, height, false);
		TEST_CHECK(AlmostEqual(GetRow(m3, 0), GetRow(m2, 0), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(m3, 1), -GetRow(m2, 1), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(m3, 2), GetRow(m2, 2), 0.000001f));
		TEST_CHECK(AlmostEqual(GetRow(m3, 3), GetRow(m2, 3), 0.000001f));
	}
	{
		const float zoom = FovToZoomFactor(Deg(60.f));
		{
			SetNDCInfos(true, true);
			const Vec2 ar = ComputeAspectRatioX(4.f, 3.f);
			Mat44 m = ComputePerspectiveProjectionMatrix(0.1f, 100.f, zoom, ar, Vec2::Zero);
			TEST_CHECK(Equal(ExtractZoomFactorFromProjectionMatrix(m, ar), zoom));
		}
		{
			SetNDCInfos(true, false);
			const Vec2 ar = ComputeAspectRatioY(4.f, 3.f);
			Mat44 m = ComputePerspectiveProjectionMatrix(0.1f, 100.f, zoom, ar, Vec2::Zero);
			TEST_CHECK(Equal(ExtractZoomFactorFromProjectionMatrix(m, ar), zoom));
		}
	}
	{
		const Vec2 ar = ComputeAspectRatioX(4.f, 3.f);
		const float zoom = FovToZoomFactor(Deg(60.f));
		const float z_near = 0.4f;
		const float z_far = 20.f;
		float z0, z1;
		{
			SetNDCInfos(true, true);
			Mat44 m = ComputePerspectiveProjectionMatrix(z_near, z_far, zoom, ar, Vec2::One);
			ExtractZRangeFromPerspectiveProjectionMatrix(m, z0, z1);
			TEST_CHECK(Equal(z0, z_near));
			TEST_CHECK(AlmostEqual(z1, z_far, 0.0001f));
		}
		{
			SetNDCInfos(true, false);
			Mat44 m = ComputePerspectiveProjectionMatrix(z_near, z_far, zoom, ar, Vec2::Zero);
			ExtractZRangeFromPerspectiveProjectionMatrix(m, z0, z1);
			TEST_CHECK(Equal(z0, z_near));
			TEST_CHECK(AlmostEqual(z1, z_far, 0.0001f));
		}
	}
	{
		const Vec2 ar = ComputeAspectRatioX(4.f, 3.f);
		const float size = 32.f;
		const float z_near = 0.4f;
		const float z_far = 20.f;
		float z0, z1;

		{
			SetNDCInfos(true, true);
			Mat44 m = ComputeOrthographicProjectionMatrix(z_near, z_far, size, Vec2::One);
			ExtractZRangeFromOrthographicProjectionMatrix(m, z0, z1);
			TEST_CHECK(AlmostEqual(z0, z_near));
			TEST_CHECK(AlmostEqual(z1, z_far, 0.0001f));
		}
		{
			SetNDCInfos(true, false);
			Mat44 m = ComputeOrthographicProjectionMatrix(z_near, z_far, size, ar, Vec2::Zero);
			ExtractZRangeFromOrthographicProjectionMatrix(m, z0, z1);
			TEST_CHECK(Equal(z0, z_near));
			TEST_CHECK(AlmostEqual(z1, z_far, 0.0001f));
		}
	}
	{
		const Vec2 ar = ComputeAspectRatioX(4.f, 3.f);
		const float size = 32.f;
		const float zoom = FovToZoomFactor(Deg(60.f));
		const float z_near = 0.5f;
		const float z_far = 40.f;
		float z0, z1;
		{
			SetNDCInfos(true, true);
			Mat44 m = ComputePerspectiveProjectionMatrix(z_near, z_far, zoom, ar, Vec2::One);
			ExtractZRangeFromProjectionMatrix(m, z0, z1);
			TEST_CHECK(Equal(z0, z_near));
			TEST_CHECK(AlmostEqual(z1, z_far, 0.0001f));
		}
		{
			SetNDCInfos(true, false);
			Mat44 m = ComputePerspectiveProjectionMatrix(z_near, z_far, zoom, ar, Vec2::Zero);
			ExtractZRangeFromProjectionMatrix(m, z0, z1);
			TEST_CHECK(Equal(z0, z_near));
			TEST_CHECK(AlmostEqual(z1, z_far, 0.0001f));
		}
		{
			SetNDCInfos(true, true);
			Mat44 m = ComputeOrthographicProjectionMatrix(z_near, z_far, size, Vec2::One);
			ExtractZRangeFromProjectionMatrix(m, z0, z1);
			TEST_CHECK(AlmostEqual(z0, z_near));
			TEST_CHECK(AlmostEqual(z1, z_far, 0.0001f));
		}
		{
			SetNDCInfos(true, false);
			Mat44 m = ComputeOrthographicProjectionMatrix(z_near, z_far, size, ar, Vec2::Zero);
			ExtractZRangeFromProjectionMatrix(m, z0, z1);
			TEST_CHECK(Equal(z0, z_near));
			TEST_CHECK(AlmostEqual(z1, z_far, 0.0001f));
		}
	}
	{
		const Vec2 res(3840.f, 2160.f);
		const Vec2 ar = ComputeAspectRatioX(res.x, res.y);
		const Mat44 m = ComputePerspectiveProjectionMatrix(0.1f, 100.f, FovToZoomFactor(Deg(60.f)), ar, Vec2::Zero);
		Vec3 clip;
		TEST_CHECK(ProjectToClipSpace(m, Vec3(-0.4f, 0.2f, 0.f), clip) == false);
		TEST_CHECK(ProjectToClipSpace(m, Vec3(-10.f * sqrt(3.f) * ar.x, 12.5f * sqrt(3.f) * ar.y, 50.f), clip));
		TEST_CHECK(AlmostEqual(clip, Vec3(-0.6f, 0.75f, 0.998998940f), 0.000001f));
	}
	{
		const Vec2 res(3840.f, 2160.f);
		const Vec2 ar = ComputeAspectRatioX(res.x, res.y);
		const Mat44 m = ComputeOrthographicProjectionMatrix(0.0f, 100.f, 40.0f, ar, Vec2::Zero);
		Vec3 clip;
		TEST_CHECK(ProjectOrthoToClipSpace(m, Vec3(100.f / 5.625f, -10.f, 50.f), clip));
		TEST_CHECK(AlmostEqual(clip, Vec3(0.5f, -0.5f, 0.5), 0.000001f));
	}
	{
		TEST_CHECK(AlmostEqual(ClipSpaceToScreenSpace(Vec3(-1.0f, -1.0f, 0.12f), Vec2(1920.0f, 1080.0f)), Vec3(0.f, 0.f, 0.12f), 0.000001f));
		TEST_CHECK(AlmostEqual(ClipSpaceToScreenSpace(Vec3(1.0f, 1.0f, 0.2f), Vec2(1920.0f, 1080.0f)), Vec3(1920.0f, 1080.0f, 0.2f), 0.000001f));
		TEST_CHECK(AlmostEqual(ClipSpaceToScreenSpace(Vec3(0.0f, 0.0f, 0.45f), Vec2(1920.0f, 1080.0f)), Vec3(960.f, 540.f, 0.45f), 0.000001f));
	}
	{
		TEST_CHECK(AlmostEqual(ScreenSpaceToClipSpace(Vec3(0.0f, 0.0f, 0.5f), Vec2(2560.f, 1440.0f)), Vec3(-1.f, -1.f, 0.5f), 0.000001f));
		TEST_CHECK(AlmostEqual(ScreenSpaceToClipSpace(Vec3(1280.0f, 720.0f, 0.1f), Vec2(2560.f, 1440.0f)), Vec3(0.f, 0.0f, 0.1f), 0.000001f));
		TEST_CHECK(AlmostEqual(ScreenSpaceToClipSpace(Vec3(2560.0f, 1440.0f, 0.8f), Vec2(2560.f, 1440.0f)), Vec3(1.f, 1.f, 0.8f), 0.000001f));
	}
	{
		const Vec2 res(3840.f, 2160.f);
		const Vec2 ar = ComputeAspectRatioX(res.x, res.y);
		const Mat44 m = ComputePerspectiveProjectionMatrix(0.1f, 100.f, FovToZoomFactor(Deg(60.f)), ar, Vec2::Zero);
		Vec3 screen;
		TEST_CHECK(ProjectToScreenSpace(m, Vec3(-0.4f, 0.2f, 0.f), res, screen) == false);
		TEST_CHECK(ProjectToScreenSpace(m, Vec3(-10.f * sqrt(3.f) * ar.x, 12.5f * sqrt(3.f) * ar.y, 50.f), res, screen));
		TEST_CHECK(AlmostEqual(screen, Vec3(768.f, 1890.f, 0.998998940f), 0.0001f));
		TEST_CHECK(ProjectToScreenSpace(m, Vec3(0.f, 0.f, 50.f), res, screen));
		TEST_CHECK(AlmostEqual(screen, Vec3(res.x / 2.f, res.y / 2.f, 0.998998940f), 0.000001f));
	}
	{
		const Vec2 res(3840.f, 2160.f);
		const Vec2 ar = ComputeAspectRatioX(res.x, res.y);
		const Mat44 m = ComputeOrthographicProjectionMatrix(0.0f, 100.f, 40.0f, ar, Vec2::Zero);
		Vec3 screen;
		TEST_CHECK(ProjectOrthoToScreenSpace(m, Vec3(100.f / 5.625f, -10.f, 50.f), res, screen));
		TEST_CHECK(AlmostEqual(screen, Vec3(2880.f, 540.f, 0.5), 0.000001f));
		TEST_CHECK(ProjectOrthoToScreenSpace(m, Vec3(0.f, 0.f, 50.f), res, screen));
		TEST_CHECK(AlmostEqual(screen, Vec3(res.x / 2.f, res.y / 2.f, 0.5), 0.000001f));
	}
	{
		const Vec2 res(3840.f, 2160.f);
		const Vec2 ar = ComputeAspectRatioX(res.x, res.y);
		const Mat44 m = Inverse(ComputePerspectiveProjectionMatrix(0.1f, 100.f, FovToZoomFactor(Deg(60.f)), ar, Vec2::Zero));
		Vec3 p;
		TEST_CHECK(UnprojectFromScreenSpace(m, Vec3(-0.4f, 0.2f, -m.m[3][3] / m.m[3][2]), res, p) == false);

		Vec3 v(-10.f * sqrt(3.f) * ar.x, 12.5f * sqrt(3.f) * ar.y, 50.f);
		TEST_CHECK(UnprojectFromScreenSpace(m, Vec3(768.f, 1890.f, 0.998998940f), res, p));
		TEST_CHECK(AlmostEqual(p, v, 0.005f));

		TEST_CHECK(UnprojectFromScreenSpace(m, Vec3(res.x / 2.f, res.y / 2.f, 0.998998940f), res, p));
		TEST_CHECK(AlmostEqual(p, Vec3(0.f, 0.f, 50.f), 0.005f));
	}
	{
		const Vec2 res(3840.f, 2160.f);
		const Vec2 ar = ComputeAspectRatioX(res.x, res.y);
		const Mat44 m = Inverse(ComputeOrthographicProjectionMatrix(0.0f, 100.f, 40.0f, ar, Vec2::Zero));
		Vec3 p;
		TEST_CHECK(UnprojectOrthoFromScreenSpace(m, Vec3(2880.f, 540.f, 0.5), res, p));
		TEST_CHECK(AlmostEqual(p, Vec3(100.f / 5.625f, -10.f, 50.f), 0.0001f));
		TEST_CHECK(UnprojectOrthoFromScreenSpace(m, Vec3(res.x / 2.f, res.y / 2.f, 0.5), res, p));
		TEST_CHECK(AlmostEqual(p, Vec3(0.f, 0.f, 50.f), 0.0001f));
	}
	{
		// The only way to have ProjectOrthoTo* and UnprojectOrthoFrom* to retun false is to pass an invalid orthographic projection matrix.
		// Here a simple perspective projection matrix will do.
		const Vec2 res(3840.f, 2160.f);
		const Vec2 ar = ComputeAspectRatioX(res.x, res.y);
		const Mat44 m = ComputePerspectiveProjectionMatrix(0.5f, 100.f, 40.0f, ar, Vec2::Zero);
		const Mat44 inv_m = Inverse(m);
		Vec3 p;
		TEST_CHECK(ProjectOrthoToScreenSpace(m, Vec3(2880.f, 540.f, 0.f), res, p) == false);
		TEST_CHECK(UnprojectOrthoFromScreenSpace(m, Vec3(2880.f, 540.f, -m.m[3][3] / m.m[3][2]), res, p) == false);
	}
	{
		const Vec2 ar = ComputeAspectRatioX(4.f, 3.f);
		const float size = 32.f;
		const float zoom = FovToZoomFactor(Deg(60.f));
		const float z_near = 0.5f;
		const float z_far = 40.f;
		float z = Lerp(z_near, z_far, 0.4f);
		{
			SetNDCInfos(true, true);
			Mat44 m = ComputePerspectiveProjectionMatrix(z_near, z_far, zoom, ar, Vec2::One);
			TEST_CHECK(Equal(ProjectZToClipSpace(z_near, m), -1.f));
			TEST_CHECK(ProjectZToClipSpace(z_near - 0.2f, m) < -1.f);
			TEST_CHECK(ProjectZToClipSpace(z_far + 10.f, m) > 1.f);
			TEST_CHECK(Equal(ProjectZToClipSpace(z, m), (m.m[2][2] * z + m.m[2][3]) / z));
		}
		{
			SetNDCInfos(true, false);
			Mat44 m = ComputePerspectiveProjectionMatrix(z_near, z_far, zoom, ar, Vec2::One);
			TEST_CHECK(Equal(ProjectZToClipSpace(z_near, m), 0.f));
			TEST_CHECK(ProjectZToClipSpace(z_near - 0.2f, m) < 0.f);
			TEST_CHECK(ProjectZToClipSpace(z_far + 10.f, m) > 1.f);
			TEST_CHECK(Equal(ProjectZToClipSpace(z, m), (m.m[2][2] * z + m.m[2][3]) / z));
		}
	}
	{
		const float width = 1900.f;
		const float height = 1080.f;
		TEST_CHECK(AlmostEqual(ComputeAspectRatioX(width, height), Vec2(width / height, 1.f), 0.000001f));
		TEST_CHECK(AlmostEqual(ComputeAspectRatioY(width, height), Vec2(1.f, height / width), 0.000001f));
	}
	{
		const Vec2 res(1900.f, 1600.f);
		const Vec2 ar = ComputeAspectRatioX(res.x, res.y);
		const float zoom = FovToZoomFactor(Deg(60.f));
		const float z_near = 0.1f;
		const float z_far = 100.f;
		const Mat44 proj = ComputePerspectiveProjectionMatrix(z_near, z_far, zoom, ar);
		const Mat44 inv_proj = Inverse(proj);
		Vec3 o, d;
		TEST_CHECK(WorldRaycastScreenPos(res.x / 2.f, res.y / 2.f, res.x, res.y, inv_proj, Mat4::Identity, o, d));
		TEST_CHECK(AlmostEqual(o, Vec3::Zero, 0.000001f));
		TEST_CHECK(AlmostEqual(Normalize(d), Vec3(0.f, 0.f, 1.f), 0.000001f));

		TEST_CHECK(WorldRaycastScreenPos(0.f, res.y / 2.f, res.x, res.y, inv_proj, Mat4::Identity, o, d));
		TEST_CHECK(AlmostEqual(o, Vec3::Zero, 0.000001f));
		TEST_CHECK(AlmostEqual(Normalize(d), Normalize(Vec3(-ar.x / zoom, 0.f, 1.0)), 0.000001f));

		TEST_CHECK(WorldRaycastScreenPos(res.x, res.y / 2.f, res.x, res.y, inv_proj, Mat4::Identity, o, d));
		TEST_CHECK(AlmostEqual(o, Vec3::Zero, 0.000001f));
		TEST_CHECK(AlmostEqual(Normalize(d), Normalize(Vec3(ar.x / zoom, 0.f, 1.f)), 0.000001f));

		TEST_CHECK(WorldRaycastScreenPos(res.x / 2.f, 0.f, res.x, res.y, inv_proj, Mat4::Identity, o, d));
		TEST_CHECK(AlmostEqual(o, Vec3::Zero, 0.000001f));
		TEST_CHECK(AlmostEqual(Normalize(d), Normalize(Vec3(0.f, -ar.y / zoom, 1.f)), 0.000001f));

		TEST_CHECK(WorldRaycastScreenPos(res.x / 2.f, res.y, res.x, res.y, inv_proj, Mat4::Identity, o, d));
		TEST_CHECK(AlmostEqual(o, Vec3::Zero, 0.000001f));
		TEST_CHECK(AlmostEqual(Normalize(d), Normalize(Vec3(0.f, ar.y / zoom, 1.f)), 0.000001f));

		TEST_CHECK(WorldRaycastScreenPos(res.x, res.y, res.x, res.y, inv_proj, Mat4::Identity, o, d));
		TEST_CHECK(AlmostEqual(o, Vec3::Zero, 0.000001f));
		TEST_CHECK(AlmostEqual(Normalize(d), Normalize(Vec3(ar.x, ar.y, zoom)), 0.000001f));

		Mat44 buggy_proj = inv_proj;
		buggy_proj.m[3][2] = -20.f;
		buggy_proj.m[3][3] = 10.f;
		TEST_CHECK(WorldRaycastScreenPos(res.x, res.y, res.x, res.y, buggy_proj, Mat4::Identity, o, d) == false);
	}
}