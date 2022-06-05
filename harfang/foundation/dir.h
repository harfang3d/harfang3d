// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/time.h"

#include <string>
#include <vector>

namespace hg {

enum DirEntryType { DE_File = 0x1, DE_Dir = 0x2, DE_Link = 0x4, DE_All = 0xffff };

struct DirEntry {
	int type{};
	std::string name;
	time_ns last_modified;
	size_t size;
};

std::vector<DirEntry> ListDir(const char *path, int mask = DE_All);
std::vector<DirEntry> ListDirRecursive(const char *path, int mask = DE_All);

size_t GetDirSize(const char *path);

bool MkDir(const char *path, int permissions = 01777, bool verbose = false);
bool RmDir(const char *path, bool verbose = false);

bool MkTree(const char *path, int permissions = 01777, bool verbose = false);
bool RmTree(const char *path, bool verbose = false);

/// tmplt should end with at least six trailing 'x' characters
char *MkTempDir(const char *tmplt);

bool IsDir(const char *path);

/// Copy a directory on the local filesystem, this function does not recurse through subdirectories.
/// @see CopyDirRecursive.
bool CopyDir(const char *src, const char *dst);
/// Copy a directory on the local filesystem, recurse through subdirectories.
bool CopyDirRecursive(const char *src, const char *dst);

/// Return `true` if a file exists on the local filesystem, `false` otherwise.
bool Exists(const char *path);

} // namespace hg
