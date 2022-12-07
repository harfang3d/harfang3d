// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include <string>

#include "engine/animation.h"

#include "foundation/math.h"
#include "foundation/unit.h"
#include "foundation/time.h"

using namespace hg;

static void test_anim_bool_track() {
	AnimTrackT<bool> bool_track;

	hg::time_ns t[4] = {time_from_ms(100), time_from_ms(400), time_from_ms(800), time_from_ms(1200)};

	TEST_CHECK(GetKey(bool_track, t[0]) == InvalidKeyIdx);

	SetKey(bool_track, t[3], false);
	SetKey(bool_track, t[0], false);
	SetKey(bool_track, t[2], true);
	SetKey(bool_track, t[1], true);

	SetKey(bool_track, t[2], false);
	SetKey(bool_track, t[3], true);

	int idx;
	idx = GetKey(bool_track, t[0]);
	TEST_CHECK(idx != InvalidKeyIdx);
	TEST_CHECK(bool_track.keys[idx].t == t[0]);
	TEST_CHECK(bool_track.keys[idx].v == false);

	idx = GetKey(bool_track, t[3]);
	TEST_CHECK(idx != InvalidKeyIdx);
	TEST_CHECK(bool_track.keys[idx].t == t[3]);
	TEST_CHECK(bool_track.keys[idx].v == true);

	hg::time_ns not_here = time_from_ms(1000);
	TEST_CHECK(GetKey(bool_track, not_here) == InvalidKeyIdx);
	DeleteKey(bool_track, not_here);
	TEST_CHECK(GetKey(bool_track, not_here) == InvalidKeyIdx);

	DeleteKey(bool_track, t[3]);
	TEST_CHECK(GetKey(bool_track, t[3]) == InvalidKeyIdx);

	int k[2];
	bool ret;
	ret = GetIntervalKeys<AnimTrackT<bool>, bool>(bool_track, time_ns(t[1] + time_from_ms(200)), k[0], k[1]);
	TEST_CHECK(ret == true);
	TEST_CHECK(k[0] == 1);
	TEST_CHECK(k[1] == 2);

	ret = GetIntervalKeys<AnimTrackT<bool>, bool>(bool_track, time_from_ms(2000), k[0], k[1]);
	TEST_CHECK(ret == false);
	TEST_CHECK(k[0] == 2);

	ret = GetIntervalKeys<AnimTrackT<bool>, bool>(bool_track, time_from_ms(20), k[0], k[1]);
	TEST_CHECK(ret == false);
	TEST_CHECK(k[0] == 0);

	bool value;

	TEST_CHECK(Evaluate(AnimTrackT<bool>(), 0, value) == false);
	
	TEST_CHECK(Evaluate(bool_track, time_from_ms(0), value) == true);
	TEST_CHECK(value == false);

	TEST_CHECK(Evaluate(bool_track, time_from_ms(50), value) == true);
	TEST_CHECK(value == false);

	TEST_CHECK(Evaluate(bool_track, time_from_ms(100), value) == true);
	TEST_CHECK(value == false);

	TEST_CHECK(Evaluate(bool_track, time_from_ms(300), value) == true);
	TEST_CHECK(value == false);

	TEST_CHECK(Evaluate(bool_track, time_from_ms(400), value) == true);
	TEST_CHECK(value == true);

	TEST_CHECK(Evaluate(bool_track, time_from_ms(600), value) == true);
	TEST_CHECK(value == true);

	TEST_CHECK(Evaluate(bool_track, time_from_ms(900), value) == true);
	TEST_CHECK(value == false);

	TEST_CHECK(Evaluate(bool_track, time_from_ms(1500), value) == true);
	TEST_CHECK(value == false);
}

static void test_anim_string_track() {
	AnimTrackT<std::string> str_track;

	hg::time_ns t[4] = {time_from_ms(100), time_from_ms(400), time_from_ms(800), time_from_ms(1200)};

	str_track.keys.resize(4);
	str_track.keys[0].t = t[3];
	str_track.keys[0].v = "Key #3";

	str_track.keys[1].t = t[0];
	str_track.keys[1].v = "Key #0";

	str_track.keys[2].t = t[2];
	str_track.keys[2].v = "Key #2";

	str_track.keys[3].t = t[1];
	str_track.keys[3].v = "Key #1";

	SortAnimTrackKeys(str_track);

	for (size_t i = 1; i < str_track.keys.size(); i++) {
		TEST_CHECK(str_track.keys[i - 1].t < str_track.keys[i].t);
	}

	std::string value;
	TEST_CHECK(Evaluate(AnimTrackT<std::string>(), time_from_ms(900), value) == false);

	TEST_CHECK(Evaluate(str_track, time_from_ms(0), value) == true);
	TEST_CHECK(value == "Key #0");

	TEST_CHECK(Evaluate(str_track, time_from_ms(50), value) == true);
	TEST_CHECK(value == "Key #0");

	TEST_CHECK(Evaluate(str_track, time_from_ms(180), value) == true);
	TEST_CHECK(value == "Key #0");

	TEST_CHECK(Evaluate(str_track, time_from_ms(400), value) == true);
	TEST_CHECK(value == "Key #1");

	TEST_CHECK(Evaluate(str_track, time_from_ms(720), value) == true);
	TEST_CHECK(value == "Key #1");

	TEST_CHECK(Evaluate(str_track, time_from_ms(1040), value) == true);
	TEST_CHECK(value == "Key #2");

	TEST_CHECK(Evaluate(str_track, time_from_ms(1600), value) == true);
	TEST_CHECK(value == "Key #3");
}

