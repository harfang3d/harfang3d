// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/math.h"
#include "foundation/matrix4.h"
#include "foundation/matrix44.h"
#include "foundation/vector2.h"
#include "foundation/vector3.h"
#include "foundation/vector4.h"

namespace hg {

/// Normalized device coordinates infos.
struct NDCInfos {
	bool origin_bottom_left; ///< if true the origin is at the bottom left.
	bool homogeneous_depth; ///< if true the depth range is [-1,1], otherwise it is [0,1].
};

void SetNDCInfos(bool origin_bottom_left, bool homogeneous_depth);
const NDCInfos &GetNDCInfos();

/// Computes field of fiew angle (in radians) from zoom factor.
float ZoomFactorToFov(float zoom_factor);
/// Computes zoom factor from field of view angle (in radians).
float FovToZoomFactor(float fov);

/*!
	@short Compute an orthographic projection matrix.

	An orthographic projection has no perspective and all lines parrallel in 3d space will
	still appear parrallel on screen after projection using the returned matrix.
	
	The size parameter controls the extends of the projected view. When projecting a 3d world
	this parameter is expressed in meters. Use the aspect_ratio parameter to prevent
	distortion from induced by non-square viewport.

	@see ComputeAspectRatioX or ComputeAspectRatioY to compute an aspect ratio factor in paysage or portrait mode.
*/
Mat44 ComputeOrthographicProjectionMatrix(float znear, float zfar, float size, const Vec2 &aspect_ratio, const Vec2 &offset = Vec2::Zero);
/*!
	Compute a perspective projection matrix.

	fov is the field of view angle (see Deg and Rad).

	@see ZoomFactorToFov, FovToZoomFactor, ComputeAspectRatioX and ComputeAspectRatioY.
*/
Mat44 ComputePerspectiveProjectionMatrix(float znear, float zfar, float zoom_factor, const Vec2 &aspect_ratio, const Vec2 &offset = Vec2::Zero);
/// Returns a projection matrix from a 2D space to the 3D world, as required by SetViewTransform() for example.
Mat44 Compute2DProjectionMatrix(float znear, float zfar, float res_x, float res_y, bool y_up);

/// Extract zoom factor from a projection matrix.
/// @see ZoomFactorToFov.
float ExtractZoomFactorFromProjectionMatrix(const Mat44 &m, const Vec2 &aspect_ratio);
/// Extract Z near and Z far from a projection matrix.
void ExtractZRangeFromPerspectiveProjectionMatrix(const Mat44 &m, float &znear, float &zfar);
void ExtractZRangeFromOrthographicProjectionMatrix(const Mat44 &m, float &znear, float &zfar);
/// Extract z near and z far clipping range from a projection matrix.
void ExtractZRangeFromProjectionMatrix(const Mat44 &m, float &znear, float &zfar);

/// Project a world position to the clipping space.
bool ProjectToClipSpace(const Mat44 &proj, const Vec3 &view, Vec3 &clip);
/// Project a world position to the orthographic clipping space.
bool ProjectOrthoToClipSpace(const Mat44 &proj, const Vec3 &view, Vec3 &clip);
/// Unproject a clip space position to perspective view space.
bool UnprojectFromClipSpace(const Mat44 &inv_proj, const Vec3 &clip, Vec3 &view);
/// Unproject a clip space position to orthographic view space.
bool UnprojectOrthoFromClipSpace(const Mat44 &inv_proj, const Vec3 &clip, Vec3 &view);

/// Convert a 3d position in clip space (homogeneous space) to a 2d position on screen.
Vec3 ClipSpaceToScreenSpace(const Vec3 &clip, const Vec2 &res);
/// Transform a screen position to clip space.
Vec3 ScreenSpaceToClipSpace(const Vec3 &screen, const Vec2 &res);

/// Project a world position to screen coordinates.
bool ProjectToScreenSpace(const Mat44 &proj, const Vec3 &view, const Vec2 &res, Vec3 &screen);
/// Project a world position to screen coordinates in orthographic mode.
bool ProjectOrthoToScreenSpace(const Mat44 &proj, const Vec3 &view, const Vec2 &res, Vec3 &screen);
/// Unproject a screen space position to perspective view space.
bool UnprojectFromScreenSpace(const Mat44 &inv_proj, const Vec3 &screen, const Vec2 &res, Vec3 &view);
/// Unproject a screen space position to orthographic view space.
bool UnprojectOrthoFromScreenSpace(const Mat44 &inv_proj, const Vec3 &screen, const Vec2 &res, Vec3 &view);

/// Project a depth value to clip space.
float ProjectZToClipSpace(float z, const Mat44 &proj);

/// Compute the aspect ratio factor for the provided viewport dimensions. Use this method to compute aspect ratio for landscape display.
/// @see ComputeAspectRatioY.
Vec2 ComputeAspectRatioX(float width, float height);
/// Compute the aspect ratio factor for the provided viewport dimensions. Use this method to compute aspect ratio for portrait display.
/// @see ComputeAspectRatioX.
Vec2 ComputeAspectRatioY(float width, float height);

/// Compute world-space ray start and direction from a 2d screen position
bool WorldRaycastScreenPos(float x, float y, float width, float height, const Mat44 &inv_proj, const Mat4 &inv_view, Vec3 &ray_o, Vec3 &ray_d);

} // namespace hg
