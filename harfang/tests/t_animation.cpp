// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/animation.h"
#include "foundation/cext.h"
#include "foundation/time.h"
#include "gtest/gtest.h"

TEST(Animation, TestAnimTrack) {
	hg::AnimTrackT<float> track;

	track.keys = {
		{hg::time_from_sec(0), 0.f},
		{hg::time_from_sec(10), 5.f},
	};

	float v;
	EXPECT_TRUE(hg::Evaluate(track, hg::time_from_sec(5), v));
	EXPECT_EQ(v, 2.5f);
}

#if 0

TEST(Animation, TestAnimKey) {
	hg::AnimTrackT<float> track;
	EXPECT_EQ(0, track.GetKeys().size());

	track.SetKey(10, 1.f);
	track.SetKey(20, 2.f);
	EXPECT_EQ(2, track.GetKeys().size());

	track.SetKey(30, 4.f);
	track.SetKey(10, 3.f); // reuse existing key
	EXPECT_EQ(3, track.GetKeys().size());
	EXPECT_EQ(3.f, track.GetKeys()[0].v);

	float v;
	EXPECT_TRUE(track.GetValue(15, v));
	EXPECT_EQ(2.5f, v);
}

TEST(Animation, TestAnimNodeReflectedProperties) {
	auto node = std::make_shared<hg::Node>();
	node->AddComponent(std::make_shared<hg::Transform>());
	node->AddComponent(std::make_shared<hg::Light>());

	//
	hg::Anim anim;

	auto pos_track = std::make_shared<hg::AnimTrackT<hg::Vec3>>("Transform.Position");
	pos_track->SetKey(hg::time_from_sec(0), {0, 0, 0});
	pos_track->SetKey(hg::time_from_sec(4), {4, 2, 0});
	anim.tracks.push_back(pos_track);

	auto scl_track = std::make_shared<hg::AnimTrackT<hg::Vec3>>("Transform.Scale");
	scl_track->SetKey(hg::time_from_sec(0), {1, 1, 1});
	scl_track->SetKey(hg::time_from_sec(4), {5, 5, 5});
	anim.tracks.push_back(scl_track);

	auto lgt_track = std::make_shared<hg::AnimTrackT<float>>("Light.DiffuseIntensity");
	lgt_track->SetKey(hg::time_from_sec(0), 0);
	lgt_track->SetKey(hg::time_from_sec(4), 6);
	anim.tracks.push_back(lgt_track);

	//
	auto bound = hg::BindNodeAnim(*node, anim);
	bound->Apply(hg::time_from_sec(2));

	//
	auto pos = node->GetComponent<hg::Transform>()->GetPos();
	EXPECT_EQ(pos, hg::Vec3(2, 1, 0));

	auto scl = node->GetComponent<hg::Transform>()->GetScale();
	EXPECT_EQ(scl, hg::Vec3(3, 3, 3));

	auto lgt = node->GetComponent<hg::Light>()->GetDiffuseIntensity();
	EXPECT_EQ(lgt, 3);
}

#endif

#if 0

#include "engine/engine.h"
#include "engine/plus.h"

TEST(Animation, TestSceneAnim) {
#if 0
	for (hg::time_ns t = hg::time_from_sec(0); t < hg::time_from_sec(50); t += hg::time_from_ms(100)) {
		auto repeat = hg::TimeRepeat(t, hg::time_from_sec(2), hg::time_from_ms(2500));
		auto ping_pong = hg::TimePingPong(t, hg::time_from_sec(2), hg::time_from_ms(2500));
		hg::error(hg::format("t: %1, repeat: %2, ping-pong: %3").arg(hg::time_to_ms(t)).arg(hg::time_to_ms(repeat)).arg(hg::time_to_ms(ping_pong)));
	}
#else
	auto &plus = hg::g_plus.get();

	plus.Mount("d:/gauge_anim_test");
	plus.RenderInit(1280, 720, 8);

	auto scene = plus.NewScene(false, false);
	plus.LoadScene(*scene, "gauge_anim_fbx2016.scn");
	plus.CreateCamera(*scene, hg::Mat4::TranslationMatrix({0, 0, -20}));
	plus.AddLight(*scene, hg::Mat4::TranslationMatrix({0, 5, -5}));

	plus.UpdateScene(*scene);

	hg::ResetLastFrameDuration();

	auto main_take = scene->anim_takes[0];
	//	auto bound_anims = BindNodesAnims(scene->GetNodes(), main_take->anims);
	auto anim = scene->StartAnimTakeRange(main_take, 0, hg::time_from_ms(1500), hg::AnimPingPong);
	//	scene->SetAnimTScale(anim, -1.f);

	hg::time_ns t = 0;
	while (!plus.IsAppEnded()) {
		//		t += hg::GetLastFrameDuration() / 10;
		//		for (auto &bound_anim : bound_anims)
		//			bound_anim->Apply(t);

		plus.UpdateScene(*scene);
		plus.Flip();
		plus.EndFrame();
	}

	plus.RenderUninit();
#endif
}

#endif // 0
