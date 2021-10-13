// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/log.h"

namespace hg {

void SetCrashReportFilename(const char *) { warn("SetCrashReportFilename() is not implemented on this platform"); }
void SetCrashDumpFilename(const char *) { warn("SetCrashDumpFilename() is not implemented on this platform"); }

void InstallCrashHandler() {}

void EnableCrashHandlerFiles(bool enable_crash_report, bool enable_crash_minidump) {}

} // namespace hg

