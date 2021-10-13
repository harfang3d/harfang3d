// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/vector3.h"
#include "foundation/vector4.h"

namespace hg {

struct Mat4;

/**
	@short Plane
	ax + by + cz + d = 0
*/
using Plane = Vec4;

Plane MakePlane(const Vec3 &p, const Vec3 &n);
Plane MakePlane(const Vec3 &p, const Vec3 &n, const Mat4 &m);

/**
	@short Return point distance to plane.
	Distance is signed, positive when the point is in front of the plane,
	negative otherwise.
*/
float DistanceToPlane(const Plane &plane, const Vec3 &p);

} // namespace hg
