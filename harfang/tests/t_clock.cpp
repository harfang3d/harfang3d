// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/clock.h"
#include "foundation/log.h"
#include "platform/process.h"
#include "gtest/gtest.h"

#include <chrono>
#include <thread>

using namespace hg;

TEST(Clock, Update) {
	EXPECT_TRUE(os_raise_timer_resolution());

	reset_clock();

	EXPECT_EQ(0, get_clock());
	EXPECT_EQ(1, get_clock_dt());

	tick_clock();
	std::this_thread::sleep_for(std::chrono::milliseconds(16));
	tick_clock();

	EXPECT_LE(15000, time_to_us(get_clock_dt()));

	for (int n = 0; n < 16; ++n) {
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
		tick_clock();
	}

	EXPECT_TRUE(os_restore_timer_resolution());
}

#if 0

TEST(Clock, StableDelta) {
	EXPECT_TRUE(os_raise_timer_resolution());

	Clock clock;

	auto t = time::now();
	for (uint i = 0; i < 100; ++i) {
		this_thread::sleep_for(16);

		clock.Update();
		auto clock_dt = clock.GetDelta().to_us();

		// tolerate a 5ms drift max
		EXPECT_GE(clock_dt, 15000);
		EXPECT_LE(clock_dt, 20000);
	}

	EXPECT_TRUE(os_restore_timer_resolution());
}

#endif