// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/ascii_encoder.h"
#include "foundation/cext.h"
#include "foundation/log.h"

#define FEED_OUT(out_c)                                                                                                                                        \
	{                                                                                                                                                          \
		if (out) {                                                                                                                                             \
			if (olen < max)                                                                                                                                    \
				reinterpret_cast<uint8_t *>(out)[olen] = uint8_t(out_c);                                                                                       \
			else                                                                                                                                               \
				break;                                                                                                                                         \
		}                                                                                                                                                      \
		++olen;                                                                                                                                                \
	}

#define FEED_IN(in_v)                                                                                                                                          \
	{                                                                                                                                                          \
		if (!len) {                                                                                                                                            \
			warn("input buffer underflow");                                                                                                                    \
			break;                                                                                                                                             \
		}                                                                                                                                                      \
		(in_v) = int(*p_in++);                                                                                                                                 \
		--len;                                                                                                                                                 \
	}

/*
	From UUencode wikipedia.
	------------------------

	(...)
	Uuencode repeatedly takes in a group of three bytes, adding trailing zeros
	if there are less than three bytes left. These 24 bits are split into four
	groups of six which are treated as numbers between 0 and 63.
	Decimal 32 is added to each number and they are ouput as ASCII characters
	which will lie in the range 32 (space) to 32+63 = 95 (underscore).
	ASCII characters greater than 95 may also be used; however, only the six
	right-most bits are relevant.
	Each group of sixty output characters (corresponding to 45 input bytes) is
	output as a separate line preceded by an 'M' (ASCII code 77 = 32+45).
	At the end of the input, if there are N output characters left after the
	last group of sixty and N>0 then they will be preceded by the character
	whose code is 32+N.
	(...)
*/

namespace hg {

size_t UUEncode(const void *in, size_t len, void *out, size_t max) {
	size_t olen = 0;
	const auto *p_in = static_cast<const uint8_t *>(in);

	while (len) {
		uint8_t *p = out ? &static_cast<uint8_t *>(out)[olen] : nullptr;
		FEED_OUT(0) // dummy feed

		uint32_t n = 0;
		for (; (n < 15) && len; n++) {
			const auto a = *p_in++;

			uint8_t b, c;
			len--;

			if (len) {
				len--;
				b = *p_in++;
			} else {
				b = 0;
			}

			if (len) {
				len--;
				c = *p_in++;
			} else {
				c = 0;
			}

			uint32_t f = (a << 16) + (b << 8) + c;

			uint8_t z = (f & 63) + 32;
			uint8_t y = ((f >> 6) & 63) + 32;
			uint8_t x = ((f >> 12) & 63) + 32;
			uint8_t w = ((f >> 18) & 63) + 32;

			FEED_OUT(w);
			FEED_OUT(x);
			FEED_OUT(y);
			FEED_OUT(z);
		}

		if (p)
			p[0] = uint8_t(n * 3 + 32);

		FEED_OUT('\n')
	}
	return olen;
}

size_t UUDecode(const void *in, size_t len, void *out, size_t max) {
	size_t olen = 0, n;
	const auto *p_in = reinterpret_cast<const uint8_t *>(in);

	while (len) {
		uint32_t lsize;
		FEED_IN(lsize);
		lsize = (lsize - 32) / 3;

		if (len)
			for (n = 0; n < lsize; n++) {
				int x, y, z, w;
				FEED_IN(w);
				w -= 32;
				FEED_IN(x);
				x -= 32;
				FEED_IN(y);
				y -= 32;
				FEED_IN(z);
				z -= 32;

				uint8_t a, b, c;
				int f = (w << 18) + (x << 12) + (y << 6) + z;
				a = (f >> 16) & 255;
				b = (f >> 8) & 255;
				c = f & 255;

				FEED_OUT(a);
				FEED_OUT(b);
				FEED_OUT(c);
			}

		if (len)
			FEED_IN(n); // line jump
	}
	return olen;
}

/*
	From yEnc.org (revision 1.3)
	----------------------------

	A typical encoding process might look something like this:

	1. Fetch a character from the input stream.
	2. Increment the character's ASCII value by 42, modulo 256
	3. If the result is a critical character (as defined in the previous
	   section), write the escape character to the output stream and increment
	   character's ASCII value by 64, modulo 256.
	4. Output the character to the output stream.
	5. Repeat from start.

	(...)
	Under special circumstances, a single escape character (ASCII 3Dh, "=") is
	used to indicate that the following output character is "critical", and
	requires special handling.

	Critical characters include the following:

	ASCII 00h (nullptr)
	ASCII 0Ah (LF)
	ASCII 0Dh (CR)
	ASCII 3Dh (=)

	> ASCII 09h (TAB)  -- removed in version (1.2)
*/

size_t yEncode(const void *in, size_t len, void *out, size_t max, int line_len) {
	const auto *p_in = reinterpret_cast<const uint8_t *>(in);
	if (line_len <= 0)
		__ERR__(warn("Invalid line-feed size for yEncoding"), 0);

	size_t olen = 0;
	auto cchr = line_len;

	while (len--) {
		// Line-feed.
		if (cchr <= 0) {
			FEED_OUT('\n')
			cchr = int(line_len);
		}

		// yEnc.
		int v = (int(*p_in++) + 42) % 256;

		switch (v) {
			case 0x00:
			case 0x0a:
			case 0x0d:
			case 0x3d:
				FEED_OUT(0x3d)
				cchr--;
				v = (v + 64) % 256;
				break;
		}

		FEED_OUT(v)
		cchr--;
	}
	return olen;
}

size_t yDecode(const void *in, size_t len, void *out, size_t max) {
	const auto *p_in = reinterpret_cast<const uint8_t *>(in);

	size_t olen = 0;
	while (len--) {
		int v = int(*p_in++);

		if (v == 0x0a)
			FEED_IN(v)
		if (v == 0x0d && p_in[0] == 0x0a) { // [EJ support for Windows-style EOL]
			++p_in;
			FEED_IN(v)
		}

		if (v == 0x3d) {
			FEED_IN(v)
			v = (v - 64) % 256;
		}
		FEED_OUT((v - 42) % 256)
	}
	return olen;
}

} // namespace hg
