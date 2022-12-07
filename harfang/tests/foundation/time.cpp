// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN 
#include "acutest.h"

#include "foundation/math.h"
#include "foundation/time.h"

using namespace hg;

void test_time() {
	{
		TEST_CHECK(AlmostEqual(time_to_sec_f(1000000LL), 0.001f));		// 1ms
		TEST_CHECK(AlmostEqual(time_to_sec_f(1000000000LL), 1.f));		// 1s
		TEST_CHECK(AlmostEqual(time_to_sec_f(60000000000LL), 60.f));	// 60s

		TEST_CHECK(AlmostEqual(time_to_ms_f(1000), 0.001f));			// 1탎
		TEST_CHECK(AlmostEqual(time_to_ms_f(1000000LL), 1.f));			// 1ms
		TEST_CHECK(AlmostEqual(time_to_ms_f(1000000000LL), 1000.f));	// 1s

		TEST_CHECK(AlmostEqual(time_to_us_f(1000), 1.f));				// 1탎
		TEST_CHECK(AlmostEqual(time_to_us_f(1000000LL), 1000.f));		// 1ms

		TEST_CHECK(time_to_day(86400000000000LL) == 1);					// 1 day
		TEST_CHECK(time_to_day(1000000000LL) == 0);						// 1s = 0d

		TEST_CHECK(time_to_hour(86400000000000LL) == 24);				// 1 day = 24 hours
		TEST_CHECK(time_to_hour(1000000000LL) == 0);					// 1s = 0h

		TEST_CHECK(time_to_min(3600000000000LL) == 60);					// 1 hour = 60 minutes
		TEST_CHECK(time_to_min(60000000000LL) == 1);					// 1m
		TEST_CHECK(time_to_min(1000000000LL) == 0);						// 1s = 0m

		TEST_CHECK(time_to_sec(60000000000LL) == 60);					// 1 minute = 60 seconds
		TEST_CHECK(time_to_sec(1000000000LL) == 1);						// 1s
		TEST_CHECK(time_to_sec(1000000LL) == 0);						// 1ms = 0s

		TEST_CHECK(time_to_ms(1000000000LL) == 1000);					// 1s = 1000ms
		TEST_CHECK(time_to_ms(1000000LL) == 1);							// 1ms
		TEST_CHECK(time_to_ms(1000LL) ==    0);							// 1탎 = 0ms

		TEST_CHECK(time_to_us(1000000LL) == 1000);						// 1ms = 1000탎
		TEST_CHECK(time_to_us(1000LL) == 1);							// 1탎
		TEST_CHECK(time_to_us(10LL) == 0);								// 10ns = 0탎

		TEST_CHECK(time_to_ns(1000LL) == 1000);							// 1탎 = 1000ns
		TEST_CHECK(time_to_ns(1LL) == 1);								// 1ns

		TEST_CHECK(time_from_sec_d(1.0)   == 1000000000LL);				// 1s
		TEST_CHECK(time_from_sec_d(0.001) == 1000000LL);				// 1ms = 0.001s
		TEST_CHECK(time_from_sec_d(0.000001) == 1000LL);				// 1탎 = 0.000001s
		TEST_CHECK(time_from_sec_d(60.0) == 60000000000LL);				// 1min = 60s

		TEST_CHECK(time_from_ms_d(1000.0) == 1000000000LL);				// 1000ms = 1s
		TEST_CHECK(time_from_ms_d(1.0) == 1000000LL);					// 1ms
		TEST_CHECK(time_from_ms_d(0.001) == 1000LL);					// 1탎 = 0.001ms
		TEST_CHECK(time_from_ms_d(0.000001) == 1LL);					// 1ns = 0.000001ms

		TEST_CHECK(time_from_us_d(1000.0) == 1000000LL);				// 1000탎 = 1ms
		TEST_CHECK(time_from_us_d(1.0) == 1000LL);						// 1탎
		TEST_CHECK(time_from_us_d(0.001) == 1LL);						// 1ns = 0.001탎

		TEST_CHECK(time_from_sec_f(1.f) == 1000000000LL);				// 1s
		TEST_CHECK(time_from_sec_f(0.001f) == 1000000LL);				// 1ms = 0.001s
		TEST_CHECK(Abs(time_from_sec_f(0.0001f) - 100000LL) <= 1);		// 100탎 = 0.0001s
		TEST_CHECK(time_from_sec_f(60.f) == 60000000000LL);				// 1min = 60s

		TEST_CHECK(time_from_ms_f(1000.f) == 1000000000LL);				// 1000ms = 1s
		TEST_CHECK(time_from_ms_f(1.f) == 1000000LL);					// 1ms
		TEST_CHECK(time_from_ms_f(0.001f) == 1000LL);					// 1탎 = 0.001ms
		TEST_CHECK(Abs(time_from_ms_f(0.0001f) - 100LL) <= 1);			// 100ns = 0.0001ms

		TEST_CHECK(time_from_us_f(1000.f) == 1000000LL);				// 1000탎 = 1ms
		TEST_CHECK(time_from_us_f(1.f) == 1000LL);						// 1탎
		TEST_CHECK(time_from_us_f(0.001f) == 1LL);						// 1ns = 0.001탎

		TEST_CHECK(time_from_day(1) == 86400000000000LL);
		TEST_CHECK(time_to_day(time_from_day(1)) == 1);
		
		TEST_CHECK(time_from_hour(24) == time_from_day(1));
		TEST_CHECK(time_from_hour(1) == 3600000000000LL);
		TEST_CHECK(time_to_hour(time_from_hour(1)) == 1);

		TEST_CHECK(time_from_min(60) == 3600000000000LL);
		TEST_CHECK(time_from_min(1) == 60000000000LL);
		TEST_CHECK(time_to_min(time_from_min(1)) == 1);

		TEST_CHECK(time_from_sec(60) == 60000000000LL);
		TEST_CHECK(time_from_sec(1) == 1000000000LL);
		TEST_CHECK(time_to_sec(time_from_sec(1)) == 1);

		TEST_CHECK(time_from_ms(1000) == 1000000000LL);
		TEST_CHECK(time_from_ms(1) == 1000000LL);
		TEST_CHECK(time_to_ms(time_from_ms(1)) == 1);

		TEST_CHECK(time_from_us(1000) == 1000000LL);
		TEST_CHECK(time_from_us(1) == 1000LL);
		TEST_CHECK(time_to_us(time_from_us(1)) == 1);

		TEST_CHECK(time_from_ns(1000) == 1000LL);
		TEST_CHECK(time_from_ns(1) == 1LL);
		TEST_CHECK(time_to_ns(time_from_ns(1)) == 1);
	}

	// simple arythmetic
	{
		time_ns one(time_from_sec(1)), two(time_from_sec(2)), three(time_from_sec(3));

		TEST_CHECK((one + two) == three);
		TEST_CHECK((three - two) == one);
	}

	// test normalization
	{
		time_ns one(time_from_ms(1000)), two(time_from_ms(2000)), three(time_from_ms(3000));

		TEST_CHECK((one + two) == three);
		TEST_CHECK((three - two) == one);
	}

	// multiplication (int)
	int set_of_int[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };

	for (int n = 0; n < 8; ++n) {
		time_ns one(time_from_sec(1)), two(time_from_sec(2)), three(time_from_sec(3));

		one *= set_of_int[n];
		TEST_CHECK(one / set_of_int[n] == time_from_sec(1));
		two *= set_of_int[n];
		TEST_CHECK(two / set_of_int[n] == time_from_sec(2));
		three *= set_of_int[n];
		TEST_CHECK(three / set_of_int[n] == time_from_sec(3));

		TEST_CHECK((one + two) == three);
		TEST_CHECK((three - two) == one);
	}

	// multiplication (float)
	float set_of_float[8] = { 0.25f, 0.5f, 2.f, 8.f, 16.f, 32.f, 64.f, 128.f };

	for (int n = 0; n < 8; ++n) {
		time_ns one(time_from_sec(1)), two(time_from_sec(2)), three(time_from_sec(3));

		one *= set_of_float[n];
		TEST_CHECK(one / set_of_float[n] == time_from_sec(1));
		two *= set_of_float[n];
		TEST_CHECK(two / set_of_float[n] == time_from_sec(2));
		three *= set_of_float[n];
		TEST_CHECK(three / set_of_float[n] == time_from_sec(3));

		TEST_CHECK((one + two) == three);
		TEST_CHECK((three - two) == one);
	}

	// division (int)
	for (int n = 0; n < 8; ++n) {
		time_ns one(time_from_sec(1)), two(time_from_sec(2)), three(time_from_sec(3));

		one /= set_of_int[n];
		TEST_CHECK(one * set_of_int[n] == time_from_sec(1));
		two /= set_of_int[n];
		TEST_CHECK(two * set_of_int[n] == time_from_sec(2));
		three /= set_of_int[n];
		TEST_CHECK(three * set_of_int[n] == time_from_sec(3));

		TEST_CHECK((one + two) == three);
		TEST_CHECK((three - two) == one);
	}

	// division (float)
	for (int n = 0; n < 8; ++n) {
		time_ns one(time_from_sec(1)), two(time_from_sec(2)), three(time_from_sec(3));

		one /= set_of_float[n];
		TEST_CHECK(one * set_of_float[n] == time_from_sec(1));
		two /= set_of_float[n];
		TEST_CHECK(two * set_of_float[n] == time_from_sec(2));
		three /= set_of_float[n];
		TEST_CHECK(three * set_of_float[n] == time_from_sec(3));

		TEST_CHECK((one + two) == three);
		TEST_CHECK((three - two) == one);
	}
}
