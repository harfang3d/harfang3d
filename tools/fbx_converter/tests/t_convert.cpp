#include "config.h"

#include <foundation/dir.h>
#include <foundation/file.h>
#include <foundation/format.h>
#include <foundation/path_tools.h>
#include <foundation/string.h>

#include <cstdlib>
#include <thread>

#include <gtest/gtest.h>
#include <process.hpp>

TEST(fbx_converter, Convert) {
	const auto fbx_dir = hg::PathJoin({FBX_CONVERTER_PATH, "fbx_converter.exe"});
	const auto out_dir = hg::PathJoin({UNIT_TEST_DATA_PATH, "out"});
	const auto cmd = hg::format("%1 -out %2 %3").arg(fbx_dir).arg(out_dir).arg(hg::PathJoin({UNIT_TEST_DATA_PATH, "fbx_material_diffuse_map.fbx"})).str();

	std::string std_out, std_err;

	TinyProcessLib::Process process(
		cmd, hg::GetFilePath(FBX_CONVERTER_PATH), [&](const char *bytes, size_t n) { std_out += std::string(bytes, n); },
		[&](const char *bytes, size_t n) { std_err += std::string(bytes, n); }, false);

	int exit_status;
	int count = 3;
	while (!process.try_get_exit_status(exit_status) && count) {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		--count;
	}

	EXPECT_GT(count, 0);
	EXPECT_EQ(exit_status, EXIT_SUCCESS) << std_out;

	EXPECT_TRUE(hg::IsFile(hg::PathJoin({out_dir, "fbx_material_diffuse_map.scn"}).c_str()));
	EXPECT_TRUE(hg::IsFile(hg::PathJoin({out_dir, "cube_diffuse_color.geo"}).c_str()));
	EXPECT_TRUE(hg::IsFile(hg::PathJoin({out_dir, "diffuse_color.png"}).c_str()));
}
