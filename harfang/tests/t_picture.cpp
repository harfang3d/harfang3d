// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/picture.h"
#include "shared.h"
#include "gtest/gtest.h"

using namespace hg;

TEST(Picture, LoadSave) {
	Picture pic;
	EXPECT_TRUE(LoadPicture(pic, GetResPath("pic/owl.jpg").c_str()));
	EXPECT_TRUE(SavePNG(pic, GetOutPath("stb_save.png").c_str()));
	EXPECT_TRUE(SaveBMP(pic, GetOutPath("stb_save.bmp").c_str()));
	EXPECT_TRUE(SaveTGA(pic, GetOutPath("stb_save.tga").c_str()));
}

#if 0

TEST(Picture, Crop) {
	EXPECT_TRUE(g_fs.get().Mount(std::make_shared<StdFileDriver>()));

	Picture pic;
	EXPECT_TRUE(g_picture_io.get().Load(pic, GetResPath("pic/owl.jpg").c_str()));
	pic.Crop(0, 0, 10, 10);
	EXPECT_TRUE(g_picture_io.get().Save(pic, GetOutPath("crop_save.tga").c_str(), "STB", "format:tga"));

	g_fs.get().UnmountAll();
}

TEST(Picture, Flip) {
	EXPECT_TRUE(g_fs.get().Mount(std::make_shared<StdFileDriver>()));

	Picture pic;
	EXPECT_TRUE(g_picture_io.get().Load(pic, GetResPath("pic/owl.jpg").c_str()));
	pic.Flip(false, true);
	EXPECT_TRUE(g_picture_io.get().Save(pic, GetOutPath("flip_save.tga").c_str(), "STB", "format:tga"));

	g_fs.get().UnmountAll();
}

TEST(Picture, ComputeAvgHash) {
	EXPECT_TRUE(g_fs.get().Mount(std::make_shared<StdFileDriver>()));

	Picture pic_a, pic_b;
	EXPECT_TRUE(g_picture_io.get().Load(pic_a, GetResPath("pic/owl.jpg").c_str()));
	EXPECT_TRUE(g_picture_io.get().Load(pic_b, GetResPath("pic/oakenshield.jpg").c_str()));
	EXPECT_FALSE(ComputePictureAvgHash(pic_a).IsEquivalent(ComputePictureAvgHash(pic_b)));

	g_fs.get().UnmountAll();
}

TEST(Picture, BlitTransform) {
	EXPECT_TRUE(g_fs.get().Mount(std::make_shared<StdFileDriver>()));

	Picture pic_a, pic_b;

	// load owl and blit transform at 45�
	EXPECT_TRUE(g_picture_io.get().Load(pic_a, GetResPath("pic/owl.jpg").c_str()));
	EXPECT_TRUE(pic_b.AllocAs(pic_a));
	EXPECT_TRUE(pic_b.BlitTransform(pic_a, pic_b.GetRect(), RotationMat2D(Deg(45.f), pic_a.GetCenter())));

	// load ref
	EXPECT_TRUE(g_picture_io.get().Load(pic_a, GetResPath("ref/picture_blit_transform.tga").c_str()));
	EXPECT_TRUE(ComputePictureAvgHash(pic_a).IsEquivalent(ComputePictureAvgHash(pic_b)));

	g_fs.get().UnmountAll();
}

#endif
