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

static void test_SetGetPixels() {
	Picture rgb(2, 2, PF_RGB24);
	Picture rgba(2, 2, PF_RGBA32);
	Picture rgbaf(2, 2, PF_RGBA32F);

	Color pal[4] = {{1.f, 1.f, 1.f, 1.f}, {1.f, 0.f, 0.f, 0.f}, {0.f, 1.f, 0.f, 1.f}, {0.f, 0.f, 1.f, 0.f}};

	for (int j = 0; j < 2; j++) {
		for (int i = 0; i < 2; i++) {
			Color in = pal[i + j * 2];


			SetPixelRGBA(rgb, i, j, in);
			SetPixelRGBA(rgba, i, j, in);
			SetPixelRGBA(rgbaf, i, j, in);

			// test if it doesn't crash
			SetPixelRGBA(rgb, i + 4, j, in);
			SetPixelRGBA(rgba, i + 4, j, in);
			SetPixelRGBA(rgbaf, i + 4, j, in);

			SetPixelRGBA(rgb, i, j + 4, in);
			SetPixelRGBA(rgba, i, j + 4, in);
			SetPixelRGBA(rgbaf, i, j + 4, in);
		}
	}

	for (int j = 0; j < 2; j++) {
		for (int i = 0; i < 2; i++) {
			Color expected = pal[i + j * 2];

			TEST_CHECK(GetPixelRGBA(rgb, i, j) == Color(expected.r, expected.g, expected.b, 0.f));
			TEST_CHECK(GetPixelRGBA(rgba, i, j) == expected);
			TEST_CHECK(GetPixelRGBA(rgbaf, i, j) == expected);

			TEST_CHECK(GetPixelRGBA(rgb, i + 10, j) == Color::Zero);
			TEST_CHECK(GetPixelRGBA(rgba, i + 10, j) == Color::Zero);
			TEST_CHECK(GetPixelRGBA(rgbaf, i + 10, j) == Color::Zero);

			TEST_CHECK(GetPixelRGBA(rgb, i, j + 10) == Color::Zero);
			TEST_CHECK(GetPixelRGBA(rgba, i, j + 10) == Color::Zero);
			TEST_CHECK(GetPixelRGBA(rgbaf, i, j + 10) == Color::Zero);
		}
	}
}

void test_picture() {
	test_LoadSave();
	test_SetGetPixels();
}