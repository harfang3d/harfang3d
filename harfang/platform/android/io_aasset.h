// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/io_driver.h"

struct AAssetManager;

namespace gs {
namespace io {

struct AAssetDriver : Driver {
	AAssetDriver(AAssetManager *manager) : aasset_manager(manager) {}

	DriverCaps::Type GetCaps() const override;

	Handle *Open(const char *path, Mode mode = ModeRead) override;
	void Close(Handle &h) override;

	bool Delete(const char *path) override;

	size_t Tell(Handle &h) override;
	size_t Seek(Handle &h, ptrdiff_t offset, SeekRef seek = SeekCurrent) override;
	size_t Size(Handle &h) override;

	bool IsEOF(Handle &h) override;

	size_t Read(Handle &h, void *buffer_out, size_t size) override;
	size_t Write(Handle &h, const void *buffer_in, size_t size) override;

	std::vector<DirEntry> Dir(const char *path, const char *wildcard, DirEntry::Type filter = DirEntry::File) override;
	bool MkDir(const char *path) override;
	bool IsDir(const char *path) override;

private:
	AAssetManager *aasset_manager = nullptr;
};

} // io
} // gs
