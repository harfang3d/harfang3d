// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <math.h>

#if !_WIN32
#	include <sys/types.h>
#	include <sys/stat.h>
#endif

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/cext.h"

#include "foundation/dir.h"

#include "foundation/path_tools.h"
#include "foundation/file.h"

#include "../utils.h"

using namespace hg;

static void test_entries(DirEntry *expected, int count, std::vector<DirEntry> &entries) {
	for(size_t j=0; j<entries.size(); j++) {
		for(int i=0; i<count; i++) {
			if((entries[j].name == expected[i].name) && (entries[j].type == expected[i].type)) {
				expected[i].size++;
			}
		}
	}
	for(int i=0; i<count; i++) {
		TEST_CHECK(expected[i].size == 1);
	}
}

void test_dir() {
	std::string base = hg::test::GetTempDirectoryName();

	TEST_CHECK(IsDir(base.c_str()) == true);

	{
		std::string dirname = PathJoin(base, "hg_core_ut__00");
		TEST_CHECK(RmDir(dirname.c_str(), false) == false);
		TEST_CHECK(IsDir(dirname.c_str()) == false);
	}
	{
		std::string dirname = PathJoin(base, "hg_core_ut__00");

		TEST_CHECK(Exists(dirname.c_str()) == false);
		TEST_CHECK(MkDir(dirname.c_str(), 01755, true) == true);
		TEST_CHECK(IsDir(dirname.c_str()) == true);
		TEST_CHECK(Exists(dirname.c_str()) == true);
		TEST_CHECK(RmDir(dirname.c_str(), true) == true);
		TEST_CHECK(RmDir(dirname.c_str(), true) == false);
	}
	{
		std::vector<std::string> path(4);
		path[0] = base;
		path[1] = "hg_core_ut__00";
		path[2] = "0000";
		path[3] = "0010";

		std::string dirname = PathJoin(path);
		TEST_CHECK(MkDir(dirname.c_str(), 01755, true) == false);
		TEST_CHECK(MkTree(dirname.c_str()) == true);

		std::string subdirname = PathJoin(path[0], path[1]);
		TEST_CHECK(RmDir(subdirname.c_str()) == false);
		TEST_CHECK(RmTree(subdirname.c_str()) == true);

		TEST_CHECK(Exists(subdirname.c_str()) == false);
		TEST_CHECK(Exists(dirname.c_str()) == false);
	}
	{
		std::string root = PathJoin(base, "hg_core_ut__00");

		std::string path, lorem_filepath;

		path = PathJoin(root, "00", "a");
		lorem_filepath = PathJoin(path, "lorem.txt");
		TEST_CHECK(MkTree(path.c_str(), 01755, true) == true);
		TEST_CHECK(StringToFile(lorem_filepath.c_str(), hg::test::LoremIpsum.c_str()) == true);
		TEST_CHECK(IsDir(lorem_filepath.c_str()) == false);

		TEST_CHECK(MkTree(PathJoin(root, "00", "b").c_str(), 01755, true) == true);

		TEST_CHECK(MkTree(PathJoin(root, "01", "c").c_str(), 01755, true) == true);
		TEST_CHECK(MkTree(PathJoin(root, "01", "d").c_str(), 01755, true) == true);
		TEST_CHECK(CopyFile(lorem_filepath.c_str(), PathJoin(root, "01", "lorem2.txt").c_str()) == true);

		TEST_CHECK(MkTree(PathJoin(root, "02", "e").c_str(), 01755, true) == true);
		TEST_CHECK(MkTree(PathJoin(root, "02", "f").c_str(), 01755, true) == true);
		TEST_CHECK(MkTree(PathJoin(root, "02", "g").c_str(), 01755, true) == true);
		TEST_CHECK(MkTree(PathJoin(root, "02", "h").c_str(), 01755, true) == true);

		TEST_CHECK(CopyDirRecursive(PathJoin(root, "03").c_str(), PathJoin(root, "02", "h").c_str()) == false);
		TEST_CHECK(CopyDirRecursive(PathJoin(root, "00").c_str(), PathJoin(root, "02", "h").c_str()) == true);

		TEST_CHECK(IsFile(lorem_filepath.c_str()) == true);

		path = PathJoin(root, "02", "h");
		path = PathJoin(path, "a", "lorem.txt");
		TEST_CHECK(IsFile(path.c_str()) == true);

		std::vector<DirEntry> entries = ListDir(root.c_str(), DE_All);
		if (TEST_CHECK(entries.size() == 3)) {
			DirEntry expected[3] = {
				{ DE_Dir, "00", 0, 0},
				{ DE_Dir, "01", 0, 0},
				{ DE_Dir, "02", 0, 0}
			};
			test_entries(expected, 3, entries);
		}

		entries = ListDirRecursive(root.c_str(), DE_File);
		if (TEST_CHECK(entries.size() == 3)) {
			DirEntry expected[3] = {
				{ DE_File, "00/a/lorem.txt", 0, 0},
				{ DE_File, "01/lorem2.txt", 0, 0},
				{ DE_File, "02/h/a/lorem.txt", 0, 0}
			};
			test_entries(expected, 3, entries);
		}
		TEST_CHECK(GetDirSize(root.c_str()) == (3 * hg::test::LoremIpsum.size()));

		path = PathJoin(root, "00");
		TEST_CHECK(RmTree(path.c_str()) == true);
		TEST_CHECK(Exists(path.c_str()) == false);

		TEST_CHECK(GetDirSize(root.c_str()) == (2 * hg::test::LoremIpsum.size()));

		entries = ListDir(path.c_str());
		TEST_CHECK(entries.empty() == true);

		TEST_CHECK(RmTree(root.c_str()) == true);

		entries = ListDirRecursive(root.c_str());
		TEST_CHECK(entries.empty() == true);
	}
	{ 
		std::string root = PathJoin(base, "hg_core_ut__01"); 
		std::string src = PathJoin(root, "01");
		std::string dst = PathJoin(root, "02");
		TEST_CHECK(MkDir(PathJoin(src, "c").c_str()) == false);
		TEST_CHECK(MkTree(src.c_str()) == true);
		TEST_CHECK(StringToFile(PathJoin(src, "lorem.txt").c_str(), hg::test::LoremIpsum.c_str()) == true);

		TEST_CHECK(CopyDir(src.c_str(), dst.c_str()) == false);

		TEST_CHECK(MkDir(dst.c_str()) == true);
		TEST_CHECK(CopyDir(PathJoin(root, "03").c_str(), dst.c_str()) == false);
		TEST_CHECK(CopyDir(src.c_str(), dst.c_str()) == true);
				
		std::vector<DirEntry> entries = ListDir(dst.c_str(), DE_File);
		if (TEST_CHECK(entries.size() == 1)) {
			TEST_CHECK(entries[0].name == "lorem.txt");
			TEST_CHECK(entries[0].type == DE_File);
			TEST_CHECK(entries[0].size = hg::test::LoremIpsum.size());
		}

		TEST_CHECK(RmTree(root.c_str()) == true);
	}
#if !_WIN32
	{
		std::string root = PathJoin(base, "hg_core_ut__02");
		TEST_CHECK(MkTree(root.c_str(), 01755) == true);

		std::string path_00 = PathJoin(root, "00");
		TEST_CHECK(MkDir(path_00.c_str(), 00000) == true);
		TEST_CHECK(MkDir(PathJoin(path_00, "01").c_str(), 00755) == false);
		TEST_CHECK(MkTree(PathJoin(path_00, "02", "03").c_str(), 01755) == false);
		
		std::string path_01 = PathJoin(root, "01");
		TEST_CHECK(MkDir(path_01.c_str(), 00755) == true);

		std::string lorem_filepath = PathJoin(path_01, "lorem.txt");
		TEST_CHECK(StringToFile(lorem_filepath.c_str(), hg::test::LoremIpsum.c_str()) == true);

		std::string path_02 = PathJoin(root, "02");
		TEST_CHECK(MkDir(path_02.c_str(), 00755) == true);
		TEST_CHECK(MkDir(PathJoin(path_02, "03").c_str(), 00755) == true);

		std::string path_0103 = PathJoin(path_01, "03");
		TEST_CHECK(MkDir(path_0103.c_str(), 00000) == true);

		TEST_CHECK(CopyDir(path_01.c_str(), path_00.c_str()) == false);
		TEST_CHECK(CopyDirRecursive(path_01.c_str(), path_00.c_str()) == false);
		TEST_CHECK(CopyDirRecursive(path_01.c_str(), path_02.c_str()) == false);

		TEST_CHECK(RmTree(root.c_str()) == false);

		TEST_CHECK(chmod(path_00.c_str(), 0755) == 0);
		TEST_CHECK(chmod(path_0103.c_str(), 0755) == 0);
		TEST_CHECK(RmTree(root.c_str()) == true);
	}
#endif
}
