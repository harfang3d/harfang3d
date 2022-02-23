// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "platform/call_stack.h"
#include "foundation/format.h"
#include <array>
#include <mutex>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <DbgHelp.h>

namespace hg {

struct StackTrace {
	UINT frame_count{};
	std::array<DWORD64, 128> frames{}; // PC-addresses of frames, index 0 contains the topmost frame
};

#ifdef _M_IX86
#pragma optimize("g", off)
#pragma warning(push)
#pragma warning(disable : 4748)
#endif

void CaptureStackTrace(StackTrace &stack_trace, UINT max_frames, CONTEXT *context_record = nullptr) {
	DWORD machine_type;

	if (max_frames > 128)
		max_frames = 128;

	CONTEXT context;

	if (!context_record) {
		ZeroMemory(&context, sizeof(CONTEXT));
		RtlCaptureContext(&context);
		context_record = &context;
	}

	// Set up stack frame.
	STACKFRAME64 StackFrame;
	ZeroMemory(&StackFrame, sizeof(STACKFRAME64));
#ifdef _M_IX86
	machine_type = IMAGE_FILE_MACHINE_I386;
	StackFrame.AddrPC.Offset = context.Eip;
	StackFrame.AddrPC.Mode = AddrModeFlat;
	StackFrame.AddrFrame.Offset = context.Ebp;
	StackFrame.AddrFrame.Mode = AddrModeFlat;
	StackFrame.AddrStack.Offset = context.Esp;
	StackFrame.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
	machine_type = IMAGE_FILE_MACHINE_AMD64;
	StackFrame.AddrPC.Offset = context.Rip;
	StackFrame.AddrPC.Mode = AddrModeFlat;
	StackFrame.AddrFrame.Offset = context.Rsp;
	StackFrame.AddrFrame.Mode = AddrModeFlat;
	StackFrame.AddrStack.Offset = context.Rsp;
	StackFrame.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
	machine_type = IMAGE_FILE_MACHINE_IA64;
	StackFrame.AddrPC.Offset = context.StIIP;
	StackFrame.AddrPC.Mode = AddrModeFlat;
	StackFrame.AddrFrame.Offset = context.IntSp;
	StackFrame.AddrFrame.Mode = AddrModeFlat;
	StackFrame.AddrBStore.Offset = context.RsBSP;
	StackFrame.AddrBStore.Mode = AddrModeFlat;
	StackFrame.AddrStack.Offset = context.IntSp;
	StackFrame.AddrStack.Mode = AddrModeFlat;
#else
#error "Unsupported platform"
#endif

	for (stack_trace.frame_count = 0; stack_trace.frame_count < max_frames; ++stack_trace.frame_count) {
		if (!StackWalk64(
				machine_type, GetCurrentProcess(), GetCurrentThread(), &StackFrame, context_record, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
			break; // maybe it failed or maybe we have finished walking the stack

		if (StackFrame.AddrPC.Offset == 0)
			break; // base reached

		stack_trace.frames[stack_trace.frame_count] = StackFrame.AddrPC.Offset; // valid frame
	}
}

#ifdef _M_IX86
#pragma warning(pop)
#pragma optimize("g", on)
#endif

static std::mutex dbghelp_mutex;

void CaptureCallstack(CallStack &callstack, uint32_t skip_frames, void *context) {
	std::lock_guard<std::mutex> dbghelp_guard(dbghelp_mutex);

	// call SymInitialize() on this process
	HANDLE hprocess = GetCurrentProcess();

	static bool sym_initialized = false;

	if (!sym_initialized) {
		SymSetOptions(SYMOPT_LOAD_LINES);
		SymInitialize(hprocess, NULL, true);
		sym_initialized = true;
	}

	// capture stack trace
	StackTrace trace;
	CaptureStackTrace(trace, 128, (CONTEXT *)context);

	// convert PC addresses to function pointers
	callstack.frames.resize(trace.frame_count - skip_frames);

	++skip_frames; // always skip this call

	uint32_t i = 0;
	for (UINT n = 0; n < trace.frame_count; ++n) {
		if (skip_frames > 0) {
			--skip_frames;
			continue;
		}

		char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
		PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

		pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		pSymbol->MaxNameLen = MAX_SYM_NAME;

		callstack.frames[i].address = (void *)trace.frames[n];

		SymFromAddr(hprocess, trace.frames[n], 0, pSymbol);
		callstack.frames[i].function = pSymbol->Name;

		DWORD dwDisplacement;
		IMAGEHLP_LINE64 line64;
		line64.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
		if (SymGetLineFromAddr64(hprocess, trace.frames[n], &dwDisplacement, &line64)) {
			callstack.frames[i].source = line64.FileName;
			callstack.frames[i].line = line64.LineNumber;
		} else {
			callstack.frames[i].source = "Missing";
			callstack.frames[i].line = 0;
		}

		++i;
	}
}

std::string to_string(const CallStack &callstack) {
	std::string out;
	for (auto &frame : callstack.frames)
		out += format("- %1:%2 in %3 (%4)\n").arg(frame.source).arg(frame.line).arg(frame.function).arg(frame.address).str();
	return out;
}

} // namespace hg
