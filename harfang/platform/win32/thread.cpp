// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/thread.h"
#include "foundation/bit.h"

#include <WTypes.h>
#include <WinBase.h>
#include <process.h>

#include <map>
#include <vector>

namespace hg {

std::map<std::thread::id, std::string> thread_names;

const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO {
	DWORD dwType; // Must be 0x1000.
	LPCSTR szName; // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void set_thread_name(const std::string &name) {
	thread_names[std::this_thread::get_id()] = name;
#if 1
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = name.c_str();
	info.dwThreadID = GetCurrentThreadId();
	info.dwFlags = 0;

#if _HAS_EXCEPTIONS
#pragma warning(push)
#pragma warning(disable : 6320 6322)
	__try {
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR *)&info);
	} __except (EXCEPTION_EXECUTE_HANDLER) {}
#pragma warning(pop)
#endif
#endif // x
}

std::string get_thread_name(std::thread::id id) {
	auto i = thread_names.find(id);
	return i == thread_names.end() ? "Unnamed" : (*i).second;
}

//
static int win32_thread_priority[7] = {THREAD_PRIORITY_IDLE, THREAD_PRIORITY_LOWEST, THREAD_PRIORITY_BELOW_NORMAL, THREAD_PRIORITY_NORMAL,
	THREAD_PRIORITY_ABOVE_NORMAL, THREAD_PRIORITY_HIGHEST, THREAD_PRIORITY_TIME_CRITICAL};

bool set_thread_priority(std::thread::native_handle_type h, int priority) {
	return priority < 7 ? (SetThreadPriority(h, win32_thread_priority[priority]) ? true : false) : false;
}

bool set_thread_affinity(std::thread::native_handle_type h, int mask) { return SetThreadAffinityMask(h, mask) != 0; }

//
static bool GetProcInfo(DWORD &numa_count, DWORD &core_count, DWORD &package_count, DWORD &logic_count) {
	DWORD buffer_length = 0;

	std::vector<char> buffer;
	if (!GetLogicalProcessorInformation(nullptr, &buffer_length)) {
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			return false;

		buffer.resize(buffer_length);
		if (!GetLogicalProcessorInformation(reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION>(buffer.data()), &buffer_length))
			return false;
	}

	numa_count = 0;
	core_count = 0;
	package_count = 0;
	logic_count = 0;

	for (auto p = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION>(buffer.data());
		 static_cast<DWORD>(reinterpret_cast<char *>(p) - buffer.data()) < buffer_length; ++p) {
		switch (p->Relationship) {
			case RelationNumaNode:
				++numa_count; // non-NUMA systems report a single record of this type
				break;

			case RelationProcessorCore:
				++core_count;
				logic_count += count_set_bit(p->ProcessorMask); // a hyper-threaded core supplies more than one logical processor
				break;

			case RelationProcessorPackage:
				++package_count; // logical processors share a physical package
				break;

			default:
				break;
		}
	}
	return true;
}

struct SystemConfig {
	SystemConfig();

	int core_count;
	int hw_thread_count;
};

SystemConfig::SystemConfig() {
	DWORD _numa_count, _core_count, _package_count, _logic_count;
	if (GetProcInfo(_numa_count, _core_count, _package_count, _logic_count)) {
		core_count = _core_count;
		hw_thread_count = _logic_count;
	} else {
		core_count = 1;
		hw_thread_count = 1;
	}
}

static const SystemConfig &GetSystemConfig() {
	static SystemConfig cfg;
	return cfg;
}

int get_system_core_count() { return GetSystemConfig().core_count; }
int get_system_thread_count() { return GetSystemConfig().hw_thread_count; }

} // namespace hg