static void test_anim_int_track() { 
	AnimTrackT<int> int_track; 

	int value;
	TEST_CHECK(Evaluate(int_track, time_from_ms(500), value) == false);

	hg::time_ns t[4] = {time_from_ms(100), time_from_ms(200), time_from_ms(300), time_from_ms(400)};

	int i0 = -10;
	int i1 = -5;
	int i2 = 5;
	int i3 = 10;
		
	int_track.keys.resize(4);

	int_track.keys[0].t = t[3];
	int_track.keys[0].v = i3;

	int_track.keys[1].t = t[2];
	int_track.keys[1].v = i2;

	int_track.keys[2].t = t[1];
	int_track.keys[2].v = i1;

	int_track.keys[3].t = t[0];
	int_track.keys[3].v = i0;

	SortAnimTrackKeys(int_track);

	TEST_CHECK(Evaluate(int_track, time_from_ms(20), value) == true);
	TEST_CHECK(value == i0);

	TEST_CHECK(Evaluate(int_track, time_from_ms(100), value) == true);
	TEST_CHECK(value == i0);

	TEST_CHECK(Evaluate(int_track, time_from_ms(150), value) == true);
	TEST_CHECK(value == (i0+i1)/2);

	TEST_CHECK(Evaluate(int_track, time_from_ms(200), value) == true);
	TEST_CHECK(value == i1);

	TEST_CHECK(Evaluate(int_track, time_from_ms(400), value) == true);
	TEST_CHECK(value == i3);

	TEST_CHECK(Evaluate(int_track, time_from_ms(1000), value) == true);
	TEST_CHECK(value == i3);
}

static void test_anim_vec3_track() {
	AnimTrackHermiteT<Vec3> vec3_track;

	Vec3 value;
	TEST_CHECK(Evaluate(vec3_track, time_from_ms(600), value) == false);

	hg::time_ns t[4] = {time_from_ms(200), time_from_ms(400), time_from_ms(600), time_from_ms(800)};

	Vec3 v0(-1.f, -1.f, -1.f);
	Vec3 v1(-1.f, 0.f, -1.f);
	Vec3 v2(0.f, 1.f, 0.f);
	Vec3 v3(0.f, 1.f, 0.f);

	vec3_track.keys.resize(4);

	vec3_track.keys[0].t = t[3];
	vec3_track.keys[0].v = v3;
	vec3_track.keys[0].bias = 0.f;
	vec3_track.keys[0].tension = 0.f;

	vec3_track.keys[1].t = t[2];
	vec3_track.keys[1].v = v2;
	vec3_track.keys[1].bias = 0.75f;
	vec3_track.keys[1].tension = 0.25f;

	vec3_track.keys[2].t = t[1];
	vec3_track.keys[2].v = v1;
	vec3_track.keys[2].bias = 0.25f;
	vec3_track.keys[2].tension = 0.75f;

	vec3_track.keys[3].t = t[0];
	vec3_track.keys[3].v = v0;
	vec3_track.keys[3].bias = 0.5f;
	vec3_track.keys[3].tension = 0.5f;

	SortAnimTrackKeys(vec3_track);

	TEST_CHECK(Evaluate(vec3_track, time_from_ms(100), value) == true);
	TEST_CHECK(value == v0);

	TEST_CHECK(Evaluate(vec3_track, time_from_ms(200), value) == true);
	TEST_CHECK(value == v0);

	TEST_CHECK(Evaluate(vec3_track, time_from_ms(400), value) == true);
	TEST_CHECK(value == v1);

	TEST_CHECK(Evaluate(vec3_track, time_from_ms(600), value) == true);
	TEST_CHECK(value == v2);

	TEST_CHECK(Evaluate(vec3_track, time_from_ms(800), value) == true);
	TEST_CHECK(value == v3);

	TEST_CHECK(Evaluate(vec3_track, time_from_ms(1000), value) == true);
	TEST_CHECK(value == v3);

	ResampleAnimTrack(vec3_track, time_from_ms(100), time_from_ms(50), 1000000, time_from_ms(16));

	TEST_CHECK(vec3_track.keys[0].t == time_from_ms(144));
	TEST_CHECK(vec3_track.keys[1].t == time_from_ms(352));
	TEST_CHECK(vec3_track.keys[2].t == time_from_ms(544));
	TEST_CHECK(vec3_track.keys[3].t == time_from_ms(752));

	ResampleAnimTrack(vec3_track, time_from_ms(120), time_from_ms(100), 1000000, time_from_ms(220));

	TEST_CHECK(vec3_track.keys.size() == 3);

	TEST_CHECK(vec3_track.keys[0].t == time_from_ms(220));
	TEST_CHECK(vec3_track.keys[1].t == time_from_ms(440));
	TEST_CHECK(vec3_track.keys[2].t == time_from_ms(660));

	vec3_track.keys.clear();

	SetKey(vec3_track, time_from_ms(0), Vec3(0.f, 1.f, 0.f));
	SetKey(vec3_track, time_from_ms(50), Vec3(0.2f, 0.8f,-0.2f));
	SetKey(vec3_track, time_from_ms(100), Vec3(0.4f, 0.6f,-0.4f));
	SetKey(vec3_track, time_from_ms(150), Vec3(0.6f, 0.4f,-0.6f));
	SetKey(vec3_track, time_from_ms(200), Vec3(0.8f, 0.2f,-0.8f));
	SetKey(vec3_track, time_from_ms(250), Vec3(1.0f, 0.0f,-1.0f));

	size_t removed = SimplifyAnimTrackT<AnimTrackHermiteT<Vec3> >(vec3_track, 0.1f);
	TEST_CHECK(removed == 4);

	TEST_CHECK(vec3_track.keys[0].t == time_from_ms(0));
	TEST_CHECK(vec3_track.keys[1].t == time_from_ms(250));
}

static void test_anim_float_track() {
	AnimTrackT<float> track;

	SetKey(track, time_from_ms(100), 0.f);
	SetKey(track, time_from_ms(200), 1.f);
	SetKey(track, time_from_ms(300), 2.f);
	SetKey(track, time_from_ms(400), 3.f);
	SetKey(track, time_from_ms(500), 4.f);
	SetKey(track, time_from_ms(600), 3.f);
	SetKey(track, time_from_ms(700), 2.f);
	SetKey(track, time_from_ms(800), 1.f);
	SetKey(track, time_from_ms(900), 0.f);
	
	size_t removed = SimplifyAnimTrackT<AnimTrackT<float> >(track, 0.00001f);
	TEST_CHECK(removed == 6);

	TEST_CHECK((track.keys[0].t == time_from_ms(100)) && (track.keys[0].v == 0.f));
	TEST_CHECK((track.keys[1].t == time_from_ms(500)) && (track.keys[1].v == 4.f));
	TEST_CHECK((track.keys[2].t == time_from_ms(900)) && (track.keys[2].v == 0.f));

	SetKey(track, time_from_ms(500), 0.f);

	removed = SimplifyAnimTrackT<AnimTrackT<float> >(track, 0.00001f);
	TEST_CHECK(removed == 2);
	TEST_CHECK((track.keys[0].t == time_from_ms(100)) && (track.keys[0].v == 0.f));
}

