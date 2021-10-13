// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/timer.h"
#include "gtest/gtest.h"

#include <atomic>
#include <thread>

using namespace hg;

static std::atomic<uint32_t> v;

void increase_atomic() { v++; }

TEST(timer, PeriodicCall) {
	start_timer();

	v = 0;
	run_periodic(std::bind(&increase_atomic), time_from_ms(200)); // call increase_atomic every 200 ms, starting now

	std::this_thread::sleep_for(std::chrono::milliseconds(1900)); // sleep for little under 2 seconds
	EXPECT_EQ(v, 10); // expect at least 10 calls

	stop_timer();
}

TEST(timer, DelayedCall) {
	start_timer();

	v = 0;
	run_delayed(std::bind(&increase_atomic), time_from_ms(200)); // call increase_atomic in 200 ms

	EXPECT_EQ(0, v);
	std::this_thread::sleep_for(std::chrono::milliseconds(100)); // quick sleep, well below the scheduled call
	EXPECT_EQ(0, v);
	std::this_thread::sleep_for(std::chrono::milliseconds(150)); // quick sleep, well over the scheduled call
	EXPECT_EQ(v, 1);

	stop_timer();
}
