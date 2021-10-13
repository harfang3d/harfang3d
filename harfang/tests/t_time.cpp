// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "gtest/gtest.h"
#include "foundation/time.h"

using namespace hg;

TEST(time, BasicArithmetic) {
	{
		time_ns one(time_from_sec(1)), two(time_from_sec(2)), three(time_from_sec(3));

		EXPECT_TRUE((one + two) == three);
		EXPECT_TRUE((three - two) == one);
	}

	// test normalization
	{
		time_ns one(time_from_ms(1000)), two(time_from_ms(2000)), three(time_from_ms(3000));

		EXPECT_TRUE((one + two) == three) << " with set_ms()";
		EXPECT_TRUE((three - two) == one) << " with set_ms()";
	}

	// multiplication (int)
	int set_of_int[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };

	for (int n = 0; n < 8; ++n) {
		time_ns one(time_from_sec(1)), two(time_from_sec(2)), three(time_from_sec(3));

		one *= set_of_int[n];
		EXPECT_TRUE(one / set_of_int[n] == time_from_sec(1)) << " with multiply by " << set_of_int[n];
		two *= set_of_int[n];
		EXPECT_TRUE(two / set_of_int[n] == time_from_sec(2)) << " with multiply by " << set_of_int[n];
		three *= set_of_int[n];
		EXPECT_TRUE(three / set_of_int[n] == time_from_sec(3)) << " with multiply by " << set_of_int[n];

		EXPECT_TRUE((one + two) == three) << " with multiply by " << set_of_int[n];
		EXPECT_TRUE((three - two) == one) << " with multiply by " << set_of_int[n];
	}

	// multiplication (float)
	float set_of_float[8] = { 0.25f, 0.5f, 2.f, 8.f, 16.f, 32.f, 64.f, 128.f };

	for (int n = 0; n < 8; ++n) {
		time_ns one(time_from_sec(1)), two(time_from_sec(2)), three(time_from_sec(3));

		one *= set_of_float[n];
		EXPECT_TRUE(one / set_of_float[n] == time_from_sec(1)) << " with multiply by " << set_of_float[n];
		two *= set_of_float[n];
		EXPECT_TRUE(two / set_of_float[n] == time_from_sec(2)) << " with multiply by " << set_of_float[n];
		three *= set_of_float[n];
		EXPECT_TRUE(three / set_of_float[n] == time_from_sec(3)) << " with multiply by " << set_of_float[n];

		EXPECT_TRUE((one + two) == three) << " with multiply by " << set_of_float[n];
		EXPECT_TRUE((three - two) == one) << " with multiply by " << set_of_float[n];
	}

	// division (int)
	for (int n = 0; n < 8; ++n) {
		time_ns one(time_from_sec(1)), two(time_from_sec(2)), three(time_from_sec(3));

		one /= set_of_int[n];
		EXPECT_TRUE(one * set_of_int[n] == time_from_sec(1)) << " with divide by " << set_of_int[n];
		two /= set_of_int[n];
		EXPECT_TRUE(two * set_of_int[n] == time_from_sec(2)) << " with divide by " << set_of_int[n];
		three /= set_of_int[n];
		EXPECT_TRUE(three * set_of_int[n] == time_from_sec(3)) << " with divide by " << set_of_int[n];

		EXPECT_TRUE((one + two) == three) << " with divide by " << set_of_int[n];
		EXPECT_TRUE((three - two) == one) << " with divide by " << set_of_int[n];
	}

	// division (float)
	for (int n = 0; n < 8; ++n) {
		time_ns one(time_from_sec(1)), two(time_from_sec(2)), three(time_from_sec(3));

		one /= set_of_float[n];
		EXPECT_TRUE(one * set_of_float[n] == time_from_sec(1)) << " with divide by " << set_of_float[n];
		two /= set_of_float[n];
		EXPECT_TRUE(two * set_of_float[n] == time_from_sec(2)) << " with divide by " << set_of_float[n];
		three /= set_of_float[n];
		EXPECT_TRUE(three * set_of_float[n] == time_from_sec(3)) << " with divide by " << set_of_float[n];

		EXPECT_TRUE((one + two) == three) << " with divide by " << set_of_float[n];
		EXPECT_TRUE((three - two) == one) << " with divide by " << set_of_float[n];
	}
}
