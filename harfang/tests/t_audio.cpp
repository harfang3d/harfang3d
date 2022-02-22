// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/timer.h"

#include "engine/audio.h"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "shared.h"

using namespace hg;

class Audio : public ::testing::Test {
protected:
	bool initialized;

	void SetUp() override {
		start_timer();
		initialized = AudioInit();
	}

	void TearDown() override {
		AudioShutdown();
		stop_timer();
	}
};

TEST_F(Audio, InitShutdown) { EXPECT_TRUE(initialized); }

TEST_F(Audio, PlayWAV) {
	EXPECT_TRUE(initialized);

	const auto snd = LoadWAVSoundFile(GetResPath("audio/sine_48S16Stereo.wav").c_str());
	EXPECT_NE(snd, InvalidSourceRef);

	auto src = PlayStereo(snd, {20.f, SR_Once, 0.5f});
	EXPECT_NE(snd, InvalidSoundRef);
	EXPECT_EQ(GetSourceState(src), SS_Playing);

	while (GetSourceState(src) == SS_Playing)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	EXPECT_EQ(GetSourceState(src), SS_Stopped);

	// replay it
	src = PlayStereo(snd, {20.f, SR_Once, 0.5f});
	EXPECT_NE(snd, InvalidSoundRef);
	EXPECT_EQ(GetSourceState(src), SS_Playing);

	while (GetSourceState(src) == SS_Playing)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	EXPECT_EQ(GetSourceState(src), SS_Stopped);
}

TEST_F(Audio, StreamWAV) {
	EXPECT_TRUE(initialized);

	const auto src = StreamWAVFileStereo(GetResPath("audio/sine_48S16Stereo.wav").c_str(), {20.f, SR_Once, 0.5f});
	EXPECT_NE(src, InvalidSourceRef);
	while (GetSourceState(src) == SS_Initial)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

	EXPECT_EQ(GetSourceState(src), SS_Playing);
	while (GetSourceState(src) == SS_Playing)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

	EXPECT_EQ(GetSourceState(src), SS_Stopped);
}

TEST_F(Audio, Timestamps) {
	EXPECT_TRUE(initialized);
	
	const auto src = StreamWAVFileStereo(GetResPath("audio/sine_48S16Stereo.wav").c_str(), {20.f, SR_Once, 0.5f});
	EXPECT_NE(src, InvalidSourceRef);

	time_ns duration = GetSourceDuration(src);
	EXPECT_EQ(duration, hg::time_from_sec(2)); // WARNING! Don't forget to change this test if you modify the input wav file.

	hg::SourceState state;

	while ((state = GetSourceState(src)) == SS_Initial) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	EXPECT_EQ(state, SS_Playing);

	hg::time_ns constexpr t_break = hg::time_from_ms(1200);
	hg::time_ns constexpr t_rewind = hg::time_from_ms(200);
	hg::time_ns t_elapsed = 0;

	while ((state == SS_Playing) && (t_elapsed < t_break)) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		state = GetSourceState(src);
		t_elapsed = GetSourceTimecode(src);
	}

	// Go back to t = 200ms.
	EXPECT_TRUE(SetSourceTimecode(src, t_rewind));

	// Remember! This is asynchronous. It means that if we call GetSourceTimecode just after, it may not return 200ms.
	// This loop should run until the timestamp is set or the call was ignored and the stream ended. The latter being an error.
	int t_wait_ms = 50;
	while ((state == SS_Playing) && (t_elapsed > t_break)) {
		std::this_thread::sleep_for(std::chrono::milliseconds(t_wait_ms));
		state = GetSourceState(src);
		t_elapsed = GetSourceTimecode(src);
	}

	EXPECT_EQ(state, SS_Playing);
	EXPECT_LT(t_elapsed, t_break);
	// We must be closer to t_rewind than t_break.
	EXPECT_LT(abs(t_elapsed - t_rewind), abs(t_elapsed - t_break));

	// Play the remaining of the audio stream.
	while (GetSourceState(src) == SS_Playing) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	EXPECT_EQ(GetSourceState(src), SS_Stopped);
}

TEST_F(Audio, StreamOGG) {
	EXPECT_TRUE(initialized);

	const auto src = StreamOGGFileStereo(GetResPath("audio/Dance_of_the_Sugar_Plum_Fairies_(ISRC_USUAN1100270).ogg").c_str(), {20.f, SR_Once, 0.5f});
	EXPECT_NE(src, InvalidSourceRef);
	while (GetSourceState(src) == SS_Initial)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

	EXPECT_EQ(GetSourceState(src), SS_Playing);
	while (GetSourceState(src) == SS_Playing)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

	EXPECT_EQ(GetSourceState(src), SS_Stopped);
}

TEST_F(Audio, PlayOGG) {
	EXPECT_TRUE(initialized);

	const auto snd = LoadOGGSoundFile(GetResPath("audio/Dance_of_the_Sugar_Plum_Fairies_(ISRC_USUAN1100270).ogg").c_str());
	EXPECT_NE(snd, InvalidSourceRef);

	auto src = PlayStereo(snd, {20.f, SR_Once, 0.5f});
	EXPECT_NE(snd, InvalidSoundRef);
	EXPECT_EQ(GetSourceState(src), SS_Playing);

	while (GetSourceState(src) == SS_Playing)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	EXPECT_EQ(GetSourceState(src), SS_Stopped);

	// replay it
	src = PlayStereo(snd, {20.f, SR_Once, 0.5f});
	EXPECT_NE(snd, InvalidSoundRef);
	EXPECT_EQ(GetSourceState(src), SS_Playing);

	while (GetSourceState(src) == SS_Playing)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	EXPECT_EQ(GetSourceState(src), SS_Stopped);
}
