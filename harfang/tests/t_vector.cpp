// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "gtest/gtest.h"
#include "foundation/vector2.h"
#include "foundation/vector3.h"
#include "foundation/vector4.h"

using namespace hg;

TEST(Vec3, Zero)
{
	Vec3	zero(0, 0, 0);
	EXPECT_EQ(0, Len(zero));
}

TEST(Vec2, Mul)
{
	Vec2	a(2.f, 4.f);
	Vec2	b(1.f, 2.f);
	EXPECT_EQ(a, b * 2.f);
	EXPECT_EQ(a, 2.f * b);
}

TEST(Vec3, Mul)
{
	Vec3	a(2.f, 4.f, 6.f);
	Vec3	b(1.f, 2.f, 3.f);
	EXPECT_EQ(a, b * 2.f);
	EXPECT_EQ(a, 2.f * b);
}

TEST(Vec4, Mul)
{
	Vec4	a(3.f, 6.f, 9.f, 12.f);
	Vec4	b(1.f, 2.f, 3.f, 4.f);
	EXPECT_EQ(a, b * 3.f);
	EXPECT_EQ(a, 3.f * b);
}
