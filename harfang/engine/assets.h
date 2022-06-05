// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/data.h"
#include "foundation/generational_vector_list.h"
#include "foundation/seek_mode.h"

#include <cstddef>
#include <map>
#include <string>

namespace hg {

/// Mount a local filesystem folder as an assets source.
bool AddAssetsFolder(const char *path);
void RemoveAssetsFolder(const char *path);

/// Mount an archive stored on the local filesystem as an assets source.
bool AddAssetsPackage(const char *path);
void RemoveAssetsPackage(const char *path);

//
struct Asset {
	gen_ref ref;
};

Asset OpenAsset(const char *name, bool silent = false);
void Close(Asset asset);

inline bool IsValid(Asset asset) { return asset.ref != invalid_gen_ref; }

size_t GetSize(Asset asset);
size_t Read(Asset asset, void *data, size_t size);
bool Seek(Asset asset, ptrdiff_t offset, SeekMode seek);
size_t Tell(Asset asset);
bool IsEOF(Asset asset);

std::string AssetToString(const char *name);
Data AssetToData(const char *name);

bool IsAssetFile(const char *name);

} // namespace hg
