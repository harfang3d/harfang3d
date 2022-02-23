// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include <foundation/data.h>
#include <foundation/rw_interface.h>
#include <vector>

namespace hg {

bool Exists(const Reader &ir, const ReadProvider &ip, const char *path) {
	auto h = ip.open(path, true);
	if (!ir.is_valid(h))
		return false;
	ip.close(h);
	return true;
}

//
bool Read(const Reader &i, const Handle &h, std::string &v) {
	uint16_t size;
	if (!Read<uint16_t>(i, h, size))
		return false;

	std::vector<char> s_(size_t(size) + 1);
	if (i.read(h, s_.data(), size) != size)
		return false;

	if (size)
		v = s_.data();
	else
		v.clear();
	return true;
}

bool Write(const Writer &i, const Handle &h, const std::string &v) {
	const auto size = uint16_t(v.size());
	return Write(i, h, size) && i.write(h, v.data(), size) == size;
}

//
bool SkipString(const Reader &i, const Handle &h) {
	uint16_t size;
	if (!Read<uint16_t>(i, h, size))
		return false;
	return Seek(i, h, size, SM_Current);
}

//
size_t Tell(const Reader &i, const Handle &h) { return i.tell(h); }
size_t Tell(const Writer &i, const Handle &h) { return i.tell(h); }

//
bool Seek(const Reader &i, const Handle &h, ptrdiff_t offset, SeekMode mode) { return i.seek(h, offset, mode); }
bool Seek(const Writer &i, const Handle &h, ptrdiff_t offset, SeekMode mode) { return i.seek(h, offset, mode); }

//
Data LoadData(const Reader &i, const Handle &h) {
	Data data;
	auto size = i.size(h);
	data.Skip(size);
	i.read(h, data.GetData(), data.GetSize());
	return data;
}

std::string LoadString(const Reader &i, const Handle &h) {
	auto size = i.size(h);
	std::string str(size, 0);
	i.read(h, &str[0], size);
	return str;
}

} // namespace hg
