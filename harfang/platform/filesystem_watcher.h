// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <string>
#include <vector>

namespace hg {

struct WatchEvent {
	enum Type { FileAdded, FileRemoved, FileModified };

	Type type;
	std::string path;

	bool operator==(const WatchEvent &e) const { return e.type == type && e.path == path; }
};

void WatchDirectory(const std::string &path, bool recursive);
void UnwatchDirectory(const std::string &path);
void UnwatchAllDirectories();

std::vector<WatchEvent> GetDirectoryWatchEvents(const std::string &path);

} // namespace hg
