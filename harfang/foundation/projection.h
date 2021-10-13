// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/math.h"
#include "foundation/vector2.h"
#include "foundation/vector3.h"
#include "foundation/vector4.h"
#include "foundation/matrix4.h"
#include "foundation/matrix44.h"

namespace hg {

/// Normalized device coordinates infos.
struct NDCInfos {
	bool origin_bottom_left; ///< if @true the origin is at the bottom left.
	bool homogeneous_depth;  ///< if @true the depth range is [-1,1], otherwise it is [0,1].
};

void SetNDCInfos(bool origin_bottom_left, bool homogeneous_depth);
const NDCInfos &GetNDCInfos();

/// Computes field of fiew angle (in radians) from zoom factor.
float ZoomFactorToFov(float zoom_factor);
/// Computes zoom factor from field of view angle (in radians).
float FovToZoomFactor(float fov);

Mat44 ComputeOrthographicProjectionMatrix(float znear, float zfar, float size, const Vec2 &aspect_ratio, const Vec2 &offset = hg::Vec2::Zero);
Mat44 ComputePerspectiveProjectionMatrix(float znear, float zfar, float zoom_factor, const Vec2 &aspect_ratio, const Vec2 &offset = hg::Vec2::Zero);
Mat44 Compute2DProjectionMatrix(float znear, float zfar, float res_x, float res_y, bool y_up);

/// Extract zoom factor from a projection matrix.
float ExtractZoomFactorFromProjectionMatrix(const Mat44 &m);
/// Extract Z near and Z far from a projection matrix.
void ExtractZRangeFromPerspectiveProjectionMatrix(const Mat44 &m, float &znear, float &zfar);
void ExtractZRangeFromOrthographicProjectionMatrix(const Mat44 &m, float &znear, float &zfar);
void ExtractZRangeFromProjectionMatrix(const Mat44 &m, float &znear, float &zfar);

bool ProjectToClipSpace(const Mat44 &proj, const Vec3 &view, Vec3 &clip);
bool UnprojectFromClipSpace(const Mat44 &inv_proj, const Vec3 &clip, Vec3 &view);

Vec3 ClipSpaceToScreenSpace(const Vec3 &clip, const Vec2 &res);
Vec3 ScreenSpaceToClipSpace(const Vec3 &screen, const Vec2 &res);

bool ProjectToScreenSpace(const Mat44 &proj, const Vec3 &view, const Vec2 &res, Vec3 &screen);
bool UnprojectFromScreenSpace(const Mat44 &inv_proj, const Vec3 &screen, const Vec2 &res, Vec3 &view);

float ProjectZToClipSpace(float z, const Mat44 &proj);

Vec2 ComputeAspectRatioX(float width, float height);
Vec2 ComputeAspectRatioY(float width, float height);

/// Compute world-space ray start and direction from a 2d screen position
bool WorldRaycastScreenPos(float x, float y, float width, float height, const hg::Mat44 &inv_proj, const hg::Mat4 &inv_view, hg::Vec3 &ray_o, hg::Vec3 &ray_d);

} // namespace hg
