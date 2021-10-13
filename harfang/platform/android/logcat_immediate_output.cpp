// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "platform/android/logcat_immediate_output.h"
#include <android/log.h>

namespace gs {

void logcat_immediate_output(const std::string &, const std::string &msg, const std::string &prefix, const std::string &details, LogLevel::mask_type output_mask, bool output_details) {
	android_LogPriority prio = ANDROID_LOG_INFO;

	if (prefix == "warn")
		prio = ANDROID_LOG_WARN;
	else if (prefix == "error")
		prio = ANDROID_LOG_ERROR;
	else if (prefix == "debug")
		prio = ANDROID_LOG_DEBUG;

	__android_log_print(prio, "harfang", "%s", msg.c_str());

	if (output_details)
		__android_log_print(ANDROID_LOG_VERBOSE, "harfang", "%s", details.c_str());
}

} // gs
