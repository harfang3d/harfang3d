// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/format.h"
#include "foundation/string.h"
#include "gtest/gtest.h"

using namespace hg;

TEST(string, CoreFunctionalities) {
	std::string s("Hello world!");

	EXPECT_EQ(12, s.length());
	EXPECT_TRUE(s == "Hello world!");
}

TEST(string, SplitIntoMultipleSubString) {
	std::string s("  This,string ,should,be,*SPLIT ,in,7.");

	auto parts = split(s, ",");
	EXPECT_EQ(7, parts.size());

	EXPECT_TRUE(parts[0] == "  This");
	EXPECT_TRUE(parts[1] == "string ");
	EXPECT_TRUE(parts[2] == "should");
	EXPECT_TRUE(parts[3] == "be");
	EXPECT_TRUE(parts[4] == "*SPLIT ");
	EXPECT_TRUE(parts[5] == "in");
	EXPECT_TRUE(parts[6] == "7.");
}

TEST(string, ReplaceAll) {
	std::string s("Replace all the *# by foo #*#* !");
	EXPECT_TRUE(replace_all(s, "*#", "foo"));
	EXPECT_TRUE(s == "Replace all the foo by foo #foo* !");

	s = "Test end of string incomplete match/";
	EXPECT_FALSE(replace_all(s, "//", "/"));
	EXPECT_TRUE(s == "Test end of string incomplete match/");
}

TEST(string, Lower) {
	std::string i("*MixeD case sTRing to 'conVeRT' [129]*"), o("*mixed case string to 'convert' [129]*");
	EXPECT_EQ(tolower(i), o);
}

TEST(string, Upper) {
	std::string i("*MixeD case sTRing to 'conVeRT' [129]*"), o("*MIXED CASE STRING TO 'CONVERT' [129]*");
	EXPECT_EQ(toupper(i), o);
}

TEST(string, ToUCS2AndBack) {
	std::string in("D:/test.bin_!@#{]]}/*-=This is a TEST,;'");
	auto wide = utf8_to_wchar(in);
	auto narrow = wchar_to_utf8(wide);
	EXPECT_EQ(in, narrow);
}

TEST(string, Slicing) {
	std::string s("Source test string");

	std::string l = left(s, 6);
	EXPECT_TRUE(l == "Source");

	auto r = right(s, 6);
	EXPECT_TRUE(r == "string");
}

TEST(string, ArgBasedFormatting) {
	auto e = format("This is an invalid marker %, this should change to %1").arg(8).str();
	EXPECT_TRUE(e == "This is an invalid marker %, this should change to 8");

	auto a = format("This is number %1").arg(8).str();
	EXPECT_TRUE(a == "This is number 8");

	auto c = format("A few reversed arguments: %3, %2, %1.").arg(8).arg(16).arg(4).str();
	EXPECT_TRUE(c == "A few reversed arguments: 4, 16, 8.");

	auto d = format("Malformed arg % 1.").arg(8).str();
	EXPECT_TRUE(d == "Malformed arg % 1.");
}

TEST(string, Append) {
	std::string a("One ");
	a += "part.";
	EXPECT_TRUE(a == "One part.");

	std::string b("Another ");
	b = b + "part.";
	EXPECT_TRUE(b == "Another part.");
}
