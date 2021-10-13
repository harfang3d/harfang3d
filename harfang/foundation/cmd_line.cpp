// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/cmd_line.h"
#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/string.h"

#include <algorithm>
#include <numeric>
#include <sstream>

namespace hg {

bool ParseCmdLine(const std::vector<std::string> &args, const CmdLineFormat &fmt, CmdLineContent &content) {
	for (int i = 0; i < args.size();) {
		// resolve potential alias
		const auto i_alias = fmt.aliases.find(args[i]);
		const auto arg_ = i_alias == std::end(fmt.aliases) ? args[i] : i_alias->second;
		++i;

		// flags
		const auto flag_i = std::find_if(std::begin(fmt.flags), std::end(fmt.flags), [&arg_](const CmdLineEntry &e) { return e.name == arg_; });

		if (flag_i != std::end(fmt.flags)) {
			content.flags.insert(flag_i->name);
			continue;
		}

		// singles
		const auto single_i = std::find_if(std::begin(fmt.singles), std::end(fmt.singles), [&arg_](const CmdLineEntry &e) { return e.name == arg_; });

		if (single_i != std::end(fmt.singles)) {
			if (i == args.size()) {
				log(format("Missing argument to %1").arg(arg_));
				return false;
			}

			content.singles[arg_] = args[i++];
			continue;
		}

		// positionals
		if (content.positionals.size() == fmt.positionals.size()) {
			log("Too many positional arguments");
			return false;
		}

		content.positionals.push_back(arg_);
	}
	return true;
}

static std::string GetNameAndAliases(const std::string &name, const std::map<std::string, std::string> &aliases) {
	std::vector<std::string> es{name};

	for (const auto &e : aliases)
		if (e.second == name)
			es.push_back(e.first);

	return join(std::begin(es), std::end(es), "|");
}

static std::string FormatOptional(const std::string &str, bool optional) {
	static const std::string os("["), cs("]");
	return optional ? os + str + cs : str;
}

std::string FormatCmdLineArgs(const CmdLineFormat &fmt) {
	std::vector<std::string> args;

	for (const auto &p : fmt.singles)
		args.push_back(FormatOptional(format("%1 (val)").arg(GetNameAndAliases(p.name, fmt.aliases)), p.optional));
	for (const auto &p : fmt.flags)
		args.push_back(format("[%1]").arg(GetNameAndAliases(p.name, fmt.aliases)));
	for (const auto &p : fmt.positionals)
		args.push_back(format("<%1>").arg(GetNameAndAliases(p.name, fmt.aliases)));

	return join(std::begin(args), std::end(args), " ");
}

std::string FormatCmdLineArgsDescription(const CmdLineFormat &fmt) {
	std::vector<CmdLineEntry> args;

	std::copy(std::begin(fmt.singles), std::end(fmt.singles), std::back_inserter(args));
	std::copy(std::begin(fmt.flags), std::end(fmt.flags), std::back_inserter(args));
	std::copy(std::begin(fmt.positionals), std::end(fmt.positionals), std::back_inserter(args));

	const int padding = std::accumulate(std::begin(args), std::end(args), 0, [](int v, const CmdLineEntry &e) { return std::max(v, int(e.name.length())); });

	std::string desc;
	for (const auto &arg : args)
		desc += format("%1: %2\n").arg(pad_right(arg.name, padding)).arg(arg.desc).str();

	return desc;
}

//
bool GetCmdLineFlagValue(const CmdLineContent &cmd_content, const std::string &name) { return cmd_content.flags.find(name) != std::end(cmd_content.flags); }

bool CmdLineHasSingleValue(const CmdLineContent &cmd_content, const std::string &name) {
	const auto i = cmd_content.singles.find(name);
	if (i == std::end(cmd_content.singles))
		return false;
	return !i->second.empty();
}

std::string GetCmdLineSingleValue(const CmdLineContent &cmd_content, const std::string &name, const std::string &default_value) {
	const auto i = cmd_content.singles.find(name);
	return i == std::end(cmd_content.singles) ? default_value : i->second;
}

int GetCmdLineSingleValue(const CmdLineContent &cmd_content, const std::string &name, int default_value) {
	const auto i = cmd_content.singles.find(name);
	return i == std::end(cmd_content.singles) ? default_value : std::stoi(i->second);
}

float GetCmdLineSingleValue(const CmdLineContent &cmd_content, const std::string &name, float default_value) {
	const auto i = cmd_content.singles.find(name);
	return i == std::end(cmd_content.singles) ? default_value : std::stof(i->second);
}

} // namespace hg
