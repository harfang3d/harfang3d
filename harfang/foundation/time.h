// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <cstdint>

namespace hg {

using time_ns = int64_t;

extern time_ns time_undefined;

constexpr float time_to_sec_f(time_ns t) { return float(double(t) / 1000000000.0); }
constexpr float time_to_ms_f(time_ns t) { return float(double(t) / 1000000.0); }
constexpr float time_to_us_f(time_ns t) { return float(double(t) / 1000.0); }

constexpr int64_t time_to_day(time_ns t) { return t / (1000000000LL * 60LL * 60LL * 24LL); }
constexpr int64_t time_to_hour(time_ns t) { return t / (1000000000LL * 60LL * 60LL); }
constexpr int64_t time_to_min(time_ns t) { return t / (1000000000LL * 60LL); }
constexpr int64_t time_to_sec(time_ns t) { return t / 1000000000LL; }
constexpr int64_t time_to_ms(time_ns t) { return t / 1000000LL; }
constexpr int64_t time_to_us(time_ns t) { return t / 1000LL; }
constexpr int64_t time_to_ns(time_ns t) { return t; }

constexpr time_ns time_from_sec_d(double sec) { return time_ns(double(sec) * 1000000000.0); }
constexpr time_ns time_from_ms_d(double ms) { return time_ns(double(ms) * 1000000.0); }
constexpr time_ns time_from_us_d(double us) { return time_ns(double(us) * 1000.0); }

constexpr time_ns time_from_sec_f(float sec) { return time_ns(double(sec) * 1000000000.0); }
constexpr time_ns time_from_ms_f(float ms) { return time_ns(double(ms) * 1000000.0); }
constexpr time_ns time_from_us_f(float us) { return time_ns(double(us) * 1000.0); }

constexpr time_ns time_from_day(int64_t day) { return day * (1000000000LL * 60LL * 60LL * 24LL); }
constexpr time_ns time_from_hour(int64_t hour) { return hour * (1000000000LL * 60LL * 60LL); }
constexpr time_ns time_from_min(int64_t min) { return min * (1000000000LL * 60LL); }
constexpr time_ns time_from_sec(int64_t sec) { return sec * 1000000000LL; }
constexpr time_ns time_from_ms(int64_t ms) { return ms * 1000000LL; }
constexpr time_ns time_from_us(int64_t us) { return us * 1000LL; }
constexpr time_ns time_from_ns(int64_t ns) { return ns; }

time_ns time_now();
time_ns wall_clock();

} // namespace hg
