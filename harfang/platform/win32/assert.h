// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

namespace hg {

// Provide support for Win32 message box based assertion.
void win32_trigger_assert(const char *source, int line, const char *function, const char *condition, const char *message);

} // namespace hg
