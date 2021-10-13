// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/time.h"
#include <chrono>

namespace hg {

std::chrono::seconds time_to_chrono_sec(time_ns t);
std::chrono::milliseconds time_to_chrono_ms(time_ns t);
std::chrono::microseconds time_to_chrono_us(time_ns t);
std::chrono::nanoseconds time_to_chrono_ns(time_ns t);
std::chrono::nanoseconds time_to_chrono(time_ns t);

} // namespace hg
