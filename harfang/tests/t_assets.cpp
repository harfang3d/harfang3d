// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include <engine/assets.h>

#include <gtest/gtest.h>

#include "shared.h"

TEST(Assets, PackageOpen) {
	std::string pkg_path = GetResPath("package0000.zip");
	hg::AddAssetsPackage(pkg_path.c_str());

	hg::Asset asset = hg::OpenAsset("0000.txt");
	EXPECT_TRUE(hg::IsValid(asset));
	hg::Close(asset);

	{
		std::string txt = hg::AssetToString("0000.txt");
		EXPECT_STREQ(txt.c_str(), "_TEST_ 0000");
	}
	
	{
		std::string txt = hg::AssetToString("dir00/0000.txt");
		EXPECT_STREQ(txt.c_str(), "test 0000.txt");
	}

	{
		std::string txt = hg::AssetToString("dir00/dir02/0200.txt");
		EXPECT_STREQ(txt.c_str(), "test 0200");
	}

	hg::RemoveAssetsPackage(pkg_path.c_str());
}