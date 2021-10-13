// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/log.h"

namespace gs {

// Implement immediate log output to the Android logcat.
void logcat_immediate_output(const std::string &timestamp, const std::string &msg, const std::string &prefix, const std::string &details, LogLevel::mask_type output_mask, bool output_details);

} // gs
