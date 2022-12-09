// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/dir.h"
#include "foundation/cext.h"
#include "foundation/file.h"
#include "foundation/log.h"
#include "foundation/format.h"
#include "foundation/path_tools.h"
#include "foundation/rand.h"
#include "foundation/string.h"

#include <sys/stat.h>
#if _WIN32
#include <direct.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shlwapi.h>
#else /* POSIX */
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#define _unlink unlink
#endif

#undef CopyFile

namespace hg {

#if _WIN32

std::vector<DirEntry> ListDir(const char *path, int mask) {
	std::vector<DirEntry> entries;

	WIN32_FIND_DATAW data;
	HANDLE hFind = FindFirstFileW(utf8_to_wchar(PathJoin(path, "*.*")).c_str(), &data);

	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			std::string name = wchar_to_utf8(data.cFileName);

			if (name == "." || name == "..") {
				continue;
			}

			const DirEntryType type = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? DE_Dir : DE_File;

			if (mask & type) {
				const int64_t last_modified =
					time_to_ns(((static_cast<uint64_t>(data.ftLastWriteTime.dwHighDateTime) << 32) + data.ftLastWriteTime.dwLowDateTime) * 100);

				LARGE_INTEGER size;
				size.HighPart = data.nFileSizeHigh;
				size.LowPart = data.nFileSizeLow;

				DirEntry entry;
				entry.type = type;
				entry.name = name;
				entry.last_modified = last_modified;
				entry.size = numeric_cast<size_t>(size.QuadPart);
				entries.push_back(entry);
			}
		} while (FindNextFileW(hFind, &data));

		FindClose(hFind);
	}

	return entries;
}

std::vector<DirEntry> ListDirRecursive(const char *path, int mask) {
	std::vector<DirEntry> entries = ListDir(path, mask);

	WIN32_FIND_DATAW data;

	HANDLE hFind = FindFirstFileW(utf8_to_wchar(PathJoin(path, "*")).c_str(), &data);

	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			const std::string name = wchar_to_utf8(data.cFileName);

			if (name == "." || name == "..") {
				continue;
			}

			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				const std::vector<DirEntry> sub_entries = ListDirRecursive(PathJoin(path, name).c_str(), mask);

				for (std::vector<DirEntry>::const_iterator i = sub_entries.begin(); i != sub_entries.end(); ++i) {
					std::vector<std::string> elms(2);
					elms[0] = name;
					elms[1] = i->name;

					DirEntry e = {i->type, PathJoin(elms), 0, 0};

					entries.push_back(e);
				}
			}
		} while (FindNextFileW(hFind, &data));

		FindClose(hFind);
	}

	return entries;
}

#else /* POSIX */

std::vector<DirEntry> ListDir(const char *path, int mask) {
	std::vector<DirEntry> entries;

	DIR *dir = opendir(path);

	if (dir != nullptr) {
		while (struct dirent *ent = readdir(dir)) {
			if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
				continue;

			DirEntry entry;

			int type;

			if (ent->d_type == DT_DIR) {
				type = DE_Dir;
			} else if (ent->d_type == DT_REG) {
				type = DE_File;
			} else if (ent->d_type == DT_LNK) {
				type = DE_Link;
			} else {
				type = 0;
			}

			// TODO: stat() missing infos
			if (mask & type) {
				DirEntry e = {type, ent->d_name, 0, 0};
				entries.push_back(e);
			}
		}

		closedir(dir);
	}

	return entries;
}

std::vector<DirEntry> ListDirRecursive(const char *path, int mask) {
	std::vector<DirEntry> entries = ListDir(path, mask);

	DIR *dir = opendir(path);
	if (dir != nullptr) {
		while (struct dirent *ent = readdir(dir)) {
			if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
				continue;
			}

			if (ent->d_type == DT_DIR) {
				std::vector<std::string> elms(2);
				elms[0] = path;
				elms[1] = ent->d_name;
				const std::vector<DirEntry> sub_entries = ListDirRecursive(PathJoin(elms).c_str(), mask);
				for (std::vector<DirEntry>::const_iterator i = sub_entries.begin(); i != sub_entries.end(); ++i) {
					elms[0] = ent->d_name;
					elms[1] = i->name;
					DirEntry e = {i->type, PathJoin(elms), 0, 0};

					entries.push_back(e);
				}
			}
		}

		closedir(dir);
	}

	return entries;
}

#endif

//
size_t GetDirSize(const char *path) {
	const std::vector<DirEntry> entries = ListDirRecursive(path);

	size_t size = 0;
	std::vector<std::string> tmp(2);
	tmp[0] = path;
	for (std::vector<DirEntry>::const_iterator e = entries.begin(); e != entries.end(); ++e) {
		if (e->type == DE_File) {
			tmp[1] = e->name;
			std::string fpath = PathJoin(tmp);
			const FileInfo finfo = GetFileInfo(fpath.c_str());
			size += finfo.size;
		}
	}
	return size;
}

//
#if _WIN32

bool MkDir(const char *path, int permissions, bool verbose) {
	permissions = 0; // unused
	const bool res = CreateDirectoryW(utf8_to_wchar(path).c_str(), nullptr) != 0;

	if (!verbose && !res) {
		warn(format("MkDir(%1) failed with error:  %2").arg(path).arg(GetLastError()));
	}

	return res;
}

bool RmDir(const char *path, bool verbose) {
	const bool res = RemoveDirectoryW(utf8_to_wchar(path).c_str()) != 0;

	if (verbose && !res) {
		warn(format("RmDir(%1) failed with error: %2").arg(path).arg(GetLastError()));
	}

	return res;
}

#else

bool MkDir(const char *path, int permissions, bool verbose) { return mkdir(path, permissions) == 0; }

