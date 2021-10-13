// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/unit.h"

namespace hg {

std::string FormatTime(time_ns t) {
	std::ostringstream str;

	const auto ms = time_to_ms(t);
	const auto sec = time_to_sec(t);
	const auto min = time_to_min(t);
	const auto hour = time_to_hour(t);

	if (ms < 1000)
		str << ms << " ms"; // eg. 750 ms
	else if (sec < 60)
		str << sec << " sec " << (ms % 1000) << " ms"; // eg. 1 sec 102 ms
	else if (min < 60)
		str << min << " min " << (sec % 60) << " sec " << (ms % 1000) << " ms"; // eg. 7 min 13 sec 12 ms
	else
		str << hour << " hour " << (min % 60) << " min " << (sec % 60) << " sec " << (ms % 1000) << " ms"; // eg. 2 hour 7 min 13 sec 12 ms

	return str.str();
}

} // namespace hg
