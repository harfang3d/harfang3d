// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/generational_vector_list.h"
#include "foundation/time.h"
#include <functional>

namespace hg {

void start_timer(time_ns resolution = time_from_ms(1));
void stop_timer();

using timer_handle = gen_ref;

timer_handle run_periodic(const std::function<void()> &call, time_ns period, time_ns delay = 0);
timer_handle run_delayed(const std::function<void()> &call, time_ns delay);

void cancel_periodic(timer_handle h);
void cancel_delayed(timer_handle h);

} // namespace hg
