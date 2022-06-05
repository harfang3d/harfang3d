// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/vector3.h"

namespace hg {

/// Compute the area of a triangle in 2 dimension.
float TriArea2D(float x0, float y0, float x1, float y1, float x2, float y2);

/*!
	Compute the intersection between a ray and a plane.
	@param [in]  a Origin of the ray.
	@param [in]  v Direction of the ray.
	@param [in]  n Normal of the plane.
	@param [in]  p A point on the plane.
	@param [out] t Distance along the ray of the intersection point.
	@return
		- false if the ray is embedded in or coplanar to the plane.
		- true otherwise.
*/
bool LineIntersectPlane(const Vec3 &a, const Vec3 &v, const Vec3 &n, const Vec3 &p, float &t);

/// Compute the barycentric coordinates (u,v,w) of a point p with respect to a triangle given by its 3 vertices a, b and c.
void Barycentric(const Vec3 &a, const Vec3 &b, const Vec3 &c, const Vec3 &p, float &u, float &v, float &w);

/*!
	Compute the intersection between a ray and sphere.
	@param [in]  a Origin of the ray.
	@param [in]  v Normalized direction of the ray.
	@param [in]  c Center of the sphere.
	@param [in]  r Radius of the sphere.
	@param [out] t0 Distance along the ray of the closest intersection point, i.e the point where the ray enters the sphere.
	@param [out] t1 Distance along the ray of the farthest intersection point, i.e the point where the ray exits the sphere.
					t1 can be equal to t0 when the ray is tangent to the sphere.
	@return
		- true if the ray intersects the sphere.
		- false otherwise.
*/
bool LineIntersectSphere(const Vec3 &a, const Vec3 &v, const Vec3 &c, float r, float &t0, float &t1);

/*!
	Compute the closest points between two lines.
	@param [in]  a	First point on first line.
	@param [in]  b	Second point on first line.
	@param [in]  u	First point on second line.
	@param [in]  v	Second point on second line.
	@param [out] t_ab Parametric coordinate of the closest point on ab to the line uv.
	@param [out] t_uv Parametric coordinate of the closest point on uv to the line ab.
	@return false if the lines are parallel.
*/
bool LineClosestPointToLine(const Vec3 &a, const Vec3 &b, const Vec3 &u, const Vec3 &v, float &t_ab, float &t_uv);

/*!
	Compute the closest point to a line from a position in space.
	@param [in]  a First point on line.
	@param [in]  b Second point on line.
	@param [in]  u Location.
	@param [out] p Closest point on line ab to u.
	@return Parametric coordinate of the closest point.
			If t < 0 or t > 1 then p lies outside the ab segment.
*/
float LineClosestPoint(const Vec3 &a, const Vec3 &b, const Vec3 &u, Vec3 *p = nullptr);

/*!
	Compute the closest point to a segment from a position in space.
	@param [in]  a First point on segment.
	@param [in]  b Second point on segment.
	@param [in]  u Location.
	@param [out] p Closest point on segment to u.
	@return Parametric coordinate of the closest point on the ab segment.
*/
float SegmentClosestPoint(const Vec3 &a, const Vec3 &b, const Vec3 &u, Vec3 *p = nullptr);

/*!
	Compute the intersection between a ray and a positive cone.
	@param [in] a Origin of the ray.
	@param [in] v Normalized direction of the ray.
	@param [in] c Cone origin.
	@param [in] d Cone axis.
	@param [in] theta Cone angle.
	@param [in] h Cone height.
	@param [out] t0 Distance along the ray of the closest intersection point, i.e the point where the ray enters the cone.
	@param [out] t1 Distance along the ray of the farthest intersection point, i.e the point where the ray exits the cone.
				t1 can be equal to t0 when the ray is tangent to the cone.
	@return
		- true if the ray intersects the cone.
		- false otherwise.
*/
bool LineIntersectCone(const Vec3 &a, const Vec3 &v, const Vec3 &c, const Vec3 &d, float theta, float h, float &t0, float &t1);

/*!
	Compute the intersection between a ray and a axis aligned box (aabb).
	@param [in] a Origin of the ray.
	@param [in] v Normalized direction of the ray.
	@param [in] min Lowest box corner.
	@param [in] max Highest box corner.
	@param [out] t0 Distance along the ray of the closest intersection point, i.e the point where the ray enters the aabb.
	@param [out] t1 Distance along the ray of the farthest intersection point, i.e the point where the ray exits the aabb.
					t1 can be equal to t0 when the ray is tangent to the aabb.
	@return
		- true if the ray intersects the cone.
		- false otherwise.
*/
bool LineIntersectAABB(const Vec3 &a, const Vec3 &v, const Vec3 &min, const Vec3 &max, float &t0, float &t1);

} // namespace hg
