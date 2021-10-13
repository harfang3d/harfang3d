// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "platform/process.h"
#include "Windows.h"

#include "foundation/log.h"
#include "foundation/format.h"

namespace hg {

static int win32_process_class[6] = {
	IDLE_PRIORITY_CLASS, BELOW_NORMAL_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS, ABOVE_NORMAL_PRIORITY_CLASS, HIGH_PRIORITY_CLASS, REALTIME_PRIORITY_CLASS};

bool set_process_priority(int priority) {
	return priority >= 0 && priority < 6 ? SetPriorityClass(GetCurrentProcess(), win32_process_class[priority]) != 0 : false;
}

bool os_raise_timer_resolution() { return timeBeginPeriod(1) == TIMERR_NOERROR; }
bool os_restore_timer_resolution() { return timeEndPeriod(1) == TIMERR_NOERROR; }

process_id get_current_process_id() { return GetCurrentProcessId(); }

bool is_process_running(process_id id, bool &running) {
	const HANDLE hnd = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, id); // [EJ] might need PROCESS_QUERY_INFORMATION on XP and Server 2003 (https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getexitcodeprocess)

	DWORD exit_code;
	const auto res = GetExitCodeProcess(hnd, &exit_code);

	CloseHandle(hnd); 

	if (!res)
		return false;

	running = exit_code == STILL_ACTIVE;
	return true;
}

} // namespace hg
