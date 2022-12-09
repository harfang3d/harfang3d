// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/file_rw_interface.h"

#include "foundation/cext.h"
#include "foundation/file.h"

#include "../utils.h"

using namespace hg;

void test_file_rw_interface() {
	static const uint32_t u32 = 0xc0ffee;
	static const uint16_t u16 = 0xcafe;
	static const uint64_t u64 = 0xcaca0c0c0a;
	static const uint8_t emiprs[8] = {73, 79, 97, 107, 113, 149, 157, 167};

	{ 
		std::string filename = hg::test::CreateTempFilepath();

		{
			ScopedWriteHandle h(g_file_write_provider, filename.c_str());

			TEST_CHECK(g_file_writer.is_valid(h) == true);
			TEST_CHECK(g_file_writer.tell(h) == 0);

			for (int i = 0; i < 8; i++) {
				TEST_CHECK(Write<uint8_t>(g_file_writer, h, emiprs[i]) == true);
			}

			DeferredWrite<uint32_t> c0ffee_writer(g_file_writer, h);

			TEST_CHECK(Write(g_file_writer, h, hg::test::LoremIpsum) == true);
			TEST_CHECK(Write<uint64_t>(g_file_writer, h, u64) == true);
			TEST_CHECK(Write<uint16_t>(g_file_writer, h, u16) == true);

			TEST_CHECK(c0ffee_writer.Commit(u32) == true);

			g_file_write_provider.close(h);
			DeferredWrite<uint8_t> fail(g_file_writer, h);
			TEST_CHECK(fail.Commit(0xcd) == false);
		}

		TEST_CHECK(Exists(g_file_reader, g_file_read_provider, filename.c_str()) == true);
		TEST_CHECK(g_file_read_provider.is_file(filename.c_str()) == true);
		TEST_CHECK(g_file_read_provider.is_file("/tmp/") == false);

		{ 
			ScopedReadHandle h(g_file_read_provider, filename.c_str());

			uint32_t v0;
			uint16_t v1;
			uint64_t v2;
			uint8_t buffer[8];
			std::string str;

			for (int i = 0; i < 8; i++) {
				TEST_CHECK(Read<uint8_t>(g_file_reader, h, buffer[i]) == true);
			}
			for (int i = 0; i < 8; i++) {
				TEST_CHECK(buffer[i] == emiprs[i]);
			}

			size_t size = g_file_reader.size(h);
			size_t offset[3];

			offset[0] = Tell(g_file_reader, h);
			TEST_CHECK(Skip<uint32_t>(g_file_reader, h) == true);
			offset[1] = Tell(g_file_reader, h);
			TEST_CHECK(SkipString(g_file_reader, h) == true);
			offset[2] = Tell(g_file_reader, h);
			TEST_CHECK(Skip<uint64_t>(g_file_reader, h) == true);
			TEST_CHECK(Read<uint16_t>(g_file_reader, h, v1) == true);
			
			TEST_CHECK(v1 == u16);

			TEST_CHECK(Seek(g_file_reader, h, offset[1], SM_Start));
			TEST_CHECK(Read(g_file_reader, h, str) == true);
			
			TEST_CHECK(str == hg::test::LoremIpsum);

			TEST_CHECK(Seek(g_file_reader, h, offset[0] - offset[2], SM_Current) == true);
			TEST_CHECK(Read<uint32_t>(g_file_reader, h, v0) == true);

			TEST_CHECK(v0 == u32);

			TEST_CHECK(Seek(g_file_reader, h, offset[2] - size, SM_End) == true);
			TEST_CHECK(Read<uint64_t>(g_file_reader, h, v2) == true);

			TEST_CHECK(v2 == u64);

			TEST_CHECK(Read<uint64_t>(g_file_reader, h, v2) == false);

			TEST_CHECK(g_file_reader.is_eof(h) == true);
		}

		Unlink(filename.c_str());
	}
}