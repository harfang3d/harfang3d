// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/time_to_string.h"
#include "foundation/string.h"

#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string.h>

namespace hg {

std::string time_to_string(time_ns v) {
	auto hour = time_to_hour(v);
	auto min = time_to_min(v) % 60;
	auto sec = fmodf(time_to_sec_f(v), 60.f);

	std::ostringstream o;
	o << std::setw(2) << std::setfill('0') << hour << ":" << std::setw(2) << std::setfill('0') << min << ":" << std::fixed << std::setw(6) << std::setfill('0')
	  << std::setprecision(3) << sec;
	return o.str();
}

bool time_from_string(const std::string &v, time_ns &out) {
	const auto vals = split(v, ":");

	float sec = 0.f;
	long hour = 0, min = 0;
	char *eon;

	switch (vals.size()) {
		case 1: {
			sec = strtod(vals[0].c_str(), &eon);
			if (eon == vals[0].c_str() || sec > 59)
				return false;
		} break;

		case 2: {
			min = strtol(vals[0].c_str(), &eon, 10);
			if (eon == vals[0].c_str() || min > 59)
				return false;

			sec = strtod(vals[1].c_str(), &eon);
			if (eon == vals[1].c_str() || sec > 59)
				return false;
		} break;

		case 3: {
			hour = strtol(vals[0].c_str(), &eon, 10);
			if (eon == vals[0].c_str())
				return false;

			min = strtol(vals[1].c_str(), &eon, 10);
			if (eon == vals[1].c_str() || min > 59)
				return false;

			sec = strtod(vals[2].c_str(), &eon);
			if (eon == vals[2].c_str() || sec > 59)
				return false;
		} break;
	}

	out = time_from_sec_d(sec);
	out += time_from_min(min);
	out += time_from_hour(hour);
	return true;
}

//
std::string wall_clock_to_string_ISO8601(time_ns v) {
	std::chrono::nanoseconds d(v);
	std::chrono::system_clock::time_point p(std::chrono::duration_cast<std::chrono::system_clock::duration>(d));

	// std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<int64_t, std::chrono::nanoseconds>> p(d);

	std::time_t now_c = std::chrono::system_clock::to_time_t(p);
	std::tm tm = *std::localtime(&now_c);

	std::stringstream ss;
	ss << std::put_time(&tm, "%FT%T%z");

	return ss.str();
}

std::string wall_clock_to_string_short_ISO8601(time_ns v) {
	std::chrono::nanoseconds d(v);
	std::chrono::system_clock::time_point p(std::chrono::duration_cast<std::chrono::system_clock::duration>(d));

	// std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<int64_t, std::chrono::nanoseconds>> p(d);

	std::time_t now_c = std::chrono::system_clock::to_time_t(p);
	std::tm tm = *std::localtime(&now_c);

	std::stringstream ss;
	ss << std::put_time(&tm, "%T%z");

	return ss.str();
}

//
std::string wall_clock_to_string(time_ns v) {
	std::chrono::nanoseconds d(v);
	std::chrono::system_clock::time_point p(std::chrono::duration_cast<std::chrono::system_clock::duration>(d));

	// std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<int64_t, std::chrono::nanoseconds>> p(d);

	std::time_t now_c = std::chrono::system_clock::to_time_t(p);
	std::tm tm = *std::localtime(&now_c);

	std::stringstream ss;
	ss << std::put_time(&tm, "%x %T");

	return ss.str();
}

std::string wall_clock_to_string_short(time_ns v) {
	std::chrono::nanoseconds d(v);
	std::chrono::system_clock::time_point p(std::chrono::duration_cast<std::chrono::system_clock::duration>(d));

	// std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<int64_t, std::chrono::nanoseconds>> p(d);

	std::time_t now_c = std::chrono::system_clock::to_time_t(p);
	std::tm tm = *std::localtime(&now_c);

	std::stringstream ss;
	ss << std::put_time(&tm, "%T");

	return ss.str();
}

} // namespace hg
