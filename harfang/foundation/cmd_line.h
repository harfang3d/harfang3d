// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

namespace hg {

struct CmdLineEntry {
	std::string name, desc;
	bool optional{false};
};

struct CmdLineFormat {
	std::vector<CmdLineEntry> flags;
	std::vector<CmdLineEntry> singles;
	std::vector<CmdLineEntry> positionals;

	std::map<std::string, std::string> aliases;
};

//
struct CmdLineContent {
	std::set<std::string> flags;
	std::map<std::string, std::string> singles;
	std::vector<std::string> positionals;
};

// http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap12.html
bool ParseCmdLine(const std::vector<std::string> &args, const CmdLineFormat &format, CmdLineContent &content);

std::string FormatCmdLineArgs(const CmdLineFormat &fmt);
std::string FormatCmdLineArgsDescription(const CmdLineFormat &fmt);

//
bool GetCmdLineFlagValue(const CmdLineContent &cmd_content, const std::string &name);

bool CmdLineHasSingleValue(const CmdLineContent &cmd_content, const std::string &name);

std::string GetCmdLineSingleValue(const CmdLineContent &cmd_content, const std::string &name, const std::string &default_value);
int GetCmdLineSingleValue(const CmdLineContent &cmd_content, const std::string &name, int default_value);
float GetCmdLineSingleValue(const CmdLineContent &cmd_content, const std::string &name, float default_value);

} // namespace hg
