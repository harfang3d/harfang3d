// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/timer.h"

#include "engine/audio.h"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "shared.h"

using namespace hg;

TEST(Audio, InitShutdown) {
	start_timer();

	EXPECT_TRUE(AudioInit());
	AudioShutdown();

	stop_timer();
}

TEST(Audio, PlayWAV) {
	EXPECT_TRUE(AudioInit());

	const auto snd = LoadWAVSoundFile(GetResPath("audio/sine_48S16Stereo.wav").c_str());
	const auto src = PlayStereo(snd, {20.f, SR_Once, 0.5f});

	while (GetSourceState(src) == SS_Playing)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

	AudioShutdown();
}

TEST(Audio, StreamWAV) {
	start_timer();

	EXPECT_TRUE(AudioInit());

	const auto src = StreamWAVFileStereo(GetResPath("audio/sine_48S16Stereo.wav").c_str(), {20.f, SR_Once, 0.5f});
	while (GetSourceState(src) == SS_Playing)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

	AudioShutdown();

	stop_timer();
}
