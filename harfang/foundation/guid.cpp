// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/guid.h"
#include "foundation/rand.h"
#include "foundation/string.h"
#include "foundation/time.h"

#if WIN32
#include <Objbase.h>
#else
#include <uuid/uuid.h>
#endif

namespace hg {

static void byte_to_hex(const uint8_t *bytes, char *out, size_t count) {
	for (size_t n = 0; n < count; ++n) {
		const auto l = bytes[n] & 0xf, h = bytes[n] >> 4;
		*out++ = h > 9 ? 'a' + h - 10 : '0' + h;
		*out++ = l > 9 ? 'a' + l - 10 : '0' + l;
	}
}

static bool hex_to_byte(const char *in, uint8_t *bytes, size_t count) {
	for (size_t n = 0; n < count; ++n) {
		auto h = static_cast<uint8_t>(*in++), l = static_cast<uint8_t>(*in++);

		if (l >= '0' && l <= '9')
			l = l - '0';
		else if (l >= 'a' && l <= 'f')
			l = l - 'a' + 10;
		else
			return false;

		if (h >= '0' && h <= '9')
			h = h - '0';
		else if (h >= 'a' && h <= 'f')
			h = h - 'a' + 10;
		else
			return false;

		*bytes++ = (h << 4) + l;
	}
	return true;
}

//
Guid MakeGuid() {
	Guid guid;
#if WIN32
	CoCreateGuid(reinterpret_cast<GUID *>(guid.data()));
#else
	uuid_generate(guid.data());
#endif
	return guid;
}

Guid MakeGuid(const std::string &guid) {
	Guid v;
	const auto u = tolower(guid);

	if (u.length() == 32) { // no separators
		if (!hex_to_byte(guid.c_str(), v.data(), v.size()))
			v = {}; // reset to non valid guid
	} else if (u.length() == 36) {
		const char *p = u.c_str();
		if (p[8] == '-' && p[13] == '-' && p[18] == '-' && p[23] == '-') {
			if (!hex_to_byte(p, v.data(), 4) || !hex_to_byte(p + 9, v.data() + 4, 2) || !hex_to_byte(p + 14, v.data() + 6, 2) ||
				!hex_to_byte(p + 19, v.data() + 8, 2) || !hex_to_byte(p + 24, v.data() + 10, 6))
				v = {}; // reset to non valid guid
		} else {
			v = {};
		}
	} else {
		v = {};
	}

	return v;
}

bool IsValid(const Guid &guid) { return (guid[6] & 0xf0) == 0x40; }

//
bool operator>(const Guid &a, const Guid &b) {
	for (size_t i = 0; i < a.size(); ++i)
		if (a[i] > b[i])
			return true; // >
		else if (a[i] < b[i])
			return false; // <
	return false; // ==
}

bool operator<(const Guid &a, const Guid &b) {
	for (size_t i = 0; i < a.size(); ++i)
		if (a[i] < b[i])
			return true; // <
		else if (a[i] > b[i])
			return false; // >
	return false; // ==
}

bool operator<=(const Guid &a, const Guid &b) { return !(a > b); }
bool operator>=(const Guid &a, const Guid &b) { return !(a < b); }

//
std::string ToString(const Guid &guid, bool use_separator) {
	char s[37];

	if (use_separator) {
		byte_to_hex(&guid[0], &s[0], 4);
		s[8] = '-';
		byte_to_hex(&guid[4], &s[9], 2);
		s[13] = '-';
		byte_to_hex(&guid[6], &s[14], 2);
		s[18] = '-';
		byte_to_hex(&guid[8], &s[19], 2);
		s[23] = '-';
		byte_to_hex(&guid[10], &s[24], 6);
		s[36] = 0;
	} else {
		byte_to_hex(&guid[0], &s[0], 16);
		s[32] = 0;
	}

	return std::string(s);
}

} // namespace hg
