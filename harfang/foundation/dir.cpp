// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/dir.h"
#include "foundation/cext.h"
#include "foundation/file.h"
#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/path_tools.h"
#include "foundation/rand.h"
#include "foundation/string.h"

#include <sys/stat.h>
#if _WIN32
#include <direct.h>
#define WIN32_LEAN_AND_MEAN
#include "platform/win32/platform.h"
#include <Windows.h>
#include <shlwapi.h>
#else /* POSIX */
#include <dirent.h>
#include <unistd.h>
#define _unlink unlink
#endif

#undef CopyFile

namespace hg {

std::vector<DirEntry> ListDir(const char *path, int mask) {
	std::vector<DirEntry> entries;

#if _WIN32
	WIN32_FIND_DATAW data;

	auto wfind_path = utf8_to_wchar(PathJoin({path, "*.*"}));
	auto hFind = FindFirstFileW(wfind_path.c_str(), &data);
	if (hFind == INVALID_HANDLE_VALUE)
		return entries;

	do {
		auto name = wchar_to_utf8(data.cFileName);
		if (name == "." || name == "..")
			continue;

		const auto type = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? DE_Dir : DE_File;
		const auto last_modified = time_to_ns(((uint64_t(data.ftLastWriteTime.dwHighDateTime) << 32) + data.ftLastWriteTime.dwLowDateTime) * 100);

		LARGE_INTEGER size;
		size.HighPart = data.nFileSizeHigh;
		size.LowPart = data.nFileSizeLow;

		if (mask & type)
			entries.push_back({type, std::move(name), last_modified, numeric_cast<size_t>(size.QuadPart)});
	} while (FindNextFileW(hFind, &data));

	FindClose(hFind);
#else /* POSIX */
	auto dir = opendir(path);
	if (!dir)
		return entries;

	while (auto ent = readdir(dir)) {
		if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
			continue;

		DirEntry entry;

		int type{};

		if (ent->d_type == DT_DIR)
			type = DE_Dir;
		else if (ent->d_type == DT_REG)
			type = DE_File;
		else if (ent->d_type == DT_LNK)
			type = DE_Link;

		// TODO: stat() missing infos
		if (mask & type)
			entries.push_back({type, ent->d_name});
	}

	closedir(dir);
#endif
	return entries;
}

std::vector<DirEntry> ListDirRecursive(const char *path, int mask) {
	std::vector<DirEntry> entries = ListDir(path, mask);
#if _WIN32
	WIN32_FIND_DATAW data;
	auto wfind_path = utf8_to_wchar(PathJoin({path, "*"}));
	auto hFind = FindFirstFileW(wfind_path.c_str(), &data);
	if (hFind == INVALID_HANDLE_VALUE)
		return entries;
	do {
		auto name = wchar_to_utf8(data.cFileName);
		if (name == "." || name == "..")
			continue;
		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			const auto sub_entries = ListDirRecursive(PathJoin({path, name}).c_str(), mask);
			for (const auto &i : sub_entries)
				entries.push_back({i.type, PathJoin({name, i.name})});
		}
	} while (FindNextFileW(hFind, &data));

	FindClose(hFind);
#else /* POSIX */
	auto dir = opendir(path);
	if (!dir)
		return entries;

	while (auto ent = readdir(dir)) {
		if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
			continue;
		if (ent->d_type == DT_DIR) {
			const auto sub_entries = ListDirRecursive(PathJoin({path, ent->d_name}).c_str(), mask);
			for (const auto &i : sub_entries)
				entries.push_back({i.type, PathJoin({ent->d_name, i.name})});
		}
	}

	closedir(dir);
#endif
	return entries;
}

//
size_t GetDirSize(const char *path) {
	const auto entries = hg::ListDirRecursive(path);

	size_t size = 0;
	for (const auto &e : entries)
		if (e.type == hg::DE_File) {
			const auto fpath = hg::PathJoin({path, e.name});
			const auto finfo = hg::GetFileInfo(fpath.c_str());
			size += finfo.size;
		}

	return size;
}

//
bool MkDir(const char *path, int permissions, bool verbose) {
#if _WIN32
	const auto res = CreateDirectoryW(utf8_to_wchar(path).c_str(), nullptr) != 0;
	if (!verbose && !res)
		warn(format("MkDir(%1) failed with error: %2").arg(path).arg(GetLastError_Win32()));
	return res;
#else
	return mkdir(path, permissions) == 0;
#endif
}

bool RmDir(const char *path, bool verbose) {
#if _WIN32
	const auto res = RemoveDirectoryW(utf8_to_wchar(path).c_str()) != 0;
	if (verbose && !res)
		warn(format("RmDir(%1) failed with error: %2").arg(path).arg(GetLastError_Win32()));
	return res;
#else
	return false;
#endif
}

