// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <string>
#include <vector>

namespace hg {

bool IsPathAbsolute(const std::string &path);

std::string PathToDisplay(const std::string &path);
std::string NormalizePath(const std::string &path);

/// Return the input path with all redundant navigation entries stripped (folder separator, `..` and `.` entries).
std::string FactorizePath(const std::string &path);

/*!
	Cleanup a local filesystem path according to the host platform conventions.

	The following operations are applied:
		- Remove redundant folder separators.
		- Remove redundant `.` and `..` folder entries.
		- Ensure forward slash (`/`) folder separators on Unix and back slash (`\`) folder separators on Windows.
*/
std::string CleanPath(const std::string &path);
std::string CleanFileName(const std::string &filename);

// Returns the absolute pathname of an existing file or directory.
std::string GetAbsolutePath(const std::string path);

/// Returns the folder navigation part of a file path. The file name and its extension are stripped.
/// @see CutFileExtension and CutFileName.
std::string CutFilePath(const std::string &path);
/// Returns the name part of a file path. All folder navigation and extension are stripped.
/// @see CutFileExtension and CutFilePath.
std::string CutFileName(const std::string &path);
/// Returns a file path with its extension stripped.
/// @see CutFilePath and CutFileName.
std::string CutFileExtension(const std::string &path);

std::string GetFilePath(const std::string &path);
std::string GetFileName(const std::string &path);
std::string GetFileExtension(const std::string &path);

bool HasFileExtension(const std::string &path);

bool PathStartsWith(const std::string &path, const std::string &with);
std::string PathStripPrefix(const std::string &path, const std::string &prefix);
std::string PathStripSuffix(const std::string &path, const std::string &suffix);
std::string PathJoin(const std::vector<std::string> &elements);
std::string PathJoin(const std::string &a, const std::string &b);
std::string PathJoin(const std::string &a, const std::string &b, const std::string &c);

std::string SwapFileExtension(const std::string &path, const std::string &ext);

std::string GetCurrentWorkingDirectory();
bool SetCurrentWorkingDirectory(const std::string &path);

std::string GetUserFolder();

} // namespace hg
