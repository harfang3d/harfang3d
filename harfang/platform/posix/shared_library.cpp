// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "platform/shared_library.h"

#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/path_tools.h"

#include <string>
#include <memory>

#include <dlfcn.h>

namespace hg {

void *GetFunctionPointer(const SharedLib &lib, const char *name) {
	void *f = dlsym(reinterpret_cast<void*>(lib), name);
	if(!f) {
		 error(format("GetFunctionPointer('%1') failed, reason: %2").arg(name).arg((const char*)dlerror()));
	}
	return f;
}

static std::string shared_library_path;

bool SetSharedLibraryPath(const char *path) {
	shared_library_path = path;
	return true;
}

SharedLib LoadSharedLibrary(const char *path) {
	std::string full_path = PathJoin({shared_library_path, std::string(path)});
	void *handle = dlopen(full_path.c_str(), RTLD_NOW | RTLD_LOCAL);
	if(!handle) {
		// try with system paths or the ones specified in LD_LIBRARY_PATH
		handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
	}
	if (!handle) {
		error(format("LoadSharedLibrary('%1') failed, reason: %2").arg(path).arg((const char*)dlerror()));
	}
	SharedLib h = reinterpret_cast<uintptr_t>(handle);
	return h;
}

} // namespace hg

