// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/cmd_line.h"
#include "foundation/log.h"
#include "platform/window_system.h"
#include "shared.h"
#include "gtest/gtest.h"

#include "foundation/sysinfo.h"

int main(int argc, char **argv) {
	const auto sysinfo = hg::GetSysInfo();

	::testing::InitGoogleTest(&argc, argv);

	hg::CmdLineFormat cmd_format = {};
	cmd_format.singles = {{"-data_path", "Path to unit tests resources", true}};

	hg::CmdLineContent cmd_content;
	if (!hg::ParseCmdLine({argv + 1, argv + argc}, cmd_format, cmd_content)) {
		hg::error("Failed to parse command line.");
		return EXIT_FAILURE;
	}
	unit_resource_path = hg::GetCmdLineSingleValue(cmd_content, "-data_path", unit_resource_path);
	hg::error(unit_resource_path.c_str());

	hg::set_log_level(hg::LL_All);

	hg::WindowSystemInit();
	auto r = RUN_ALL_TESTS();
	return r;
}
