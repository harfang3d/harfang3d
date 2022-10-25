// HARFANG(R) Copyright (C) 2019 Emmanuel Julien, Movida Production. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include "../platform.h"
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

enum class Action : int {
	FileOpen = 0,
	FileSave,
	SelectFolder,
	Count
};

std::string GetPlatformLocale() {
	return "C";
}

#pragma message "SDL2 dialogs not implemented for WASM"

bool OpenFolderDialog(const std::string &title, std::string &OUTPUT, const std::string &initial_dir) {
    puts(__FILE__  ":OpenFolderDialog");
	return false;
}

bool OpenFileDialog(const std::string &title, const std::vector<FileFilter> &filters, std::string &output, const std::string &initial_dir) {
    puts(__FILE__ ":OpenFileDialog");
	return false;
}

bool SaveFileDialog(const std::string &title, const std::vector<FileFilter> &filters, std::string &output, const std::string &initial_dir) {
    puts(__FILE__  ":SaveFileDialog");
	return false;
}

void DebugBreak() { }


} // namespace hg
