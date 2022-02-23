// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/intersection.h"
#include "foundation/math.h"
#include "foundation/vector3.h"
#include <cfloat>
#include <cmath>

namespace hg {

float TriArea2D(float x0, float y0, float x1, float y1, float x2, float y2) { return (x0 - x1) * (y1 - y2) - (x1 - x2) * (y0 - y1); }

void Barycentric(const Vec3 &a, const Vec3 &b, const Vec3 &c, const Vec3 &p, float &u, float &v, float &w) {
	auto m = Cross(b - a, c - a);

	float nu, nv, ood;
	auto x = fabs(m.x), y = fabs(m.y), z = fabs(m.z);

	if (x >= y && x >= z) {
		nu = TriArea2D(p.y, p.z, b.y, b.z, c.y, c.z);
		nv = TriArea2D(p.y, p.z, c.y, c.z, a.y, a.z);
		ood = 1.f / m.x;
	} else if (y >= x && y >= z) {
		nu = TriArea2D(p.x, p.z, b.x, b.z, c.x, c.z);
		nv = TriArea2D(p.x, p.z, c.x, c.z, a.x, a.z);
		ood = 1.f / -m.y;
	} else {
		nu = TriArea2D(p.x, p.y, b.x, b.y, c.x, c.y);
		nv = TriArea2D(p.x, p.y, c.x, c.y, a.x, a.y);
		ood = 1.f / m.z;
	}
	u = nu * ood;
	v = nv * ood;
	w = 1.f - u - v;
}

bool LineIntersectPlane(const Vec3 &a, const Vec3 &v, const Vec3 &n, const Vec3 &p, float &t) {
	auto k = Dot(v, n);
	if (EqualZero(k))
		return false;
	t = (Dot(p, n) - Dot(a, n)) / k;
	return true;
}

bool LineIntersectSphere(const Vec3 &a, const Vec3 &v, const Vec3 &c, float r, float &t0, float &t1) {
	auto e = c - a;

	auto k = Dot(e, v);
	auto d = r * r - (Len2(e) - k * k);
	if (d < 0)
		return false;

	d = Sqrt(d);

	t0 = k - d;
	t1 = k + d;
	return true;
}

float LineClosestPoint(const Vec3 &a, const Vec3 &b, const Vec3 &u, Vec3 *p) {
	auto _u = u - a;
	auto _v = b - a;

	auto t = Dot(_u, _v) / Dot(_v, _v);

	if (p)
		p[0] = _v * t + a;

	return t;
}

bool LineClosestPointToLine(const Vec3 &a, const Vec3 &b, const Vec3 &la, const Vec3 &lb, float &t0, float &t1) {
	auto u = b - a, v = lb - la;
	auto ul2 = Len2(u), vl2 = Len2(v);

	auto d = Dot(u, v), k = ul2 * vl2 - d * d;

	if (fabs(k) < 0.00000001f)
		return false;

	k = 1.f / k;
	auto uv = d, du = Dot(la - a, u), dv = Dot(a - la, v);

	t0 = (vl2 * du + uv * dv) * k;
	t1 = (uv * du + ul2 * dv) * k;
	return true;
}

float SegmentClosestPoint(const Vec3 &a, const Vec3 &b, const Vec3 &u, Vec3 *p) {
	auto _u = u - a, _v = b - a;
	auto t = Clamp(Dot(_u, _v) / Dot(_v, _v), 0.f, 1.f);

	if (p)
		p[0] = _v * t + a;
	return t;
}

bool LineIntersectCone(const Vec3 &a, const Vec3 &v, const Vec3 &c, const Vec3 &d, float theta, float h, float &t0, float &t1) {
	Vec3 ac = a - c;

	float ddv = Dot(d, v);
	float ddac = Dot(d, ac);
	float dvac = Dot(v, ac);
	float cs = cos(theta);

	float c0 = ddv * ddv - cs * cs;
	float c1 = ddv * ddac - cs * cs * dvac;
	float c2 = ddac * ddac - cs * cs * Dot(ac, ac);

	if (c0 != 0.f) {
		float delta = c1 * c1 - c0 * c2;
		if (delta < 0.f) {
			return false;
		} else if (delta > 0.f) {
			t0 = (-c1 - sqrt(delta)) / c0;
			if ((ddv * t0 + ddac) < 0.f) {
				t0 = FLT_MAX;
			}

			t1 = (-c1 + sqrt(delta)) / c0;
			if ((ddv * t1 + ddac) >= 0.f) {
				if (t1 < t0) {
					float tmp = t1;
					t1 = t0;
					t0 = tmp;
				}
			} else if (t0 == FLT_MAX) {
				return false;
			}
		} else {
			t0 = -c1 / c0;
			t1 = FLT_MAX;
			if ((ddv * t0 + ddac) < 0.f) {
				return false;
			}
		}
	} else if (c1 != 0.f) {
		t0 = -0.5f * c0 / c1;
		t1 = FLT_MAX;
		if ((ddv * t0 + ddac) < 0.f) {
			return false;
		}
	} else if (c2 != 0.f) {
		return false;
	}

	if (ddv != 0.f) {
		float h0 = -ddac / ddv;
		float h1 = (h - ddac) / ddv;
		if (ddv < 0.f) {
			float tmp = h1;
			h1 = h0;
			h0 = tmp;
		}

		if (((t0 < h0) || (t0 > h1)) && ((t1 < h0) || (t1 > h1))) {
			return false;
		}
	} else if (dvac > h) {
		return false;
	}

	return true;
}

bool LineIntersectAABB(const Vec3 &a, const Vec3 &v, const Vec3 &min, const Vec3 &max, float & /*t0*/, float & /*t1*/) {
	Vec3 p0 = (min - a) / v;
	Vec3 p1 = (max - a) / v;

	Vec3 p2 = Min(p0, p1);
	Vec3 p3 = Max(p0, p1);

	float tmin = Max(Max(p2.x, p2.y), p2.z);
	float tmax = Min(Min(p3.x, p3.y), p3.z);

	if ((tmax < 0) || (tmin > tmax))
		return false;

	return true;
}

} // namespace hg
