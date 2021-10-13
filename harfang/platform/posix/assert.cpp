#include "foundation/string.h"
#include "platform/assert.h"
#include "platform/call_stack.h"
#include <iostream>
#include <sstream>
#include <signal.h>

namespace hg {

void trigger_assert(const char *source, int line, const char *function, const char *condition, const char *message) {
	std::ostringstream description;
	description << "ASSERT(" << condition << ") failed!\n\nFile: " << source << "\nLine " << line << " in function '" << function << "'\n";
	if (message)
		description << "\nDetail: " << message << "\n";

	CallStack callstack;
	CaptureCallstack(callstack);
	description << "\nCallstack:\n"
				<< std::to_string(callstack);

	std::cout << description.str() << "\n";
	raise(SIGTRAP);
}

} // namespace hg

