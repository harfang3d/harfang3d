// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/cmd_line.h"
#include "foundation/log.h"

using namespace hg;

void test_cmd_line() {
	CmdLineFormat fmt;

	// flags
	{
		fmt.flags.resize(8);

		fmt.flags[0].name = "-daemon";
		fmt.flags[0].desc = "Run in the background and watch for modifications to the input folder";
		fmt.flags[0].optional = true;

		fmt.flags[1].name = "-progress";
		fmt.flags[1].desc = "Output progress to the standard C output stream";
		fmt.flags[1].optional = true;

		fmt.flags[2].name = "-log_errors_to_stderr";
		fmt.flags[2].desc = "Log errors as JSON to the standard C error output stream";
		fmt.flags[2].optional = true;

		fmt.flags[3].name = "-debug";
		fmt.flags[3].desc = "Compile in debug mode (eg. output debug informations in shader)";
		fmt.flags[3].optional = true;

		fmt.flags[4].name = "-quiet";
		fmt.flags[4].desc = "Disable all build information but errors";
		fmt.flags[4].optional = true;

		fmt.flags[5].name = "-verbose";
		fmt.flags[5].desc = "Output additional information about the compilation process";
		fmt.flags[5].optional = true;

		fmt.flags[6].name = "-fast_check";
		fmt.flags[6].desc = "Perform modification detection using input file timestamp";
		fmt.flags[6].optional = true;

		fmt.flags[7].name = "-no_clean_removed_inputs";
		fmt.flags[7].desc = "Do not remove outputs for removed input files";
		fmt.flags[7].optional = true;
	}

	// singles
	{
		fmt.singles.resize(6);

		fmt.singles[0].name = "-job";
		fmt.singles[0].desc = "Maximum number of parallel job (0 - automatic)";
		fmt.singles[0].optional = true;

		fmt.singles[1].name = "-toolchain";
		fmt.singles[1].desc = "Path to the toolchain folder";
		fmt.singles[1].optional = false;

		fmt.singles[2].name = "-platform";
		fmt.singles[2].desc = "Select the target platform to compile for (defaults to current platform)";
		fmt.singles[2].optional = true;

		fmt.singles[3].name = "-api";
		fmt.singles[3].desc = "Select the platform graphic API to compile for";
		fmt.singles[3].optional = false;

		fmt.singles[4].name = "-defines";
		fmt.singles[4].desc = "Semicolon separated defines to pass to shaderc (eg. FLAG;VALUE=2)";
		fmt.singles[4].optional = true;

		fmt.singles[5].name = "-poll_pid";
		fmt.singles[5].desc = "Poll the provided process and exit assetc if down";
		fmt.singles[5].optional = true;
	}

	// positionals
	{
		fmt.positionals.resize(2);

		fmt.positionals[0].name = "input";
		fmt.positionals[0].desc = "Input folder to compile sources from";
		fmt.positionals[0].optional = false;

		fmt.positionals[1].name = "output";
		fmt.positionals[1].desc = "Output folder for compiled assets";
		fmt.positionals[1].optional = true;
	}

	// aliases
	{
		fmt.aliases["-d"] = "-daemon";
		fmt.aliases["-l"] = "-log_errors_to_stderr";
		fmt.aliases["-j"] = "-job";
		fmt.aliases["-t"] = "-toolchain";
		fmt.aliases["-p"] = "-platform";
		fmt.aliases["-q"] = "-quiet";
		fmt.aliases["-v"] = "-verbose";
		fmt.aliases["-D"] = "-defines";
		fmt.aliases["-f"] = "-fast_check";
		fmt.aliases["-n"] = "-no_clean_removed_inputs";
	}
		
	TEST_CHECK(FormatCmdLineArgs(fmt) ==
			   "[-job|-j (val)] -toolchain|-t (val) [-platform|-p (val)] -api (val) [-defines|-D (val)] [-poll_pid (val)] [-daemon|-d] [-progress] "
			   "[-log_errors_to_stderr|-l] [-debug] [-quiet|-q] [-verbose|-v] [-fast_check|-f] [-no_clean_removed_inputs|-n] <input> <output>");
	{
		const char *description = "-job                    : Maximum number of parallel job (0 - automatic)\n"
								  "-toolchain              : Path to the toolchain folder\n"
								  "-platform               : Select the target platform to compile for (defaults to current platform)\n"
								  "-api                    : Select the platform graphic API to compile for\n"
								  "-defines                : Semicolon separated defines to pass to shaderc (eg. FLAG;VALUE=2)\n"
								  "-poll_pid               : Poll the provided process and exit assetc if down\n"
								  "-daemon                 : Run in the background and watch for modifications to the input folder\n"
								  "-progress               : Output progress to the standard C output stream\n"
								  "-log_errors_to_stderr   : Log errors as JSON to the standard C error output stream\n"
								  "-debug                  : Compile in debug mode (eg. output debug informations in shader)\n"
								  "-quiet                  : Disable all build information but errors\n"
								  "-verbose                : Output additional information about the compilation process\n"
								  "-fast_check             : Perform modification detection using input file timestamp\n"
								  "-no_clean_removed_inputs: Do not remove outputs for removed input files\n"
								  "input                   : Input folder to compile sources from\n"
								  "output                  : Output folder for compiled assets\n";
		TEST_CHECK(FormatCmdLineArgsDescription(fmt) == description);
	}

	std::vector<std::string> args;

	{
		CmdLineContent content;
		args.push_back("-j");
		TEST_CHECK(ParseCmdLine(args, fmt, content) == false);
		args.clear();
	}
	{
		CmdLineContent content;
		args.push_back("-progress");
		args.push_back("-toolchain");
		args.push_back("/path/to/toolchain");
		args.push_back("-j");
		args.push_back("8");
		args.push_back("input_file");
		TEST_CHECK(ParseCmdLine(args, fmt, content) == true);

		TEST_CHECK(GetCmdLineFlagValue(content, "-progress") == true);
		TEST_CHECK(GetCmdLineFlagValue(content, "-bicycle") == false);

		TEST_CHECK(CmdLineHasSingleValue(content, "-toolchain") == true);
		TEST_CHECK(CmdLineHasSingleValue(content, "-job") == true);
		TEST_CHECK(CmdLineHasSingleValue(content, "-x") == false);
		TEST_CHECK(CmdLineHasSingleValue(content, "-mouse") == false);

		TEST_CHECK(GetCmdLineSingleValue(content, "-p", "default_platform") == "default_platform");
		TEST_CHECK(GetCmdLineSingleValue(content, "-toolchain", "default_toolchain") == "/path/to/toolchain");
		TEST_CHECK(GetCmdLineSingleValue(content, "-job", "default_job") == "8");
		TEST_CHECK(GetCmdLineSingleValue(content, "-job", 16) == 8);
		TEST_CHECK(GetCmdLineSingleValue(content, "-poll_id", 0xcaffe) == 0xcaffe);
		TEST_CHECK(GetCmdLineSingleValue(content, "-dog", "fluffy") == "fluffy");
		
		TEST_CHECK(GetCmdLineSingleValue(content, "-threshold", -0.00125f) == -0.00125f);

		content.singles["-threshold"] = "2.5";
		TEST_CHECK(GetCmdLineSingleValue(content, "-threshold", -0.00125f) == 2.5f);
	}

	{
		CmdLineContent content;
		args.push_back("output_file");
		args.push_back("extra_arg");
		TEST_CHECK(ParseCmdLine(args, fmt, content) == false);
	}
}