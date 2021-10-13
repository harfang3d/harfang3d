// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/timer.h"
#include "foundation/thread.h"
#include "foundation/time_chrono.h"

#include <atomic>
#include <mutex>

namespace hg {

static std::mutex timer_mutex;

//
struct periodic_call {
	std::function<void()> call;
	time_ns period, delay;
};

static generational_vector_list<periodic_call> periodic_calls;

timer_handle run_periodic(const std::function<void()> &call, time_ns period, time_ns delay) {
	std::lock_guard<std::mutex> guard(timer_mutex);
	return periodic_calls.add_ref({call, period, delay});
}

void cancel_periodic(timer_handle h) {
	std::lock_guard<std::mutex> guard(timer_mutex);
	periodic_calls.remove_ref(h);
}

//
struct delayed_call {
	std::function<void()> call;
	time_ns delay;
};

static generational_vector_list<delayed_call> delayed_calls;

timer_handle run_delayed(const std::function<void()> &call, time_ns delay) {
	std::lock_guard<std::mutex> guard(timer_mutex);
	return delayed_calls.add_ref({call, delay});
}

void cancel_delayed(timer_handle h) {
	std::lock_guard<std::mutex> guard(timer_mutex);
	delayed_calls.remove_ref(h);
}

//
static std::thread timer_thread;
static std::atomic<bool> timer_thread_running;

static void timer_thread__(time_ns resolution) {
	set_thread_name("Harfang - timer thread");
	// set_thread_affinity(native_handle(), 1); // lock to the first core (causes mouse cursor drop in Windows with high log activity)

	time_ns last_t = time_now(), elapsed_t = 0;

	while (timer_thread_running) {
		timer_mutex.lock();

		// execute and drop elapsed delayed calls
		for (uint32_t i = delayed_calls.first(); i != 0xffffffff;) {
			auto &c = delayed_calls[i];

			c.delay -= elapsed_t;
			if (c.delay <= 0) {
				c.call();
				i = delayed_calls.remove(i);
			} else {
				i = delayed_calls.next(i);
			}
		}

		// execute periodic calls
		for (auto &c : periodic_calls) {
			c.delay -= elapsed_t;
			while (c.delay <= 0) {
				c.call();
				c.delay += c.period; // reset delay
			}
		}

		// compute elapsed time
		auto now = time_now();
		elapsed_t = now - last_t;
		if (elapsed_t < 0) // typical bullying
			elapsed_t = 0;
		last_t = now;

		timer_mutex.unlock();

		//
		std::this_thread::sleep_for(time_to_chrono(resolution));
	}

	std::lock_guard<std::mutex> guard(timer_mutex);

	periodic_calls.clear();
	delayed_calls.clear();
}

void start_timer(time_ns resolution) {
	if (!timer_thread.joinable()) {
		timer_thread_running = true;
		timer_thread = std::thread(timer_thread__, resolution);
	}
}

void stop_timer() {
	if (timer_thread.joinable()) {
		timer_thread_running = false;
		timer_thread.join();
	}
}

} // namespace hg
