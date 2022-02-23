// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include <cstdlib>
#if !defined ANDROID && !defined EMSCRIPTEN
#include <execinfo.h>
#endif
#include "foundation/format.h"
#include "platform/call_stack.h"

namespace hg {

#if defined ANDROID || defined EMSCRIPTEN
void CaptureCallstack(CallStack &callstack, unsigned int skip_frames, void *) {}
#else
void CaptureCallstack(CallStack &callstack, uint skip_frames, void *) {
	++skip_frames; // skip this call from the collected trace

	void *frames[64];
	int frame_count = backtrace(frames, 64);

	char **symbols = backtrace_symbols(frames, frame_count);

	callstack.frames.resize(frame_count - skip_frames);

	uint i = 0;
	for (uint n = skip_frames; n < frame_count; ++n) {
		callstack.frames[i].function = symbols[n];
		callstack.frames[i].source = "nil";
		callstack.frames[i].line = 0;
		++i;
	}

	free(symbols);
}
#endif

} // namespace hg

namespace std {

string to_string(const hg::CallStack &callstack) {
	string out;
	for (auto &frame : callstack.frames)
		out += hg::format("%1:%2 in %3\n").arg(frame.function).arg(frame.line).arg(frame.source).str();
	return out;
}

} // namespace std
