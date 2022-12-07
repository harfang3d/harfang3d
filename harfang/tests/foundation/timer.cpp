// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/timer.h"

#include <atomic>
#include <thread>

using namespace hg;

static std::atomic<uint32_t> v;

void increase_atomic() { v++; }

void test_timer() {
	{
		start_timer();

		v = 0;
		run_periodic(std::bind(&increase_atomic), time_from_ms(200)); // call increase_atomic every 200 ms, starting now

		std::this_thread::sleep_for(std::chrono::milliseconds(1900)); // sleep for little under 2 seconds
		TEST_CHECK(v == 10); // expect at least 10 calls

		stop_timer();
	}

	{
		start_timer();

		v = 0;
		run_delayed(std::bind(&increase_atomic), time_from_ms(200)); // call increase_atomic in 200 ms

		TEST_CHECK(v == 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(100)); // quick sleep, well below the scheduled call
		TEST_CHECK(v == 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(150)); // quick sleep, well over the scheduled call
		TEST_CHECK(v == 1);

		stop_timer();
	}
}