static InstanceAnimKey SetKey(AnimTrackT<InstanceAnimKey> &track, int64_t ms, const std::string &name, AnimLoopMode mode, float scale) {
	InstanceAnimKey key;
	key.anim_name = name;
	key.loop_mode = mode;
	key.t_scale = scale;
	SetKey(track, time_from_ms(ms), key);

	return key;
}

static bool operator==(InstanceAnimKey &k0, InstanceAnimKey &k1) {
	return (k0.anim_name == k1.anim_name) && (k0.loop_mode == k1.loop_mode) && (k0.t_scale == k1.t_scale);
}

static void test_anim_instance_track() {
	AnimTrackT<InstanceAnimKey> track;

	InstanceAnimKey k0 = SetKey(track, 100, "000", ALM_Once, 1.f);
	InstanceAnimKey k1 = SetKey(track, 200, "001", ALM_Infinite, 0.5f);
	InstanceAnimKey k2 = SetKey(track, 300, "002", ALM_Loop, 2.f);

	InstanceAnimKey value;
	TEST_CHECK(Evaluate(AnimTrackT<InstanceAnimKey>(), 0, value) == false);
		
	TEST_CHECK(Evaluate(track, time_from_ms(0), value) == true);
	TEST_CHECK(value == k0);

	TEST_CHECK(Evaluate(track, time_from_ms(100), value) == true);
	TEST_CHECK(value == k0);

	TEST_CHECK(Evaluate(track, time_from_ms(150), value) == true);
	TEST_CHECK(value == k0);

	TEST_CHECK(Evaluate(track, time_from_ms(260), value) == true);
	TEST_CHECK(value == k1);

	TEST_CHECK(Evaluate(track, time_from_ms(300), value) == true);
	TEST_CHECK(value == k2);

	TEST_CHECK(Evaluate(track, time_from_ms(400), value) == true);
	TEST_CHECK(value == k2);
}

static void test_misc() {
	Vec3 axis = Normalize(Vec3::One);
	Quaternion q0 = QuaternionFromAxisAngle(Deg(300.f), axis);
	Quaternion q1 = QuaternionFromAxisAngle(Deg(-60.f), axis);
	Quaternion q2 = QuaternionFromAxisAngle(Deg(30.f), axis);
	Quaternion q3 = QuaternionFromAxisAngle(Deg(170.f), Vec3::Back);

#if _WIN32 and NDEBUG
	const float epsilon = 0.002f;
#else
	const float epsilon = 0.00001f;
#endif
	TEST_CHECK(CompareKeyValue(q0, q1, epsilon) == true);
	TEST_CHECK(CompareKeyValue(q1, q2, 0.00001f) == false);
	TEST_CHECK(CompareKeyValue(q2, q3, 0.00001f) == false);
	
	TEST_CHECK(CompareKeyValue(Color::Orange, Color::Yellow, 0.00001f) == false);
	TEST_CHECK(CompareKeyValue(Color(0.7f, 0.2f, 0.1f), Color(0.69f, 0.18f, 0.11f), 0.02f) == true);	
}

