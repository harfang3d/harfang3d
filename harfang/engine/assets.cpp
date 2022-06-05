// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/assets.h"
#include "foundation/dir.h"
#include "foundation/file.h"
#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/path_tools.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <string>

#include <miniz/miniz.h>

namespace hg {

enum AssetsSourceType { AssetsFolder, AssetsPackage };

struct AssetsSource {
	AssetsSourceType type;
	std::string name;
};

//
static std::mutex assets_mutex;

static std::deque<std::string> assets_folders;

bool AddAssetsFolder(const char *path) {
	std::lock_guard<std::mutex> lock(assets_mutex);
	if (std::find(std::begin(assets_folders), std::end(assets_folders), path) == std::end(assets_folders)) {
		assets_folders.push_front(path);
		return true;
	}
	return false;
}

void RemoveAssetsFolder(const char *path) {
	std::lock_guard<std::mutex> lock(assets_mutex);
	assets_folders.erase(
		std::remove_if(std::begin(assets_folders), std::end(assets_folders), [&](const std::string &i) { return i == path; }), std::end(assets_folders));
}

//
struct ZipPackage {
	mz_zip_archive archive;
	std::string filename;

	ZipPackage() { mz_zip_zero_struct(&archive); }
	~ZipPackage() { mz_zip_reader_end(&archive); }
};

static std::deque<ZipPackage> assets_packages;

bool AddAssetsPackage(const char *path) {
	std::lock_guard<std::mutex> lock(assets_mutex);
	auto it = std::find_if(std::begin(assets_packages), std::end(assets_packages), [path](const ZipPackage &h) { return h.filename == path; });
	if (it != std::end(assets_packages)) {
		return false;
	}

	assets_packages.emplace_front();
	ZipPackage &pkg = assets_packages.front();

	pkg.filename = path;
	if (mz_zip_reader_init_file(&pkg.archive, path, MZ_ZIP_FLAG_VALIDATE_HEADERS_ONLY) == MZ_FALSE) {
		return false;
	}

	return true;
}

void RemoveAssetsPackage(const char *path) {
	std::lock_guard<std::mutex> lock(assets_mutex);
	assets_packages.erase(std::begin(assets_packages),
		std::remove_if(std::begin(assets_packages), std::end(assets_packages), [path](const ZipPackage &h) { return h.filename == path; }));
}

struct PackageFile {
	std::string data;
	ptrdiff_t cursor;
};

//
struct Asset_ {
	File file;
	PackageFile pkg_file;

