// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/log.h"
#include "foundation/format.h"
#include "foundation/time_chrono.h"

#include <cstring>
#include <iostream>
#include <sstream>

namespace hg {

static int log_level = ~0;
static bool log_is_detailed = false;

void set_log_level(int level) { log_level = level; }
void set_log_detailed(bool is_detailed) { log_is_detailed = is_detailed; }

//
static void default_log_hook(const char *msg, int mask, const char *details, void *user) {
	if (!(mask & log_level))
		return; // skip masked entries

	const auto now = time_now();
	const auto timestamp =
		format("%1:%2:%3:%4").arg(now / 1000000000).arg(int((now / 1000000) % 1000), 3).arg(int((now / 1000) % 1000), 3).arg(int(now % 1000), 3).str();

	std::ostringstream m;

	m << "(" << timestamp << ") ";
	if (mask & LL_Error)
		m << "ERROR: ";
	else if (mask & LL_Warning)
		m << "WARNING: ";
	else if (mask & LL_Debug)
		m << "DEBUG: ";
	m << msg;

	if (log_is_detailed)
		if (details)
			m << "\n  Details:\n" << details;
	m << std::endl;

	std::cout << m.str();
}

//
static void (*on_log_hook)(const char *msg, int mask, const char *details, void *user) = &default_log_hook;
static void *on_log_hook_user = nullptr;

void set_log_hook(void (*on_log)(const char *msg, int mask, const char *details, void *user), void *user) {
	on_log_hook = on_log ? on_log : &default_log_hook;
	on_log_hook_user = user;
}

//
void log(const char *msg, const char *details) { on_log_hook(msg, LL_Normal, details, on_log_hook_user); }
void warn(const char *msg, const char *details) { on_log_hook(msg, LL_Warning, details, on_log_hook_user); }
void error(const char *msg, const char *details) { on_log_hook(msg, LL_Error, details, on_log_hook_user); }
void debug(const char *msg, const char *details) { on_log_hook(msg, LL_Debug, details, on_log_hook_user); }

} // namespace hg
