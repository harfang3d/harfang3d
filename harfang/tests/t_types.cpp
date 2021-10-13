// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/math.h"
#include "foundation/pack_float.h"
#include "gtest/gtest.h"
#include <cstdint>

using namespace hg;

TEST(Types, PackFloat) {
	float v = 0.25;

	uint8_t u8 = pack_float<uint8_t>(v);
	EXPECT_EQ(63, u8) << "pack_float to uint8_t";

	int8_t i8 = pack_float<int8_t>(v);
	EXPECT_EQ(31, i8) << "pack_float to int8_t";
}
