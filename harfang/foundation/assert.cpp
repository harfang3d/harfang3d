// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include <cassert>
#include <cstdio>

namespace hg {

static void generic_trigger_assert(const char *, int, const char *, const char *condition, const char *message) {
	printf("Assertion failed (%s)\n", condition ? condition : "");
	if (message)
		printf("	Reason: %s\n", message);
	assert(false);
}

void (*trigger_assert)(const char *source, int line, const char *function, const char *condition, const char *message) = &generic_trigger_assert;

} // namespace hg
