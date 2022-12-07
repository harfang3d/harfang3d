// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <vector>
#include <algorithm>

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/cext.h"

#include "foundation/rw_interface.h"

#include "foundation/data.h"

#include "../utils.h"

using namespace hg;

static size_t DummyReaderRead(Handle h, void* data, size_t size) { 
	Data *buffer = *(reinterpret_cast<Data **>(&h.v[0])); 
	return buffer->Read(data, size);
}

static size_t DummyReaderSize(Handle h) {
	Data *buffer = *(reinterpret_cast<Data **>(&h.v[0]));
	return buffer->GetSize();
}

static bool DummyReaderSeek(Handle h, ptrdiff_t offset, SeekMode mode) {
	Data *buffer = *(reinterpret_cast<Data **>(&h.v[0]));
	if (mode == SM_Start) {
		if ((offset < 0) || (offset > buffer->GetSize())) {
			return false;
		}
		buffer->SetCursor(offset);
	} else if (mode == SM_Current) {
		if (offset >= 0) {
			if ((buffer->GetCursor() + offset) > buffer->GetSize()) {
				return false;
			}
		} else if (offset < 0) {
			if ((buffer->GetCursor() + offset) < 0) {
				return false;
			}
		}
		buffer->SetCursor(buffer->GetCursor() + offset);
	} else {
		if ((offset > 0) || (offset < -buffer->GetSize())) {
			return false;
		}
		buffer->SetCursor(buffer->GetSize() + offset);
	}
	return true;
}

static size_t DummyReaderTell(Handle h) {
	Data *buffer = *(reinterpret_cast<Data **>(&h.v[0]));
	return buffer->GetCursor();
}

static bool DummyReaderIsValid(Handle h) {
	Data *buffer = *(reinterpret_cast<Data **>(&h.v[0]));
	return !buffer->Empty();
}

static bool DummyReaderIsEOF(Handle h) {
	Data *buffer = *(reinterpret_cast<Data **>(&h.v[0]));
	return (buffer->GetCursor() >= buffer->GetSize());
}

static Handle DummyReaderOpen(const char* path, bool silent) {
	Handle h;
	Data *buffer = new Data;
	*(reinterpret_cast<Data **>(&h.v[0])) = buffer;
	if (strcmp(path, "valid") == 0) {
		Write(*buffer, hg::test::LoremIpsum);
		Write<uint32_t>(*buffer, 0xc0ffee);
		Write<uint16_t>(*buffer, 0xcafe);
		buffer->Rewind();
	}
	return h;
}

static void DummyReaderClose(Handle h) {
	Data *buffer = *(reinterpret_cast<Data **>(&h.v[0]));
	delete buffer;
	*(reinterpret_cast<Data **>(&h.v[0])) = nullptr;
}

static bool DummyReaderIsFile(const char *path) { return (strlen(path) != 0); }

