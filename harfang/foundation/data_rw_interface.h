// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/data.h"
#include "foundation/rw_interface.h"

namespace hg {

extern const Reader g_data_reader;
extern const Writer g_data_writer;

struct DataReadHandle {
#ifdef ENABLE_BINARY_DEBUG_HANDLE
	DataReadHandle(const Data &data, bool debug = false);
#else
	DataReadHandle(const Data &data);
#endif
	operator const Handle &() const { return h_; }
private:
	Handle h_;
};

struct DataWriteHandle {
#ifdef ENABLE_BINARY_DEBUG_HANDLE
	DataWriteHandle(Data &data, bool debug = false);
#else
	DataWriteHandle(Data &data);
#endif
	operator const Handle &() const { return h_; }
private:
	Handle h_;
};

} // namespace hg
