// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/thread.h"
//#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <string>

namespace hg {

void set_thread_name(const std::string &name) {}
std::string get_thread_name(std::thread::id id) { return "Unsupported"; }

bool set_thread_priority(std::thread::native_handle_type handle, unsigned int priority) {
	if (!handle)
		return false;

	sched_param param;
	memset(&param, 0, sizeof(param));
	param.sched_priority = priority;

	return pthread_setschedparam(handle, SCHED_OTHER, &param) == 0;
}

bool set_thread_affinity(std::thread::native_handle_type handle, unsigned int mask) { return false; }

int get_system_thread_count() { return int(sysconf(_SC_NPROCESSORS_CONF)); }
int get_system_core_count() { return get_system_thread_count(); }

} // namespace hg

