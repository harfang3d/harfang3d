// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/string.h"
#include "foundation/path_tools.h"
#include "gtest/gtest.h"

using namespace hg;

TEST(PathTools, CleanPath) { 
	std::string out;

	out = CleanPath("./first/second/third/");
	EXPECT_EQ(out, "first/second/third");

	out = CleanPath("../first/second/third/");
	EXPECT_EQ(out, "../first/second/third");

	out = CleanPath("./first/second/third/./fourth");
	EXPECT_EQ(out, "first/second/third/fourth");

	out = CleanPath("./first/../second/third/");
	EXPECT_EQ(out, "second/third");

	out = CleanPath("./first/second/third/../../fourth");
	EXPECT_EQ(out, "first/fourth");

	out = CleanPath("first/second/");
	EXPECT_EQ(out, "first/second");

	out = CleanPath("test/");
	EXPECT_EQ(out, "test");

	out = CleanPath("/test/");
	EXPECT_EQ(out, "/test");

	out = CleanPath("./test/");
	EXPECT_EQ(out, "test");
}

TEST(PathTools, FilePathManipulation) {
	std::string s("c:/unixlikepath/filename.exe");

	EXPECT_TRUE(GetFileExtension(s) == "exe");
	EXPECT_TRUE(GetFilePath(s) == "c:/unixlikepath/");
	EXPECT_TRUE(GetFileName(s) == "filename");

	EXPECT_TRUE(CleanPath(s) == s);

	s = "c:\\winlikepath\\filename.exe";

	EXPECT_TRUE(GetFileExtension(s) == "exe");
	EXPECT_TRUE(GetFilePath(s) == "c:\\winlikepath\\");
	EXPECT_TRUE(GetFileName(s) == "filename");

	EXPECT_TRUE(CleanPath(s) == "c:/winlikepath/filename.exe");

	s = "//redundant_separators///////a/////name.ext";

	EXPECT_TRUE(CleanPath(s) == "/redundant_separators/a/name.ext");

	s = "d:/stuff";

	EXPECT_FALSE(HasFileExtension(s));
}

TEST(PathTools, Factorize) {
	EXPECT_EQ(std::string("../../../c.txt"), FactorizePath("../../../c.txt"));

	EXPECT_EQ(std::string("../c.txt"), FactorizePath("../a/../c.txt"));
	EXPECT_EQ(std::string("../c.txt"), FactorizePath("a/../../c.txt"));
	EXPECT_EQ(std::string("c.txt"), FactorizePath("a/c/../../c.txt"));
	EXPECT_EQ(std::string("../a/c.txt"), FactorizePath("../a/c/../c.txt"));

	EXPECT_EQ(std::string("e/c.txt"), FactorizePath("a/c/../../e/c.txt"));
	EXPECT_EQ(std::string("c.txt"), FactorizePath("a/c/../../e/../c.txt"));
	EXPECT_EQ(std::string("../c.txt"), FactorizePath("../a/c/../../e/../c.txt"));
}
