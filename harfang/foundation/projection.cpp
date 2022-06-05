// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "projection.h"
#include "assert.h"
#include "unit.h"

namespace hg {

static NDCInfos g_ndcInfos = {};

void SetNDCInfos(bool origin_bottom_left, bool homogeneous_depth) {
	g_ndcInfos.origin_bottom_left = origin_bottom_left;
	g_ndcInfos.homogeneous_depth = homogeneous_depth;
}

const NDCInfos &GetNDCInfos() { return g_ndcInfos; }

float ZoomFactorToFov(float zoom_factor) { return ATan(1.f / zoom_factor) * 2.f; }
float FovToZoomFactor(float fov) { return 1.f / Tan(Clamp(fov, Deg(0.1f), Deg(179.9f)) * 0.5f); }

Mat44 ComputeOrthographicProjectionMatrix(float znear, float zfar, float size, const Vec2 &aspect_ratio, const Vec2 &offset) {
	const NDCInfos &ndc_infos = GetNDCInfos();

	const float qA = ndc_infos.homogeneous_depth ? (2.f / (zfar - znear)) : (1.f / (zfar - znear));
	const float qB = ndc_infos.homogeneous_depth ? (-(zfar + znear) / (zfar - znear)) : (-qA * znear);
	return {2.f / size / aspect_ratio.x, 0, 0, 0, 0, 2.f / size / aspect_ratio.y, 0, 0, 0, 0, qA, 0, offset.x, offset.y, qB, 1};
}

Mat44 ComputePerspectiveProjectionMatrix(float znear, float zfar, float zoom_factor, const Vec2 &aspect_ratio, const Vec2 &offset) {
	const NDCInfos &ndc_infos = GetNDCInfos();

	const float qA = ndc_infos.homogeneous_depth ? ((zfar + znear) / (zfar - znear)) : (zfar / (zfar - znear));
	const float qB = ndc_infos.homogeneous_depth ? (-2 * zfar * znear / (zfar - znear)) : (-qA * znear);
	return {zoom_factor / aspect_ratio.x, 0, 0, 0, 0, zoom_factor / aspect_ratio.y, 0, 0, 0, 0, qA, 1, offset.x, offset.y, qB, 0};
}

Mat44 Compute2DProjectionMatrix(float znear, float zfar, float res_x, float res_y, bool y_up) {
	const NDCInfos &ndc_infos = GetNDCInfos();

	const float qA = ndc_infos.homogeneous_depth ? (2.f / (zfar - znear)) : (1.f / (zfar - znear));
	const float qB = ndc_infos.homogeneous_depth ? (-(zfar + znear) / (zfar - znear)) : (-qA * znear);

	Mat44 p_m(2.f / res_x, 0, 0, 0, 0, 2.f / res_y, 0, 0, 0, 0, qA, 0, -1.f, -1.f, qB, 1.f);

	if (!y_up) {
		p_m.m[1][1] *= -1;
		p_m.m[1][3] *= -1;
	}
	return p_m;
}

//
float ExtractZoomFactorFromProjectionMatrix(const Mat44 &m) { return m.m[1][1]; }

void ExtractZRangeFromPerspectiveProjectionMatrix(const Mat44 &m, float &znear, float &zfar) {
	const NDCInfos &ndc_infos = GetNDCInfos();
	znear = ndc_infos.homogeneous_depth ? (-m.m[2][3] / (m.m[2][2] + 1.f)) : (-m.m[2][3] / m.m[2][2]);
	zfar = -m.m[2][3] / (m.m[2][2] - 1.f);
}

void ExtractZRangeFromOrthographicProjectionMatrix(const Mat44 &m, float &znear, float &zfar) {
	const NDCInfos &ndc_infos = GetNDCInfos();
	znear = ndc_infos.homogeneous_depth ? (-(m.m[2][3] + 1.f) / m.m[2][2]) : (-m.m[2][3] / m.m[2][2]);
	zfar = -(m.m[2][3] - 1.f) / m.m[2][2];
}

void ExtractZRangeFromProjectionMatrix(const Mat44 &m, float &znear, float &zfar) {
	// Here we assume that the projection is either a well formed perspective or orthographic projection matrix.
	if (m.m[3][3] == 0.f) {
		ExtractZRangeFromPerspectiveProjectionMatrix(m, znear, zfar);
	} else {
		ExtractZRangeFromOrthographicProjectionMatrix(m, znear, zfar);
	}
}

//
bool ProjectToClipSpace(const Mat44 &proj, const Vec3 &view, Vec3 &clip) {
	Vec4 ndc = proj * Vec4(view);
	if (ndc.w <= 0.f)
		return false;
	clip = Vec3(ndc) / ndc.w;
	return true;
}

bool ProjectOrthoToClipSpace(const Mat44& proj, const Vec3& view, Vec3& clip) {
	Vec4 ndc = proj * Vec4(view);
	if (ndc.w <= 0.f)
		return false;
	clip = Vec3(ndc);
	return true;
}

bool UnprojectFromClipSpace(const Mat44 &inv_proj, const Vec3 &clip, Vec3 &view) {
	Vec4 ndc = inv_proj * Vec4(clip);
	if (ndc.w <= 0.f)
		return false;
	view = Vec3(ndc) / ndc.w;
	return true;
}

bool UnprojectOrthoFromClipSpace(const Mat44 &inv_proj, const Vec3 &clip, Vec3 &view) {
	Vec4 ndc = inv_proj * Vec4(clip);
	if (ndc.w <= 0.f)
		return false;
	view = Vec3(ndc);
	return true;
}

// NOTE [EJ] Z is a problem here... DX -> [0;1], GL -> [-1;1]
Vec3 ClipSpaceToScreenSpace(const Vec3 &clip, const Vec2 &res) { return {(clip.x + 1.f) * res.x * 0.5f, (clip.y + 1.f) * res.y * 0.5f, clip.z}; }
Vec3 ScreenSpaceToClipSpace(const Vec3 &screen, const Vec2 &res) { return {screen.x / res.x * 2.f - 1.f, screen.y / res.y * 2.f - 1.f, screen.z}; }

//
bool ProjectToScreenSpace(const Mat44 &proj, const Vec3 &view, const Vec2 &res, Vec3 &screen) {
	Vec3 clip;
	if (!ProjectToClipSpace(proj, view, clip))
		return false;
	screen = ClipSpaceToScreenSpace(clip, res);
	return true;
}

bool ProjectOrthoToScreenSpace(const Mat44& proj, const Vec3& view, const Vec2& res, Vec3& screen) {
	Vec3 clip;
	if (!ProjectOrthoToClipSpace(proj, view, clip))
		return false;
	screen = ClipSpaceToScreenSpace(clip, res);
	return true;
}

bool UnprojectFromScreenSpace(const Mat44 &inv_proj, const Vec3 &screen, const Vec2 &res, Vec3 &view) {
	const auto clip = ScreenSpaceToClipSpace(screen, res);
	return UnprojectFromClipSpace(inv_proj, clip, view);
}

bool UnprojectOrthoFromScreenSpace(const Mat44 &inv_proj, const Vec3 &screen, const Vec2 &res, Vec3 &view) {
	const auto clip = ScreenSpaceToClipSpace(screen, res);
	return UnprojectOrthoFromClipSpace(inv_proj, clip, view);
}

//
float ProjectZToClipSpace(float z, const Mat44 &proj) { return (z * proj.m[2][2] + proj.m[2][3]) / z; }

//
Vec2 ComputeAspectRatioX(float width, float height) { return {width / height, 1.f}; }
Vec2 ComputeAspectRatioY(float width, float height) { return {1.f, height / width}; }

//
bool WorldRaycastScreenPos(float x, float y, float width, float height, const Mat44 &inv_proj, const Mat4 &inv_view, Vec3 &ray_o, Vec3 &ray_d) {
	Vec3 view_pos;
	if (!UnprojectFromScreenSpace(inv_proj, {x, y, 1.f}, {width, height}, view_pos))
		return false;

	ray_o = GetT(inv_view);
	ray_d = inv_view * view_pos - ray_o;
	return true;
}

} // namespace hg
