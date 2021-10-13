// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/format.h"
#include "foundation/string.h"
#include <sstream>

namespace hg {

std::string to_string(const void *v) {
	std::ostringstream o;
	o << v;
	return o.str();
}

std::string to_string(void *v) { return to_string(reinterpret_cast<const void *>(v)); }

format &format::arg(int v, int width, char fill) {
	std::stringstream ss;
	ss.width(width);
	ss.fill(fill);
	ss << v;
	return replace_next_token(ss.str());
}

format &format::arg(float v, int precision) {
	std::stringstream ss;
	ss.precision(precision);
	ss << v;
	return replace_next_token(ss.str());
}

format &format::replace_next_token(const std::string &by) {
	std::string token = "%";
	token += std::to_string(i++);
	replace_all(text, token, by);
	return *this;
}

} // namespace hg