static void test_anim_reverse() {
	Anim anim;
	anim.t_start = time_from_ms(200);
	anim.t_end = time_from_ms(700);

	anim.bool_tracks.resize(1);
	SetKey(anim.bool_tracks[0], time_from_ms(220), true);
	SetKey(anim.bool_tracks[0], time_from_ms(240), false);
	SetKey(anim.bool_tracks[0], time_from_ms(250), false);
	SetKey(anim.bool_tracks[0], time_from_ms(260), true);
	SetKey(anim.bool_tracks[0], time_from_ms(270), true);

	anim.color_tracks.resize(1);
	SetKey(anim.color_tracks[0], time_from_ms(300), Color::Red);
	SetKey(anim.color_tracks[0], time_from_ms(500), Color::Purple);
	SetKey(anim.color_tracks[0], time_from_ms(700), Color::Orange);

	anim.float_tracks.resize(1);
	SetKey(anim.float_tracks[0], time_from_ms(400),-1.f);
	SetKey(anim.float_tracks[0], time_from_ms(600), 1.f);
		
	anim.int_tracks.resize(1);
	SetKey(anim.int_tracks[0], time_from_ms(200), 1);
	SetKey(anim.int_tracks[0], time_from_ms(320), 2);
	SetKey(anim.int_tracks[0], time_from_ms(460), 3);
	SetKey(anim.int_tracks[0], time_from_ms(512), 4);
	SetKey(anim.int_tracks[0], time_from_ms(640), 5);

	anim.quat_tracks.resize(1);
	SetKey(anim.quat_tracks[0], time_from_ms(400), Quaternion(-1.f, 0.f, 0.f, 0.f));
	SetKey(anim.quat_tracks[0], time_from_ms(600), Quaternion(0.f, 1.f, 0.f, 0.f));
	SetKey(anim.quat_tracks[0], time_from_ms(700), Quaternion(0.f, 0.f, 2.f, 0.f));

	anim.string_tracks.resize(2);
	SetKey(anim.string_tracks[0], time_from_ms(220), "1");
	SetKey(anim.string_tracks[0], time_from_ms(240), "2");
	SetKey(anim.string_tracks[1], time_from_ms(250), "3");
	SetKey(anim.string_tracks[1], time_from_ms(260), "4");

	anim.vec2_tracks.resize(4);
	SetKey(anim.vec2_tracks[0], time_from_ms(300), Vec2(4.f, 3.f));
	SetKey(anim.vec2_tracks[1], time_from_ms(300), Vec2(2.f, 1.f));
	SetKey(anim.vec2_tracks[2], time_from_ms(300), Vec2(0.f, -1.f));
	SetKey(anim.vec2_tracks[3], time_from_ms(300), Vec2(-2.f, -3.f));

	anim.vec3_tracks.resize(1);
	SetKey(anim.vec3_tracks[0], time_from_ms(450), Vec3::Zero);

	anim.vec4_tracks.resize(1);
	SetKey(anim.vec4_tracks[0], time_from_ms(450), Vec4::Zero);
	SetKey(anim.vec4_tracks[0], time_from_ms(500), Vec4::One);

	ReverseAnim(anim, time_from_ms(100), time_from_ms(800));

	if (TEST_CHECK(anim.vec4_tracks.size() == 1)) {
		TEST_CHECK(anim.vec4_tracks[0].keys[0].t == time_from_ms(400));
		TEST_CHECK(anim.vec4_tracks[0].keys[0].v == Vec4::One);

		TEST_CHECK(anim.vec4_tracks[0].keys[1].t == time_from_ms(450));
		TEST_CHECK(anim.vec4_tracks[0].keys[1].v == Vec4::Zero);
	}

	if (TEST_CHECK(anim.vec3_tracks.size() == 1)) {
		TEST_CHECK(anim.vec3_tracks[0].keys[0].t == time_from_ms(450));
		TEST_CHECK(anim.vec3_tracks[0].keys[0].v == Vec3::Zero);
	}

	if (TEST_CHECK(anim.vec2_tracks.size() == 4)) {
		TEST_CHECK(anim.vec2_tracks[0].keys[0].t == time_from_ms(600));
		TEST_CHECK(anim.vec2_tracks[0].keys[0].v == Vec2(4.f, 3.f));

		TEST_CHECK(anim.vec2_tracks[1].keys[0].t == time_from_ms(600));
		TEST_CHECK(anim.vec2_tracks[1].keys[0].v == Vec2(2.f, 1.f));

		TEST_CHECK(anim.vec2_tracks[2].keys[0].t == time_from_ms(600));
		TEST_CHECK(anim.vec2_tracks[2].keys[0].v == Vec2(0.f, -1.f));

		TEST_CHECK(anim.vec2_tracks[3].keys[0].t == time_from_ms(600));
		TEST_CHECK(anim.vec2_tracks[3].keys[0].v == Vec2(-2.f, -3.f));
	}

	if (TEST_CHECK((anim.string_tracks.size() == 2) && (anim.string_tracks[0].keys.size() == 2) && (anim.string_tracks[1].keys.size() == 2))) {
		TEST_CHECK(anim.string_tracks[1].keys[0].t == time_from_ms(640));
		TEST_CHECK(anim.string_tracks[1].keys[0].v == "4");

		TEST_CHECK(anim.string_tracks[1].keys[1].t == time_from_ms(650));
		TEST_CHECK(anim.string_tracks[1].keys[1].v == "3");

		TEST_CHECK(anim.string_tracks[0].keys[0].t == time_from_ms(660));
		TEST_CHECK(anim.string_tracks[0].keys[0].v == "2");

		TEST_CHECK(anim.string_tracks[0].keys[1].t == time_from_ms(680));
		TEST_CHECK(anim.string_tracks[0].keys[1].v == "1");
	}

	if (TEST_CHECK((anim.quat_tracks.size() == 1) && (anim.quat_tracks[0].keys.size() == 3))) {
		TEST_CHECK(anim.quat_tracks[0].keys[0].t == time_from_ms(200));
		TEST_CHECK(anim.quat_tracks[0].keys[0].v == Quaternion(0.f, 0.f, 2.f, 0.f));

		TEST_CHECK(anim.quat_tracks[0].keys[1].t == time_from_ms(300));
		TEST_CHECK(anim.quat_tracks[0].keys[1].v == Quaternion(0.f, 1.f, 0.f, 0.f));

		TEST_CHECK(anim.quat_tracks[0].keys[2].t == time_from_ms(500));
		TEST_CHECK(anim.quat_tracks[0].keys[2].v == Quaternion(-1.f, 0.f, 0.f, 0.f));
	}

	if (TEST_CHECK((anim.bool_tracks.size() == 1) && (anim.bool_tracks[0].keys.size() == 5))) {
		TEST_CHECK(anim.bool_tracks[0].keys[0].t == time_from_ms(630));
		TEST_CHECK(anim.bool_tracks[0].keys[0].v == true);

		TEST_CHECK(anim.bool_tracks[0].keys[1].t == time_from_ms(640));
		TEST_CHECK(anim.bool_tracks[0].keys[1].v == true);

		TEST_CHECK(anim.bool_tracks[0].keys[2].t == time_from_ms(650));
		TEST_CHECK(anim.bool_tracks[0].keys[2].v == false);

		TEST_CHECK(anim.bool_tracks[0].keys[3].t == time_from_ms(660));
		TEST_CHECK(anim.bool_tracks[0].keys[3].v == false);

		TEST_CHECK(anim.bool_tracks[0].keys[4].t == time_from_ms(680));
		TEST_CHECK(anim.bool_tracks[0].keys[4].v == true);
	}

	if (TEST_CHECK((anim.color_tracks.size() == 1) && (anim.color_tracks[0].keys.size() == 3))) {
		TEST_CHECK(anim.color_tracks[0].keys[0].t == time_from_ms(200));
		TEST_CHECK(anim.color_tracks[0].keys[0].v == Color::Orange);

		TEST_CHECK(anim.color_tracks[0].keys[1].t == time_from_ms(400));
		TEST_CHECK(anim.color_tracks[0].keys[1].v == Color::Purple);

		TEST_CHECK(anim.color_tracks[0].keys[2].t == time_from_ms(600));
		TEST_CHECK(anim.color_tracks[0].keys[2].v == Color::Red);
	}

	if (TEST_CHECK((anim.float_tracks.size() == 1) && (anim.float_tracks[0].keys.size() == 2))) {
		TEST_CHECK(anim.float_tracks[0].keys[0].t == time_from_ms(300));
		TEST_CHECK(anim.float_tracks[0].keys[0].v == 1.f);

		TEST_CHECK(anim.float_tracks[0].keys[1].t == time_from_ms(500));
		TEST_CHECK(anim.float_tracks[0].keys[1].v == -1.f);
	}

	if (TEST_CHECK((anim.int_tracks.size() == 1) && (anim.int_tracks[0].keys.size() == 5))) {
		TEST_CHECK(anim.int_tracks[0].keys[0].t == time_from_ms(260));
		TEST_CHECK(anim.int_tracks[0].keys[0].v == 5);

		TEST_CHECK(anim.int_tracks[0].keys[1].t == time_from_ms(388));
		TEST_CHECK(anim.int_tracks[0].keys[1].v == 4);

		TEST_CHECK(anim.int_tracks[0].keys[2].t == time_from_ms(440));
		TEST_CHECK(anim.int_tracks[0].keys[2].v == 3);

		TEST_CHECK(anim.int_tracks[0].keys[3].t == time_from_ms(580));
		TEST_CHECK(anim.int_tracks[0].keys[3].v == 2);

		TEST_CHECK(anim.int_tracks[0].keys[4].t == time_from_ms(700));
		TEST_CHECK(anim.int_tracks[0].keys[4].v == 1);
	}
}

