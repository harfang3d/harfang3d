// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/time.h"

namespace hg {

// Reset clock
void reset_clock();

// Call at a constant rate (once per frame usually), return dt since last tick_clock call
time_ns tick_clock();
// Get current clock
time_ns get_clock();
// Get current clock delta since last call
time_ns get_clock_dt();

// Use to ignore an unusually long period without a tick_clock call
void skip_clock();

void set_clock_scale(float scale);
float get_clock_scale();

} // namespace hg