	size_t (*get_size)(Asset_ &asset);
	size_t (*read)(Asset_ &asset, void *data, size_t size);
	bool (*seek)(Asset_ &asset, ptrdiff_t offset, SeekMode mode);
	size_t (*tell)(Asset_ &asset);
	void (*close)(Asset_ &asset);
	bool (*is_eof)(Asset_ &asset);
};

//
static size_t Asset_file_GetSize(Asset_ &asset) { return GetSize(asset.file); }
static size_t Asset_file_Read(Asset_ &asset, void *data, size_t size) { return Read(asset.file, data, size); }
static bool Asset_file_Seek(Asset_ &asset, ptrdiff_t offset, SeekMode mode) { return Seek(asset.file, offset, mode); }
static size_t Asset_file_Tell(Asset_ &asset) { return Tell(asset.file); }
static void Asset_file_Close(Asset_ &asset) { Close(asset.file); }
static bool Asset_file_is_EOF(Asset_ &asset) { return IsEOF(asset.file); }

//
static size_t Package_file_GetSize(Asset_ &asset) { return asset.pkg_file.data.size(); }
static size_t Package_file_Read(Asset_ &asset, void *data, size_t size) {
	if (asset.pkg_file.cursor + size <= asset.pkg_file.data.size()) {
		memcpy(data, asset.pkg_file.data.data() + sizeof(char) * asset.pkg_file.cursor, size);
		asset.pkg_file.cursor += size;
		return size;
	}
	return 0;
}
static bool Package_file_Seek(Asset_ &asset, ptrdiff_t offset, SeekMode mode) {
	if (offset <= asset.pkg_file.data.size()) {
		asset.pkg_file.cursor = offset;
		return true;
	}
	return false;
}
static size_t Package_file_Tell(Asset_ &asset) { return asset.pkg_file.cursor; }
static void Package_file_Close(Asset_ &asset) {}
static bool Package_file_is_EOF(Asset_ &asset) { return asset.pkg_file.cursor >= asset.pkg_file.data.size(); }

//
static generational_vector_list<Asset_> assets;

Asset OpenAsset(const char *name, bool silent) {
	std::lock_guard<std::mutex> lock(assets_mutex);

	for (auto &p : assets_folders) {
		const auto asset_path = PathJoin({p, name});

		const auto file = Open(asset_path.c_str(), true);
		if (IsValid(file))
			return {assets.add_ref({file, {}, Asset_file_GetSize, Asset_file_Read, Asset_file_Seek, Asset_file_Tell, Asset_file_Close, Asset_file_is_EOF})};
	}

	// look in archive
	for (auto &p : assets_packages) {
		const int index = mz_zip_reader_locate_file(&p.archive, name, NULL, MZ_ZIP_FLAG_CASE_SENSITIVE);
		if (index == -1)
			continue; // missing file

		size_t size;
		const char *buffer = (char *)mz_zip_reader_extract_to_heap(&p.archive, index, &size, 0);
		if (buffer) {
			std::string data(buffer, size);
			return {assets.add_ref({{}, {std::move(data), 0}, Package_file_GetSize, Package_file_Read, Package_file_Seek, Package_file_Tell, Package_file_Close,
				Package_file_is_EOF})};
		} else {
			const mz_zip_error err = mz_zip_get_last_error(&p.archive);
			if (!silent)
				warn(format("Failed to open asset '%1' from file '%2' (asset was found but failed to open) : %3")
						  .arg(name)
						  .arg(p.filename)
						  .arg(mz_zip_get_error_string(err)));
			break;
		}
	}

	if (!silent)
		warn(format("Failed to open asset '%1' (file not found)").arg(name));

	return {};
}

void Close(Asset asset) {
	std::lock_guard<std::mutex> lock(assets_mutex);

	if (assets.is_valid(asset.ref)) {
		auto &asset_ = assets[asset.ref.idx];
		asset_.close(asset_);
		assets.remove_ref(asset.ref);
	}
}

bool IsAssetFile(const char *name) {
	std::lock_guard<std::mutex> lock(assets_mutex);

	for (auto &p : assets_folders)
		if (IsFile(PathJoin({p, name}).c_str()))
			return true;

	// look in archive
	for (auto &p : assets_packages) {
		if (mz_zip_reader_locate_file(&p.archive, name, NULL, MZ_ZIP_FLAG_CASE_SENSITIVE) != -1)
			return true;
	}

	return false;
}

size_t GetSize(Asset asset) {
	std::lock_guard<std::mutex> lock(assets_mutex);
	if (assets.is_valid(asset.ref)) {
		auto &asset_ = assets[asset.ref.idx];
		return asset_.get_size(asset_);
	}
	return 0;
}

size_t Read(Asset asset, void *data, size_t size) {
	std::lock_guard<std::mutex> lock(assets_mutex);
	if (assets.is_valid(asset.ref)) {
		auto &asset_ = assets[asset.ref.idx];
		return asset_.read(asset_, data, size);
	}
	return 0;
}

bool Seek(Asset asset, ptrdiff_t offset, SeekMode mode) {
	std::lock_guard<std::mutex> lock(assets_mutex);
	if (assets.is_valid(asset.ref)) {
		auto &asset_ = assets[asset.ref.idx];
		return asset_.seek(asset_, offset, mode);
	}
	return false;
}

size_t Tell(Asset asset) {
	std::lock_guard<std::mutex> lock(assets_mutex);
	if (assets.is_valid(asset.ref)) {
		auto &asset_ = assets[asset.ref.idx];
		return asset_.tell(asset_);
	}
	return 0;
}

bool IsEOF(Asset asset) {
	std::lock_guard<std::mutex> lock(assets_mutex);
	if (assets.is_valid(asset.ref)) {
		auto &asset_ = assets[asset.ref.idx];
		return asset_.is_eof(asset_);
	}
	return false;
}

//
std::string AssetToString(const char *name) {
	const auto h = OpenAsset(name);
	if (h.ref == invalid_gen_ref)
		return {};
	const auto size = GetSize(h);
	std::string str;
	str.resize(size + 1);
	Read(h, &str[0], size);
	Close(h);
	return str;
}

Data AssetToData(const char *name) {
	Data data;
	Asset asset = OpenAsset(name);

	if (IsValid(asset)) {
		data.Resize(GetSize(asset));
		Read(asset, data.GetData(), data.GetSize());
		Close(asset);
	}

	return data;
}

} // namespace hg