template <typename T> void anim_clear_track(T &track) {
	for (size_t i = 0; i < track.size(); i++) {
		track[i].keys.clear();
	}
	track.clear();
}

static void anim_clear(Anim& anim) {
	anim_clear_track(anim.bool_tracks);
	anim_clear_track(anim.string_tracks);
	anim_clear_track(anim.int_tracks);
	anim_clear_track(anim.float_tracks);
	anim_clear_track(anim.vec2_tracks);
	anim_clear_track(anim.vec3_tracks);
	anim_clear_track(anim.vec4_tracks);
	anim_clear_track(anim.quat_tracks);
	anim_clear_track(anim.color_tracks);
	anim.instance_anim_track.keys.clear();
}

static void test_anim_has_keys() {
	Anim anim;
	anim.t_start = time_from_ms(200);
	anim.t_end = time_from_ms(700);

	anim.bool_tracks.resize(1);
	anim.string_tracks.resize(1);
	
	TEST_CHECK(AnimHasKeys(anim) == false);

	anim_clear(anim);
	anim.vec2_tracks.resize(3);
	SetKey(anim.vec2_tracks[2], time_from_ms(400), Vec2::One);
	TEST_CHECK(AnimHasKeys(anim) == true);

	anim_clear(anim);
	anim.vec3_tracks.resize(2);
	SetKey(anim.vec3_tracks[1], time_from_ms(300), Vec3::Zero);
	TEST_CHECK(AnimHasKeys(anim) == true);

	anim_clear(anim);
	anim.vec4_tracks.resize(2);
	SetKey(anim.vec4_tracks[1], time_from_ms(500), Vec4::One);
	TEST_CHECK(AnimHasKeys(anim) == true);

	anim_clear(anim);
	anim.float_tracks.resize(8);
	SetKey(anim.float_tracks[5], time_from_ms(100), 1.f);
	TEST_CHECK(AnimHasKeys(anim) == true);

	anim_clear(anim);
	anim.bool_tracks.resize(8);
	SetKey(anim.bool_tracks[5], time_from_ms(600), false);
	TEST_CHECK(AnimHasKeys(anim) == true);

	anim_clear(anim);
	anim.quat_tracks.resize(1);
	SetKey(anim.quat_tracks[0], time_from_ms(220), Quaternion::Identity);
	TEST_CHECK(AnimHasKeys(anim) == true);

	anim_clear(anim);
	anim.color_tracks.resize(7);
	SetKey(anim.color_tracks[2], time_from_ms(500), Color::White);
	SetKey(anim.color_tracks[4], time_from_ms(640), Color::Grey);
	TEST_CHECK(AnimHasKeys(anim) == true);

	anim_clear(anim);
	anim.int_tracks.resize(1);
	SetKey(anim.int_tracks[0], time_from_ms(464), 1);

	anim.string_tracks.resize(4);
	SetKey(anim.string_tracks[0], time_from_ms(320), "A");
	SetKey(anim.string_tracks[2], time_from_ms(555), "B");

	(void)SetKey(anim.instance_anim_track, 400, "_0_", ALM_Once, 2.f);
	(void)SetKey(anim.instance_anim_track, 404, "_1_", ALM_Infinite, 1.5f);
	(void)SetKey(anim.instance_anim_track, 444, "_2_", ALM_Loop, 0.5f);

	TEST_CHECK(AnimHasKeys(anim) == true);

	anim.int_tracks.clear();
	TEST_CHECK(AnimHasKeys(anim) == true);

	anim.string_tracks.clear();
	TEST_CHECK(AnimHasKeys(anim) == true);
}

