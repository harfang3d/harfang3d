#include "config.h"

#include <cstdlib>

#include <foundation/cmd_line.h>
#include <foundation/path_tools.h>
#include <foundation/string.h>

#include <gtest/gtest.h>

std::string g_fbx_converter_path;
std::string g_data_path;

static void OutputUsage(const hg::CmdLineFormat &cmd_format) {
	std::cout << "Usage: fbx_converter " << hg::word_wrap(hg::FormatCmdLineArgs(cmd_format), 80, 21) << std::endl << std::endl;
	std::cout << hg::FormatCmdLineArgsDescription(cmd_format);
}

int main(int argc, char **argv) {
	hg::CmdLineFormat cmd_format;
	cmd_format.singles = {{"-fbx_converter", "path to FBXConverter binary", true}, {"-data_path", "Path to the test data", true}};

	::testing::InitGoogleTest(&argc, argv);

	hg::CmdLineContent cmd_content;
	if (!hg::ParseCmdLine({argv + 1, argv + argc}, cmd_format, cmd_content)) {
		OutputUsage(cmd_format);
		return EXIT_FAILURE;
	}

	std::string default_fbx_converter_path = hg::PathJoin({
		hg::GetCurrentWorkingDirectory(),
#if defined(_WIN32)
			"fbx_converter.exe"
#else
			"fbx_converter"
#endif // _WIN32
	});

	g_fbx_converter_path = hg::GetCmdLineSingleValue(cmd_content, "-fbx_converter", default_fbx_converter_path);
	g_data_path = hg::GetCmdLineSingleValue(cmd_content, "-data_path", hg::GetCurrentWorkingDirectory());

	return RUN_ALL_TESTS();
}
