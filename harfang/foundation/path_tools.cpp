// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <ShlObj.h>
#include <Windows.h>
#else // POSIX
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "foundation/assert.h"
#include "foundation/format.h"
#include "foundation/path_tools.h"
#include "foundation/string.h"
#include <array>
#include <cstring>

namespace hg {

std::string GetFilePath(const std::string &p) {
	ptrdiff_t i = p.length() - 1;
	for (; i >= 0; --i)
		if (p[i] == '/' || p[i] == '\\') // found last separator in file name
			break;

	if (i < 0)
		return "./"; // explicit current path

	if (i == 0) {
#if _WIN32
		__ASSERT__(i > 0);
		return "./"; // a path starting with a slash on windows is a non-sense...
#else
		return "/"; // root file
#endif
	}

	if (p[i] == '/')
		return slice(p, 0, i) + "/";

	return slice(p, 0, i) + "\\";
}

std::string GetFileName(const std::string &path) { return CutFilePath(CutFileExtension(path)); }

//
bool IsPathAbsolute(const std::string &path) {
#if WIN32
	return path.length() > 2 && path[1] == ':';
#else
	return path.length() > 1 && path[0] == '/';
#endif
}

bool PathStartsWith(const std::string &path, const std::string &with) { return starts_with(CleanPath(path), CleanPath(with)); }

std::string PathStripPrefix(const std::string &path, const std::string &prefix) { return strip_prefix(strip_prefix(CleanPath(path), CleanPath(prefix)), "/"); }
std::string PathStripSuffix(const std::string &path, const std::string &suffix) { return strip_suffix(strip_suffix(CleanPath(path), CleanPath(suffix)), "/"); }

//
std::string PathToDisplay(const std::string &path) {
	std::string out(path);
	replace_all(out, "_", " ");
	return out;
}

std::string NormalizePath(const std::string &path) {
	std::string out(path);
	replace_all(out, " ", "_");
	return out;
}

//
std::string PathJoin(const std::vector<std::string> &elements) {
	std::vector<std::string> stripped_elements;
	stripped_elements.reserve(elements.size());
#if !defined(_WIN32)
	if (!elements.empty()) {
		if (starts_with(elements[0], "/")) {
			stripped_elements.push_back("");
		}
	}
#endif
	for (auto &element : elements)
		if (!element.empty())
			stripped_elements.push_back(rstrip(lstrip(element, "/"), "/"));
	return CleanPath(join(stripped_elements.begin(), stripped_elements.end(), "/"));
}

//
std::string CutFilePath(const std::string &path) {
	if (path.empty())
		return {};
	for (auto n = path.length() - 1; n > 0; --n)
		if (path[n] == '\\' || path[n] == '/')
			return slice(path, n + 1);
	return path;
}

std::string CutFileExtension(const std::string &path) {
	if (path.empty())
		return {};
	for (auto n = path.length() - 1; n > 0; --n) {
		if (path[n] == '.')
			return slice(path, 0, n);
		if (path[n] == '\\' || path[n] == '/' || path[n] == ':')
			break;
	}
	return path;
}

std::string CutFileName(const std::string &path) {
	if (path.empty())
		return {};
	for (auto n = path.length() - 1; n > 0; --n)
		if (path[n] == '\\' || path[n] == '/' || path[n] == ':')
			return slice(path, 0, n + 1);
	return path;
}

//
std::string GetFileExtension(const std::string &path) {
	if (path.empty())
		return {};
	for (auto n = path.length() - 1; n > 0; --n)
		if (path[n] == '.')
			return slice(path, n + 1);
	return {};
}

bool HasFileExtension(const std::string &path) { return !GetFileExtension(path).empty(); }

//
std::string SwapFileExtension(const std::string &path, const std::string &ext) { return CutFileExtension(path) + "." + ext; }

//
std::string FactorizePath(const std::string &path) {
	auto dirs = split(path, "/");
	if (dirs.size() < 2) {
		return dirs.empty() ? path : dirs[0];
	}

	bool factorized = false;

	while (dirs.size() > 1 && !factorized) {
		factorized = true;

		auto i = dirs.begin();
		for (; i != dirs.end() - 1; ++i)
			if (*i != ".." && *(i + 1) == "..") {
				factorized = false;
				break;
			}

		if (!factorized) {
			dirs.erase(i + 1);
			dirs.erase(i);
		}
	}

	return join(dirs.begin(), dirs.end(), "/");
}

//
std::string CleanPath(const std::string &path) {
	std::string out(path);

	// drive letter to lower case on Windows platform
#if _WIN32
	if (path.length() > 2 && path[1] == ':')
		out = tolower(path, 0, 1);
	bool is_network_path = starts_with(out, "\\\\");
#endif

	// convert directory separator from backslash to forward slash
	replace_all(out, "\\", "/");

	// remove redundant forward slashes
	while (replace_all(out, "//", "/"))
		;

	while (replace_all(out, "/./", "/"))
		;

#if _WIN32
	if (is_network_path)
		out = std::string("\\\\") + slice(out, 1);
#endif

	// remove pointless ./ when it is starting the file path
	while (starts_with(out, "./"))
		out = slice(out, 2);

	// remove pointless /. when it is ending the file path
	while (ends_with(out, "/."))
		out = slice(out, 0, -2);

	return FactorizePath(out);
}

std::string CleanFileName(const std::string &filename) {
	std::string out(filename);

	const char filename_invalid_chars[] = "<>:\"/\\|?*";

	for (size_t i = 0; i < sizeof(filename_invalid_chars); i++) {
		std::string str_to_replace(1, filename_invalid_chars[i]);
		replace_all(out, str_to_replace, "_");
	}

	return out;
}

//
#if _WIN32

std::string GetCurrentWorkingDirectory() {
	WCHAR path[1024];
	GetCurrentDirectoryW(1024 - 1, path); // poorly worded documentation makes it unclear if nBufferLength should account for the terminator or not...
	return wchar_to_utf8(path);
}

bool SetCurrentWorkingDirectory(const std::string &path) { return SetCurrentDirectoryW(utf8_to_wchar(path).c_str()) == TRUE; }

std::string GetUserFolder() {
	HRESULT res;
	PWSTR path;
	res = SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, NULL, &path);
	if (FAILED(res)) {
		return {};
	}
	std::string ret = wchar_to_utf8(path);
	CoTaskMemFree(path);
	return ret;
}

#else // POSIX

std::string GetCurrentWorkingDirectory() {
	std::array<char, 1024> cwd;
	return getcwd(cwd.data(), 1024) ? std::string(cwd.data()) : "";
}

bool SetCurrentWorkingDirectory(const std::string &path) { return chdir(path.c_str()) == 0; }

std::string GetUserFolder() { return getpwuid(getuid())->pw_dir; }

#endif // POSIX

} // namespace hg
