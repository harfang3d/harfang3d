// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <string>

namespace hg {

bool DeclareLogFile(const std::string &id, const std::string &path);
bool LogToFile(const std::string &id, const std::string &msg);

} // namespace hg
