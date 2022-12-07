// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/math.h"
#include "foundation/rect.h"

using namespace hg;

void test_rect() {
	{
		fRect a(1.f, 2.f);
		TEST_CHECK(a.sx == 1.f);
		TEST_CHECK(a.sy == 2.f);
		TEST_CHECK(a.sx == a.ex);
		TEST_CHECK(a.sy == a.ey);
	}
	{
		iRect a(3, 5);
		TEST_CHECK(a.sx == 3);
		TEST_CHECK(a.sy == 5);
		TEST_CHECK(a.sx == a.ex);
		TEST_CHECK(a.sy == a.ey);
	} 
	{
		fRect a(1.f, 2.f, 3.f, 4.f);
		TEST_CHECK(a.sx == 1.f);
		TEST_CHECK(a.sy == 2.f);
		TEST_CHECK(a.ex == 3.f);
		TEST_CHECK(a.ey == 4.f);
		TEST_CHECK(GetX(a) == 1.f);
		TEST_CHECK(GetY(a) == 2.f);
		TEST_CHECK(GetWidth(a) == 2.f);
		TEST_CHECK(GetHeight(a) == 2.f);

		SetWidth(a, 4.25f);
		TEST_CHECK(GetWidth(a) == 4.25f);
		TEST_CHECK(a.ex == 5.25f);

		SetHeight(a, 9.4f);
		TEST_CHECK(GetHeight(a) == 9.4f);
		TEST_CHECK(a.ey == 11.4f);

		SetX(a, -2.f);
		TEST_CHECK(a.sx == -2.f);
		TEST_CHECK(a.ex == 2.25f);
		
		SetY(a, 0.6f);
		TEST_CHECK(a.sy == 0.6f);
		TEST_CHECK(a.ey == 10.f);

		TEST_CHECK(GetSize(a) == Vec2(GetWidth(a), GetHeight(a)));

		fRect b = OffsetX(a, 0.8f);
		TEST_CHECK(b.sx == -1.2f);
		TEST_CHECK(b.ex == 3.05f);

		fRect c = OffsetY(a, -1.6f);
		TEST_CHECK(c.sy == -1.f);
		TEST_CHECK(c.ey == 8.4f);

		a = Offset(a, 0.8f, -1.6f);
		TEST_CHECK(a.sx == -1.2f);
		TEST_CHECK(a.ex == 3.05f);
		TEST_CHECK(a.sy == -1.f);
		TEST_CHECK(a.ey == 8.4f);
	}
	{
		iRect a(3, 5, 6, 7);
		TEST_CHECK(a.sx == 3);
		TEST_CHECK(a.sy == 5);
		TEST_CHECK(a.ex == 6);
		TEST_CHECK(a.ey == 7);
		TEST_CHECK(GetX(a) == 3);
		TEST_CHECK(GetY(a) == 5);
		TEST_CHECK(GetWidth(a) == 3);
		TEST_CHECK(GetHeight(a) == 2);

		SetWidth(a, 15);
		TEST_CHECK(GetWidth(a) == 15);
		TEST_CHECK(a.ex == 18);

		SetHeight(a, 5);
		TEST_CHECK(GetHeight(a) == 5);
		TEST_CHECK(a.ey == 10);

		SetX(a, 4);
		TEST_CHECK(a.sx == 4);
		TEST_CHECK(a.ex == 19);

		SetY(a, -7);
		TEST_CHECK(a.sy == -7);
		TEST_CHECK(a.ey == -2);

		TEST_CHECK(GetSize(a) == iVec2(GetWidth(a), GetHeight(a)));

		iRect b = OffsetX(a, -2);
		TEST_CHECK(b.sx == 2);
		TEST_CHECK(b.ex == 17);

		iRect c = OffsetY(a, 3);
		TEST_CHECK(c.sy == -4);
		TEST_CHECK(c.ey == 1);

		a = Offset(a, -2, 3);
		TEST_CHECK(a.sx == 2);
		TEST_CHECK(a.ex == 17);
		TEST_CHECK(a.sy == -4);
		TEST_CHECK(a.ey == 1);
	}
	{
		fRect f0;
		f0 = Reorder(fRect(3.f, 2.f, -1.f, 4.f));
		TEST_CHECK(f0.sx == -1.f);
		TEST_CHECK(f0.sy == 2.f);
		TEST_CHECK(f0.ex == 3.f);
		TEST_CHECK(f0.ey == 4.f);

		f0 = Reorder(fRect(-2.f, 11.f, -7.f, 4.f));
		TEST_CHECK(f0.sx == -7.f);
		TEST_CHECK(f0.sy == 4.f);
		TEST_CHECK(f0.ex == -2.f);
		TEST_CHECK(f0.ey == 11.f);

		iRect i0;
		i0 = Reorder(iRect(3, 2, -1, 4));
		TEST_CHECK(i0.sx == -1);
		TEST_CHECK(i0.sy == 2);
		TEST_CHECK(i0.ex == 3);
		TEST_CHECK(i0.ey == 4);

		i0 = Reorder(iRect(-2, 11, -7, 4));
		TEST_CHECK(i0.sx == -7);
		TEST_CHECK(i0.sy == 4);
		TEST_CHECK(i0.ex == -2);
		TEST_CHECK(i0.ey == 11);

		f0 = MakeRectFromWidthHeight(0.5f, -2.6f, 10.f, 4.125f);
		TEST_CHECK(f0.sx == 0.5f);
		TEST_CHECK(f0.sy == -2.6f);
		TEST_CHECK(GetWidth(f0) == 10.f);
		TEST_CHECK(GetHeight(f0) == 4.125f);

		i0 = ToIntRect(f0);
		TEST_CHECK(i0.sx == 0);
		TEST_CHECK(i0.sy == -2);
		TEST_CHECK(GetWidth(i0) == 10);
		TEST_CHECK(GetHeight(i0) == 3);

		i0 = MakeRectFromWidthHeight(17, -30, 4, 120);
		TEST_CHECK(i0.sx == 17);
		TEST_CHECK(i0.sy == -30);
		TEST_CHECK(GetWidth(i0) == 4);
		TEST_CHECK(GetHeight(i0) == 120);

		f0 = ToFloatRect(i0);
		TEST_CHECK(f0.sx == 17.f);
		TEST_CHECK(f0.sy == -30.f);
		TEST_CHECK(GetWidth(f0) == 4.f);
		TEST_CHECK(GetHeight(f0) == 120.f);
	}
	{
		fRect f;
		iRect i;
		f = AlignTop(MakeRectFromWidthHeight(-1.f, 5.f, 2.f, 2.f), 1.4f);
		TEST_CHECK(f.sx == -1.f);
		TEST_CHECK(f.sy == 1.4f);
		TEST_CHECK(f.ex == 1.f);
		TEST_CHECK(f.ey == 3.4f);

		i = AlignTop(MakeRectFromWidthHeight(-1, 5, 2, 2), -2);
		TEST_CHECK(i.sx == -1);
		TEST_CHECK(i.sy == -2);
		TEST_CHECK(i.ex == 1);
		TEST_CHECK(i.ey == 0);

		f = AlignBottom(MakeRectFromWidthHeight(-1.f, 5.f, 2.f, 2.f), 1.4f);
		TEST_CHECK(f.sx == -1.f);
		TEST_CHECK(f.sy == -0.6f);
		TEST_CHECK(f.ex == 1.f);
		TEST_CHECK(f.ey == 1.4f);

		i = AlignBottom(MakeRectFromWidthHeight(-1, 5, 2, 2), -2);
		TEST_CHECK(i.sx == -1);
		TEST_CHECK(i.sy == -4);
		TEST_CHECK(i.ex == 1);
		TEST_CHECK(i.ey == -2);

		f = AlignLeft(MakeRectFromWidthHeight(-1.f, 5.f, 2.f, 2.f), 1.4f);
		TEST_CHECK(f.sx == 1.4f);
		TEST_CHECK(f.sy == 5.f);
		TEST_CHECK(f.ex == 3.4f);
		TEST_CHECK(f.ey == 7.f);

		i = AlignLeft(MakeRectFromWidthHeight(-1, 5, 2, 2), -2);
		TEST_CHECK(i.sx == -2);
		TEST_CHECK(i.sy == 5);
		TEST_CHECK(i.ex == 0);
		TEST_CHECK(i.ey == 7);

		f = AlignRight(MakeRectFromWidthHeight(-1.f, 5.f, 2.f, 2.f), 1.4f);
		TEST_CHECK(f.sx == -0.6f);
		TEST_CHECK(f.sy == 5.f);
		TEST_CHECK(f.ex == 1.4f);
		TEST_CHECK(f.ey == 7.f);

		i = AlignRight(MakeRectFromWidthHeight(-1, 5, 2, 2), -2);
		TEST_CHECK(i.sx == -4);
		TEST_CHECK(i.sy == 5);
		TEST_CHECK(i.ex == -2);
		TEST_CHECK(i.ey == 7);
	}
	{
		fRect f0 = MakeRectFromWidthHeight(-1.f, 5.f, 2.f, 2.f);
		fRect f1 = MakeRectFromWidthHeight( 1.2f, 4.4f, 6.2f, 4.2f);
		TEST_CHECK((f0 == f0) == true);
		TEST_CHECK((f0 == f1) == false);
		TEST_CHECK((f0 != f0) == false);
		TEST_CHECK((f0 != f1) == true);

		iRect i0 = MakeRectFromWidthHeight(1, 5, 2, 2);
		iRect i1 = MakeRectFromWidthHeight(-5, 4, 3, 1);
		TEST_CHECK((i0 == i0) == true);
		TEST_CHECK((i0 == i1) == false);
		TEST_CHECK((i0 != i0) == false);
		TEST_CHECK((i0 != i1) == true);
	}
	{
		fRect f0 = MakeRectFromWidthHeight(-1.f, 5.f, 2.f, 2.f);
		fRect f1 = f0 * 4.f;
		TEST_CHECK(f1.sx == -4.f);
		TEST_CHECK(f1.sy == 20.f);
		TEST_CHECK(f1.ex == 4.f);
		TEST_CHECK(f1.ey == 28.f);

		iRect i0 = MakeRectFromWidthHeight(1, 5, 2, 2);
		iRect i1 = i0 * 3;
		TEST_CHECK(i1.sx == 3);
		TEST_CHECK(i1.sy == 15);
		TEST_CHECK(i1.ex == 9);
		TEST_CHECK(i1.ey == 21);
	}
	{
		fRect f0 = MakeRectFromWidthHeight(-1.f, 5.f, 2.f, 2.f);
		fRect f1 = f0 / 2.f;
		TEST_CHECK(f1.sx == -0.5f);
		TEST_CHECK(f1.sy == 2.5f);
		TEST_CHECK(f1.ex == 0.5f);
		TEST_CHECK(f1.ey == 3.5f);

		iRect i0 = MakeRectFromWidthHeight(1, 5, 2, 2);
		iRect i1 = i0 / 2;
		TEST_CHECK(i1.sx == 0);
		TEST_CHECK(i1.sy == 2);
		TEST_CHECK(i1.ex == 1);
		TEST_CHECK(i1.ey == 3);
	}
	{
		fRect f = MakeRectFromWidthHeight(-2.f, 5.f, 4.f, 2.f);
		TEST_CHECK(Inside(f, Vec2(0.5f, 6.5f)) == true);
		TEST_CHECK(Inside(f, Vec2(-2.4f, 6.5f)) == false);
		TEST_CHECK(Inside(f, Vec2(3.5f, 5.6f)) == false);
		TEST_CHECK(Inside(f, Vec2(1.f, 2.f)) == false);
		TEST_CHECK(Inside(f, Vec2(-0.5f, 11.5f)) == false);
	}
// [todo]
// { bool FitsInside(const Rect<T> &a, const Rect<T> &b); }
// { bool Intersects(const Rect<T> &a, const Rect<T> &b); }
// { Rect<T> Intersection(const Rect<T> &a, const Rect<T> &b); }
// { Rect<T> Union(const Rect<T> &a, const Rect<T> &b); }
// { Rect<T> Grow(const Rect<T> &r, T border); }
// { Rect<T> Crop(const Rect<T> &r, T left, T top, T right, T bottom); }
// { bool Clamp(const Rect<T> &r, O &o); }
// { Rect<T> FitRectByWidth(const Rect<T> &src, const Rect<T> &fit_to); }
// { Rect<T> FitRectByHeight(const Rect<T> &src, const Rect<T> &fit_to); }
// { Rect<T> FitRectByAspectRatio(const Rect<T> &src, const Rect<T> &fit_to); }
	{
		iRect a(-149, 499, -149, 499), b(0, 0, 400, 400);
		TEST_CHECK(Intersects(a, Crop(b, 0, 0, 1, 1)) == false);
	}
}
