// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/endian.h"
#include <cstdint>

namespace hg {

Endianness GetHostEndianness() {
	union T {
		uint32_t i;
		char c[4];
	};
	static T t{0x01020304};

	return t.c[0] == 1 ? BigEndian : LittleEndian;
}

} // namespace hg
