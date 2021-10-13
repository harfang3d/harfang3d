// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/meta.h"
#include "shared.h"
#include "gtest/gtest.h"

#include <json/json.hpp>

using namespace hg;

TEST(Meta, LoadAndGet) {
	const auto path = GetResPath("res/t_meta/in.meta");
	const auto meta = LoadJsonFromFile(path.c_str());

	std::string compression;
	EXPECT_TRUE(GetMetaValue(meta, "compression", compression, "default"));
	EXPECT_TRUE(compression == "DXTx");

	EXPECT_TRUE(GetMetaValue(meta, "compression", compression, "bcx"));
	EXPECT_TRUE(compression == "BCx");

	bool generate_mips;
	EXPECT_TRUE(GetMetaValue(meta, "generate-mips", generate_mips, "default"));
	EXPECT_EQ(generate_mips, true);

	generate_mips = false;
	EXPECT_TRUE(GetMetaValue(meta, "generate-mips", generate_mips, "bcx")); // should fall back to default
	EXPECT_EQ(generate_mips, true);

	EXPECT_FALSE(GetMetaValue(meta, "non-existent", generate_mips));
}

TEST(Meta, LoadAndSet) {
	const auto path = GetResPath("res/t_meta/in.meta");
	auto meta = LoadJsonFromFile(path.c_str());

	SetMetaValue(meta, "generate-mips", false, "bcx");

	bool generate_mips;
	EXPECT_TRUE(GetMetaValue(meta, "generate-mips", generate_mips, "default"));
	EXPECT_EQ(generate_mips, true);

	EXPECT_TRUE(GetMetaValue(meta, "generate-mips", generate_mips, "bcx"));
	EXPECT_EQ(generate_mips, false);

	// create a new key on BCX
	SetMetaValue(meta, "dummy", "key", "bcx");

	std::string dummy;
	EXPECT_FALSE(GetMetaValue(meta, "dummy", dummy, "default"));
	EXPECT_TRUE(GetMetaValue(meta, "dummy", dummy, "bcx"));
	EXPECT_TRUE(dummy == "key");
}

TEST(Meta, Create) {
	{
		json db;

		SetMetaValue(db, "test_key", 2, "profile");
		SetMetaValue(db, "test_key", 1);

		EXPECT_TRUE(SaveJsonToFile(db, GetOutPath("test_meta.db").c_str()));
	}

	{
		const auto db = LoadJsonFromFile(GetOutPath("test_meta.db").c_str());

		int v = -1;
		EXPECT_TRUE(GetMetaValue(db, "test_key", v, "profile"));
		EXPECT_EQ(v, 2);
		v = -1;
		EXPECT_TRUE(GetMetaValue(db, "test_key", v));
		EXPECT_EQ(v, 1);

		EXPECT_FALSE(GetMetaValue(db, "unknown", v));
	}
}
