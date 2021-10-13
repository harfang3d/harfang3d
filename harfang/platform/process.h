// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/math.h"

namespace hg {

using process_id = unsigned long;

/**
	@short Set the process priority from 0 (idle) to 7 (time critical).

	0 - idle, 1 - below normal, 2 - normal, 3 - above normal, 4 - high, 5 - real time
*/
bool set_process_priority(int priority);

/// Raise the OS timer resolution. Note that this might cause power consumption to increase.
bool os_raise_timer_resolution();
/// Restore the OS timer resolution to its default value.
bool os_restore_timer_resolution();

/// Get current process id.
process_id get_current_process_id();
/// Test if a process is running from its process id.
bool is_process_running(process_id id, bool &running);

} // namespace hg
