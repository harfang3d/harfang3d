// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "platform/process.h"
#include <sys/resource.h>
#include <signal.h>
#include <unistd.h>

namespace hg {

static int posix_process_priority[6] = {20, 10, 0, -5, -10, -20};

bool set_process_priority(int priority) {
	return (priority >= 0 && priority < 6) ? ::setpriority(PRIO_PROCESS, 0, posix_process_priority[priority]) == 0 : false;
}

bool os_raise_timer_resolution() { return true; }
bool os_restore_timer_resolution() { return true; }

process_id get_current_process_id() {
	return getpid();
}

bool is_process_running(process_id id, bool &running) {
	pid_t pid = id;
	const auto res = kill(pid, 0);
	if (res != 0)
		return false; // cannot determine state
	running = res == 0;
	return true;
}

} // namespace hg
