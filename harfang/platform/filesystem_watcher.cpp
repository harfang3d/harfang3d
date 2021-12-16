// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "platform/filesystem_watcher.h"

#include "foundation/dir.h"
#include "foundation/path_tools.h"

#include <array>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

#define ENABLE_HASH_CONFIRMATION 0

#if ENABLE_HASH_CONFIRMATION
#include "foundation/data.h"
#include "foundation/file.h"
#include "foundation/murmur3.h"
#endif

namespace hg {

struct DirectoryWatchPull {
	DirectoryWatchPull(const std::string &path, bool recursive);
	~DirectoryWatchPull();

	std::vector<WatchEvent> GetEvents() {
		std::lock_guard<std::mutex> lock(mutex);
		std::vector<WatchEvent> e(std::move(events));
		return e;
	}

private:
	std::map<std::string, std::vector<DirEntry>> path_entries; // entries for a given path

#if ENABLE_HASH_CONFIRMATION
	using Hash = std::array<uint64_t, 2>;
	std::map<std::string, std::map<std::string, Hash>> path_hashes; // hashes for entries of a given path
#endif

	std::mutex mutex;
	std::vector<WatchEvent> events;

	std::thread thread;
	std::atomic<bool> running{false};

	void Thread(const std::string &path, bool recursive);
	void Update(const std::string &root, const std::string &path, bool recursive);
};

static std::map<std::string, std::unique_ptr<DirectoryWatchPull>> watch_list;

DirectoryWatchPull::DirectoryWatchPull(const std::string &path, bool recursive) : thread(&DirectoryWatchPull::Thread, this, path, recursive) {}

DirectoryWatchPull::~DirectoryWatchPull() {
	if (thread.joinable()) {
		while (running == false) // wait for thread to start
			std::this_thread::yield();

		running = false; // request exit...
		thread.join(); // ...and join
	}
}

void DirectoryWatchPull::Update(const std::string &root, const std::string &path, bool recursive) {
	std::this_thread::sleep_for(std::chrono::milliseconds(4));

	auto new_entries = ListDir(PathJoin({root, path}).c_str());

	const auto &current_entry = path_entries.find(path);

	if (current_entry == std::end(path_entries)) {
#if ENABLE_HASH_CONFIRMATION
		{
			auto &hashes = path_hashes[path];

			for (auto &e : new_entries)
				if (e.type == DE_File) {
					const auto data = FileToData(PathJoin({root, path, e.name}).c_str());
					MurmurHash3_x64_128(data.GetData(), data.GetSize(), 0, &hashes[e.name]);
				}
		}
#endif
		path_entries[path] = std::move(new_entries);
	} else {
		const auto &old_entries = current_entry->second;

		//
		std::map<std::string, DirEntry> old_;
		for (auto &i : old_entries)
			old_[i.name] = i;

		std::map<std::string, DirEntry> new_;
		for (auto &i : new_entries)
			new_[i.name] = i;

		// push WatchEvent
		{
			std::lock_guard<std::mutex> lock(this->mutex);

			// look for modified/deleted entries
			for (auto &i : old_) {
				const auto &j = new_.find(i.first);

				if (j == std::end(new_)) {
					events.push_back({WatchEvent::FileRemoved, PathJoin({path, i.first})});
				} else {
					if (i.second.last_modified != j->second.last_modified) {
#if ENABLE_HASH_CONFIRMATION
						// perform hash-based confirmation
						Hash &path_hash = path_hashes[path][i.first];
						Hash ref_hash = path_hash;

						const auto data = FileToData(PathJoin({root, path, i.first}).c_str());
						MurmurHash3_x64_128(data.GetData(), data.GetSize(), 0, &path_hash);

						if (path_hash != ref_hash)
							events.push_back({WatchEvent::FileModified, PathJoin({path, i.first})});
#else
						events.push_back({WatchEvent::FileModified, PathJoin({path, i.first})});
#endif
					}
				}
			}

			// look for new entries
			for (auto &i : new_) {
				const auto &j = old_.find(i.first);

				if (j == std::end(old_)) {
					events.push_back({WatchEvent::FileAdded, PathJoin({path, i.first})});
				}
			}
		}

		// update state
		current_entry->second = std::move(new_entries);

		//
		if (recursive)
			for (auto &j : current_entry->second)
				if (j.type == DE_Dir)
					Update(root, PathJoin({path, j.name}), recursive);
	}
}

void DirectoryWatchPull::Thread(const std::string &path, bool recursive) {
	for (running = true; running == true;)
		Update(path, "", recursive);
}

//
void WatchDirectory(const std::string &path, bool recursive) {
	const auto &i = watch_list.find(path);
	if (i == std::end(watch_list))
		watch_list[path] = std::make_unique<DirectoryWatchPull>(path, recursive);
}

std::vector<WatchEvent> GetDirectoryWatchEvents(const std::string &path) {
	const auto &i = watch_list.find(path);
	if (i == std::end(watch_list))
		return {};
	return i->second->GetEvents();
}

void UnwatchDirectory(const std::string &path) {
	const auto &i = watch_list.find(path);
	if (i != std::end(watch_list))
		watch_list.erase(i);
}

void UnwatchAllDirectories() { watch_list.clear(); }

} // namespace hg
