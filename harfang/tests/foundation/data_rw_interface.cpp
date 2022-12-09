// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/data_rw_interface.h"

#include "foundation/cext.h"

#include "../utils.h"

using namespace hg;

void test_data_rw_interface() {
	const uint32_t u32 = 0xc0ffee;
	const uint16_t u16 = 0xcafe;
	const uint64_t u64 = 0xcaca0c0c0a;

	{ 
		Data d0;

		TEST_CHECK(Write<uint32_t>(d0, u32) == true);
		TEST_CHECK(Write<uint16_t>(d0, u16) == true);
		TEST_CHECK(Write(d0, hg::test::LoremIpsum) == true);
		TEST_CHECK(Write<uint64_t>(d0, u64) == true);
		
		DataReadHandle h(d0); 
	
		TEST_CHECK(g_data_reader.is_valid(h) == true);
		TEST_CHECK(g_data_reader.size(h) == d0.GetSize());
		TEST_CHECK(g_data_reader.tell(h) == d0.GetSize());
		TEST_CHECK(g_data_reader.is_eof(h) == true);

		TEST_CHECK(Tell(g_data_reader, h) == d0.GetSize());
		TEST_CHECK(Seek(g_data_reader, h, 0, SM_Start) == true);
		TEST_CHECK(Tell(g_data_reader, h) == 0);
		TEST_CHECK(Seek(g_data_reader, h, 0, SM_End) == true);
		TEST_CHECK(Tell(g_data_reader, h) == d0.GetSize());
		TEST_CHECK(Seek(g_data_reader, h, -d0.GetSize(), SM_Current) == true);
		TEST_CHECK(Tell(g_data_reader, h) == 0);


		uint32_t v0;
		uint16_t v1;
		uint64_t v2;
		std::string str;

		TEST_CHECK(Read<uint32_t>(g_data_reader, h, v0) == true);
		TEST_CHECK(v0 == u32);
		TEST_CHECK(Read<uint16_t>(g_data_reader, h, v1) == true);
		TEST_CHECK(v1 == u16);
		TEST_CHECK(Read(g_data_reader, h, str) == true);
		TEST_CHECK(str == hg::test::LoremIpsum);
		TEST_CHECK(Read<uint64_t>(g_data_reader, h, v2) == true);
		TEST_CHECK(v2 == u64);
	}

	{
		Data d0;

		DataWriteHandle h(d0);

		TEST_CHECK(g_data_writer.is_valid(h) == true);
		TEST_CHECK(g_data_writer.tell(h) == 0);

		TEST_CHECK(Tell(g_data_reader, h) == 0);
	
		TEST_CHECK(Write<uint32_t>(g_data_writer, h, u32) == true);
		TEST_CHECK(Write(g_data_writer, h, hg::test::LoremIpsum) == true);

		size_t offset = Tell(g_data_reader, h);

		TEST_CHECK(Write<uint64_t>(g_data_writer, h, u64) == true);
		TEST_CHECK(Write<uint16_t>(g_data_writer, h, u16) == true);
		
		TEST_CHECK(Tell(g_data_reader, h) == d0.GetSize());

		d0.Rewind();

		{
			uint32_t v0;
			uint16_t v1;
			uint64_t v2;
			std::string str;

			TEST_CHECK(Read<uint32_t>(d0, v0) == true);
			TEST_CHECK(v0 == u32);
			TEST_CHECK(Read(d0, str) == true);
			TEST_CHECK(str == hg::test::LoremIpsum);
			TEST_CHECK(Read<uint64_t>(d0, v2) == true);
			TEST_CHECK(v2 == u64);
			TEST_CHECK(Read<uint16_t>(d0, v1) == true);
			TEST_CHECK(v1 == u16);
		}

		TEST_CHECK(Seek(g_data_writer, h, offset, SM_Start) == true);

		TEST_CHECK(Write<uint16_t>(g_data_writer, h, u16) == true);
		TEST_CHECK(Write<uint64_t>(g_data_writer, h, u64) == true);
		
		d0.SetCursor(offset);

		{
			uint16_t v1;
			uint64_t v2;
		
			TEST_CHECK(Read<uint16_t>(d0, v1) == true);
			TEST_CHECK(v1 == u16);
			TEST_CHECK(Read<uint64_t>(d0, v2) == true);
			TEST_CHECK(v2 == u64);
		}

		ptrdiff_t delta = d0.GetCursor() - offset;
		TEST_CHECK(Seek(g_data_writer, h, -delta, SM_End) == true);
		{
			uint8_t emiprs[8] = {73, 79, 97, 107, 113, 149, 157, 167};
			for (int i = 0; i < 8; i++) {
				TEST_CHECK(Write<uint8_t>(g_data_writer, h, emiprs[i]) == true);
			}
		
			delta = d0.GetCursor() - offset;
			TEST_CHECK(Seek(g_data_writer, h, -delta, SM_Current) == true);

			for (int i = 0; i < 8; i++) {
				uint8_t b;
				TEST_CHECK(Read<uint8_t>(d0, b) == true);
				TEST_CHECK(b == emiprs[i]);
			}
		}		
	}
}