static void test_reader_interface() { 
	Data buffer;

	Reader reader;
	reader.read = DummyReaderRead;
	reader.size = DummyReaderSize;
	reader.seek = DummyReaderSeek;
	reader.tell = DummyReaderTell;
	reader.is_valid = DummyReaderIsValid;
	reader.is_eof = DummyReaderIsEOF;

	Handle h;
	*(reinterpret_cast<Data **>(&h.v[0])) = &buffer;

#ifdef ENABLE_BINARY_DEBUG_HANDLE
	h.debug = true;
#endif

	{
		TEST_CHECK(Tell(reader, h) == 0);
#if defined(ENABLE_BINARY_DEBUG_HANDLE)
		TEST_CHECK(Write<uint16_t>(buffer, sizeof(uint16_t)) == true);
#endif
		TEST_CHECK(Write(buffer, hg::test::LoremIpsum) == true);
		buffer.Rewind();

		std::string str;
		TEST_CHECK(Read(reader, h, str) == true);
		TEST_CHECK(str == hg::test::LoremIpsum);
		TEST_CHECK(Tell(reader, h) == buffer.GetSize());
		TEST_CHECK(Seek(reader, h, 0, SM_Start) == true);
		TEST_CHECK(Tell(reader, h) == 0);
	}
#if defined(ENABLE_BINARY_DEBUG_HANDLE)
	{ 
		buffer.Reset();
		TEST_CHECK(Write<uint32_t>(buffer, 0xc0ffee) == true);

		uint32_t v;
		TEST_CHECK(Read<uint32_t>(reader, h, v) == false);
	}
#endif
	{ 
		std::string str;

		buffer.Reset();

		TEST_CHECK(Read(reader, h, str) == false);

		buffer.Reset();
#if defined(ENABLE_BINARY_DEBUG_HANDLE)
		TEST_CHECK(Write<uint16_t>(buffer, sizeof(uint16_t)) == true);
#endif
		TEST_CHECK(Write<uint16_t>(buffer, hg::test::LoremIpsum.size()) == true);
		TEST_CHECK(buffer.Write(&hg::test::LoremIpsum[0], 10) == 10);
		buffer.Rewind();

		TEST_CHECK(Read(reader, h, str) == false);

		buffer.Reset();

#if defined(ENABLE_BINARY_DEBUG_HANDLE)
		TEST_CHECK(Write<uint16_t>(buffer, sizeof(uint16_t)) == true);
#endif
		TEST_CHECK(Write<uint16_t>(buffer, 0) == true);
		buffer.Rewind();

		str = "this should be cleared";
		TEST_CHECK(Read(reader, h, str) == true);
		TEST_CHECK(str.empty());
	}

	{
		buffer.Reset();

		uint32_t v0 = 0xcafe;
#if defined(ENABLE_BINARY_DEBUG_HANDLE) 
		TEST_CHECK(Write<uint16_t>(buffer, sizeof(uint32_t)) == true);
#endif
		Write<uint32_t>(buffer, v0);
		buffer.Rewind();

		uint32_t v1;
		TEST_CHECK(Read<uint32_t>(reader, h, v1) == true);
		TEST_CHECK(v0 == v1);

		TEST_CHECK(Seek(reader, h, -sizeof(uint32_t), SM_End) == true);
#if defined(ENABLE_BINARY_DEBUG_HANDLE)
		TEST_CHECK(Seek(reader, h, -sizeof(uint16_t), SM_Current) == true);
#endif
		TEST_CHECK(Tell(reader, h) == 0);
	}
	{
		buffer.Reset();

#if defined(ENABLE_BINARY_DEBUG_HANDLE)
		TEST_CHECK(Write<uint16_t>(buffer, sizeof(uint16_t)) == true);
#endif
		TEST_CHECK(Write(buffer, hg::test::LoremIpsum) == true);
			
		size_t after_string = Tell(reader, h);

		uint64_t v0 = 0xfacade;
#if defined(ENABLE_BINARY_DEBUG_HANDLE)
		TEST_CHECK(Write<uint16_t>(buffer, sizeof(uint64_t)) == true);
#endif
		TEST_CHECK(Write<uint64_t>(buffer, v0) == true);

		size_t after_facade = Tell(reader, h);

		uint64_t v1 = 0xf00dc0ffee;
#if defined(ENABLE_BINARY_DEBUG_HANDLE)
		TEST_CHECK(Write<uint16_t>(buffer, sizeof(uint64_t)) == true);
#endif
		TEST_CHECK(Write<uint64_t>(buffer, v1) == true);
			
		buffer.Rewind();

		TEST_CHECK(Tell(reader, h) == 0);

		TEST_CHECK(SkipString(reader, h) == true);

		TEST_CHECK(Tell(reader, h) == after_string);

		TEST_CHECK(Skip<uint64_t>(reader, h) == true);

		TEST_CHECK(Tell(reader, h) == after_facade);

		uint64_t v2;
		TEST_CHECK(Read<uint64_t>(reader, h, v2) == true);
		TEST_CHECK(v2 == v1);

		TEST_CHECK(SkipString(reader, h) == false);
		TEST_CHECK(Skip<uint32_t>(reader, h) == false);
	}
	{ 
		buffer.Reset();
#if defined(ENABLE_BINARY_DEBUG_HANDLE)
		TEST_CHECK(Write<uint16_t>(buffer, sizeof(uint16_t)) == true);
#endif
		TEST_CHECK(Write(buffer, hg::test::LoremIpsum) == true);
		buffer.Rewind();

		Data d0 = LoadData(reader, h);
		TEST_CHECK(d0.Empty() == false);
		TEST_CHECK(d0.GetCursor() == d0.GetSize());

		d0.Rewind();

		std::string str;
#if defined(ENABLE_BINARY_DEBUG_HANDLE)
		TEST_CHECK(d0.Skip(sizeof(uint16_t)) == true);
#endif
		TEST_CHECK(Read(d0, str) == true);
		TEST_CHECK(str == hg::test::LoremIpsum);
	}

	{
		ReadProvider provider;
		provider.open = DummyReaderOpen;
		provider.close = DummyReaderClose;
		provider.is_file = DummyReaderIsFile;

		TEST_CHECK(Exists(reader, provider, "valid") == true);
		TEST_CHECK(Exists(reader, provider, "invalid") == false);

		{ 
			ScopedReadHandle hr(provider, "valid", true);
			std::string str;
			uint32_t d0;
			uint16_t w0;

			TEST_CHECK(Read(reader, hr, str) == true);
			TEST_CHECK(Read<uint32_t>(reader, hr, d0) == true);
			TEST_CHECK(Read<uint16_t>(reader, hr) == 0xcafe);
			TEST_CHECK(str == hg::test::LoremIpsum);
			TEST_CHECK(d0 == 0xc0ffee);
		}
	}
}

