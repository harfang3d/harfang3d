// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/data_rw_interface.h"
#include "foundation/data.h"

namespace hg {

const Reader g_data_reader = {
	[](Handle hnd, void *data, size_t size) { return (*reinterpret_cast<const Data **>(&hnd))->Read(data, size); },
	[](Handle hnd) -> size_t { return (*reinterpret_cast<const Data **>(&hnd))->GetSize(); },
	[](Handle hnd, ptrdiff_t offset, SeekMode mode) -> bool {
		auto data = (*reinterpret_cast<Data **>(&hnd));

		if (mode == SM_Start)
			data->SetCursor(offset);
		else if (mode == SM_Current)
			data->SetCursor(data->GetCursor() + offset);
		else if (mode == SM_End)
			data->SetCursor(data->GetSize() + offset);

		return true;
	},
	[](Handle hnd) -> size_t { return (*reinterpret_cast<const Data **>(&hnd))->GetCursor(); },
	[](Handle hnd) -> bool { return *reinterpret_cast<const Data **>(&hnd) != nullptr; },
	[](Handle hnd) -> bool {
		auto data = *reinterpret_cast<const Data **>(&hnd);
		return data->GetCursor() >= data->GetSize();
	},
};

const Writer g_data_writer = {
	[](Handle hnd, const void *data, size_t size) { return (*reinterpret_cast<Data **>(&hnd))->Write(data, size); },
	[](Handle hnd, ptrdiff_t offset, SeekMode mode) -> bool {
		auto data = (*reinterpret_cast<Data **>(&hnd));

		if (mode == SM_Start)
			data->SetCursor(offset);
		else if (mode == SM_Current)
			data->SetCursor(data->GetCursor() + offset);
		else if (mode == SM_End)
			data->SetCursor(data->GetSize() + offset);

		return true;
	},
	[](Handle hnd) -> size_t { return (*reinterpret_cast<const Data **>(&hnd))->GetCursor(); },
	[](Handle hnd) -> bool { return *reinterpret_cast<Data **>(&hnd) != nullptr; },
};

#ifdef ENABLE_BINARY_DEBUG_HANDLE
DataReadHandle::DataReadHandle(const Data &data, bool debug) {
	*reinterpret_cast<const Data **>(&h_) = &data;
	h_.debug = debug;
}

DataWriteHandle::DataWriteHandle(Data &data, bool debug) {
	*reinterpret_cast<Data **>(&h_) = &data;
	h_.debug = debug;
}
#else
DataReadHandle::DataReadHandle(const Data &data) { *reinterpret_cast<const Data **>(&h_) = &data; }
DataWriteHandle::DataWriteHandle(Data &data) { *reinterpret_cast<Data **>(&h_) = &data; }
#endif

} // namespace hg