bool RmDir(const char *path, bool verbose) {
	int ret = remove(path);

	if (verbose && ret) {
		warn(format("RmDir(%1) failed with error: %2").arg(path).arg(errno));
	}

	return ret == 0;
}

#endif

//
bool MkTree(const char *path, int permissions, bool verbose) {
	bool res = true;

	const std::vector<std::string> dirs = split(CleanPath(path), "/");

	std::string p;
	for (std::vector<std::string>::const_iterator dir = dirs.begin(); dir != dirs.end(); ++dir) {
		p += *dir + "/";

		if (ends_with(*dir, ":")) {
			continue; // skip c:
		}

		if (Exists(p.c_str())) {
			continue;
		}

		if (!MkDir(p.c_str(), permissions, verbose)) {
			res = false;
			break;
		}
	}

	return res;
}

//
#if _WIN32

bool RmTree(const char *path, bool verbose) {
	bool ok = true;

	std::string _path(path);
	if (!ends_with(_path, "/")) {
		_path += "/";
	}

	const std::wstring wpath = utf8_to_wchar(_path);
	const std::wstring wfilter = wpath + L"*.*";

	WIN32_FIND_DATAW FindFileData;
	ZeroMemory(&FindFileData, sizeof(FindFileData));
	HANDLE hFind = FindFirstFileW(wfilter.c_str(), &FindFileData);

	if ((hFind != nullptr) && (hFind != INVALID_HANDLE_VALUE)) {
		while (ok && (FindNextFileW(hFind, &FindFileData) != 0)) {
			const std::wstring filename = FindFileData.cFileName;
			if ((filename != L".") && (filename != L"..")) {
				if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					ok = RmTree(wchar_to_utf8(wpath + filename).c_str(), verbose);
				} else {
					if (!DeleteFileW((wpath + filename).c_str())) {
						ok = false;
					}
				}
			}
		}

		FindClose(hFind);
	}

	if (ok) {
		ok = RmDir(path, verbose);
	}
	return ok;
}

#else

bool RmTree(const char *path, bool verbose) {
	DIR *dir = opendir(path);
	if (dir == nullptr) { //-V2506
		return false;
	}

	bool ok = true;
	for (struct dirent *ent = readdir(dir); ok && (ent != nullptr); ent = readdir(dir)) {
		if (strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..")) {
			const std::string ent_path = PathJoin(path, ent->d_name);
			if (ent->d_type == DT_DIR) {
				ok = RmTree(ent_path.c_str(), verbose);
			} else {
				if (remove(ent_path.c_str())) {
					ok = false;
					if (verbose) {
						warn(format("RmTree(%1) failed to delete %2: %3").arg(path).arg(ent_path).arg((const char*)strerror(errno)));
					}
				}
			}
		}
	}
	closedir(dir);
	if (ok) {
		ok = RmDir(path, verbose);
	}
	return ok;
}

#endif

//
#if _WIN32

bool IsDir(const char *path) {
	bool res;
	struct _stat info;
	if (_wstat(utf8_to_wchar(path).c_str(), &info) == 0) {
		res = ((info.st_mode & S_IFDIR) == S_IFDIR);
	} else {
		res = false;
	}
	return res;
}

#else

bool IsDir(const char *path) {
	bool res;
	struct stat info;
	if (stat(path, &info) == 0) {
		res = ((info.st_mode & S_IFDIR) == S_IFDIR);
	} else {
		res = false;
	}
	return res;
}

#endif

//
bool CopyDir(const char *src, const char *dst) {
	bool res;

	if (IsDir(src)) {
		res = true;

		const std::vector<DirEntry> entries = ListDir(src);
		for (std::vector<DirEntry>::const_iterator e = entries.begin(); e != entries.end(); ++e) {
			if (e->type & DE_File) {
				std::vector<std::string> elms(2);
				elms[1] = e->name;

				elms[0] = src;
				const std::string file_src = PathJoin(elms);
				elms[0] = dst;
				const std::string file_dst = PathJoin(elms);

				if (!CopyFile(file_src.c_str(), file_dst.c_str())) {
					res = false;
					break;
				}
			}
		}
	} else {
		res = false;
	}

	return res;
}

bool CopyDirRecursive(const char *src, const char *dst) {
	bool res;

	if (IsDir(src) && IsDir(dst)) {
		res = true;

		const std::vector<DirEntry> entries = ListDir(src);
		for (std::vector<DirEntry>::const_iterator e = entries.begin(); e != entries.end(); ++e) {
			if (e->type & DE_Dir) {
				std::vector<std::string> elms(2);
				elms[1] = e->name;

				elms[0] = src;
				const std::string src_path = PathJoin(elms);
				elms[0] = dst;
				const std::string dst_path = PathJoin(elms);

				if (MkDir(dst_path.c_str())) {
					if (!CopyDirRecursive(src_path.c_str(), dst_path.c_str())) {
						res = false;
					}
				} else {
					res = false;
				}
			} else if (e->type & DE_File) {
				std::vector<std::string> elms(2);
				elms[1] = e->name;

				elms[0] = src;
				const std::string file_src = PathJoin(elms);
				elms[0] = dst;
				const std::string file_dst = PathJoin(elms);

				if (!CopyFile(file_src.c_str(), file_dst.c_str())) {
					res = false;
				}
			} else {
				//
			}

			if (!res) {
				break;
			}
		}
	} else {
		res = false;
	}

	return res;
}

//
#if _WIN32

bool Exists(const char *path) {
	struct _stat info;
	return _wstat(utf8_to_wchar(path).data(), &info) == 0;
}

#else

bool Exists(const char *path) {
	struct stat info;
	return stat(path, &info) == 0;
}

#endif

} // namespace hg
