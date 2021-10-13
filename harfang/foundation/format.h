// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <string>

namespace hg {

std::string to_string(void *v);
std::string to_string(const void *v);

inline std::string to_string(const std::string &v) { return v; }
inline std::string to_string(const char *v) { return v ? v : "(nullptr)"; }

template <typename T> std::string to_string(T v) { return std::to_string(v); }

/// String formatting class
struct format {
	explicit format(std::string _text) : text(std::move(_text)) {}
	explicit format(const char *_text) : text(_text) {}

	operator const std::string &() const { return text; }
	operator const char *() const { return text.c_str(); }

	const char *c_str() const { return text.c_str(); }
	const std::string &str() const { return text; }

	template <typename T> format &arg(const T &v) { return replace_next_token(to_string(v)); }

	format &arg(int v, int width, char fill = '0');
	format &arg(float v, int precision);

private:
	format &replace_next_token(const std::string &by);

	std::string text;
	int i{1};
};

} // namespace hg
