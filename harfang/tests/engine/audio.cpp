// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/timer.h"

#include "engine/audio.h"

#include <chrono>
#include <thread>

using namespace hg;

struct Audio {
	bool initialized;

	Audio() {
		start_timer();
		initialized = AudioInit();
		TEST_CHECK(initialized == true);
	}

	~Audio() {
		initialized = false;
		AudioShutdown();
		stop_timer();
	}
};

static void test_InitShutdown() {
	Audio audio;
}

static void test_PlayWAV() {
	Audio audio;

	const auto snd = LoadWAVSoundFile("./data/audio/sine_48S16Stereo.wav");
	TEST_CHECK(snd != InvalidSourceRef);

	auto src = PlayStereo(snd, {20.f, SR_Once, 0.5f});
	TEST_CHECK(snd != InvalidSoundRef);
	TEST_CHECK(GetSourceState(src) == SS_Playing);

	while (GetSourceState(src) == SS_Playing)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	TEST_CHECK(GetSourceState(src) == SS_Stopped);

	// replay it
	src = PlayStereo(snd, {20.f, SR_Once, 0.5f});
	TEST_CHECK(snd != InvalidSoundRef);
	TEST_CHECK(GetSourceState(src) == SS_Playing);

	while (GetSourceState(src) == SS_Playing)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	TEST_CHECK(GetSourceState(src) == SS_Stopped);

	StopSource(src);
	UnloadSound(snd);
}


static void test_StreamWAV() {
	Audio audio;

	const auto src = StreamWAVFileStereo("./data/audio/sine_48S16Stereo.wav", {20.f, SR_Once, 0.5f});
	TEST_CHECK(src != InvalidSourceRef);
	while (GetSourceState(src) == SS_Initial)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

	TEST_CHECK(GetSourceState(src) == SS_Playing);
	while (GetSourceState(src) == SS_Playing)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

	TEST_CHECK(GetSourceState(src) == SS_Stopped);
}

static void test_Timestamps() {
	Audio audio;

	const auto src = StreamWAVFileStereo("./data/audio/sine_48S16Stereo.wav", {20.f, SR_Once, 0.5f});
	TEST_CHECK(src != InvalidSourceRef);

	time_ns duration = GetSourceDuration(src);
	TEST_CHECK(duration == hg::time_from_sec(2)); // WARNING! Don't forget to change this test if you modify the input wav file.

	hg::SourceState state;

	while ((state = GetSourceState(src)) == SS_Initial) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	TEST_CHECK(state == SS_Playing);

	hg::time_ns constexpr t_break = hg::time_from_ms(1200);
	hg::time_ns constexpr t_rewind = hg::time_from_ms(200);
	hg::time_ns t_elapsed = 0;

	while ((state == SS_Playing) && (t_elapsed < t_break)) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		state = GetSourceState(src);
		t_elapsed = GetSourceTimecode(src);
	}

	// Go back to t = 200ms.
	TEST_CHECK(SetSourceTimecode(src, t_rewind));

	// Remember! This is asynchronous. It means that if we call GetSourceTimecode just after, it may not return 200ms.
	// This loop should run until the timestamp is set or the call was ignored and the stream ended. The latter being an error.
	int t_wait_ms = 50;
	while ((state == SS_Playing) && (t_elapsed > t_break)) {
		std::this_thread::sleep_for(std::chrono::milliseconds(t_wait_ms));
		state = GetSourceState(src);
		t_elapsed = GetSourceTimecode(src);
	}

	TEST_CHECK(state == SS_Playing);
	TEST_CHECK(t_elapsed < t_break);
	// We must be closer to t_rewind than t_break.
	TEST_CHECK(Abs(t_elapsed - t_rewind) < Abs(t_elapsed - t_break));

	// Play the remaining of the audio stream.
	while (GetSourceState(src) == SS_Playing) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	TEST_CHECK(GetSourceState(src) == SS_Stopped);
}

static void test_StreamOGG() {
	Audio audio;

	const auto src = StreamOGGFileStereo("./data/audio/Dance_of_the_Sugar_Plum_Fairies_(ISRC_USUAN1100270).ogg", {20.f, SR_Once, 0.5f});
	TEST_CHECK(src != InvalidSourceRef);
	while (GetSourceState(src) == SS_Initial)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

	TEST_CHECK(GetSourceState(src) == SS_Playing);
	while (GetSourceState(src) == SS_Playing)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

	TEST_CHECK(GetSourceState(src) == SS_Stopped);
}

static void test_PlayOGG() {
	Audio audio;
	
	const auto snd = LoadOGGSoundFile("./data/audio/Dance_of_the_Sugar_Plum_Fairies_(ISRC_USUAN1100270).ogg");
	TEST_CHECK(snd != InvalidSourceRef);

	auto src = PlayStereo(snd, {20.f, SR_Once, 0.5f});
	TEST_CHECK(snd != InvalidSoundRef);
	TEST_CHECK(GetSourceState(src) == SS_Playing);

	while (GetSourceState(src) == SS_Playing)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	TEST_CHECK(GetSourceState(src) == SS_Stopped);

	// replay it
	src = PlayStereo(snd, {20.f, SR_Once, 0.5f});
	TEST_CHECK(snd != InvalidSoundRef);
	TEST_CHECK(GetSourceState(src) == SS_Playing);

	while (GetSourceState(src) == SS_Playing)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	TEST_CHECK(GetSourceState(src) == SS_Stopped);

	StopAllSources();
	UnloadSound(snd);
}

void test_audio() { 
	test_InitShutdown();
	test_PlayWAV();
	test_StreamWAV();
	test_Timestamps();
	test_StreamOGG();
	test_PlayOGG();
}
