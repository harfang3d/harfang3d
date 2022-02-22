// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/video_stream.h"
#include "gtest/gtest.h"

#include "shared.h"

#include "foundation/format.h"
#include <chrono>
#include <thread>

using namespace hg;

#if WIN32
	static const char *module_name = "DummyVideoStream.dll";
#else
	static const char *module_name = "DummyVideoStream.so";
#endif

TEST(VideoStream, LoadModuleAndMakeVideoStreamer) {
	IVideoStreamer streamer;
	SharedLib module = LoadSharedLibrary(GetResPath(module_name).c_str());
	
	streamer = MakeVideoStreamer(module);
	EXPECT_TRUE(IsValid(streamer));

	EXPECT_EQ(streamer.Startup(), 1);
	streamer.Shutdown();
}

TEST(VideoStream, MakeVideoStreamer) {
	IVideoStreamer streamer = MakeVideoStreamer(GetResPath(module_name).c_str());
	EXPECT_TRUE(IsValid(streamer));

	EXPECT_EQ(streamer.Startup(), 1);

	VideoStreamHandle handle = streamer.Open(GetResPath("Harfang 2021-12-14 15-13-52.mp4").c_str());
	EXPECT_NE(handle, 0);

	EXPECT_EQ(streamer.Play(handle), 1);
	
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
	
	EXPECT_EQ(streamer.Pause(handle), 1);
	
	EXPECT_EQ(streamer.Close(handle), 1);

	streamer.Shutdown();
}
