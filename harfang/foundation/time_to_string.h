// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/time.h"

#include <string>

namespace hg {

std::string time_to_string(time_ns t);
bool time_from_string(const std::string &v, time_ns &out);

std::string wall_clock_to_string(time_ns t);
std::string wall_clock_to_string_short(time_ns v);
std::string wall_clock_to_string_ISO8601(time_ns v);
std::string wall_clock_to_string_short_ISO8601(time_ns v);

} // namespace hg
