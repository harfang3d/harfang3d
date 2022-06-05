// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

namespace hg {

enum LogLevel { LL_Normal = 0x1, LL_Warning = 0x2, LL_Error = 0x4, LL_Debug = 0x8, LL_All = 0xff };

void log(const char *msg, const char *details = nullptr);
void warn(const char *msg, const char *details = nullptr);
void error(const char *msg, const char *details = nullptr);
void debug(const char *msg, const char *details = nullptr);

void set_log_hook(void (*on_log)(const char *msg, int mask, const char *details, void *user), void *user);

/// Set log levels filter.
void set_log_level(int log_level);
/// Enable detailed log output.
void set_log_detailed(bool is_detailed);

} // namespace hg
