// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/plane.h"
#include "foundation/matrix4.h"
#include "foundation/vector3.h"

namespace hg {

Plane MakePlane(const Vec3 &p, const Vec3 &n) { return {n.x, n.y, n.z, -Dot(p, n)}; }
Plane MakePlane(const Vec3 &p, const Vec3 &n, const Mat4 &m) {
	Vec3 tp = m * p, tn = m * n;
	return {tn.x, tn.y, tn.z, -Dot(tp, tn)};
}

float DistanceToPlane(const Plane &plane, const Vec3 &p) { return Dot(p, {plane.x, plane.y, plane.z}) + plane.w; }

} // namespace hg
