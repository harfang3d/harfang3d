// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "../utils.h"

#include "foundation/file.h"
#include "foundation/path_tools.h"

#include "engine/meta.h"

#include <json/json.hpp>

using namespace hg;

static void test_LoadAndGet() {
	const std::string path = "./data/res/t_meta/in.meta";
	const auto meta = LoadJsonFromFile(path.c_str());

	std::string compression;
	TEST_CHECK(GetMetaValue(meta, "compression", compression, "default") == true);
	TEST_CHECK(compression == "DXTx");

	TEST_CHECK(GetMetaValue(meta, "compression", compression, "bcx") == true);
	TEST_CHECK(compression == "BCx");

	bool generate_mips;
	TEST_CHECK(GetMetaValue(meta, "generate-mips", generate_mips, "default") == true);
	TEST_CHECK(generate_mips == true);

	generate_mips = false;
	TEST_CHECK(GetMetaValue(meta, "generate-mips", generate_mips, "bcx") == true); // should fall back to default
	TEST_CHECK(generate_mips == true);

	TEST_CHECK(GetMetaValue(meta, "non-existent", generate_mips) == false);
}

static void test_LoadAndSet() {
	const std::string path = "./data/res/t_meta/in.meta";
	auto meta = LoadJsonFromFile(path.c_str());

	SetMetaValue(meta, "generate-mips", false, "bcx");

	bool generate_mips;
	TEST_CHECK(GetMetaValue(meta, "generate-mips", generate_mips, "default") == true);
	TEST_CHECK(generate_mips == true);

	TEST_CHECK(GetMetaValue(meta, "generate-mips", generate_mips, "bcx") == true);
	TEST_CHECK(generate_mips == false);

	// create a new key on BCX
	SetMetaValue(meta, "dummy", "key", "bcx");

	std::string dummy;
	TEST_CHECK(GetMetaValue(meta, "dummy", dummy, "default") == false);
	TEST_CHECK(GetMetaValue(meta, "dummy", dummy, "bcx") == true);
	TEST_CHECK(dummy == "key");
}

static void test_Create() {
	const std::string tmp_dir = test::GetTempDirectoryName();
	const std::string out = PathJoin(tmp_dir, "test_meta.db");

	{
		json db;

		SetMetaValue(db, "test_key", 2, "profile");
		SetMetaValue(db, "test_key", 1);

		TEST_CHECK(SaveJsonToFile(db, out.c_str()) == true);
	}

	{
		const auto db = LoadJsonFromFile(out.c_str());

		int v = -1;
		TEST_CHECK(GetMetaValue(db, "test_key", v, "profile") == true);
		TEST_CHECK(v == 2);
		v = -1;
		TEST_CHECK(GetMetaValue(db, "test_key", v) == true);
		TEST_CHECK(v == 1);

		TEST_CHECK(GetMetaValue(db, "unknown", v) == false);
	}

	Unlink(out.c_str());
}

void test_meta() {
	test_LoadAndGet();
	test_LoadAndSet();
	test_Create();
}