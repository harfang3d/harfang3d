// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "shared.h"
#include "gtest/gtest.h"

std::string unit_resource_path("./data");

std::string GetResPath(const char *name) { return unit_resource_path + "/" + name; }

std::string GetOutPath(const char *name) {
	const ::testing::TestInfo *const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
	return unit_resource_path + "/out/gtest_" + test_info->test_case_name() + "_" + test_info->name() + "_" + name;
}
