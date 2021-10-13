// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "platform/osx/osx_input_system.h"
#include <string>

namespace hg {

bool InitPlatform() { return true; }

bool OpenFileDialog(const std::string &title, const std::string &filter, std::string &OUTPUT, const std::string &initial_dir) { return false; }
bool SaveFileDialog(const std::string &title, const std::string &filter, std::string &OUTPUT, const std::string &initial_dir) { return false; }
bool OpenFolderDialog(const std::string &title, std::string &OUTPUT, const std::string &initial_dir) { return false; }

void DebugBreak() { /* STUB */ }

} // hg
