// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <math.h>
#include <stdlib.h>

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/cext.h"

#include "foundation/file.h"

#include "foundation/path_tools.h"
#include "foundation/data.h"

#include "../utils.h"

using namespace hg;

static const char g_dummy_data[37] = "abcdefghijklmnopqrstuvwxyz0123456789";

 void CreateDummyFile(std::string &filename, size_t &size) { 
	
	filename = hg::test::CreateTempFilepath();
	FILE *f = fopen(filename.c_str(), "wb");
	if (f) {
		size = fwrite(g_dummy_data, 1, 37, f);
		fclose(f);
	}
}

void test_file() {
	{
		File f = Open("invalid.txt");
		TEST_CHECK(f.ref == invalid_gen_ref);
		TEST_CHECK(Close(f) == false);
		TEST_CHECK(IsValid(f) == false);
		TEST_CHECK(GetSize(f) == 0);

		FileInfo info = GetFileInfo("invalid.txt");
		TEST_CHECK(info.is_file == false);
		TEST_CHECK(info.size == 0);
		TEST_CHECK(info.created == 0);
		TEST_CHECK(info.modified == 0);
	}
	{
		File f = Open("invalid.txt", true);
		TEST_CHECK(f.ref == invalid_gen_ref);
		TEST_CHECK(Close(f) == false);
		TEST_CHECK(IsValid(f) == false);
	}
	{
		std::string filename;
		size_t size;
		CreateDummyFile(filename, size);

		FileInfo info = GetFileInfo(filename.c_str());
		TEST_CHECK(info.is_file == true);
		TEST_CHECK(info.size == size);

		std::string filepath = GetFilePath(filename);
		TEST_CHECK(IsFile(filename.c_str()) == true);
		TEST_CHECK(IsFile(filepath.c_str()) == false);

		TEST_CHECK(IsDirectory(filepath.c_str()) == true);
		TEST_CHECK(IsDirectory(filename.c_str()) == false);

		File f = Open(filename.c_str(), true);
		uint8_t dummy = 'A';

		TEST_CHECK(GetSize(f) == size);

		TEST_CHECK(Write(f, &dummy, 1) == 0);

		std::vector<char> buffer(size);

		TEST_CHECK(IsEOF(f) == false);
		TEST_CHECK(Tell(f) == 0);

		TEST_CHECK(Read(f, buffer.data(), size) == size);
		TEST_CHECK(memcmp(buffer.data(), g_dummy_data, size) == 0);

		memset(buffer.data(), 0, size);

		TEST_CHECK(Tell(f) == size);

		uint8_t byte = 0xff;
		TEST_CHECK(Read(f, &byte, 1) == 0);
		TEST_CHECK(IsEOF(f) == true);

		TEST_CHECK(Seek(f, 4, SM_Start));
		TEST_CHECK(Tell(f) == 4);
		TEST_CHECK(Seek(f, 4, SM_Current));
		TEST_CHECK(Tell(f) == 8);
		TEST_CHECK(Seek(f, -1, SM_Current));
		TEST_CHECK(Tell(f) == 7);
		TEST_CHECK(Seek(f, -4, SM_End));
		TEST_CHECK(Tell(f) == (size - 4));

		Rewind(f);
		TEST_CHECK(Tell(f) == 0);

		TEST_CHECK(IsValid(f));
		TEST_CHECK(Close(f));

		f = OpenText(filename.c_str());
		TEST_CHECK(IsValid(f));
		TEST_CHECK(Read(f, buffer.data(), size) == size);
		TEST_CHECK(memcmp(buffer.data(), g_dummy_data, size) == 0);
		TEST_CHECK(Close(f));

		TEST_CHECK(Unlink(filename.c_str()) == true);
		TEST_CHECK(IsFile(filename.c_str()) == false);
	}

	{
		std::string filename = hg::test::CreateTempFilepath();
		
		File f = OpenWriteText(filename.c_str());
		TEST_CHECK(Write(f, g_dummy_data, 36) == 36);
		Close(f);

		f = OpenAppendText(filename.c_str());
		TEST_CHECK(WriteStringAsText(f, g_dummy_data) == true);
		Close(f);

		FileInfo info;
		info = GetFileInfo(filename.c_str());
		TEST_CHECK(info.is_file == true);
		TEST_CHECK(info.size == 72);

		{
			std::string str = FileToString(filename.c_str());
			TEST_CHECK(memcmp(&str[0], g_dummy_data, 36) == 0);
			TEST_CHECK(memcmp(&str[36], g_dummy_data, 36) == 0);
		}

		{
			Data in;
			TEST_CHECK(FileToData("invalid.txt", in) == false);
		
			TEST_CHECK(FileToData(filename.c_str(), in) == true);
			in.SetCursor(0);
			TEST_CHECK(memcmp(in.GetCursorPtr(), g_dummy_data, 36) == 0);
			in.SetCursor(36);
			TEST_CHECK(memcmp(in.GetCursorPtr(), g_dummy_data, 36) == 0);
		}

		uint64_t data = 0xcafe0000cdcd;
		f = OpenWrite(filename.c_str());
		TEST_CHECK(Write(f, data) == true);
		Close(f);

		info = GetFileInfo(filename.c_str());
		TEST_CHECK(info.is_file == true);
		TEST_CHECK(info.size == sizeof(uint64_t));

		f = Open(filename.c_str());
		TEST_CHECK(Read<uint64_t>(f) == data);
		Close(f);

		TEST_CHECK(IsFile(filename.c_str()) == true);
		TEST_CHECK(Unlink(filename.c_str()) == true);
	}

	{
		std::string filename = hg::test::CreateTempFilepath();

		std::string input(g_dummy_data);

		File f = OpenWrite(filename.c_str());
		TEST_CHECK(WriteString(f, input) == true);
		Close(f);

		f = Open(filename.c_str());
		TEST_CHECK(ReadString(f) == input);
		Close(f);

		Unlink(filename.c_str());
	}

	{
		std::string filename_0 = hg::test::CreateTempFilepath();
		std::string filename_1 = hg::test::CreateTempFilepath();
		TEST_CHECK(filename_0 != filename_1);

		std::string input(g_dummy_data);
		TEST_CHECK(StringToFile(filename_0.c_str(), input.c_str()) == true);
		TEST_CHECK(CopyFile(filename_0.c_str(), filename_1.c_str()) == true);
		TEST_CHECK(FileToString(filename_1.c_str()) == input);

		Unlink(filename_0.c_str());
		Unlink(filename_1.c_str());
	}

	{ 
		ScopedFile file(Open("invalid.bin"));
		TEST_CHECK((file ==true) == false);
		TEST_CHECK(file == false);
	}

	{ 
		ScopedFile file(OpenWrite("dummy.bin"));
		TEST_CHECK(file == true);
		TEST_CHECK(Write(file, hg::test::LoremIpsum) == true);
	}
	TEST_CHECK(IsFile("dummy.bin"));

	Unlink("dummy.bin");
}
