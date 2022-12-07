// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "engine/picture.h"

#include "foundation/path_tools.h"
#include "foundation/file.h"
#include "../utils.h"

using namespace hg;

static void test_LoadSave() {
	const std::string tmp = test::GetTempDirectoryName();
	
	const std::string filename[3] = {
		PathJoin(tmp, "stb_save.png"),
		PathJoin(tmp, "stb_save.bmp"),
		PathJoin(tmp, "stb_save.tga"),
	};

	Picture pic;
	TEST_CHECK(LoadPicture(pic, "./data/pic/owl.jpg") == true);
	TEST_CHECK(SavePNG(pic, filename[0].c_str()) == true);
	TEST_CHECK(SaveBMP(pic, filename[1].c_str()) == true);
	TEST_CHECK(SaveTGA(pic, filename[2].c_str()) == true);

	for (int i = 0; i < 3; i++) {
		Unlink(filename[i].c_str());
	}
}

void test_picture() {
	test_LoadSave();
}