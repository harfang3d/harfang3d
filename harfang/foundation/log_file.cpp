// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/file.h"

#include <map>
#include <mutex>

namespace hg {

static std::map<std::string, std::string> id_to_path;
static std::mutex mutex;

bool DeclareLogFile(const std::string &id, const std::string &path) {
	std::lock_guard<std::mutex> lock(mutex);
	id_to_path[id] = path;

	const auto file = OpenWriteText(path.c_str());
	bool result = IsValid(file);
	Close(file);

	return result;
}

bool LogToFile(const std::string &id, const std::string &msg) {
	std::lock_guard<std::mutex> lock(mutex);

	const auto i = id_to_path.find(id);
	if (i == std::end(id_to_path))
		return false;

	const auto file = OpenAppendText(i->second.c_str());
	const auto result = WriteStringAsText(file, msg + "\n");
	Close(file);

	return result;
}

} // namespace hg
