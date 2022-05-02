#include "foundation/string.h"
#include "platform/call_stack.h"

#include <sstream>

#define WINDOWS_LEAN_AND_MEAN
#include <Windows.h>

namespace hg {

void win32_trigger_assert(const char *source, int line, const char *function, const char *condition, const char *message) {
	std::ostringstream description;
	description << "ASSERT(" << condition << ") failed!\n\nFile: " << source << "\nLine " << line << " in function '" << function << "'\n";
	if (message)
		description << "\nDetail: " << message << "\n";

	CallStack callstack;
	CaptureCallstack(callstack);
	description << "\nCallstack:\n" << to_string(callstack);

	MessageBoxW(nullptr, utf8_to_wchar(description.str()).c_str(), L"Assertion failed", MB_ICONSTOP);
#if _DEBUG || MIXED_RELEASE
	DebugBreak();
#endif
}

} // namespace hg