static bool g_next_op_will_fail = false;

static void will_fail() { g_next_op_will_fail = true; }
static void will_succeed() { g_next_op_will_fail = false; }

static size_t DummyWriterWrite(Handle h, const void* data, size_t size) {
	if (bool_gate(g_next_op_will_fail)) {
		return 0;
	}
	Data *buffer = *(reinterpret_cast<Data **>(&h.v[0]));
	return buffer->Write(data, size);
}
static bool DummyWriterSeek(Handle h, ptrdiff_t offset, SeekMode mode) {
	if (bool_gate(g_next_op_will_fail)) {
		return false;
	}
	Data *buffer = *(reinterpret_cast<Data **>(&h.v[0]));
	if (mode == SM_Start) {
		if (offset < 0) {
			return false;
		}
		buffer->SetCursor(offset);
	} else if (mode == SM_Current) {
		if (offset < 0) {
			if ((buffer->GetCursor() + offset) < 0) {
				return false;
			}
		}
		buffer->SetCursor(buffer->GetCursor() + offset);
	} else {
		if (offset < -buffer->GetSize()) {
			return false;
		}
		buffer->SetCursor(buffer->GetSize() + offset);
	}
	return true;
}
static size_t DummyWriterTell(Handle h) {
	if (bool_gate(g_next_op_will_fail)) {
		return 0;
	}
	Data *buffer = *(reinterpret_cast<Data **>(&h.v[0]));
	return buffer->GetCursor();
}
static bool DummyWriterIsValid(Handle h) { 
	if (bool_gate(g_next_op_will_fail)) {
		return false;
	}
	Data *buffer = *(reinterpret_cast<Data **>(&h.v[0]));
	return (buffer != nullptr);
}

static Handle DummyWriterOpen(const char* path) {
	if (bool_gate(g_next_op_will_fail)) {
		return Handle();
	}
	Handle h;
	Data *buffer = nullptr;
	if (strcmp(path, "valid") == 0) {
		buffer = new Data;
	}
	*(reinterpret_cast<Data **>(&h.v[0])) = buffer;
	return h;
}

static void DummyWriterClose(Handle h) {
	Data *buffer = *(reinterpret_cast<Data **>(&h.v[0]));
	if (buffer) {
		delete buffer;
		*(reinterpret_cast<Data **>(&h.v[0])) = nullptr;
	}
}

static void test_writer_interface() {
	Data buffer;

	Writer writer;
	writer.write = DummyWriterWrite;
	writer.seek = DummyWriterSeek;
	writer.tell = DummyWriterTell;
	writer.is_valid = DummyWriterIsValid;

	Reader reader;
	reader.read = DummyReaderRead;
	reader.size = DummyReaderSize;
	reader.seek = DummyReaderSeek;
	reader.tell = DummyReaderTell;
	reader.is_valid = DummyReaderIsValid;
	reader.is_eof = DummyReaderIsEOF;

	Handle h;
	*(reinterpret_cast<Data **>(&h.v[0])) = &buffer;

#ifdef ENABLE_BINARY_DEBUG_HANDLE
	h.debug = true;
#endif

	uint64_t v0, v1;
	v0 = 0x1cec0ffee;
		
	TEST_CHECK(Tell(writer, h) == 0);
		
	DeferredWrite<uint64_t> dw(writer, h);
		
	size_t before_string = Tell(writer, h);
	TEST_CHECK(Write(writer, h, hg::test::LoremIpsum) == true);

	TEST_CHECK(dw.Commit(v0) == true);

	TEST_CHECK(Seek(writer, h, before_string, SM_Start) == true);
	TEST_CHECK(Tell(writer, h) == before_string);

	std::string str;
	TEST_CHECK(Read(reader, h, str) == true);
	TEST_CHECK(str == hg::test::LoremIpsum);

	TEST_CHECK(Seek(writer, h, -buffer.GetSize(), SM_End) == true);
	TEST_CHECK(Tell(writer, h) == 0);

	TEST_CHECK(Read<uint64_t>(reader, h, v1) == true);
	TEST_CHECK(v1 == v0);

	TEST_CHECK(Tell(writer, h) == before_string);

	WriteProvider provider;
	provider.open = DummyWriterOpen;
	provider.close = DummyWriterClose;

	{ 
#ifdef ENABLE_BINARY_DEBUG_HANDLE
		ScopedWriteHandle hw(provider, "valid", true);
#else
		ScopedWriteHandle hw(provider, "valid");
#endif
		TEST_CHECK(Write<uint32_t>(writer, hw, 0xbeef) == true);

#ifdef ENABLE_BINARY_DEBUG_HANDLE
		will_fail();
		TEST_CHECK(Write<uint16_t>(writer, hw, 0xcafe) == false);
#endif
	}
}

void test_rw_interface() { 
	test_reader_interface();	
	test_writer_interface();
}