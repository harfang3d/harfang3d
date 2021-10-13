// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <string>
#include <vector>

namespace hg {

struct NamedParm {
	NamedParm() = default;
	NamedParm(std::string name_, std::string value_) : name(std::move(name_)), value(std::move(value_)) {}
	NamedParm(NamedParm &&p) : name(std::move(p.name)), value(std::move(p.value)) {}
	std::string name, value;
};

typedef std::vector<NamedParm> NamedParmArray;

/**
	@short Clean a parameter name or value string.

	Strip the string left and right of empty characters, and normalize end of
	line characters.
*/
void CleanNamedParmString(std::string &str);

/**
	@short Parse a string of named parameters

	The string is expected to embed parameters in the following format:
	<name_0:value_0>,<name_1:value_1>,...

	@note The name and value will be cleaned when extracted.
*/
NamedParmArray ParseNamedParmString(const std::string &parm_str, const std::string &parm_separator = ",", const std::string &key_value_separator = ":");

} // namespace hg