static void test_anim_resample() {
	Anim anim;
	anim.t_start = time_from_ms(500);
	anim.t_end = time_from_ms(1500);
	
	anim.bool_tracks.resize(1);
	SetKey(anim.bool_tracks[0], time_from_ms(600), true);
	SetKey(anim.bool_tracks[0], time_from_ms(740), false);

	anim.int_tracks.resize(1);
	SetKey(anim.int_tracks[0], time_from_ms(820), 1);
	SetKey(anim.int_tracks[0], time_from_ms(860), 2);
	SetKey(anim.int_tracks[0], time_from_ms(980), 3);
	SetKey(anim.int_tracks[0], time_from_ms(1020), 4);

	anim.float_tracks.resize(1);
	SetKey(anim.float_tracks[0], time_from_ms(512), 0.f);

	anim.vec2_tracks.resize(3);
	SetKey(anim.vec2_tracks[0], time_from_ms(850), Vec2::Zero);
	SetKey(anim.vec2_tracks[1], time_from_ms(860), Vec2::One);
	SetKey(anim.vec2_tracks[2], time_from_ms(870), -Vec2::One);

	anim.vec3_tracks.resize(1);
	SetKey(anim.vec3_tracks[0], time_from_ms(850), Vec3::One);
	SetKey(anim.vec3_tracks[0], time_from_ms(860), Vec3::Zero);

	anim.vec4_tracks.resize(1);
	SetKey(anim.vec4_tracks[0], time_from_ms(1240), Vec4::One);

	anim.quat_tracks.resize(1);
	SetKey(anim.quat_tracks[0], time_from_ms(1240), Quaternion::Identity);

	anim.color_tracks.resize(1);
	SetKey(anim.color_tracks[0], time_from_ms(1111), Color::Green);

	anim.string_tracks.resize(1);
	SetKey(anim.string_tracks[0], time_from_ms(1111), "A");

	InstanceAnimKey k0 = SetKey(anim.instance_anim_track, 500, "000", ALM_Loop, 2.f);
	InstanceAnimKey k1 = SetKey(anim.instance_anim_track, 1500, "001", ALM_Once, 1.f);

	time_ns start = time_from_ms(0), end = time_from_ms(2000);
	ResampleAnim(anim, anim.t_start, anim.t_end, start, end, time_from_ms(100));
	
	TEST_CHECK(anim.t_start == start);
	TEST_CHECK(anim.t_end == end);

	if (TEST_CHECK(anim.bool_tracks.size() == 1)) {
		if (TEST_CHECK(anim.bool_tracks[0].keys.size() == 2)) {
			TEST_CHECK(anim.bool_tracks[0].keys[0].t == time_from_ms(200));
			TEST_CHECK(anim.bool_tracks[0].keys[0].v == true);

			TEST_CHECK(anim.bool_tracks[0].keys[1].t == time_from_ms(500));
			TEST_CHECK(anim.bool_tracks[0].keys[1].v == false);
		}
	}

	if (TEST_CHECK(anim.int_tracks.size() == 1)) {
		if (TEST_CHECK(anim.int_tracks[0].keys.size() == 3)) {
			TEST_CHECK(anim.int_tracks[0].keys[0].t == time_from_ms(600));
			TEST_CHECK(anim.int_tracks[0].keys[0].v == 1);

			TEST_CHECK(anim.int_tracks[0].keys[1].t == time_from_ms(700));
			TEST_CHECK(anim.int_tracks[0].keys[1].v == 2);

			TEST_CHECK(anim.int_tracks[0].keys[2].t == time_from_ms(1000));
			TEST_CHECK(anim.int_tracks[0].keys[2].v == 3);
		}
	}

	if (TEST_CHECK(anim.float_tracks.size() == 1)) {
		if (TEST_CHECK(anim.float_tracks[0].keys.size() == 1)) {
			TEST_CHECK(anim.float_tracks[0].keys[0].t == time_from_ms(0));
			TEST_CHECK(anim.float_tracks[0].keys[0].v == 0.f);
		}
	}

	if (TEST_CHECK(anim.vec2_tracks.size() == 3)) {
		if (TEST_CHECK(anim.vec2_tracks[0].keys.size() == 1)) {
			TEST_CHECK(anim.vec2_tracks[0].keys[0].t == time_from_ms(700));
			TEST_CHECK(anim.vec2_tracks[0].keys[0].v == Vec2::Zero);
		}
		if (TEST_CHECK(anim.vec2_tracks[1].keys.size() == 1)) {
			TEST_CHECK(anim.vec2_tracks[1].keys[0].t == time_from_ms(700));
			TEST_CHECK(anim.vec2_tracks[1].keys[0].v == Vec2::One);
		}
		if (TEST_CHECK(anim.vec2_tracks[2].keys.size() == 1)) {
			TEST_CHECK(anim.vec2_tracks[2].keys[0].t == time_from_ms(700));
			TEST_CHECK(anim.vec2_tracks[2].keys[0].v == -Vec2::One);
		}
	}

	if (TEST_CHECK(anim.vec3_tracks.size() == 1)) {
		if (TEST_CHECK(anim.vec3_tracks[0].keys.size() == 1)) {
			TEST_CHECK(anim.vec3_tracks[0].keys[0].t == time_from_ms(700));
			TEST_CHECK(anim.vec3_tracks[0].keys[0].v == Vec3::One);
		}
	}

	if (TEST_CHECK(anim.vec4_tracks.size() == 1)) {
		if (TEST_CHECK(anim.vec4_tracks[0].keys.size() == 1)) {
			TEST_CHECK(anim.vec4_tracks[0].keys[0].t == time_from_ms(1500));
			TEST_CHECK(anim.vec4_tracks[0].keys[0].v == Vec4::One);
		}
	}
	
	if (TEST_CHECK(anim.quat_tracks.size() == 1)) {
		if (TEST_CHECK(anim.quat_tracks[0].keys.size() == 1)) {
			TEST_CHECK(anim.quat_tracks[0].keys[0].t == time_from_ms(1500));
			TEST_CHECK(anim.quat_tracks[0].keys[0].v == Quaternion::Identity);
		}
	}

	if (TEST_CHECK(anim.color_tracks.size() == 1)) {
		if (TEST_CHECK(anim.color_tracks[0].keys.size() == 1)) {
			TEST_CHECK(anim.color_tracks[0].keys[0].t == time_from_ms(1200));
			TEST_CHECK(anim.color_tracks[0].keys[0].v == Color::Green);
		}
	}

	if (TEST_CHECK(anim.string_tracks.size() == 1)) {
		if (TEST_CHECK(anim.string_tracks[0].keys.size() == 1)) {
			TEST_CHECK(anim.string_tracks[0].keys[0].t == time_from_ms(1200));
			TEST_CHECK(anim.string_tracks[0].keys[0].v == "A");
		}
	}

	if (TEST_CHECK(anim.instance_anim_track.keys.size() == 2)) {
		TEST_CHECK(anim.instance_anim_track.keys[0].t == start);
		TEST_CHECK(anim.instance_anim_track.keys[0].v == k0);

		TEST_CHECK(anim.instance_anim_track.keys[1].t == end);
		TEST_CHECK(anim.instance_anim_track.keys[1].v == k1);
	}
}

