// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "engine/assets.h"

using namespace hg;

void test_assets() {
	std::string pkg_path = "./data/package0000.zip";
	AddAssetsPackage(pkg_path.c_str());

	Asset asset = OpenAsset("0000.txt");
	TEST_CHECK(IsValid(asset) == true);
	Close(asset);

	{
		std::string txt = AssetToString("0000.txt");
		TEST_CHECK(strcmp(txt.c_str(), "_TEST_ 0000") == 0);
	}
	
	{
		std::string txt = AssetToString("dir00/0000.txt");
		TEST_CHECK(strcmp(txt.c_str(), "test 0000.txt") == 0);
	}

	{
		std::string txt = AssetToString("dir00/dir02/0200.txt");
		TEST_CHECK(strcmp(txt.c_str(), "test 0200") == 0);
	}

	RemoveAssetsPackage(pkg_path.c_str());
}