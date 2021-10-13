// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/rect.h"
#include "gtest/gtest.h"

using namespace hg;

TEST(Rect, Intersects) {
	iRect a(-149, 499, -149, 499), b(0, 0, 400, 400);
	EXPECT_FALSE(Intersects(a, Crop(b, 0, 0, 1, 1)));
}