bool MkTree(const char *path, int permissions, bool verbose) {
	const auto dirs = split(CleanPath(path), "/");

	std::string p;
	for (auto &dir : dirs) {
		p += dir + "/";

		if (ends_with(dir.c_str(), ":"))
			continue; // skip c:

		if (Exists(p.c_str()))
			continue;

		if (!MkDir(p.c_str(), permissions, verbose))
			return false;
	}

	return true;
}

bool RmTree(const char *path, bool verbose) {
#if _WIN32
	std::string _path(path);
	if (!ends_with(_path, "/"))
		_path += "/";

	const std::wstring wpath = utf8_to_wchar(_path);
	const std::wstring wfilter = wpath + L"*.*";

	WIN32_FIND_DATAW FindFileData;
	ZeroMemory(&FindFileData, sizeof(FindFileData));
	HANDLE hFind = FindFirstFileW(wfilter.c_str(), &FindFileData);

	if (hFind != nullptr && hFind != INVALID_HANDLE_VALUE) {
		while (FindNextFileW(hFind, &FindFileData) != 0) {
			if (wcscmp(FindFileData.cFileName, L".") != 0 && wcscmp(FindFileData.cFileName, L"..") != 0) {
				if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					if (!RmTree(wchar_to_utf8(wpath + std::wstring(FindFileData.cFileName)).c_str(), verbose))
						return false;
				} else {
					if (!DeleteFileW((wpath + std::wstring(FindFileData.cFileName)).c_str()))
						return false;
				}
			}
		}
		FindClose(hFind);
	}

	return RemoveDirectoryW(wpath.c_str());
#else
	return false;
#endif
}

bool IsDir(const char *path) {
#if WIN32
	struct _stat info;
	if (_wstat(utf8_to_wchar(path).c_str(), &info) != 0)
		return false;
#else
	struct stat info;
	if (stat(path, &info) != 0)
		return false;
#endif

	if (info.st_mode & S_IFDIR)
		return true;
	return false;
}

char *MkTempDir(const char *tmplt) {
	size_t len = strlen(tmplt);
	if ((len < 6) || !ends_with(tmplt, "XXXXXX"))
		return nullptr;

	char *path = strdup(tmplt);
	char *suffix = path + len - 6;
#ifndef TMP_MAX
#define TMP_MAX 238328
#endif
	int attempts;
	for (attempts = 0; attempts < TMP_MAX; attempts++) {
		for (int j = 0; j < 6; j++) {
			uint32_t r = Rand(26 * 2 + 10 + 2);
			char c;
			if (r < 26) {
				c = 'a' + r;
			} else if (r < 52) {
				c = 'A' + r - 26;
			} else if (r < 62) {
				c = '0' + r - 52;
			} else {
				c = (r & 1) ? '_' : '-';
			}
			suffix[j] = c;
		}
		if (MkDir(path, 01700)) {
			break;
		}
	}

	if (attempts == TMP_MAX) {
		free(path);
		return nullptr;
	}

	return path;
}

bool CopyDir(const char *src, const char *dst) {
	if (!IsDir(src))
		return false;

	const auto entries = ListDir(src);
	for (auto &e : entries) {
		if (e.type & DE_File) {
			const auto file_src = PathJoin({src, e.name});
			const auto file_dst = PathJoin({dst, e.name});
			if (!CopyFile(file_src.c_str(), file_dst.c_str()))
				return false;
		}
	}
	return true;
}

bool CopyDirRecursive(const char *src, const char *dst) {
	if (!IsDir(src) || !IsDir(dst))
		return false;

	const auto entries = ListDir(src);
	for (auto &e : entries) {
		if (e.type & DE_Dir) {
			const auto src_path = PathJoin({src, e.name});
			const auto dst_path = PathJoin({dst, e.name});
			if (!MkDir(dst_path.c_str()))
				return false;
			if (!CopyDirRecursive(src_path.c_str(), dst_path.c_str()))
				return false;
		} else if (e.type & DE_File) {
			const auto file_src = PathJoin({src, e.name});
			const auto file_dst = PathJoin({dst, e.name});
			if (!CopyFile(file_src.c_str(), file_dst.c_str()))
				return false;
		}
	}
	return true;
}

bool Exists(const char *path) {
#if _WIN32
	struct _stat info;
	return _wstat(utf8_to_wchar(path).data(), &info) == 0;
#else
	struct stat info;
	return stat(path, &info) == 0;
#endif
}

} // namespace hg
