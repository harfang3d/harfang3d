// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include <foundation/dir.h>
#include <foundation/path_tools.h>

#include <gtest/gtest.h>

#include <thread>
#include <algorithm>

#include "shared.h"

TEST(Dir, ListDirRecursive) {
	std::vector<hg::DirEntry> entries = hg::ListDirRecursive(GetResPath("").c_str());
	EXPECT_FALSE(entries.empty());
	std::string expected = hg::PathJoin({"pic", "owl.jpg"});
	auto i = std::find_if(entries.begin(), entries.end(), [&](const hg::DirEntry &e) { return e.name == expected; });
	EXPECT_NE(i, entries.end());
}

#if 0

#include "platform/filesystem_watcher.h"

TEST(Dir, Watch2) {
	hg::WatchDirectory("d:/sonovision-poc/content", true);

	while (true) {
		const auto events = hg::GetDirectoryWatchEvents("d:/sonovision-poc/content");

		for (const auto &event : events) {
			if (event.type == hg::WatchEvent::FileAdded)
				std::cout << "FILE ADDED: " << event.path << std::endl;
			else if (event.type == hg::WatchEvent::FileRemoved)
				std::cout << "FILE REMOVED: " << event.path << std::endl;
			else if (event.type == hg::WatchEvent::FileModified)
				std::cout << "FILE MODIFIED: " << event.path << std::endl;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	hg::UnwatchAllDirectories();
}

#endif
