// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "android/asset_manager.h"
#include "foundation/io_driver.h"
#include "foundation/io_handle.h"
#include "foundation/string.h"
#include "platform/android/io_aasset.h"

namespace gs {
namespace io {

struct AAssetHandle : Handle {
	explicit AAssetHandle(sDriver driver) : Handle(driver) {}
	~AAssetHandle() { driver->Close(*this); }
	AAsset *asset = nullptr;
};

//
DriverCaps::Type AAssetDriver::GetCaps() const { return DriverCaps::CanRead | DriverCaps::CanSeek | DriverCaps::IsCaseSensitive; }

//
Handle *AAssetDriver::Open(const char *path, Mode mode) {
	if (mode == ModeWrite)
		return nullptr;
	std::unique_ptr<AAssetHandle> h(new AAssetHandle(shared_from_this()));
	h->asset = AAssetManager_open(aasset_manager, path, AASSET_MODE_UNKNOWN);
	if (!h->asset)
		return nullptr;
	return h.release();
}

void AAssetDriver::Close(Handle &h) {
	auto &ch = static_cast<AAssetHandle &>(h);
	std::lock_guard<Handle> ch_lock(ch);

	if (ch.asset) {
		AAsset_close(ch.asset);
		ch.asset = nullptr;
	}
}

bool AAssetDriver::Delete(const char *path) { return false; }

size_t AAssetDriver::Seek(Handle &h, ptrdiff_t offset, SeekRef ref) {
	auto &ch = static_cast<AAssetHandle &>(h);
	std::lock_guard<Handle> ch_lock(ch);

	if (ch.asset) {
		int c_seek[] = { SEEK_SET, SEEK_CUR, SEEK_END };
		AAsset_seek(ch.asset, off_t(offset), c_seek[ref]);
	}
	return size_t(-1);
}

size_t AAssetDriver::Tell(Handle &h) {
	auto &ch = static_cast<AAssetHandle &>(h);
	std::lock_guard<Handle> ch_lock(ch);
	return AAsset_getLength(ch.asset) - AAsset_getRemainingLength(ch.asset);
}

size_t AAssetDriver::Size(Handle &h) {
	auto &ch = static_cast<AAssetHandle &>(h);
	std::lock_guard<Handle> ch_lock(ch);
	return AAsset_getLength(ch.asset);
}

bool AAssetDriver::IsEOF(Handle &h) {
	auto &ch = static_cast<AAssetHandle &>(h);
	std::lock_guard<Handle> ch_lock(ch);
	return ch.asset ? AAsset_getRemainingLength(ch.asset) == 0 : true;
}

//
size_t AAssetDriver::Read(Handle &h, void *b, size_t size) {
	auto &ch = static_cast<AAssetHandle &>(h);
	std::lock_guard<Handle> ch_lock(ch);
	return ch.asset ? AAsset_read(ch.asset, b, size) : 0;
}

size_t AAssetDriver::Write(Handle &h, const void *b, size_t size) { return 0; }

//
std::vector<DirEntry> AAssetDriver::Dir(const char *path, const char *wildcard, DirEntry::Type filter) {
	std::vector<DirEntry> entries;

	if (auto dir = AAssetManager_openDir(aasset_manager, path)) {
		while (auto name = AAssetDir_getNextFileName(dir)) {
			if (!name)
				break;

			if (!strcmp(name, ".") || !strcmp(name, ".."))
				continue;

			if (!match_wildcard(name, wildcard))
				continue;

			DirEntry entry;
			entry.name = name;
			entry.type = DirEntry::File;
			entries.push_back(entry);
		}
		AAssetDir_close(dir);
	}

	return entries;
}

//
bool AAssetDriver::MkDir(const char *path) { return false; }

bool AAssetDriver::IsDir(const char *path) {
	auto dir = AAssetManager_openDir(aasset_manager, path);
	if (dir)
		AAssetDir_close(dir);
	return asbool(dir);
}

} // io
} // gs
