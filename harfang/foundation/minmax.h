// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/axis.h"
#include "foundation/vector3.h"

namespace hg {

struct Mat4;

struct MinMax {
	MinMax() = default;
	MinMax(const Vec3 &min, const Vec3 &max) : mn(min), mx(max) {}

	Vec3 mn, mx;
};

/// Get the min-max area.
inline float GetArea(const MinMax &mm) { return (mm.mx.x - mm.mn.x) * (mm.mx.y - mm.mn.y) * (mm.mx.z - mm.mn.z); }
/// Return the minmax center.
inline Vec3 GetCenter(const MinMax &mm) { return (mm.mn + mm.mx) * 0.5f; }
/// Return the minmax size.
inline Vec3 GetSize(const MinMax &mm) { return mm.mx - mm.mn; }

/// Fill an array of vector3 with the position of the minmax vertices.
void GetMinMaxVertices(const MinMax &minmax, Vec3 out[8]);
/// Return the origin and radius of the minmax bounding sphere.
void ComputeMinMaxBoundingSphere(const MinMax &minmax, Vec3 &origin, float &radius);

/// Test minmax overlap on a specific axis.
inline bool Overlap(const MinMax &a, const MinMax &b, Axis axis) { return !(b.mn[axis] > a.mx[axis] || b.mx[axis] < a.mn[axis]); }
/// Return whether two MinMax overlap at a given time.
inline bool Overlap(const MinMax &a, const MinMax &b) {
	return !(a.mx.x < b.mn.x || a.mx.y < b.mn.y || a.mx.z < b.mn.z || b.mx.x < a.mn.x || b.mx.y < a.mn.y || b.mx.z < a.mn.z);
}

inline bool operator==(const MinMax &a, const MinMax &b) { return a.mn == b.mn && a.mx == b.mx; }
inline bool operator!=(const MinMax &a, const MinMax &b) { return a.mn != b.mn || a.mx != b.mx; }

/// Returns true if the provided position is inside the bounding volume, false otherwise.
inline bool Contains(const MinMax &mm, const Vec3 &p) {
	return !(p.x < mm.mn.x || p.y < mm.mn.y || p.z < mm.mn.z || p.x > mm.mx.x || p.y > mm.mx.y || p.z > mm.mx.z);
}

/// Return the union of two minmax.
inline MinMax Union(const MinMax &a, const MinMax &b) { return {Min(a.mn, b.mn), Max(a.mx, b.mx)}; }
/// Return the union of a minmax and vector.
inline MinMax Union(const MinMax &mm, const Vec3 &p) { return {Min(mm.mn, p), Max(mm.mx, p)}; }

/// Return a transformed copy of a minmax instance.
MinMax operator*(const MinMax &mm, const Mat4 &m);

/// Intersect ray with this minmax.
bool IntersectRay(const MinMax &mm, const Vec3 &o, const Vec3 &d, float &tmin, float &tmax);
bool IntersectRay(const MinMax &mm, const Vec3 &o, const Vec3 &d);

/// Returns whether a line intersect with the MinMax.
bool ClassifyLine(const MinMax &mm, const Vec3 &p, const Vec3 &d, Vec3 &i, Vec3 *n = nullptr);
/// Returns whether a segment intersect with the MinMax.
bool ClassifySegment(const MinMax &mm, const Vec3 &p1, const Vec3 &p2, Vec3 &itr, Vec3 *n = nullptr);

/// Set from position and size.
inline MinMax MinMaxFromPositionSize(const Vec3 &p, const Vec3 &s) { return {p - s * 0.5f, p + s * 0.5f}; }

} // namespace hg
