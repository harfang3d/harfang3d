#include <cstdlib>
#include <iostream>
#include <thread>

#include <process.hpp>

#include <foundation/cmd_line.h>
#include <foundation/path_tools.h>
#include <foundation/string.h>
#include <foundation/dir.h>
#include <foundation/file.h>
#include <foundation/format.h>
#include <foundation/log.h>

bool convert(const std::string &fbx_converter_path, const std::string &data_path) {
	const std::string out_dir = hg::PathJoin(data_path, "out");
	const std::string cmd =
		hg::format("%1 -out %2 %3").arg(fbx_converter_path).arg(out_dir).arg(hg::PathJoin({data_path, "fbx_material_diffuse_map.fbx"})).str();

	std::string std_out, std_err;

	TinyProcessLib::Process process(
		cmd, hg::GetFilePath(fbx_converter_path), [&](const char *bytes, size_t n) { std_out += std::string(bytes, n); },
		[&](const char *bytes, size_t n) { std_err += std::string(bytes, n); }, false);

	int exit_status;
	int count = 3;
	while (!process.try_get_exit_status(exit_status) && count) {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		--count;
	}

	if (count <= 0) {
		hg::error("fbx converter failed to respond");
		return false;
	}
	if (exit_status != EXIT_SUCCESS) {
		hg::error("fbx converter failed");
		return false;
	}
	if (!hg::IsFile(hg::PathJoin({ out_dir, "fbx_material_diffuse_map.scn" }).c_str())) {
		hg::error("can't find scene");
		return false;
	}
	if (!hg::IsFile(hg::PathJoin({ out_dir, "cube_diffuse_color.geo" }).c_str())) {
		hg::error("can't find geometry");
		return false;
	}
	if (!hg::IsFile(hg::PathJoin({out_dir, "diffuse_color.png"}).c_str())) {
		hg::error("can't find texture");
		return false;
	}
	return true;
}

static void OutputUsage(const hg::CmdLineFormat &cmd_format) {
	std::cout << "Usage: fbx_converter " << hg::word_wrap(hg::FormatCmdLineArgs(cmd_format), 80, 21) << std::endl << std::endl;
	std::cout << hg::FormatCmdLineArgsDescription(cmd_format);
}

int main(int argc, char **argv) {
	hg::CmdLineFormat cmd_format;
	cmd_format.singles = {{"-fbx_converter", "path to FBXConverter binary", true}, {"-data_path", "Path to the test data", true}};
		
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

	std::string fbx_converter_path = hg::GetCmdLineSingleValue(cmd_content, "-fbx_converter", default_fbx_converter_path);
	std::string data_path = hg::GetCmdLineSingleValue(cmd_content, "-data_path", hg::GetCurrentWorkingDirectory());

	return convert(fbx_converter_path, data_path) ? EXIT_SUCCESS : EXIT_FAILURE;
}
