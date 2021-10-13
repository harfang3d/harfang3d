// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/matrix3.h"
#include "foundation/vector3.h"

namespace hg {

struct MinMax;

/// Oriented bounding box
struct OBB {
	OBB() = default;
	OBB(const Vec3 &pos_, const Vec3 &scl_) : pos(pos_), scl(scl_), rot(Mat3::Identity) {}
	OBB(const Vec3 &pos_, const Vec3 &scl_, const Mat3 &rot_) : pos(pos_), scl(scl_), rot(rot_) {}

	Vec3 pos, scl;
	Mat3 rot;
};

/// OBB from min-max.
OBB OBBFromMinMax(const MinMax &minmax);
/// Compute min-max from OBB.
MinMax MinMaxFromOBB(const OBB &obb);

/// Transform OBB.
OBB TransformOBB(const OBB &obb, const Mat4 &m);

} // namespace hg
