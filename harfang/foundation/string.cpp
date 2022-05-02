// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/string.h"
#include "foundation/assert.h"
#include "foundation/utf8-cpp/utf8.h"
#include "foundation/utf8.h"

#include <algorithm>
#include <cctype>
#include <locale>
#include <sstream>

#include <codecvt>

namespace hg {

/*
 * Copyright (c) 1999 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 */

/*
 * Routine to see if a text string is matched by a wildcard pattern.
 * Returns TRUE if the text is matched, or FALSE if it is not matched
 * or if the pattern is invalid.
 *  *		matches zero or more characters
 *  ?		matches a single character
 *  [abc]	matches 'a', 'b' or 'c'
 *  \c		quotes character c
 *  Adapted from code written by Ingo Wilken.
 */
bool match_wildcard(const char *text, const char *pattern) {
	if (text == nullptr)
		return false;

	const char *retryPat = nullptr;
	const char *retryText = text;

	bool found;
	while (*text || *pattern) {
		int ch = *pattern++;

		switch (ch) {
			case '*':
				retryPat = pattern;
				retryText = text;
				break;

			case '[':
				found = false;

				while ((ch = *pattern++) != ']') {
					if (ch == '\\')
						ch = *pattern++;

					if (ch == '\0')
						return false;

					if (*text == ch)
						found = true;
				}

				if (!found) {
					pattern = retryPat;
					text = ++retryText;
				}
				// fall into next case

			case '?':
				if (*text++ == '\0')
					return false;
				break;

			case '\\':
				ch = *pattern++;

				if (ch == '\0')
					return false;
				// fall into next case

			default:
				if (*text == ch) {
					if (*text)
						text++;
					break;
				}

				if (*text) {
					pattern = retryPat;
					text = ++retryText;
					break;
				}
				return false;
		}

		if (!pattern)
			return false;
	}
	return true;
}

std::string slice(const std::string &str, ptrdiff_t from, ptrdiff_t count) {
	auto l = str.length();

	if (from < 0)
		from += l; // start from right of std::string

	if (from < 0)
		from = 0; // clamp to start

	if (count == 0)
		count = l - from; // all characters left
	else if (count < 0)
		count += l - from; // all characters left - count

	if (count > (ptrdiff_t(l) - from))
		count = l - from; // clamp to length

	return count > 0 ? str.substr(from, count) : "";
}

std::string left(const std::string &str, size_t count) { return slice(str, 0, count); }
std::string right(const std::string &str, size_t count) { return slice(str, -(ptrdiff_t)count, 0); }

size_t replace_all(std::string &value, const std::string &what, const std::string &by) {
	auto what_len = what.length(), by_len = by.length();

	int count = 0;
	for (std::string::size_type i = 0; (i = value.find(what, i)) != std::string::npos;) {
		value.replace(i, what_len, by);
		i += by_len;
		++count;
	}

	return count;
}

bool contains(const std::string &in, const std::string &what) { return in.find(what) != std::string::npos; }

std::vector<std::string> split(const std::string &value, const std::string &separator, const std::string &trim) {
	std::vector<std::string> elements; // keep here to help NVRO
	elements.reserve(8);

	auto value_length = value.length();
	auto separator_length = separator.length();
	auto trim_length = trim.length();

	for (std::string::size_type s = 0, i = 0; i != std::string::npos;) {
		i = value.find(separator, i);

		if (i == std::string::npos) {
			auto v = value.substr(s);
			if (!v.empty())
				elements.push_back(std::move(v));
			break;
		} else {
			std::string element(value.substr(s, i - s));
			if (trim_length)
				replace_all(element, trim, "");
			elements.push_back(std::move(element));
		}

		i += separator_length;
		s = i;
		if (i >= value_length)
			i = std::string::npos;
	}

	return elements;
}

std::string trim(const std::string &str, const std::string &pattern) {
	const auto str_begin = str.find_first_not_of(pattern);
	if (str_begin == std::string::npos)
		return ""; // no content

	const auto str_end = str.find_last_not_of(pattern);
	const auto str_range = str_end - str_begin + 1;

	return str.substr(str_begin, str_range);
}

std::string reduce(const std::string &str, const std::string &fill, const std::string &pattern) {
	// trim first
	auto result = trim(str, pattern);

	// replace sub ranges
	auto begin_space = result.find_first_of(pattern);
	while (begin_space != std::string::npos) {
		const auto endSpace = result.find_first_not_of(pattern, begin_space);
		const auto range = endSpace - begin_space;

		result.replace(begin_space, range, fill);

		const auto newStart = begin_space + fill.length();
		begin_space = result.find_first_of(pattern, newStart);
	}

	return result;
}

//
std::string lstrip(const std::string &str, const std::string &pattern) {
	const auto str_begin = str.find_first_not_of(pattern);

	if (str_begin == std::string::npos)
		return str;

	return str.substr(str_begin, str.length() - str_begin);
}

std::string rstrip(const std::string &str, const std::string &pattern) {
	const auto str_end = str.find_last_not_of(pattern);

	if (str_end == std::string::npos)
		return str;

	return str.substr(0, str_end + 1);
}

std::string strip(const std::string &str, const std::string &pattern) { return lstrip(rstrip(str, pattern), pattern); }

//
std::string lstrip_space(const std::string &str) {
	size_t i = 0;
	for (; i < str.size(); ++i)
		if (!std::isspace(str[i]))
			break;
	return slice(str, i);
}

std::string rstrip_space(const std::string &str) {
	auto i = str.size();
	for (; i > 0; --i)
		if (!std::isspace(str[i - 1]))
			break;
	return slice(str, 0, i);
}

std::string strip_space(const std::string &str) { return lstrip_space(rstrip_space(str)); }

//
std::string strip_prefix(const std::string &str, const std::string &prefix) { return starts_with(str, prefix) ? slice(str, prefix.length()) : str; }
std::string strip_suffix(const std::string &str, const std::string &suffix) { return ends_with(str, suffix) ? slice(str, 0, -int(suffix.length())) : str; }

//
std::string utf32_to_utf8(const std::u32string &str) {
	std::vector<unsigned char> utf8string;
	utf8::utf32to8(str.begin(), str.end(), std::back_inserter(utf8string));
	return std::string((char *)utf8string.data(), utf8string.size());
}

std::u32string utf8_to_utf32(const std::string &str) {
	std::vector<char32_t> utf32string;
	utf8::utf8to32(str.begin(), str.end(), std::back_inserter(utf32string));
	return std::u32string(utf32string.data(), utf32string.size());
}

std::string wchar_to_utf8(const std::wstring &str) {
	std::wstring_convert<std::codecvt_utf8<wchar_t>> wcv;
	return wcv.to_bytes(str);
}

std::wstring utf8_to_wchar(const std::string &str) {
	std::wstring_convert<std::codecvt_utf8<wchar_t>> wcv;
	return wcv.from_bytes(str);
}

std::wstring ansi_to_wchar(const std::string &str) {
	std::wstring ret;
	std::mbstate_t state = {};
	const char *src = str.data();
	size_t len = std::mbsrtowcs(nullptr, &src, 0, &state);
	if (static_cast<size_t>(-1) != len) {
		std::unique_ptr<wchar_t[]> buff(new wchar_t[len + 1]);
		len = std::mbsrtowcs(buff.get(), &src, len, &state);
		if (static_cast<size_t>(-1) != len) {
			ret.assign(buff.get(), len);
		}
	}
	return ret;
}

std::string ansi_to_utf8(const std::string &string) { return wchar_to_utf8(ansi_to_wchar(string)); }

//
void tolower_inplace(std::string &str, size_t start, size_t end) {
	transform(std::begin(str) + start, end ? std::begin(str) + end : std::end(str), std::begin(str) + start,
		[](char c) -> char { return c >= 65 && c <= 90 ? c + (97 - 65) : c; });
}

std::string tolower(std::string str, size_t start, size_t end) {
	tolower_inplace(str, start, end);
	return str;
}

void toupper_inplace(std::string &str, size_t start, size_t end) {
	transform(std::begin(str) + start, end ? std::begin(str) + end : std::end(str), std::begin(str) + start,
		[](char c) -> char { return c >= 97 && c <= 122 ? c - (97 - 65) : c; });
}

std::string toupper(std::string str, size_t start, size_t end) {
	toupper_inplace(str, start, end);
	return str;
}

void normalize_eol(std::string &inplace_normalize, EOLConvention eol_convention) {
	if (eol_convention == EOLUnix)
		replace_all(inplace_normalize, "\r\n", "\n");
	else
		replace_all(inplace_normalize, "\n", "\r\n");
}

std::string word_wrap(const std::string &str, int width, int lead, char lead_char) {
	if (width < 1)
		width = 1;

	std::string o;
	o.reserve(str.length() + (str.length() / width) * (lead + 1));

	const std::string lead_str(lead, lead_char);

	int n = width;
	for (auto &c : str) {
		if (c == '\n') {
			o += c;
			o += lead_str;
			n = width;
		} else {
			const bool is_split_possible = (c == ' ') || (c == ';');

			if (is_split_possible && n <= 0) {
				if (c != ' ')
					o += c;
				o += '\n';
				o += lead_str;
				n = width;
			} else {
				o += c;
				--n;
			}
		}
	}
	return o;
}

std::string name_to_path(std::string name) {
	name = tolower(name);
	static const std::vector<std::string> blacklist = {" ", "\\", "/", "!", "@"};
	for (const auto &s : blacklist)
		replace_all(name, s, "-");
	return name;
}

std::string pad_left(const std::string &str, int padded_width, char padding_char) {
	int count = padded_width - int(str.size());
	if (count <= 0)
		return str;
	return std::string(count, padding_char) + str;
}

std::string pad_right(const std::string &str, int padded_width, char padding_char) {
	int count = padded_width - int(str.size());
	if (count <= 0)
		return str;
	return str + std::string(count, padding_char);
}

} // namespace hg
