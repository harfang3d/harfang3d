// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#if 0

#include "engine/factories.h"
#include "engine/mixer_async.h"
#include "foundation/filesystem.h"
#include "foundation/std_file_driver.h"
#include "foundation/log.h"
#include "foundation/math.h"
#include "foundation/time.h"
#include "shared.h"
#include "gtest/gtest.h"
#include <array>

using namespace hg;

#define SINGLE_THREADED_TEST 1
#define MULTI_THREADED_TEST 1

#if SINGLE_THREADED_TEST

TEST(Mixer, OpenCloseSingleThreaded) {
	auto mixer = g_mixer_factory.get().Instantiate();
	EXPECT_TRUE(mixer) << "No mixer available";

	if (mixer) {
		EXPECT_TRUE(mixer->Open());
		mixer->Close();
	}
}

TEST(Mixer, LoadAndPlaySoundSingleThreaded) {
	auto mixer = g_mixer_factory.get().Instantiate();
	EXPECT_TRUE(mixer) << "No mixer available";

	if (mixer) {
		EXPECT_TRUE(g_fs.get().Mount(std::make_shared<StdFileDriver>(unit_resource_path)));
		EXPECT_TRUE(mixer->Open());

		auto sound = mixer->LoadSound("res/t_mixer/okay.wav");
		EXPECT_TRUE(sound);

		if (sound) {
			auto channel = mixer->Start(*sound);
			EXPECT_NE(Mixer::ChannelError, channel);
			while (mixer->GetPlayState(channel) == MixerPlaying)
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			sound = nullptr;
		}

		mixer->Close();
		g_fs.get().UnmountAll();
	}
}

TEST(Mixer, LoadAndPlayStreamSingleThreaded) {
	auto mixer = g_mixer_factory.get().Instantiate();
	EXPECT_TRUE(mixer) << "No mixer available";

	if (mixer) {
		EXPECT_TRUE(g_fs.get().Mount(std::make_shared<StdFileDriver>(unit_resource_path)));
		EXPECT_TRUE(mixer->Open());

		EXPECT_NE(-1, mixer->Stream("res/t_mixer/stream.ogg"));
		for (auto t = time_now(); (time_now() - t) < time_from_sec(9);)
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		mixer->Close();

		g_fs.get().UnmountAll();
	}
}

#endif

//
#if MULTI_THREADED_TEST

TEST(Mixer, OpenCloseAsync) {
	auto _mixer = g_mixer_factory.get().Instantiate();
	EXPECT_TRUE(_mixer) << "No mixer available";

	if (_mixer) {
		g_task_system.get().create_workers();

		MixerAsync mixer(_mixer);
		EXPECT_TRUE(mixer.Open().get());
		mixer.Close();

		g_task_system.get().delete_workers();
	}
}

TEST(Mixer, LoadAndPlaySoundAsync) {
	auto _mixer = g_mixer_factory.get().Instantiate();
	EXPECT_TRUE(_mixer) << "No mixer available";

	if (_mixer) {
		g_task_system.get().create_workers();
		EXPECT_TRUE(g_fs.get().Mount(std::make_shared<StdFileDriver>(unit_resource_path)));

		MixerAsync mixer(_mixer);
		EXPECT_TRUE(mixer.Open().get());

		auto sound = mixer.LoadSound("res/t_mixer/okay.wav");

		auto channel = mixer.Start(sound).get();
		EXPECT_NE(Mixer::ChannelError, channel);
		while (mixer.GetPlayState(channel).get() == MixerPlaying)
			std::this_thread::sleep_for(std::chrono::milliseconds(1));

		mixer.Close();

		g_fs.get().UnmountAll();
		g_task_system.get().delete_workers();
	}
}

TEST(Mixer, LoadAndPlayStreamAsync) {
	auto _mixer = g_mixer_factory.get().Instantiate();
	EXPECT_TRUE(_mixer) << "No mixer available";

	if (_mixer) {
		g_task_system.get().create_workers();

		EXPECT_TRUE(g_fs.get().Mount(std::make_shared<StdFileDriver>(unit_resource_path)));

		MixerAsync mixer(_mixer);
		EXPECT_TRUE(mixer.Open().get());

		EXPECT_NE(Mixer::ChannelError, mixer.Stream("res/t_mixer/stream.ogg").get());
		for (auto t = time_now(); (time_now() - t) < time_from_sec(9);)
			std::this_thread::sleep_for(std::chrono::milliseconds(1));

		mixer.Close();

		g_fs.get().UnmountAll();

		g_task_system.get().delete_workers();
	}
}

#endif

//
TEST(Mixer, Stream3D) {
	auto mixer = g_mixer_factory.get().Instantiate();
	EXPECT_TRUE(mixer) << "No mixer available";

	if (mixer) {
		EXPECT_TRUE(g_fs.get().Mount(std::make_shared<StdFileDriver>(unit_resource_path)));
		EXPECT_TRUE(mixer->Open());

		auto channel = mixer->Stream("res/t_mixer/mono.ogg");

		EXPECT_NE(-1, channel);

		float a = 0;
		for (auto t = time_now(); (time_now() - t) < time_from_sec(6);) {
			MixerChannelLocation location;

			location.position = Vec3(Sin(a), 0, Cos(a)) * 6;
			a += 0.001f;

			mixer->SetChannelLocation(channel, location);

			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		mixer->Close();

		g_fs.get().UnmountAll();
	}
}

//
TEST(Mixer, SynchronizedStreams) {
	auto mixer = g_mixer_factory.get().Instantiate();
	EXPECT_TRUE(mixer) << "No mixer available";

	if (mixer) {
		EXPECT_TRUE(g_fs.get().Mount(std::make_shared<StdFileDriver>(unit_resource_path)));
		EXPECT_TRUE(mixer->Open());

		log("Start buffering streams...");

		std::array<MixerChannel, 16> channels;
		for (size_t i = 0; i < channels.size(); ++i) {
			channels[i] = mixer->Stream("res/t_mixer/mono.ogg", true);
			EXPECT_NE(-1, channels[i]);
		}

		std::this_thread::sleep_for(std::chrono::seconds(1)); // let all streams buffer

		log("Starting playback...");

		// now start all channels
		for (size_t i = 0; i < channels.size(); ++i)
			mixer->Resume(channels[i]);

		for (int i = 0; i < 6; ++i) {
			time_ns min_t, max_t;
			min_t = max_t = mixer->GetChannelTimestamp(channels[0]);

			for (size_t i = 1; i < channels.size(); ++i) {
				auto t = mixer->GetChannelTimestamp(channels[i]);
				min_t = Min(min_t, t);
				max_t = Max(max_t, t);
			}

			log(format("MIN_T: %1, MAX_T: %2 (dt ms: %3)").arg(min_t).arg(max_t).arg(time_to_ms_f(max_t) - time_to_ms_f(min_t)));

			std::this_thread::sleep_for(std::chrono::seconds(1));
		}

		mixer->Close();

		g_fs.get().UnmountAll();
	}
}

#endif
