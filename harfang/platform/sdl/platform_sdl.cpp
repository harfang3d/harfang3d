// HARFANG(R) Copyright (C) 2019 Emmanuel Julien, Movida Production. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/assert.h"
#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/path_tools.h"
#include "foundation/string.h"
#include "platform/input_system.h"

namespace hg {

bool InitPlatform() {
	return true;
}

std::string GetPlatformLocale() {
	return "fr";
}

bool OpenFolderDialog(const std::string &title, std::string &OUTPUT, const std::string &initial_dir) {
	return false;
}

bool OpenFileDialog(const std::string &title, const std::string &filter, std::string &OUTPUT, const std::string &initial_dir) {
	return false;
}

bool SaveFileDialog(const std::string &title, const std::string &filter, std::string &OUTPUT, const std::string &initial_dir) {
	return false;
}

void DebugBreak() { }


} // namespace hg
