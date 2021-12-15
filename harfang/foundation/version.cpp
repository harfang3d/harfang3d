// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/version.h"
#include "foundation/string.h"

#include <sstream>
#include <stdexcept>

namespace hg {

bool decode_version(const std::string &version_string, Version &version) {
	auto out = split(version_string, ".");

	auto size = out.size();
	if (!size)
		return false;

	for (size_t i = 0; i < Version::Count; ++i) {
		try {
			version.number[i] = size > i ? std::stoi(out[i]) : 0;
		} catch (std::invalid_argument) { return false; }
	}

	return size <= Version::Count;
}

std::string encode_version(const Version &version) {
	int l = Version::Count;
	for (; l > 0; --l)
		if (version.number[l - 1] != 0)
			break;

	std::ostringstream version_string;
	for (auto i = 0; i < l; ++i) {
		version_string << std::to_string(version.number[i]);
		if (i < l - 1)
			version_string << ".";
	}

	return version_string.str();
}

} // namespace hg
