// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/utf8.h"
#include "foundation/assert.h"

namespace hg {

static const utf8_cp first_byte_mark[7] = {0x00, 0x00, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc};

static const utf32_cp max_legal_utf32 = 0x0010ffff;
static const utf32_cp replacement_char = 0x0000fffd;

size_t utf32_to_utf8(utf32_cp utf32, utf8_cp *utf8) {
	size_t bytes_to_write;

	if (utf32 < utf32_cp(0x80)) {
		bytes_to_write = 1;
	} else if (utf32 < utf32_cp(0x800)) {
		bytes_to_write = 2;
	} else if (utf32 < utf32_cp(0x10000)) {
		bytes_to_write = 3;
	} else if (utf32 <= max_legal_utf32) {
		bytes_to_write = 4;
	} else {
		bytes_to_write = 3;
		utf32 = replacement_char;
	}

	switch (bytes_to_write) {
		case 4:
			utf8[3] = utf8_cp((utf32 | 0x80) & 0xbf);
			utf32 >>= 6;
		case 3:
			utf8[2] = utf8_cp((utf32 | 0x80) & 0xbf);
			utf32 >>= 6;
		case 2:
			utf8[1] = utf8_cp((utf32 | 0x80) & 0xbf);
			utf32 >>= 6;
		case 1:
			utf8[0] = utf8_cp(utf32 | first_byte_mark[bytes_to_write]);
	}

	return bytes_to_write;
}

size_t utf8_to_utf32(const utf8_cp *utf8, utf32_cp &utf32) {
	__ASSERT__(utf8);

	auto p = utf8;
	auto length = get_utf8_char_size(utf8);

	switch (length) {
		case 4:
			utf32 = *p ^ 0xf0;
			break;
		case 3:
			utf32 = *p ^ 0xe0;
			break;
		case 2:
			utf32 = *p ^ 0xc0;
			break;
		case 1:
			utf32 = *p;
			break;

		default:
			utf32 = 0;
			break;
	}

	for (size_t n = length; n > 1; --n) {
		++p;
		utf32 <<= 6;
		utf32 |= *p ^ 0x80;
	}

	return length;
}

size_t get_utf8_char_size(const utf8_cp *utf8) {
	__ASSERT__(utf8 != nullptr);

	auto c = *utf8 >> 3;

	// 6 => 0x7e
	// 5 => 0x3e
	if (c == 0x1e)
		return 4;

	c >>= 1;
	if (c == 0xe)
		return 3;
	c >>= 1;
	if (c == 0x6)
		return 2;

	return 1;
}

uint32_t get_utf8_char_count(const utf8_cp *utf8) {
	uint32_t count = 0;
	for (auto p = utf8; p[0]; ++count)
		p += get_utf8_char_size(p);
	return count;
}

void convert_utf8_to_utf32(const char *utf8, std::vector<utf32_cp> &cps) {
	for (auto p = reinterpret_cast<const utf8_cp *>(utf8); *p;) {
		utf32_cp cp;
		p += utf8_to_utf32(p, cp);
		cps.push_back(cp);
	}
}

} // namespace hg
