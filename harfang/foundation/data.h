// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <cstdint>
#include <string>

namespace hg {

class Data {
public:
	Data() = default;
	~Data();

	explicit Data(size_t size) { Resize(size); }

	Data(const Data &data) { *this = data; }
//	Data(Data &&data) { *this = data; }

	Data(const void *data, size_t size) { Write(data, size); }
	Data(void *data, size_t size) : data_(reinterpret_cast<uint8_t *>(data)), size_(size) {}

	Data &operator=(const Data &data);
//	Data &operator=(Data &&data);

	uint8_t *GetData() { return data_; }
	const uint8_t *GetData() const { return data_; }

	size_t GetSize() const { return size_; }
	size_t GetCapacity() const { return capacity_; }

	size_t GetCursor() const { return cursor; }
	void SetCursor(size_t pos) { Reserve(cursor = pos); }
	void Rewind() { SetCursor(0); }

	void TakeOwnership() { Reserve(size_); }

	uint8_t *GetCursorPtr() { return data_ + cursor; }
	const uint8_t *GetCursorPtr() const { return data_ + cursor; }

	void Reset() { size_ = cursor = 0; }
	bool Empty() const { return size_ == 0; }

	void Reserve(size_t size);
	void Resize(size_t size);

	void Skip(size_t count);

	size_t Write(const void *data, size_t size);
	size_t Read(void *data, size_t size) const;

	void Free();

private:
	uint8_t *data_{nullptr};
	size_t size_{0}, capacity_{0};

	bool has_ownership{false};
	mutable size_t cursor{0};
};

//
bool Read(Data &data, std::string &str);
bool Write(Data &data, const std::string &str);

//
template <typename T> bool Read(Data &data, T &v) { return data.Read(&v, sizeof(T)); }

template <typename T> T Read(Data &data) {
	T v;
	data.Read(&v, sizeof(T));
	return v;
}

template <typename T> bool Write(Data &data, const T &v) { return data.Write(&v, sizeof(T)) == sizeof(T); }

//
template <typename T> struct DeferredDataWrite {
	DeferredDataWrite(Data &data_) : data(data_) {
		cursor = data.GetCursor();
		data.Skip(sizeof(T)); // leave space for deferred write
	}

	void Commit(const T &v) {
		const auto seek_ = data.GetCursor();

		data.SetCursor(cursor);
		data.Write(&v, sizeof(T));

		data.SetCursor(seek_);
	}

	void CommitAsChunkSize() {
		const auto chunk_size = data.GetCursor() - (cursor + sizeof(T));
		Commit(T(chunk_size));
	}

	size_t cursor;
	Data &data;
};

//
bool LoadDataFromFile(const char *path, Data &data);
bool SaveDataToFile(const char *path, const Data &data);

} // namespace hg
