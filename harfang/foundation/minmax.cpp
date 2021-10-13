// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/minmax.h"
#include "foundation/matrix4.h"
#include "foundation/math.h"
#include "foundation/obb.h"
#include <cfloat>

namespace hg {

enum {
	ClipNone = 0,
	ClipRight = 1,
	ClipLeft = 2,
	ClipTop = 4,
	ClipBottom = 8,
	ClipFront = 16,
	ClipBack = 32
};

static uint32_t cc_oc(const Vec3 &min, const Vec3 &max, const Vec3 &p) {
	uint32_t oc = ClipNone;
	if (p.x > max.x)
		oc |= ClipRight;
	else if (p.x < min.x)
		oc |= ClipLeft;
	if (p.y > max.y)
		oc |= ClipTop;
	else if (p.y < min.y)
		oc |= ClipBottom;
	if (p.z > max.z)
		oc |= ClipBack;
	else if (p.z < min.z)
		oc |= ClipFront;
	return oc;
}

static uint32_t ss_oc(const Vec3 &p) {
	uint32_t oc = ClipNone;
	oc |= p.x > 0 ? ClipRight : ClipLeft;
	oc |= p.y > 0 ? ClipTop : ClipBottom;
	oc |= p.z > 0 ? ClipBack : ClipFront;
	return oc;
}

bool IntersectRay(const MinMax &mm, const Vec3 &o, const Vec3 &d, float &tmin, float &tmax) {
	tmin = 0;
	tmax = FLT_MAX;

	for (uint32_t n = 0; n < 3; ++n)
		if (EqualZero(d[n])) {
			if (o[n] < mm.mn[n] || o[n] > mm.mx[n])
				return false;
		} else {
			float ood = 1.f / d[n];
			float t0 = (mm.mn[n] - o[n]) * ood, t1 = (mm.mx[n] - o[n]) * ood;

			if (t0 > t1) {
				float swp = t1;
				t1 = t0;
				t0 = swp;
			}

			tmin = tmin < t0 ? t0 : tmin;
			tmax = tmax < t1 ? tmax : t1;

			if (tmin > tmax)
				return false;
		}

	return true;
}

bool IntersectRay(const MinMax &mm, const Vec3 &o, const Vec3 &d) {
	float tmin, tmax;
	return IntersectRay(mm, o, d, tmin, tmax);
}

bool ClassifyLine(const MinMax &mm, const Vec3 &p1, const Vec3 &direction, Vec3 &itr, Vec3 *n) {
	uint32_t oc1, oc2;

	oc1 = cc_oc(mm.mn, mm.mx, p1);
	if (oc1 == ClipNone) {
		if (n)
			*n = Vec3(0, 0, 0);
		itr = p1;
		return true; // point inside bounding box
	}

	oc2 = ss_oc(direction);

	if ((oc1 & oc2) > ClipNone)
		return false; // same side

	if (oc1 & (ClipRight | ClipLeft)) {
		if (oc1 & ClipRight) {
			if (n)
				*n = Vec3(1, 0, 0);
			itr.x = mm.mx.x;
		} else {
			if (n)
				*n = Vec3(-1, 0, 0);
			itr.x = mm.mn.x;
		}
		float x1 = direction.x, x2 = itr.x - p1.x;
		itr.y = p1.y + x2 * direction.y / x1;
		itr.z = p1.z + x2 * direction.z / x1;

		if (itr.y <= mm.mx.y && itr.y >= mm.mn.y && itr.z <= mm.mx.z && itr.z >= mm.mn.z)
			return true;
	}

	if (oc1 & (ClipTop | ClipBottom)) {
		if (oc1 & ClipTop) {
			if (n)
				*n = Vec3(0, 1, 0);
			itr.y = mm.mx.y;
		} else {
			if (n)
				*n = Vec3(0, -1, 0);
			itr.y = mm.mn.y;
		}
		float y1 = direction.y, y2 = itr.y - p1.y;
		itr.x = p1.x + y2 * direction.x / y1;
		itr.z = p1.z + y2 * direction.z / y1;

		if (itr.x <= mm.mx.x && itr.x >= mm.mn.x && itr.z <= mm.mx.z && itr.z >= mm.mn.z)
			return true;
	}

	if (oc1 & (ClipFront | ClipBack)) {
		if (oc1 & ClipBack) {
			if (n)
				*n = Vec3(0, 0, 1);
			itr.z = mm.mx.z;
		} else {
			if (n)
				*n = Vec3(0, 0, -1);
			itr.z = mm.mn.z;
		}
		float z1 = direction.z, z2 = itr.z - p1.z;
		itr.x = p1.x + z2 * direction.x / z1;
		itr.y = p1.y + z2 * direction.y / z1;

		if (itr.x <= mm.mx.x && itr.x >= mm.mn.x && itr.y <= mm.mx.y && itr.y >= mm.mn.y)
			return true;
	}
	return false;
}

bool ClassifySegment(const MinMax &mm, const Vec3 &p1, const Vec3 &p2, Vec3 &itr, Vec3 *n) {
	uint32_t oc1, oc2;

	oc1 = cc_oc(mm.mn, mm.mx, p1);
	if (oc1 == ClipNone) { // point inside bounding box
		if (n)
			*n = Vec3(0, 0, 0);
		itr = p1;
		return true;
	}

	oc2 = cc_oc(mm.mn, mm.mx, p2);
	if (oc2 == ClipNone) { // point inside bounding box
		itr = p2;
		return true;
	}

	if ((oc1 & oc2) > ClipNone)
		return false; // on the same side

	if (oc1 & (ClipRight | ClipLeft)) {
		if (oc1 & ClipRight) {
			if (n)
				*n = Vec3(1, 0, 0);
			itr.x = mm.mx.x;
		} else {
			if (n)
				*n = Vec3(-1, 0, 0);
			itr.x = mm.mn.x;
		}

		float x1 = p2.x - p1.x, x2 = itr.x - p1.x;
		itr.y = p1.y + x2 * (p2.y - p1.y) / x1;
		itr.z = p1.z + x2 * (p2.z - p1.z) / x1;

		if (itr.y <= mm.mx.y && itr.y >= mm.mn.y && itr.z <= mm.mx.z && itr.z >= mm.mn.z)
			return true;
	}

	if (oc1 & (ClipTop | ClipBottom)) {
		if (oc1 & ClipTop) {
			if (n)
				*n = Vec3(0, 1, 0);
			itr.y = mm.mx.y;
		} else {
			if (n)
				*n = Vec3(0, -1, 0);
			itr.y = mm.mn.y;
		}
		float y1 = p2.y - p1.y, y2 = itr.y - p1.y;
		itr.x = p1.x + y2 * (p2.x - p1.x) / y1;
		itr.z = p1.z + y2 * (p2.z - p1.z) / y1;

		if (itr.x <= mm.mx.x && itr.x >= mm.mn.x && itr.z <= mm.mx.z && itr.z >= mm.mn.z)
			return true;
	}

	if (oc1 & (ClipFront | ClipBack)) {
		if (oc1 & ClipBack) {
			if (n)
				*n = Vec3(0, 0, 1);
			itr.z = mm.mx.z;
		} else {
			if (n)
				*n = Vec3(0, 0, -1);
			itr.z = mm.mn.z;
		}
		float z1 = p2.z - p1.z, z2 = itr.z - p1.z;
		itr.x = p1.x + z2 * (p2.x - p1.x) / z1;
		itr.y = p1.y + z2 * (p2.y - p1.y) / z1;

		if (itr.x <= mm.mx.x && itr.x >= mm.mn.x && itr.y <= mm.mx.y && itr.y >= mm.mn.y)
			return true;
	}
	return false;
}

MinMax operator*(const MinMax &mm, const Mat4 &m) {
	MinMax out(GetT(m), GetT(m));

	// find extreme points by considering product of min and max with each component of M
	for (uint32_t j = 0; j < 3; ++j) {
		for (uint32_t i = 0; i < 3; ++i) {
			float a = m.m[j][i] * mm.mn[i], b = m.m[j][i] * mm.mx[i];

			if (a < b) {
				out.mn[j] += a;
				out.mx[j] += b;
			} else {
				out.mn[j] += b;
				out.mx[j] += a;
			}
		}
	}
	return out;
}

void GetMinMaxVertices(const MinMax &minmax, Vec3 out[8]) {
	out[0] = Vec3(minmax.mn.x, minmax.mn.y, minmax.mn.z);
	out[1] = Vec3(minmax.mx.x, minmax.mn.y, minmax.mn.z);
	out[2] = Vec3(minmax.mx.x, minmax.mx.y, minmax.mn.z);
	out[3] = Vec3(minmax.mn.x, minmax.mx.y, minmax.mn.z);
	out[4] = Vec3(minmax.mn.x, minmax.mn.y, minmax.mx.z);
	out[5] = Vec3(minmax.mx.x, minmax.mn.y, minmax.mx.z);
	out[6] = Vec3(minmax.mx.x, minmax.mx.y, minmax.mx.z);
	out[7] = Vec3(minmax.mn.x, minmax.mx.y, minmax.mx.z);
}

void ComputeMinMaxBoundingSphere(const MinMax &minmax, Vec3 &origin, float &radius) {
	origin = (minmax.mn + minmax.mx) * 0.5f;
	radius = Dist(minmax.mn, minmax.mx) * 0.5f;
}

} // namespace hg
