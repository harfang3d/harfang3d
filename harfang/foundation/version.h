// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <string>

namespace hg {

struct Version {
	enum Element { Major, Minor, Patch, Count };

	Version() = default;

	Version(int major, int minor, int patch) {
		number[0] = major;
		number[1] = minor;
		number[2] = patch;
	}

	bool operator==(const Version &v) const {
		for (auto i = 0; i < Count; ++i)
			if (number[i] != v.number[i])
				return false;
		return true;
	}

	bool operator!=(const Version &v) const { return !(*this == v); }

	bool operator<(const Version &v) const {
		for (auto i = 0; i < Count; ++i) {
			if (number[i] < v.number[i])
				return true;
			if (number[i] > v.number[i])
				return false;
		}
		return false;
	}

	bool operator>(const Version &v) const {
		for (auto i = 0; i < Count; ++i) {
			if (number[i] > v.number[i])
				return true;
			if (number[i] < v.number[i])
				return false;
		}
		return false;
	}

	bool operator<=(const Version &v) const {
		for (auto i = 0; i < Count; ++i) {
			if (number[i] < v.number[i])
				return true;
			if (number[i] > v.number[i])
				return false;
		}
		return true;
	}

	bool operator>=(const Version &v) const {
		for (auto i = 0; i < Count; ++i) {
			if (number[i] > v.number[i])
				return true;
			if (number[i] < v.number[i])
				return false;
		}
		return true;
	}

	int number[Count]{};
};

/// Decode a version string to a version structure.
bool decode_version(const std::string &version_string, Version &version_out);
/// Encode a version structure to a string.
std::string encode_version(const Version &version);

} // namespace hg
