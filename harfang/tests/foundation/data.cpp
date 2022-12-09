// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#if !_WIN32
#	include <fcntl.h>
#	include <sys/types.h>
#	include <sys/stat.h>
#endif

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/cext.h"

#include "foundation/data.h"

#include "foundation/dir.h"
#include "foundation/file.h"

#include "../utils.h"

using namespace hg;

static int g_alloc_sentinel = 0;

void *operator new[](const size_t size) {
	++g_alloc_sentinel;
	return operator new(size);
}

#if __cplusplus < 201103L
void operator delete[](void *block)
#else
void operator delete[](void *block) noexcept
#endif
{
	--g_alloc_sentinel;
	operator delete(block);
}

void test_data() {
	{
		Data d0;
		TEST_CHECK(d0.Empty() == true);
		TEST_CHECK(d0.GetData() == nullptr);
		TEST_CHECK(d0.GetSize() == 0);
		TEST_CHECK(d0.GetCapacity() == 0);
		TEST_CHECK(d0.GetCursor() == 0);

		Data d1(d0);
		TEST_CHECK(d1.Empty() == true);
		TEST_CHECK(d1.GetData() == nullptr);
		TEST_CHECK(d1.GetSize() == 0);
		TEST_CHECK(d1.GetCapacity() == 0);
		TEST_CHECK(d1.GetCursor() == 0);
	}
	{
		Data d0(10);
		TEST_CHECK(d0.Empty() == false);
		TEST_CHECK(d0.GetSize() == 10);
		TEST_CHECK(d0.GetCapacity() == 8192);
		TEST_CHECK(d0.GetCursor() == 0);

		d0.SetCursor(10000);
		TEST_CHECK(d0.GetCursor() == 10000);
		TEST_CHECK(d0.GetSize() == 10);
		TEST_CHECK(d0.GetCapacity() == 16384);

		d0.Resize(4000);
		TEST_CHECK(d0.GetCursor() == 4000);
		TEST_CHECK(d0.GetSize() == 4000);
		TEST_CHECK(d0.GetCapacity() == 16384);

		d0.Skip(1000);
		TEST_CHECK(d0.GetCursor() == 5000);
		TEST_CHECK(d0.GetSize() == 5000);
		TEST_CHECK(d0.GetCapacity() == 16384);

		d0.Rewind();
		TEST_CHECK(d0.GetCursor() == 0);

		for (unsigned int i = 0; i < 50; i++) {
			TEST_CHECK(Write(d0, i) == true);
		}
		TEST_CHECK(d0.GetCursor() == (50 * sizeof(unsigned int)));

		d0.SetCursor(d0.GetSize());
		for (unsigned int i = 1000; i < 1040; i++) {
			TEST_CHECK(Write(d0, i) == true);
		}
		TEST_CHECK(d0.GetSize() == (5000 + (40 * sizeof(unsigned int))));

		d0.Rewind();
		for (unsigned int i = 0; i < 50; i++) {
			unsigned int v;
			TEST_CHECK(Read(d0, v) == true);
			TEST_CHECK(v == i);
		}
		TEST_CHECK(d0.GetCursor() == (50 * sizeof(unsigned int)));

		d0.SetCursor(5000);
		for (unsigned int i = 1000; i < 1040; i++) {
			unsigned int v;
			TEST_CHECK(Read(d0, v) == true);
			TEST_CHECK(v == i);
		}
		TEST_CHECK(d0.GetCursor() == d0.GetSize());
		{
			unsigned int v;
			TEST_CHECK(Read(d0, v) == false);
		}

		Data d1(d0);
		TEST_CHECK(d1.GetCursor() == d0.GetCursor());
		TEST_CHECK(d1.GetSize() == d0.GetSize());
		TEST_CHECK(d1.GetCapacity() == 8192);
		TEST_CHECK(d1.GetCursorPtr() != d0.GetCursorPtr());
		TEST_CHECK(d1.GetData() != d0.GetData());

		d1.Rewind();
		d0.Rewind();
		for (unsigned int i = 0; i < 50; i++) {
			unsigned int v0, v1;
			TEST_CHECK(Read(d0, v0) == true);
			TEST_CHECK(Read(d1, v1) == true);
			TEST_CHECK(v1 == v0);
		}
		d0.SetCursor(5000);
		d1.SetCursor(5000);
		for (unsigned int i = 1000; i < 1040; i++) {
			unsigned int v0, v1;
			TEST_CHECK(Read(d0, v0) == true);
			TEST_CHECK(Read(d1, v1) == true);
			TEST_CHECK(v1 == v0);
		}
	}
	{
		Data d0(reinterpret_cast<const uint8_t *>(hg::test::LoremIpsum.data()), hg::test::LoremIpsum.size());
		TEST_CHECK(d0.GetSize() == hg::test::LoremIpsum.size());

		d0.Rewind();

		{
			std::string str;
			str.resize(d0.GetSize());
			TEST_CHECK(d0.Read(&str[0], str.size()) == str.size());
			TEST_CHECK(str == hg::test::LoremIpsum);
		}

		d0.Reset();
		TEST_CHECK(d0.Empty() == true);
		TEST_CHECK(d0.GetSize() == 0);
		TEST_CHECK(d0.GetCursor() == 0);

		TEST_CHECK(Write(d0, hg::test::LoremIpsum) == true);

		d0.Rewind();

		{
			std::string str;
			TEST_CHECK(Read(d0, str) == true);
			TEST_CHECK(str == hg::test::LoremIpsum);
		}

		{
			std::string str(hg::test::LoremIpsum);

			d0.Reset();
			TEST_CHECK(Read(d0, str) == false);

			Write<uint16_t>(d0, 4);
			d0.Rewind();
			TEST_CHECK(Read(d0, str) == false);

			d0.Reset();
			Write<uint16_t>(d0, 0);
			Write<char>(d0, '\0');
			d0.Rewind();
			TEST_CHECK(Read(d0, str) == true);
			TEST_CHECK(str.empty());
		}
	}

	{
		g_alloc_sentinel = 0;

		size_t size = hg::test::LoremIpsum.size();
		char *data = new char[size];
		std::copy(hg::test::LoremIpsum.begin(), hg::test::LoremIpsum.end(), data);

		TEST_CHECK(g_alloc_sentinel == 1);
		{
			Data d0(reinterpret_cast<uint8_t *>(data), size);
			TEST_CHECK(d0.Empty() == false);
			TEST_CHECK(d0.GetSize() == size);
			TEST_CHECK(d0.GetCapacity() == 0);
			TEST_CHECK(g_alloc_sentinel == 1);
		}
		TEST_CHECK(g_alloc_sentinel == 1);

		{
			Data d0(reinterpret_cast<uint8_t *>(data), size);
			TEST_CHECK(d0.Empty() == false);
			TEST_CHECK(d0.GetSize() == size);
			TEST_CHECK(d0.GetCapacity() == 0);

			d0.TakeOwnership();
			TEST_CHECK(g_alloc_sentinel == 2);
			TEST_CHECK(d0.Empty() == false);
			TEST_CHECK(d0.GetSize() == size);
			TEST_CHECK(d0.GetCapacity() == 8192);

			d0.Rewind();
			for (size_t i = 0; i < size; i++) {
				char c = Read<char>(d0);
				TEST_CHECK(c == data[i]);
			}
		}
		TEST_CHECK(g_alloc_sentinel == 1);

		delete[] data;
	}

	{
		std::string filename = hg::test::CreateTempFilepath();

		Data d0;
		TEST_CHECK(Write(d0, hg::test::LoremIpsum) == true);
		TEST_CHECK(SaveDataToFile(filename.c_str(), d0) == true);

		Data d1;
		TEST_CHECK(LoadDataFromFile(filename.c_str(), d1) == true);
		TEST_CHECK(d1.Empty() == false);
		TEST_CHECK(d1.GetSize() == d0.GetSize());

		d1.Rewind();

		std::string str;
		TEST_CHECK(Read(d1, str) == true);
		TEST_CHECK(str == hg::test::LoremIpsum);

		TEST_CHECK(Unlink(filename.c_str()) == true);

		TEST_CHECK(LoadDataFromFile(filename.c_str(), d1) == false);

#if _WIN32
		TEST_CHECK(SaveDataToFile("ZZ:\\bla.bin", d0) == false);
#else
		int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR);
		TEST_CHECK(fd >= 0);
		close(fd);

		TEST_CHECK(chmod(filename.c_str(), 0444) == 0);

		TEST_CHECK(SaveDataToFile(filename.c_str(), d1) == false);

		TEST_CHECK(chmod(filename.c_str(), 0644) == 0);
		TEST_CHECK(Unlink(filename.c_str()) == true);
#endif
	}
}
