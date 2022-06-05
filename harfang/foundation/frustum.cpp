// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/frustum.h"
#include "foundation/matrix4.h"
#include "foundation/matrix44.h"
#include "foundation/minmax.h"
#include "foundation/vector4.h"
#include <cmath>

namespace hg {

static inline Plane NormalizePlane(const Plane &p) {
	const auto l = sqrtf(p.x * p.x + p.y * p.y + p.z * p.z);
	return {p.x / l, p.y / l, p.z / l, p.w / l};
}

Frustum MakeFrustum(const Mat44 &projection) {
	const auto X = GetRow(projection, 0), Y = GetRow(projection, 1), Z = GetRow(projection, 2), W = GetRow(projection, 3);
	return {Opposite(NormalizePlane(W + Y)), Opposite(NormalizePlane(W - Y)), Opposite(NormalizePlane(W + X)), Opposite(NormalizePlane(W - X)),
		Opposite(NormalizePlane(W + Z)), Opposite(NormalizePlane(W - Z))};
}

Frustum MakeFrustum(const Mat44 &projection, const Mat4 &m) { return TransformFrustum(MakeFrustum(projection), m); }

Frustum TransformFrustum(const Frustum &frustum, const Mat4 &m) {
	const auto iMt = Transpose(Mat44(InverseFast(m)));
	return {iMt * frustum[FP_Top], iMt * frustum[FP_Bottom], iMt * frustum[FP_Left], iMt * frustum[FP_Right], iMt * frustum[FP_Near], iMt * frustum[FP_Far]};
}

//
Visibility TestVisibility(const Frustum &planes, uint32_t count, const Vec3 *point, float distance) {
	auto vis = V_Inside;
	for (uint32_t n = 0; n < FP_Count; ++n) {
		uint32_t out = 0;
		for (uint32_t i = 0; i < count; ++i)
			if (DistanceToPlane(planes[n], point[i]) > distance)
				++out;

		if (out == count)
			return V_Outside;
		if (out > 0)
			vis = V_Clipped;
	}
	return vis;
}

Visibility TestVisibility(const Frustum &planes, const Vec3 &point, float radius) {
	auto vis = V_Inside;
	for (uint32_t n = 0; n < FP_Count; ++n) {
		const float d = DistanceToPlane(planes[n], point);
		if (d > radius)
			return V_Outside;
		if (d > -radius)
			vis = V_Clipped;
	}
	return vis;
}

Visibility TestVisibility(const Frustum &planes, const MinMax &minmax) {
	const auto center_x2 = minmax.mn + minmax.mx;
	const auto extend_x2 = minmax.mx - minmax.mn;

	auto vis = V_Inside;
	for (uint32_t n = 0; n < FP_Count; ++n) {
		// complete demonstration at: https://fgiesen.wordpress.com/2010/10/17/view-frustum-culling/
		const auto &plane = planes[n];

		const float d = Dot({plane.x, plane.y, plane.z}, center_x2);
		const float r = Dot(Abs(Vec3(plane.x, plane.y, plane.z)), extend_x2); // "where there's one, there's many" would take care of that Abs()

		const float plane_d_x2 = -plane.w * 2.f;

		if (d - r > plane_d_x2)
			return V_Outside;
		if (d + r > plane_d_x2)
			vis = V_Clipped;
	}
	return vis;
}

} // namespace hg