static void test_anim_quantize() {
	Anim anim;
	anim.t_start = time_from_ms(0);
	anim.t_end = time_from_ms(2000);

	anim.bool_tracks.resize(2);
	SetKey(anim.bool_tracks[0], time_from_ms(180), false);
	SetKey(anim.bool_tracks[0], time_from_ms(10), true);
	SetKey(anim.bool_tracks[1], time_from_ms(760), false);
	SetKey(anim.bool_tracks[1], time_from_ms(1024), true);

	anim.int_tracks.resize(1);
	SetKey(anim.int_tracks[0], time_from_ms(720),  1);
	SetKey(anim.int_tracks[0], time_from_ms(500),  2);
	SetKey(anim.int_tracks[0], time_from_ms(1080), 3);
	SetKey(anim.int_tracks[0], time_from_ms(360),  4);

	anim.float_tracks.resize(1);
	SetKey(anim.float_tracks[0], time_from_ms(555), 0.f);

	anim.vec2_tracks.resize(3);
	SetKey(anim.vec2_tracks[0], time_from_ms(850), Vec2(1.f, 0.f));
	SetKey(anim.vec2_tracks[1], time_from_ms(860), Vec2(0.f, 2.f));
	SetKey(anim.vec2_tracks[2], time_from_ms(870), Vec2(3.f, 3.f));

	anim.vec3_tracks.resize(1);
	SetKey(anim.vec3_tracks[0], time_from_ms(880), Vec3::Front);
	SetKey(anim.vec3_tracks[0], time_from_ms(860), Vec3::Up);
	SetKey(anim.vec3_tracks[0], time_from_ms(840), Vec3::Right);

	anim.vec4_tracks.resize(5);
	SetKey(anim.vec4_tracks[0], time_from_ms(1240), Vec4::One);

	anim.quat_tracks.resize(1);
	SetKey(anim.quat_tracks[0], time_from_ms(140), Quaternion(1.f, 0.f, 0.f, 0.f));
	SetKey(anim.quat_tracks[0], time_from_ms(280), Quaternion(0.f, 1.f, 0.f, 0.f));
	SetKey(anim.quat_tracks[0], time_from_ms(390), Quaternion(0.f, 0.f, 1.f, 0.f));
	
	anim.color_tracks.resize(2);
	SetKey(anim.color_tracks[0], time_from_ms(1111), Color::Green);
	SetKey(anim.color_tracks[1], time_from_ms(500), Color::Blue);
	SetKey(anim.color_tracks[1], time_from_ms(980), Color::Red);
	SetKey(anim.color_tracks[1], time_from_ms(916), Color::Orange);

	anim.string_tracks.resize(1);
	SetKey(anim.string_tracks[0], time_from_ms(404), "A");

	InstanceAnimKey k0 = SetKey(anim.instance_anim_track, 500, "000", ALM_Loop, 2.f);
	InstanceAnimKey k1 = SetKey(anim.instance_anim_track, 1500, "001", ALM_Once, 1.f);

	QuantizeAnim(anim, time_from_ms(200));

	if (TEST_CHECK(anim.instance_anim_track.keys.size() == 2)) {
		TEST_CHECK(anim.instance_anim_track.keys[0].t == time_from_ms(400));
		TEST_CHECK(anim.instance_anim_track.keys[0].v == k0);

		TEST_CHECK(anim.instance_anim_track.keys[1].t == time_from_ms(1400));
		TEST_CHECK(anim.instance_anim_track.keys[1].v == k1);
	}
	
	if (TEST_CHECK(anim.string_tracks.size() == 1)) {
		if (TEST_CHECK(anim.string_tracks[0].keys.size() == 1)) {
			TEST_CHECK(anim.string_tracks[0].keys[0].t == time_from_ms(400));
			TEST_CHECK(anim.string_tracks[0].keys[0].v == "A");
		}
	}

	if (TEST_CHECK(anim.color_tracks.size() == 2)) {
		if (TEST_CHECK(anim.color_tracks[0].keys.size() == 1)) {
			TEST_CHECK(anim.color_tracks[0].keys[0].t == time_from_ms(1000));
			TEST_CHECK(anim.color_tracks[0].keys[0].v == Color::Green);
		}

		if (TEST_CHECK(anim.color_tracks[1].keys.size() == 2)) {
			TEST_CHECK(anim.color_tracks[1].keys[0].t == time_from_ms(400));
			TEST_CHECK(anim.color_tracks[1].keys[0].v == Color::Blue);

			TEST_CHECK(anim.color_tracks[1].keys[1].t == time_from_ms(800));
			TEST_CHECK(anim.color_tracks[1].keys[1].v == Color::Orange);
		}
	}

	if (TEST_CHECK(anim.quat_tracks.size() == 1)) {
		if (TEST_CHECK(anim.quat_tracks[0].keys.size() == 2)) {
			TEST_CHECK(anim.quat_tracks[0].keys[0].t == time_from_ms(0));
			TEST_CHECK(anim.quat_tracks[0].keys[0].v == Quaternion(1.f, 0.f, 0.f, 0.f));

			TEST_CHECK(anim.quat_tracks[0].keys[1].t == time_from_ms(200));
			TEST_CHECK(anim.quat_tracks[0].keys[1].v == Quaternion(0.f, 1.f, 0.f, 0.f));
		}
	}
	
	if (TEST_CHECK(anim.vec4_tracks.size() == 5)) {
		if (TEST_CHECK(anim.vec4_tracks[0].keys.size() == 1)) {
			TEST_CHECK(anim.vec4_tracks[0].keys[0].t == time_from_ms(1200));
			TEST_CHECK(anim.vec4_tracks[0].keys[0].v == Vec4::One);
		}
	}

	if (TEST_CHECK(anim.vec3_tracks.size() == 1)) {
		if (TEST_CHECK(anim.vec3_tracks[0].keys.size() == 1)) {
			TEST_CHECK(anim.vec3_tracks[0].keys[0].t == time_from_ms(800));
			TEST_CHECK(anim.vec3_tracks[0].keys[0].v == Vec3::Right);
		}
	}

	if (TEST_CHECK(anim.vec2_tracks.size() == 3)) {
		if (TEST_CHECK(anim.vec2_tracks[0].keys.size() == 1)) {
			TEST_CHECK(anim.vec2_tracks[0].keys[0].t == time_from_ms(800));
			TEST_CHECK(anim.vec2_tracks[0].keys[0].v == Vec2(1.f, 0.f));
		}
		if (TEST_CHECK(anim.vec2_tracks[1].keys.size() == 1)) {
			TEST_CHECK(anim.vec2_tracks[1].keys[0].t == time_from_ms(800));
			TEST_CHECK(anim.vec2_tracks[1].keys[0].v == Vec2(0.f, 2.f));
		}
		if (TEST_CHECK(anim.vec2_tracks[2].keys.size() == 1)) {
			TEST_CHECK(anim.vec2_tracks[2].keys[0].t == time_from_ms(800));
			TEST_CHECK(anim.vec2_tracks[2].keys[0].v == Vec2(3.f, 3.f));
		}
	}

	if (TEST_CHECK(anim.float_tracks.size() == 1)) {
		if (TEST_CHECK(anim.float_tracks[0].keys.size() == 1)) {
			TEST_CHECK(anim.float_tracks[0].keys[0].t == time_from_ms(400));
			TEST_CHECK(anim.float_tracks[0].keys[0].v == 0.f);
		}
	}

	if (TEST_CHECK(anim.int_tracks.size() == 1)) {
		if (TEST_CHECK(anim.int_tracks[0].keys.size() == 4)) {
			TEST_CHECK(anim.int_tracks[0].keys[0].t == time_from_ms(200));
			TEST_CHECK(anim.int_tracks[0].keys[0].v == 4);

			TEST_CHECK(anim.int_tracks[0].keys[1].t == time_from_ms(400));
			TEST_CHECK(anim.int_tracks[0].keys[1].v == 2);

			TEST_CHECK(anim.int_tracks[0].keys[2].t == time_from_ms(600));
			TEST_CHECK(anim.int_tracks[0].keys[2].v == 1);

			TEST_CHECK(anim.int_tracks[0].keys[3].t == time_from_ms(1000));
			TEST_CHECK(anim.int_tracks[0].keys[3].v == 3);
		}
	}

	if (TEST_CHECK(anim.bool_tracks.size() == 2)) {
		if (TEST_CHECK(anim.bool_tracks[0].keys.size() == 1)) {
			TEST_CHECK(anim.bool_tracks[0].keys[0].t == time_from_ms(0));
			TEST_CHECK(anim.bool_tracks[0].keys[0].v == true);
		}
		if (TEST_CHECK(anim.bool_tracks[1].keys.size() == 2)) {
			TEST_CHECK(anim.bool_tracks[1].keys[0].t == time_from_ms(600));
			TEST_CHECK(anim.bool_tracks[1].keys[0].v == false);

			TEST_CHECK(anim.bool_tracks[1].keys[1].t == time_from_ms(1000));
			TEST_CHECK(anim.bool_tracks[1].keys[1].v == true);
		}
	}
}

