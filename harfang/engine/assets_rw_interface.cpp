// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/assets_rw_interface.h"
#include "engine/assets.h"

namespace hg {

const Reader g_assets_reader = {
	[](Handle hnd, void *data, size_t size) -> size_t { return Read(reinterpret_cast<Asset &>(hnd), data, size); },
	[](Handle hnd) -> size_t { return GetSize(reinterpret_cast<Asset &>(hnd)); },
	[](Handle hnd, ptrdiff_t offset, SeekMode mode) -> bool { return Seek(reinterpret_cast<Asset &>(hnd), offset, mode); },
	[](Handle hnd) -> size_t { return Tell(reinterpret_cast<Asset &>(hnd)); },
	[](Handle hnd) -> bool { return reinterpret_cast<Asset &>(hnd).ref != invalid_gen_ref; },
	[](Handle hnd) -> bool { return IsEOF(reinterpret_cast<Asset &>(hnd)); },
};

const ReadProvider g_assets_read_provider = {
	[](const char *path, bool silent) {
		Handle hnd;
		reinterpret_cast<Asset &>(hnd) = OpenAsset(path, silent);
		return hnd;
	},
	[](Handle hnd) { Close(reinterpret_cast<Asset &>(hnd)); },
	[](const char *path) { return IsAssetFile(path); },
};

} // namespace hg
