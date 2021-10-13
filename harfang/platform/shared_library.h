// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <cstdint>

namespace hg {

using SharedLib = uintptr_t;

/// Load a shared library.
SharedLib LoadSharedLibrary(const char *path);

/// Lookup for a function exported by the shared library.
void *GetFunctionPointer(const SharedLib &lib, const char *name);

/// Set the shared library search path.
bool SetSharedLibraryPath(const char *path);

} // namespace hg