static void test_anim_conform() {
	AnimTrackT<Quaternion> track;

	Quaternion q0 = QuaternionFromAxisAngle(Deg(30.f), Vec3::Up);
	Quaternion q1 = QuaternionFromAxisAngle(TwoPi + Deg(60.f), Vec3::Right);
	Quaternion q2 = QuaternionFromAxisAngle(Deg(1080.f), Vec3::Right);
	Quaternion q3 = QuaternionFromAxisAngle(Deg(60.f), Vec3::Right);
	Quaternion q4 = QuaternionFromAxisAngle(Deg(0.f), Vec3::Right);
	
	SetKey(track, time_from_ms(0), q0);
	SetKey(track, time_from_ms(100), q1);
	SetKey(track, time_from_ms(200), q2);

	ConformAnimTrackKeys(track);

	if (TEST_CHECK(track.keys.size() == 3)) {
		TEST_CHECK(AlmostEqual(track.keys[1].v.x, q3.x, 1e-6f));
		TEST_CHECK(AlmostEqual(track.keys[1].v.y, q3.y, 1e-6f));
		TEST_CHECK(AlmostEqual(track.keys[1].v.z, q3.z, 1e-6f));
		TEST_CHECK(AlmostEqual(track.keys[1].v.w, q3.w, 1e-6f));

		TEST_CHECK(AlmostEqual(track.keys[2].v.x, q4.x, 1e-6f));
		TEST_CHECK(AlmostEqual(track.keys[2].v.y, q4.y, 1e-6f));
		TEST_CHECK(AlmostEqual(track.keys[2].v.z, q4.z, 1e-6f));
		TEST_CHECK(AlmostEqual(track.keys[2].v.w, q4.w, 1e-6f));
	}
}

void test_animation() {
	test_anim_bool_track();
	test_anim_string_track();
	test_anim_int_track();
	test_anim_vec3_track();
	test_anim_float_track();
	test_anim_instance_track();
	test_anim_has_keys();
	test_anim_reverse();
	test_anim_resample();
	test_anim_quantize();
	test_anim_conform();
	test_misc();

	Anim anim;
	anim.t_start = time_from_ms(50);
	anim.t_end   = time_from_ms(1200);
	// [todo] anim.flags seems unused atm.

	TEST_CHECK(AnimHasKeys(anim) == false);

	anim.bool_tracks.resize(2);
	anim.float_tracks.resize(1);

	SetKey(anim.bool_tracks[0], time_from_ms(50), false);
	SetKey(anim.bool_tracks[0], time_from_ms(100), true);
	SetKey(anim.bool_tracks[0], time_from_ms(1000), false);
	
	SetKey(anim.float_tracks[0], time_from_ms(50), 0.f);
	SetKey(anim.float_tracks[0], time_from_ms(100), 0.2f);
	SetKey(anim.float_tracks[0], time_from_ms(400), 1.f);
	SetKey(anim.float_tracks[0], time_from_ms(1000), 0.2f);
	SetKey(anim.float_tracks[0], time_from_ms(1200), 0.f);

	TEST_CHECK(AnimHasKeys(anim) == true);

	DeleteEmptyAnimTracks(anim);
	TEST_CHECK(anim.bool_tracks.size() == 1);
}
