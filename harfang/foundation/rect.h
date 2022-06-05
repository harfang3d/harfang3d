// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/math.h"
#include "foundation/vector2.h"

namespace hg {

/// Rectangle 2D
template <class T> struct Rect {
	Rect() = default;
	Rect(T x_, T y_) : sx(x_), sy(y_), ex(x_), ey(y_) {}
	Rect(T sx_, T sy_, T ex_, T ey_) : sx(sx_), sy(sy_), ex(ex_), ey(ey_) {}

	T sx, sy, ex, ey;
};

using iRect = Rect<int>;
using fRect = Rect<float>;

//
template <typename T> T GetX(const Rect<T> &r) { return r.sx; }
template <typename T> T GetY(const Rect<T> &r) { return r.sy; }
template <typename T> void SetX(Rect<T> &r, T x) {
	const auto w = GetWidth(r);
	r.sx = x;
	r.ex = x + w;
}
template <typename T> void SetY(Rect<T> &r, T y) {
	const auto h = GetHeight(r);
	r.sy = y;
	r.ey = y + h;
}

template <typename T> Rect<T> Reorder(const Rect<T> &r) {
	return {r.sx < r.ex ? r.sx : r.ex, r.sy < r.ey ? r.sy : r.ey, r.sx > r.ex ? r.sx : r.ex, r.sy > r.ey ? r.sy : r.ey};
}

template <typename T> T GetWidth(const Rect<T> &r) { return r.ex - r.sx; }
template <typename T> T GetHeight(const Rect<T> &r) { return r.ey - r.sy; }
template <typename T> void SetWidth(Rect<T> &r, T w) { r.ex = r.sx + w; }
template <typename T> void SetHeight(Rect<T> &r, T h) { r.ey = r.sy + h; }

template <typename T> tVec2<T> GetSize(const Rect<T> &r) { return {r.ex - r.sx, r.ey - r.sy}; }

//
template <typename T> Rect<T> operator*(const Rect<T> &r, T v) { return {r.sx * v, r.sy * v, r.ex * v, r.ey * v}; }
template <typename T> Rect<T> operator/(const Rect<T> &r, T v) { return {r.sx / v, r.sy / v, r.ex / v, r.ey / v}; }

template <typename T> bool operator==(const Rect<T> &a, const Rect<T> &b) { return a.sx == b.sx && a.sy == b.sy && a.ex == b.ex && a.ey == b.ey; }
template <typename T> bool operator!=(const Rect<T> &a, const Rect<T> &b) { return a.sx != b.sx || a.sy != b.sy || a.ex != b.ex || a.ey != b.ey; }

template <typename T, typename O> bool Inside(const Rect<T> &r, const O &o) { return T(o.x) > r.sx && T(o.y) > r.sy && T(o.x) < r.ex && T(o.y) < r.ey; }

/// Return whether `a` fits in `b`.
template <typename T> bool FitsInside(const Rect<T> &a, const Rect<T> &b) { return GetWidth(a) <= GetWidth(b) && GetHeight(a) <= GetHeight(b); }
/// Return `true` if rect `a` intersects rect `b`.
template <typename T> bool Intersects(const Rect<T> &a, const Rect<T> &b) { return !(a.ex < b.sx || a.ey < b.sy || a.sx > b.ex || a.sy > b.ey); }

template <typename T> Rect<T> Intersection(const Rect<T> &a, const Rect<T> &b) {
	const auto _sx = Max(a.sx, b.sx), _sy = Max(a.sy, b.sy), _ex = Min(a.ex, b.ex), _ey = Min(a.ey, b.ey);
	const auto n_sx = Min(_sx, _ex), n_sy = Min(_sy, _ey), n_ex = Max(_sx, _ex), n_ey = Max(_sy, _ey);
	return {n_sx, n_sy, n_ex, n_ey};
}

template <typename T> Rect<T> Union(const Rect<T> &a, const Rect<T> &b) { return {Min(a.sx, b.sx), Min(a.sy, b.sy), Max(a.ex, b.ex), Max(a.ey, b.ey)}; }
/// Grow a rectangle by the specified amount of units.
/// @see Crop.
template <typename T> Rect<T> Grow(const Rect<T> &r, T border) { return {r.sx - border, r.sy - border, r.ex + border, r.ey + border}; }

/// Crop a rectangle. Remove the specified amount of units on each side of the rectangle. 
/// @see Grow.
template <typename T> Rect<T> Crop(const Rect<T> &r, T left, T top, T right, T bottom) { return {r.sx + left, r.sy + top, r.ex - right, r.ey - bottom}; }

template <typename T, typename O> bool Clamp(const Rect<T> &r, O &o) {
	if (Inside(r, o))
		return false; // no clamp
	o.x = decltype(o.x)(Clamp(T(o.x), r.sx, r.ex));
	o.y = decltype(o.y)(Clamp(T(o.y), r.sy, r.ey));
	return true;
}

//
template <typename T> Rect<T> Offset(const Rect<T> &r, T x, T y) { return {r.sx + x, r.sy + y, r.ex + x, r.ey + y}; }
template <typename T> Rect<T> OffsetX(const Rect<T> &r, T x) { return {r.sx + x, r.sy, r.ex + x, r.ey}; }
template <typename T> Rect<T> OffsetY(const Rect<T> &r, T y) { return {r.sx, r.sy + y, r.ex, r.ey + y}; }

template <typename T> Rect<T> MakeRectFromWidthHeight(T x, T y, T w, T h) { return {x, y, x + w, y + h}; }

//
template <typename T> Rect<T> FitRectByWidth(const Rect<T> &src, const Rect<T> &fit_to) {
	Rect<T> out;
	out.sx = src.sx;
	out.ex = src.ex;
	const auto dy = (GetHeight(fit_to) * GetWidth(src)) / GetWidth(fit_to) / 2;
	out.sy = (src.sy + src.ey) / 2 - dy;
	out.ey = (src.sy + src.ey) / 2 + dy;
	return out;
}

template <typename T> Rect<T> FitRectByHeight(const Rect<T> &src, const Rect<T> &fit_to) {
	Rect<T> out;
	out.sy = src.sy;
	out.ey = src.ey;
	const auto dx = (GetWidth(fit_to) * GetHeight(src)) / GetHeight(fit_to) / 2;
	out.sx = (src.sx + src.ex) / 2 - dx;
	out.ex = (src.sx + src.ex) / 2 + dx;
	return out;
}

template <typename T> Rect<T> FitRectByAspectRatio(const Rect<T> &src, const Rect<T> &fit_to) {
	const auto dst_ar = float(GetWidth(fit_to)) / GetHeight(fit_to), src_ar = float(GetWidth(src)) / GetHeight(src);
	return src_ar > dst_ar ? FitRectByHeight(src, fit_to) : FitRectByWidth(src, fit_to);
}

//
template <typename T> Rect<T> AlignTop(const Rect<T> &r, T top) { return {r.sx, top, r.ex, top + GetHeight(r)}; }
template <typename T> Rect<T> AlignBottom(const Rect<T> &r, T bottom) { return {r.sx, bottom - GetHeight(r), r.ex, bottom}; }
template <typename T> Rect<T> AlignLeft(const Rect<T> &r, T left) { return {left, r.sy, left + GetWidth(r), r.ey}; }
template <typename T> Rect<T> AlignRight(const Rect<T> &r, T right) { return {right - GetWidth(r), r.sy, right, r.ey}; }

//
template <typename T> fRect ToFloatRect(const Rect<T> &r) { return {float(r.sx), float(r.sy), float(r.ex), float(r.ey)}; }
template <typename T> iRect ToIntRect(const Rect<T> &r) { return {int(r.sx), int(r.sy), int(r.ex), int(r.ey)}; }

} // namespace hg
