// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/animation.h"
#include "engine/to_json.h"

namespace hg {

template <typename T> void SaveAnimKey(json &js, const AnimKeyT<T> &key) { js = {{"t", key.t}, {"v", key.v}}; }

template <typename T> void SaveAnimKey(json &js, const AnimKeyHermiteT<T> &key) {
	js = {{"t", key.t}, {"v", key.v}, {"tension", key.tension}, {"bias", key.bias}};
}

template <typename Track> void SaveAnimTrack(json &js, const Track &track) {
	js["target"] = track.target;

	if (!track.keys.empty()) {
		auto &keys_json = js["keys"];
		for (const auto &key : track.keys) {
			json key_json;
			SaveAnimKey(key_json, key);
			keys_json.push_back(std::move(key_json));
		}
	}
}

template <typename Track> void SaveAnimTracks(json &js, const std::string &name, const std::vector<Track> &tracks) {
	if (!tracks.empty()) {
		auto &tracks_js = js[name];
		for (const auto &track : tracks) {
			json track_js;
			SaveAnimTrack(track_js, track);
			tracks_js.push_back(std::move(track_js));
		}
	}
}

void SaveInstanceAnimTrack(json &js, const AnimTrackT<InstanceAnimKey> &track) {
	if (!track.keys.empty()) {
		auto &keys_js = js["instance_anim"]["keys"];
		for (const auto &key : track.keys)
			keys_js.push_back({{"t", key.t}, {"name", key.v.anim_name}, {"loop", key.v.loop_mode}, {"scale", key.v.t_scale}});
	}
}

void SaveAnimToJson(json &js, const Anim &anim) {
	js["t_start"] = anim.t_start;
	js["t_end"] = anim.t_end;

	json flags = json::array();
	if (anim.flags & AF_UseQuaternionForRotation)
		flags.push_back("UseQuaternionForRotation");

	js["flags"] = flags;

	SaveAnimTracks(js, "bool", anim.bool_tracks);
	SaveAnimTracks(js, "int", anim.int_tracks);
	SaveAnimTracks(js, "float", anim.float_tracks);
	SaveAnimTracks(js, "vec2", anim.vec2_tracks);
	SaveAnimTracks(js, "vec3", anim.vec3_tracks);
	SaveAnimTracks(js, "vec4", anim.vec4_tracks);
	SaveAnimTracks(js, "quat", anim.quat_tracks);
	SaveAnimTracks(js, "color", anim.color_tracks);
	SaveAnimTracks(js, "string", anim.string_tracks);

	SaveInstanceAnimTrack(js, anim.instance_anim_track);
}

//
template <typename T> void LoadAnimKey(const json &js, AnimKeyT<T> &key) {
	key.t = js.at("t").get<time_ns>();
	key.v = js.at("v").get<T>();
}

template <typename T> void LoadAnimKey(const json &js, AnimKeyHermiteT<T> &key) {
	key.t = js.at("t").get<time_ns>();
	key.v = js.at("v").get<T>();
	key.tension = js.at("tension").get<float>();
	key.bias = js.at("bias").get<float>();
}

template <typename Track> void LoadAnimTrack(const json &js, Track &track) {
	track.target = js.at("target").get<std::string>();

	const auto &keys_js = js.find("keys");
	if (keys_js != std::end(js)) {
		track.keys.resize(keys_js->size());
		for (size_t i = 0; i < keys_js->size(); ++i)
			LoadAnimKey(keys_js->at(i), track.keys[i]);
	}

	SortAnimTrackKeys(track);
}

void LoadInstanceAnimTrack(const json &js, AnimTrackT<InstanceAnimKey> &track) {
	const auto &track_js = js.find("instance_anim");
	if (track_js == std::end(js))
		return; // no such track

	const auto &keys_js = track_js->find("keys");
	if (keys_js != std::end(*track_js)) {
		track.keys.resize(keys_js->size());
		for (size_t i = 0; i < keys_js->size(); ++i) {
			auto &key_js = keys_js->at(i);
			auto &key = track.keys[i];

			key.t = key_js.at("t").get<time_ns>();
			key.v.anim_name = key_js.at("name").get<std::string>();
			key.v.loop_mode = key_js.at("loop").get<AnimLoopMode>();
			key.v.t_scale = key_js.at("scale").get<float>();
		}
	}

	SortAnimTrackKeys(track);
}

template <typename Track> void LoadAnimTracks(const json &js, const std::string &name, std::vector<Track> &tracks) {
	const auto &tracks_js = js.find(name);
	if (tracks_js == std::end(js))
		return; // no such group

	tracks.resize(tracks_js->size());
	for (size_t i = 0; i < tracks_js->size(); ++i)
		LoadAnimTrack(tracks_js->at(i), tracks[i]);
}

void LoadAnimFromJson(const json &js, Anim &anim) {
	anim.t_start = js.at("t_start").get<time_ns>();
	anim.t_end = js.at("t_end").get<time_ns>();

	LoadAnimTracks(js, "bool", anim.bool_tracks);
	LoadAnimTracks(js, "int", anim.int_tracks);
	LoadAnimTracks(js, "float", anim.float_tracks);
	LoadAnimTracks(js, "vec2", anim.vec2_tracks);
	LoadAnimTracks(js, "vec3", anim.vec3_tracks);
	LoadAnimTracks(js, "vec4", anim.vec4_tracks);
	LoadAnimTracks(js, "quat", anim.quat_tracks);
	LoadAnimTracks(js, "color", anim.color_tracks);
	LoadAnimTracks(js, "string", anim.string_tracks);

	LoadInstanceAnimTrack(js, anim.instance_anim_track);

	auto i = js.find("flags");
	if (i != std::end(js)) {
		for (auto j : *i)
			if (j == "UseQuaternionForRotation")
				anim.flags |= AF_UseQuaternionForRotation;
	} else {
		// legacy fallback
		if (!anim.quat_tracks.empty()) // any quaternion track takes precedence
			anim.flags |= AF_UseQuaternionForRotation;
	}

	MigrateLegacyAnimTracks(anim);
}

} // namespace hg
