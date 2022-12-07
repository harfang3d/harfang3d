// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/log.h"
#include "foundation/version.h"

using namespace hg;

void test_version() {
	// Decode
	{
		Version out;

		TEST_CHECK(decode_version("1", out));
		TEST_CHECK(out == Version(1, 0, 0));
		TEST_CHECK(decode_version("1.0", out));
		TEST_CHECK(out == Version(1, 0, 0));
		TEST_CHECK(decode_version("1.0.0", out));
		TEST_CHECK(out == Version(1, 0, 0));

		TEST_CHECK(decode_version("1.2", out));
		TEST_CHECK(out == Version(1, 2, 0));
		TEST_CHECK(decode_version("1.2.3", out));
		TEST_CHECK(out == Version(1, 2, 3));

		TEST_CHECK(decode_version("", out) == false);
		TEST_CHECK(decode_version("2.2.0.1", out) == false);
	}

	// Encode
	{
		std::string out;

		out = encode_version(Version(1, 0, 0));
		TEST_CHECK(out == "1");
		out = encode_version(Version(1, 2, 0));
		TEST_CHECK(out == "1.2");
		out = encode_version(Version(1, 2, 3));
		TEST_CHECK(out == "1.2.3");
	}

	// Compare
	{
		TEST_CHECK(Version(1, 0, 0) < Version(2, 0, 0));
		TEST_CHECK(Version(2, 0, 0) <= Version(2, 0, 0));
		TEST_CHECK(Version(1, 1, 0) < Version(1, 2, 0));
		TEST_CHECK(Version(1, 2, 0) <= Version(1, 2, 0));
		TEST_CHECK(Version(1, 1, 1) < Version(1, 1, 2));
		TEST_CHECK(Version(1, 1, 2) <= Version(1, 1, 2));

		TEST_CHECK(Version(2, 0, 0) > Version(1, 0, 0));
		TEST_CHECK(Version(2, 0, 0) >= Version(2, 0, 0));
		TEST_CHECK(Version(1, 2, 0) > Version(1, 1, 0));
		TEST_CHECK(Version(1, 2, 0) >= Version(1, 2, 0));
		TEST_CHECK(Version(1, 1, 2) > Version(1, 1, 1));
		TEST_CHECK(Version(1, 1, 2) >= Version(1, 1, 2));

		TEST_CHECK(Version(1, 2, 3) == Version(1, 2, 3));
		TEST_CHECK(Version(3, 2, 1) != Version(1, 2, 3));

		TEST_CHECK(Version(3, 2, 1) > Version(1, 2, 3));
		TEST_CHECK(Version(1, 2, 3) < Version(3, 2, 1));
	}
}