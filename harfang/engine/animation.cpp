// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/animation.h"

#include "foundation/log.h"

namespace hg {

template <> bool Evaluate<bool>(const AnimTrackT<bool> &track, time_ns t, bool &v) { return EvaluateStep<AnimTrackT<bool>, bool>(track, t, v); }

template <> bool Evaluate<std::string>(const AnimTrackT<std::string> &track, time_ns t, std::string &v) {
	return EvaluateStep<AnimTrackT<std::string>, std::string>(track, t, v);
}

template <> bool Evaluate(const AnimTrackT<InstanceAnimKey> &track, time_ns t, InstanceAnimKey &v) {
	return EvaluateStep<AnimTrackT<InstanceAnimKey>, InstanceAnimKey>(track, t, v);
}

void ResampleAnim(Anim &anim, time_ns old_start, time_ns old_end, time_ns new_start, time_ns new_end, time_ns frame_duration) {
	const auto old_duration = old_end - old_start;
	const auto new_duration = new_end - new_start;

	const auto scale = (new_duration * 1000) / (old_duration / 1000); // 10^6 precision

	for (auto &track : anim.bool_tracks)
		ResampleAnimTrack(track, old_start, new_start, scale, frame_duration);
	for (auto &track : anim.int_tracks)
		ResampleAnimTrack(track, old_start, new_start, scale, frame_duration);
	for (auto &track : anim.float_tracks)
		ResampleAnimTrack(track, old_start, new_start, scale, frame_duration);
	for (auto &track : anim.vec2_tracks)
		ResampleAnimTrack(track, old_start, new_start, scale, frame_duration);
	for (auto &track : anim.vec3_tracks)
		ResampleAnimTrack(track, old_start, new_start, scale, frame_duration);
	for (auto &track : anim.vec4_tracks)
		ResampleAnimTrack(track, old_start, new_start, scale, frame_duration);
	for (auto &track : anim.quat_tracks)
		ResampleAnimTrack(track, old_start, new_start, scale, frame_duration);
	for (auto &track : anim.color_tracks)
		ResampleAnimTrack(track, old_start, new_start, scale, frame_duration);
	for (auto &track : anim.string_tracks)
		ResampleAnimTrack(track, old_start, new_start, scale, frame_duration);

	ResampleAnimTrack(anim.instance_anim_track, old_start, new_start, scale, frame_duration);

	anim.t_start = new_start;
	anim.t_end = new_end;
}

template <typename Track> void ReverseAnimTrack(Track &track, time_ns t_start, time_ns t_end) {
	for (auto &key : track.keys)
		key.t = t_end - (key.t - t_start);
	SortAnimTrackKeys(track);
}

void ReverseAnim(Anim &anim, time_ns t_start, time_ns t_end) {
	for (auto &track : anim.bool_tracks)
		ReverseAnimTrack(track, t_start, t_end);
	for (auto &track : anim.int_tracks)
		ReverseAnimTrack(track, t_start, t_end);
	for (auto &track : anim.float_tracks)
		ReverseAnimTrack(track, t_start, t_end);
	for (auto &track : anim.vec2_tracks)
		ReverseAnimTrack(track, t_start, t_end);
	for (auto &track : anim.vec3_tracks)
		ReverseAnimTrack(track, t_start, t_end);
	for (auto &track : anim.vec4_tracks)
		ReverseAnimTrack(track, t_start, t_end);
	for (auto &track : anim.quat_tracks)
		ReverseAnimTrack(track, t_start, t_end);
	for (auto &track : anim.color_tracks)
		ReverseAnimTrack(track, t_start, t_end);
	for (auto &track : anim.string_tracks)
		ReverseAnimTrack(track, t_start, t_end);

	ReverseAnimTrack(anim.instance_anim_track, t_start, t_end);
}

template <typename Track> void QuantizeAnimTrack(Track &track, time_ns t_step) {
	for (auto &key : track.keys)
		key.t = (key.t / t_step) * t_step;
	SortAnimTrackKeys(track);
}

void QuantizeAnim(Anim &anim, time_ns t_step) {
	for (auto &track : anim.bool_tracks)
		QuantizeAnimTrack(track, t_step);
	for (auto &track : anim.int_tracks)
		QuantizeAnimTrack(track, t_step);
	for (auto &track : anim.float_tracks)
		QuantizeAnimTrack(track, t_step);
	for (auto &track : anim.vec2_tracks)
		QuantizeAnimTrack(track, t_step);
	for (auto &track : anim.vec3_tracks)
		QuantizeAnimTrack(track, t_step);
	for (auto &track : anim.vec4_tracks)
		QuantizeAnimTrack(track, t_step);
	for (auto &track : anim.quat_tracks)
		QuantizeAnimTrack(track, t_step);
	for (auto &track : anim.color_tracks)
		QuantizeAnimTrack(track, t_step);
	for (auto &track : anim.string_tracks)
		QuantizeAnimTrack(track, t_step);

	QuantizeAnimTrack(anim.instance_anim_track, t_step);
}

void ConformAnimTrackKeys(AnimTrackT<Quaternion> &track) {
	// make sure adjacent quaternions use the shortest path when interpolated
	for (size_t i = 1; i < track.keys.size(); i++) {
		auto prev_quat = track.keys[i - 1].v;
		auto curr_quat = track.keys[i].v;

		if (Dot(prev_quat, curr_quat) < 0) {
			curr_quat *= -1.0f;
			track.keys[i].v = curr_quat;
		}
	}
}

//
void MigrateLegacyAnimTracks(Anim &anim) {
	for (auto i = std::begin(anim.string_tracks); i != std::end(anim.string_tracks);) {
		const auto &track = *i;

		if (track.target == "Instance.Anim") { // migrate legacy instance animation track
			if (anim.instance_anim_track.keys.empty()) {
				for (auto &k : track.keys)
					anim.instance_anim_track.keys.push_back({k.t, {k.v, ALM_Once}});
			} else {
				warn("Not migrating legacy instance animation track as a modern track of this type exists");
			}

			i = anim.string_tracks.erase(i);
		} else {
			++i;
		}
	}
}

//
template <typename AnimTrack> bool AnimTracksHaveKeys(const std::vector<AnimTrack> &tracks) {
	for (auto &track : tracks)
		if (!track.keys.empty())
			return true;
	return false;
}

bool AnimHasKeys(const Anim &anim) {
	if (AnimTracksHaveKeys(anim.vec3_tracks))
		return true;
	if (AnimTracksHaveKeys(anim.vec4_tracks))
		return true;
	if (AnimTracksHaveKeys(anim.quat_tracks))
		return true;
	if (AnimTracksHaveKeys(anim.color_tracks))
		return true;
	if (AnimTracksHaveKeys(anim.float_tracks))
		return true;
	if (AnimTracksHaveKeys(anim.bool_tracks))
		return true;
	if (AnimTracksHaveKeys(anim.int_tracks))
		return true;
	if (AnimTracksHaveKeys(anim.vec2_tracks))
		return true;
	if (AnimTracksHaveKeys(anim.string_tracks))
		return true;
	if (!anim.instance_anim_track.keys.empty())
		return true;
	return false;
}

//
template <typename AnimTrack> void DeleteEmptyAnimTracks_(std::vector<AnimTrack> &tracks) {
	tracks.erase(std::remove_if(std::begin(tracks), std::end(tracks), [](const AnimTrack &track) { return track.keys.empty(); }), std::end(tracks));
}

void DeleteEmptyAnimTracks(Anim &anim) {
	DeleteEmptyAnimTracks_(anim.vec3_tracks);
	DeleteEmptyAnimTracks_(anim.vec4_tracks);
	DeleteEmptyAnimTracks_(anim.quat_tracks);
	DeleteEmptyAnimTracks_(anim.color_tracks);
	DeleteEmptyAnimTracks_(anim.float_tracks);
	DeleteEmptyAnimTracks_(anim.bool_tracks);
	DeleteEmptyAnimTracks_(anim.int_tracks);
	DeleteEmptyAnimTracks_(anim.vec2_tracks);
	DeleteEmptyAnimTracks_(anim.string_tracks);
}

} // namespace hg
