// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "engine/video_stream.h"

#include "foundation/format.h"
#include "foundation/path_tools.h"

#include "../utils.h"

#include <chrono>
#include <thread>

using namespace hg;

#if WIN32
	static const char *module_name = "./data/DummyVideoStream.dll";
#elif __APPLE__
	static const char *module_name = "./data/DummyVideoStream.dylib";
#else
	static const char *module_name = "./data/DummyVideoStream.so";
#endif

static void test_LoadModuleAndMakeVideoStreamer() {
	IVideoStreamer streamer;
	SharedLib module = LoadSharedLibrary(module_name);
	
	streamer = MakeVideoStreamer(module);
	TEST_CHECK(IsValid(streamer) == true);

	TEST_CHECK(streamer.Startup() == 1);
	streamer.Shutdown();
}

static void test_MakeVideoStreamer() {
	IVideoStreamer streamer = MakeVideoStreamer(module_name);
	TEST_CHECK(IsValid(streamer) == true);

	TEST_CHECK(streamer.Startup() == 1);

	VideoStreamHandle handle = streamer.Open(PathJoin(test::GetTempDirectoryName(), "Harfang 2021-12-14 15-13-52.mp4").c_str());
	TEST_CHECK(handle != 0);

	TEST_CHECK(streamer.Play(handle) == 1);
	
	uint8_t *data;
	int width;
	int height;
	int pitch;
	VideoFrameFormat fmt;

	for (int i=0; i<4; i++) {
		std::this_thread::sleep_for(std::chrono::milliseconds(200));

		int id = streamer.GetFrame(handle, (const void **)&data, &width, &height, &pitch, &fmt);
		Picture pic = MakePicture(data, width, height, PF_RGB24);
		SavePNG(pic, format("%1.png").arg(i).c_str());
		streamer.FreeFrame(handle, id);
	}
	
	TEST_CHECK(streamer.Pause(handle) == 1);
	
	TEST_CHECK(streamer.Close(handle) == 1);

	streamer.Shutdown();
}

void test_video_stream() {
	test_LoadModuleAndMakeVideoStreamer();
	test_MakeVideoStreamer();
}
