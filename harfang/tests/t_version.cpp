// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/log.h"
#include "foundation/version.h"
#include "gtest/gtest.h"

using namespace hg;

TEST(Version, Decode) {
	Version out;

	EXPECT_TRUE(decode_version("1", out));
	EXPECT_EQ(out, Version(1, 0, 0));
	EXPECT_TRUE(decode_version("1.0", out));
	EXPECT_EQ(out, Version(1, 0, 0));
	EXPECT_TRUE(decode_version("1.0.0", out));
	EXPECT_EQ(out, Version(1, 0, 0));

	EXPECT_TRUE(decode_version("1.2", out));
	EXPECT_EQ(out, Version(1, 2, 0));
	EXPECT_TRUE(decode_version("1.2.3", out));
	EXPECT_EQ(out, Version(1, 2, 3));

	EXPECT_FALSE(decode_version("", out));
	EXPECT_FALSE(decode_version("2.2.0.1", out));
}

TEST(Version, Encode) {
	std::string out;

	out = encode_version(Version(1, 0, 0));
	EXPECT_EQ(out, "1");
	out = encode_version(Version(1, 2, 0));
	EXPECT_EQ(out, "1.2");
	out = encode_version(Version(1, 2, 3));
	EXPECT_EQ(out, "1.2.3");
}

TEST(Version, Compare) {
	EXPECT_LT(Version(1, 0, 0), Version(2, 0, 0));
	EXPECT_LE(Version(2, 0, 0), Version(2, 0, 0));
	EXPECT_LT(Version(1, 1, 0), Version(1, 2, 0));
	EXPECT_LE(Version(1, 2, 0), Version(1, 2, 0));
	EXPECT_LT(Version(1, 1, 1), Version(1, 1, 2));
	EXPECT_LE(Version(1, 1, 2), Version(1, 1, 2));

	EXPECT_GT(Version(2, 0, 0), Version(1, 0, 0));
	EXPECT_GE(Version(2, 0, 0), Version(2, 0, 0));
	EXPECT_GT(Version(1, 2, 0), Version(1, 1, 0));
	EXPECT_GE(Version(1, 2, 0), Version(1, 2, 0));
	EXPECT_GT(Version(1, 1, 2), Version(1, 1, 1));
	EXPECT_GE(Version(1, 1, 2), Version(1, 1, 2));

	EXPECT_EQ(Version(1, 2, 3), Version(1, 2, 3));
	EXPECT_NE(Version(3, 2, 1), Version(1, 2, 3));

	EXPECT_GT(Version(3, 2, 1), Version(1, 2, 3));
	EXPECT_LT(Version(1, 2, 3), Version(3, 2, 1));
}
