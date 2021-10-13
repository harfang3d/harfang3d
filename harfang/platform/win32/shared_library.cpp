// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "platform/shared_library.h"
#include "platform/platform.h"

#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/string.h"

#include "platform/win32/platform.h"

#define WIN32_LEAN_AND_MEAN
#include <Shlobj.h>
#include <Windows.h>
#include <locale.h>
#include <shellapi.h>

namespace hg {

bool SetSharedLibraryPath(const char *path) { return SetDllDirectoryW((LPCWSTR)utf8_to_utf16(path).c_str()) != 0; }

SharedLib LoadSharedLibrary(const char *path) {
	static_assert(sizeof(SharedLib) >= sizeof(HMODULE), "cannot fit HMODULE in SharedLib structure");

	HMODULE mod = LoadLibraryW((LPCWSTR)utf8_to_utf16(path).c_str());
	if (mod == 0)
		error(format("LoadSharedLibrary('%1') failed, reason: %2").arg(path).arg(GetLastError_Win32()).c_str());

	SharedLib h = reinterpret_cast<uintptr_t>(mod);
	return h;
}

void *GetFunctionPointer(const SharedLib &h, const char *fn) { return GetProcAddress(reinterpret_cast<HMODULE>(h), fn); }

} // namespace hg
