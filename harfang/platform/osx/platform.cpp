// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "platform/platform.h"
#include <string>

namespace hg {

bool InitPlatform() { return true; }

bool OpenFolderDialog(const std::string &title, std::string &output, const std::string &initial_dir) {
	return false; 
}

bool OpenFileDialog(const std::string &title, const std::vector<hg::FileFilter> &filters, std::string &output, const std::string &initial_dir) { 
	return false; 
}

bool SaveFileDialog(const std::string &title, const std::vector<hg::FileFilter> &filters, std::string &output, const std::string &initial_dir) { 
	return false; 
}

void DebugBreak() { /* STUB */ }

} // hg
