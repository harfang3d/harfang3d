// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/assert.h"
#include "foundation/seek_mode.h"

#include <cassert>
#include <cstddef>
#include <string>

namespace hg {

#ifndef NDEBUG
#define ENABLE_BINARY_DEBUG_HANDLE
#endif

struct Handle { // 16 bytes
	uint32_t v[4];
#ifdef ENABLE_BINARY_DEBUG_HANDLE
	bool debug{false};
#endif
};

struct Reader {
	size_t (*read)(Handle h, void *data, size_t size);
	size_t (*size)(Handle h);
	bool (*seek)(Handle h, ptrdiff_t offset, SeekMode mode);
	size_t (*tell)(Handle h);
	bool (*is_valid)(Handle h);
	bool (*is_eof)(Handle h);
};

struct Writer {
	size_t (*write)(Handle h, const void *data, size_t size);
	bool (*seek)(Handle h, ptrdiff_t offset, SeekMode mode);
	size_t (*tell)(Handle h);
	bool (*is_valid)(Handle hnd);
};

//
#ifdef ENABLE_BINARY_DEBUG_HANDLE
template <typename T> bool Read(const Reader &i, const Handle &h, T &v) {
	if (h.debug) {
		uint16_t _check;
		if (i.read(h, &_check, sizeof(uint16_t)) != sizeof(uint16_t))
			return false;
		assert(_check == sizeof(T));
	}
	return i.read(h, &v, sizeof(T)) == sizeof(T);
}

template <typename T> bool Write(const Writer &i, const Handle &h, const T &v) {
	if (h.debug) {
		uint16_t _check = sizeof(T);
		if (i.write(h, &_check, sizeof(uint16_t)) != sizeof(uint16_t))
			return false;
	}
	return i.write(h, &v, sizeof(T)) == sizeof(T);
}
#else
template <typename T> bool Read(const Reader &i, const Handle &h, T &v) { return i.read(h, &v, sizeof(T)) == sizeof(T); }
template <typename T> bool Write(const Writer &i, const Handle &h, const T &v) { return i.write(h, &v, sizeof(T)) == sizeof(T); }
#endif

//
bool Read(const Reader &i, const Handle &h, std::string &v);
bool Write(const Writer &i, const Handle &h, const std::string &v);

//
size_t Tell(const Reader &i, const Handle &h);
size_t Tell(const Writer &i, const Handle &h);

//
bool Seek(const Reader &i, const Handle &h, ptrdiff_t offset, SeekMode mode);
bool Seek(const Writer &i, const Handle &h, ptrdiff_t offset, SeekMode mode);

//
template <typename T> T Read(const Reader &i, const Handle &h) {
	T v;
	bool r = Read(i, h, v);
	__ASSERT__(r == true);
	return v;
}

template <typename T> bool Skip(const Reader &i, const Handle &h) { return Seek(i, h, sizeof(T), SM_Current); }

bool SkipString(const Reader &i, const Handle &h);

//
struct ReadProvider {
	Handle (*open)(const char *path, bool silent);
	void (*close)(Handle hnd);
	bool (*is_file)(const char *path);
};

bool Exists(const Reader &ir, const ReadProvider &i, const char *path);

struct WriteProvider {
	Handle (*open)(const char *path);
	void (*close)(Handle hnd);
};

//
struct ScopedReadHandle {
#ifdef ENABLE_BINARY_DEBUG_HANDLE
	ScopedReadHandle(ReadProvider i, const char *path, bool silent = false, bool debug = false) : i_(i), h_(i.open(path, silent)) { h_.debug = debug; }
#else
	ScopedReadHandle(ReadProvider i, const char *path, bool silent = false) : i_(i), h_(i.open(path, silent)) {}
#endif
	~ScopedReadHandle() { i_.close(h_); }

	operator const Handle &() const { return h_; }

private:
	Handle h_;
	ReadProvider i_;
};

struct ScopedWriteHandle {
#ifdef ENABLE_BINARY_DEBUG_HANDLE
	ScopedWriteHandle(WriteProvider i, const char *path, bool debug = false) : i_(i), h_(i.open(path)) { h_.debug = debug; }
#else
	ScopedWriteHandle(WriteProvider i, const char *path) : i_(i), h_(i.open(path)) {}
#endif
	~ScopedWriteHandle() { i_.close(h_); }

	operator const Handle &() const { return h_; }

private:
	Handle h_;
	WriteProvider i_;
};

//
class Data;

Data LoadData(const Reader &i, const Handle &h);
std::string LoadString(const Reader &i, const Handle &h);

//
template <typename T> struct DeferredWrite {
	DeferredWrite(const Writer &iw_, const Handle &h_) : iw(iw_), h(h_) {
		cursor = Tell(iw, h);

#ifdef ENABLE_BINARY_DEBUG_HANDLE
		if (h.debug)
			Seek(iw, h, sizeof(uint16_t), SM_Current); // leave space for debug size marker
#endif
		Seek(iw, h, sizeof(T), SM_Current); // leave space for deferred write
	}

	bool Commit(const T &v) {
		const auto seek_ = Tell(iw, h);

		if (!Seek(iw, h, cursor, SM_Start) || !Write(iw, h, v) || !Seek(iw, h, seek_, SM_Start))
			return false;

		return true;
	}

	size_t cursor;

	const Writer &iw;
	const Handle &h;
};

} // namespace hg
