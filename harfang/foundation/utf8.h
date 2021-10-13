// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

namespace hg {

typedef uint8_t utf8_cp; // UTF-8 codepoint
typedef uint32_t utf32_cp; // UTF-32 codepoint

typedef uint16_t ucs2_t; // UCS2 codepoint

/// Get UTF8 character size in byte.
size_t get_utf8_char_size(const utf8_cp *utf8);
/// Convert a UTF8 character to UTF32, returns the number of consumed bytes.
size_t utf8_to_utf32(const utf8_cp *utf8, utf32_cp &utf32);

/// Return the number of UTF8 characters in a C string buffer.
uint32_t get_utf8_char_count(const utf8_cp *utf8);

/// Convert an UTF8 string to a vector of UTF32 codepoint.
void convert_utf8_to_utf32(const char *utf8, std::vector<utf32_cp> &cps);

/// Convert a UTF32 character to UTF8, returns the number of output bytes.
size_t utf32_to_utf8(utf32_cp utf32, utf8_cp *utf8);

} // namespace hg
