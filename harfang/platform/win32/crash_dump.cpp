// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "platform/crash_dump.h"
#include "foundation/build_info.h"
#include "foundation/log.h"
#include "foundation/string.h"
#include "platform/call_stack.h"
#include <cstdint>
#include <fstream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <DbgHelp.h>

namespace hg {

static const char *report_filename = nullptr;
static const char *minidump_filename = nullptr;

static bool write_crash_report = false;
static bool write_crash_minidump = false;

void SetCrashReportFilename(const char *name) { report_filename = name; }
void SetCrashDumpFilename(const char *name) { minidump_filename = name; }

void EnableCrashHandlerFiles(bool enable_crash_report, bool enable_crash_minidump) {
	write_crash_report = enable_crash_report;
	write_crash_minidump = enable_crash_minidump;
}

static bool WriteMiniDump(EXCEPTION_POINTERS *pExceptionInfo, LPCWSTR filename) {
	HANDLE hDump = ::CreateFileW(filename, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	if (hDump == INVALID_HANDLE_VALUE)
		return false;

	MINIDUMP_EXCEPTION_INFORMATION dumpInfo = {0};
	dumpInfo.ClientPointers = TRUE;
	dumpInfo.ExceptionPointers = pExceptionInfo;
	dumpInfo.ThreadId = ::GetCurrentThreadId();

	MINIDUMP_TYPE dumpType = (MINIDUMP_TYPE)(MiniDumpWithPrivateReadWriteMemory | MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules);

	const BOOL success = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDump, dumpType, &dumpInfo, 0, 0);
	CloseHandle(hDump);

	return success == TRUE;
}

static const char *GetExceptionString(DWORD exc) {
#define EXC_CASE(EXC)                                                                                                                                          \
	case EXCEPTION_##EXC:                                                                                                                                      \
		return "EXCEPTION_" #EXC

	switch (exc) {
		EXC_CASE(ACCESS_VIOLATION);
		EXC_CASE(DATATYPE_MISALIGNMENT);
		EXC_CASE(BREAKPOINT);
		EXC_CASE(SINGLE_STEP);
		EXC_CASE(ARRAY_BOUNDS_EXCEEDED);
		EXC_CASE(FLT_DENORMAL_OPERAND);
		EXC_CASE(FLT_DIVIDE_BY_ZERO);
		EXC_CASE(FLT_INEXACT_RESULT);
		EXC_CASE(FLT_INVALID_OPERATION);
		EXC_CASE(FLT_OVERFLOW);
		EXC_CASE(FLT_STACK_CHECK);
		EXC_CASE(FLT_UNDERFLOW);
		EXC_CASE(INT_DIVIDE_BY_ZERO);
		EXC_CASE(INT_OVERFLOW);
		EXC_CASE(PRIV_INSTRUCTION);
		EXC_CASE(IN_PAGE_ERROR);
		EXC_CASE(ILLEGAL_INSTRUCTION);
		EXC_CASE(NONCONTINUABLE_EXCEPTION);
		EXC_CASE(STACK_OVERFLOW);
		EXC_CASE(INVALID_DISPOSITION);
		EXC_CASE(GUARD_PAGE);
		EXC_CASE(INVALID_HANDLE);
		default:
			return "UNKNOWN";
	}
}

static std::string GetTempFilename(const char *prefix) {
	WCHAR temp_path[MAX_PATH + 1];
	GetTempPathW(MAX_PATH, temp_path);

	WCHAR filename[MAX_PATH + 1];
	GetTempFileNameW(temp_path, utf8_to_wchar(prefix).c_str(), 0, filename);

	return wchar_to_utf8(filename);
}

LONG WINAPI TopLevelExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo) {
	// output dump
	std::string _minidump_filename = minidump_filename ? minidump_filename : GetTempFilename("hfg_dump");

	if (write_crash_minidump)
		WriteMiniDump(pExceptionInfo, utf8_to_wchar(_minidump_filename).c_str());

	// output report
	std::string _report_filename = report_filename ? report_filename : GetTempFilename("hfg_crash_report");

	if (write_crash_report) {
#if WIN32
		std::ofstream f(utf8_to_wchar(_report_filename).c_str(), std::ios::out | std::ios::trunc);
#else
		std::ofstream f(_report_filename.c_str(), std::ios::out | std::ios::trunc);
#endif

		if (!f.is_open())
			return EXCEPTION_CONTINUE_SEARCH;

		// Harfang core
		f << "Harfang crash dump" << std::endl;
		f << std::endl;
		f << "Build SHA: " << get_build_sha() << std::endl;
		f << "Core: " << get_version_string() << std::endl;

		f << std::endl;

		// exception info
		char module_file_name[MAX_PATH + 1];
		GetModuleFileName(NULL, module_file_name, MAX_PATH);

		f << "Process: " << module_file_name << std::endl;
		f << std::hex;

		f << "Reason: 0x" << pExceptionInfo->ExceptionRecord->ExceptionCode << " - " << GetExceptionString(pExceptionInfo->ExceptionRecord->ExceptionCode)
		  << std::endl;
		f << " at segment 0x" << pExceptionInfo->ContextRecord->SegCs << " address 0x" << pExceptionInfo->ExceptionRecord->ExceptionAddress << std::endl;

		if (pExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
			switch (pExceptionInfo->ExceptionRecord->ExceptionInformation[0]) {
				case 0:
					f << "Attempt to read from 0x" << pExceptionInfo->ExceptionRecord->ExceptionInformation[1] << std::endl;
					break;
				case 1:
					f << "Attempt to write to 0x" << pExceptionInfo->ExceptionRecord->ExceptionInformation[1] << std::endl;
					break;
				case 8:
					f << "User-mode data execution prevention at 0x" << pExceptionInfo->ExceptionRecord->ExceptionInformation[1] << std::endl;
					break;
			}

		const DWORD threadId = GetCurrentThreadId();
		f << "Thread ID: 0x" << threadId << " [" << std::dec << threadId << "]" << std::endl;

		f << std::endl;

		// OS info
		OSVERSIONINFOEX sysInfo;
		memset(&sysInfo, 0, sizeof(sysInfo));
		sysInfo.dwOSVersionInfoSize = sizeof(sysInfo);
		GetVersionEx((OSVERSIONINFO *)&sysInfo);

		f << "Operating System:" << std::endl;

		if (sysInfo.dwMajorVersion == 6 && sysInfo.dwMinorVersion == 0) {
			if (sysInfo.wProductType != VER_NT_WORKSTATION)
				f << "Windows Server 2008";
			else
				f << "Windows Vista";
		} else if (sysInfo.dwMajorVersion == 5) {
			if (sysInfo.dwMinorVersion == 2)
				f << "Windows Server 2003";
			else if (sysInfo.dwMinorVersion == 1)
				f << "Windows XP";
			else if (sysInfo.dwMinorVersion == 0)
				f << "Windows 2000";
		} else {
			f << "Windows version " << sysInfo.dwMajorVersion << "." << sysInfo.dwMinorVersion;
		}
		f << std::endl;

		f << std::endl;

		// system info
		MEMORY_BASIC_INFORMATION info;
		unsigned char *address = nullptr;

		SIZE_T bytesInfo = ::VirtualQuery(address, &info, sizeof(info));
		SIZE_T total_free = 0, largest_free = 0, total_reserved = 0, total_committed = 0;

		while (bytesInfo != 0) {
			if (info.State & MEM_FREE) {
				total_free += info.RegionSize;
				if (info.RegionSize > largest_free)
					largest_free = info.RegionSize;
			} else {
				if (info.State & MEM_RESERVE)
					total_reserved += info.RegionSize;
				if (info.State & MEM_COMMIT)
					total_committed += info.RegionSize;
			}

			address += info.RegionSize;
			memset(&info, 0, sizeof(info));
			bytesInfo = ::VirtualQuery(address, &info, sizeof(info));
		}

		f << std::dec;
		f << "Free memory total: " << total_free << std::endl;
		f << "Free memory largest region: " << largest_free << std::endl;
		f << "Total memory reserved: " << total_reserved << std::endl;
		f << "Total memory committed: " << total_committed << std::endl;

		f << std::endl;

		// CPU info
		if (!IsBadReadPtr(pExceptionInfo, sizeof(EXCEPTION_POINTERS))) {
			const CONTEXT *ctx = pExceptionInfo->ContextRecord;
			f << "Registers:" << std::endl;

			char str[1024];

#ifdef _WIN64
			sprintf(str,
				"EAX=%08zX EBX=%08zX ECX=%08zX EDX=%08zX\n"
				"ESI=%08zX EDI=%08zX EBP=%08zX ESP=%08zX EIP=%08zX\n"
				"FLG=%08zX CS=%04X DS=%04X SS=%04X ES=%04X FS=%04X GS=%04X\n",
				ctx->Rax, ctx->Rbx, ctx->Rcx, ctx->Rdx, ctx->Rsi, ctx->Rdi, ctx->Rbp, ctx->Rsp, ctx->Rip, ctx->EFlags, ctx->SegCs, ctx->SegDs, ctx->SegSs,
				ctx->SegEs, ctx->SegFs, ctx->SegGs);
#else
			sprintf(str,
				"EAX=%08X EBX=%08X ECX=%08X EDX=%08X\n"
				"ESI=%08X EDI=%08X EBP=%08X ESP=%08X EIP=%08X\n"
				"FLG=%08X CS=%04X DS=%04X SS=%04X ES=%04X FS=%04X GS=%04X\n",
				ctx->Eax, ctx->Ebx, ctx->Ecx, ctx->Edx, ctx->Esi, ctx->Edi, ctx->Ebp, ctx->Esp, ctx->Eip, ctx->EFlags, ctx->SegCs, ctx->SegDs, ctx->SegSs,
				ctx->SegEs, ctx->SegFs, ctx->SegGs);
#endif

			f << str;
		}

		f << std::endl;

		// callstack info
		CallStack callstack;
		CaptureCallstack(callstack, 0, pExceptionInfo->ContextRecord);

		f << "Callstack:" << std::endl << to_string(callstack) << std::endl;

		// link to mini-dump
		f << "Associated dump: " << _minidump_filename << std::endl;

		f.close();
	}

	// display user message
	std::string msg("Harfang core has crashed!\n\n");

	msg += std::string("Version: ") + get_version_string() + ", build SHA: " + get_build_sha() + "\n\n";
	if (write_crash_report || write_crash_minidump) {
		msg += "A complete report has been generated and saved to the following files:\n\n";

		if (write_crash_report)
			msg += "* Report: " + _report_filename + "\n";
		if (write_crash_minidump)
			msg += "* Dump: " + _minidump_filename + "\n";
	}
	MessageBoxW(nullptr, utf8_to_wchar(msg).c_str(), L"Fatal error", MB_ICONSTOP);

	return EXCEPTION_CONTINUE_SEARCH;
}

void InstallCrashHandler() { SetUnhandledExceptionFilter(TopLevelExceptionHandler); }

} // namespace hg
