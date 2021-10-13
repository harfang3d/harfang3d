// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/named_parm_string.h"
#include "foundation/string.h"

namespace hg {

static const std::vector<std::string> what = {"\t", "\r", "\n"};
static const std::vector<std::string> by = {"", "", ""};

void CleanNamedParmString(std::string &s) {
	normalize_eol(s);
	replace_all(s, what, by);
	s = trim(s);
}

NamedParmArray ParseNamedParmString(const std::string &parm_string, const std::string &parm_separator, const std::string &key_value_separator) {
	NamedParmArray parm_array; // keep here to help NRVO

	auto parms = split(parm_string, parm_separator);

	for (auto &parm : parms) {
		auto kv = split(parm, key_value_separator);

		if (kv.size() == 2) {
			if (kv[0].empty() && kv[1].empty())
				continue;
			parm_array.push_back({kv[0], kv[1]});
		} else {
			if (parm.empty())
				continue;
			parm_array.push_back({parm, {}});
		}

		CleanNamedParmString(parm_array.back().name);
		CleanNamedParmString(parm_array.back().value);
	}

	return parm_array;
}

} // namespace hg
