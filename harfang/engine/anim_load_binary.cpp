// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/animation.h"

#include "foundation/format.h"
#include "foundation/log.h"

namespace hg {

void Write(const Writer &iw, const Handle &h, const tVec2<float> &v) {
	Write(iw, h, v.x);
	Write(iw, h, v.y);
}

void Write(const Writer &iw, const Handle &h, const Vec3 &v) {
	Write(iw, h, v.x);
	Write(iw, h, v.y);
	Write(iw, h, v.z);
}

void Write(const Writer &iw, const Handle &h, const Vec4 &v) {
	Write(iw, h, v.x);
	Write(iw, h, v.y);
	Write(iw, h, v.z);
	Write(iw, h, v.w);
}

void Write(const Writer &iw, const Handle &h, const Quaternion &v) {
	Write(iw, h, v.x);
	Write(iw, h, v.y);
	Write(iw, h, v.z);
	Write(iw, h, v.w);
}

void Write(const Writer &iw, const Handle &h, const Color &v) {
	Write(iw, h, v.r);
	Write(iw, h, v.g);
	Write(iw, h, v.b);
	Write(iw, h, v.a);
}

template <typename T> void SaveAnimKey(const Writer &iw, const Handle &h, const AnimKeyT<T> &key) {
	Write(iw, h, key.t);
	Write(iw, h, key.v);
}

template <typename T> void SaveAnimKey(const Writer &iw, const Handle &h, const AnimKeyHermiteT<T> &key) {
	Write(iw, h, key.t);
	Write(iw, h, key.v);
	Write(iw, h, key.tension);
	Write(iw, h, key.bias);
}

template <typename Track> void SaveAnimTrack(const Writer &iw, const Handle &h, const Track &track) {
	Write(iw, h, track.target);
	Write(iw, h, numeric_cast<uint32_t>(track.keys.size()));
	for (const auto &key : track.keys)
		SaveAnimKey(iw, h, key);
}

template <typename Track> void SaveAnimTracks(const Writer &iw, const Handle &h, const std::vector<Track> &tracks) {
	Write(iw, h, numeric_cast<uint32_t>(tracks.size()));
	for (const auto &track : tracks)
		SaveAnimTrack(iw, h, track);
}

void SaveInstanceAnimTrack(const Writer &iw, const Handle &h, const AnimTrackT<InstanceAnimKey> &track) {
	Write(iw, h, numeric_cast<uint32_t>(track.keys.size()));
	for (const auto &key : track.keys) {
		Write(iw, h, key.t);
		Write(iw, h, key.v.anim_name);
		Write(iw, h, key.v.loop_mode);
		Write(iw, h, key.v.t_scale);
	}
}

void SaveAnimToBinary(const Writer &iw, const Handle &h, const Anim &anim) {
	/*
		version 0: no version byte
		version 1: initial versioning on 16 bits
		version 2: instance anim track support
	*/
	Write<uint16_t>(iw, h, 2);

	Write(iw, h, anim.t_start);
	Write(iw, h, anim.t_end);
	Write<uint8_t>(iw, h, anim.flags & 0x0f);

	SaveAnimTracks(iw, h, anim.bool_tracks);
	SaveAnimTracks(iw, h, anim.int_tracks);
	SaveAnimTracks(iw, h, anim.float_tracks);
	SaveAnimTracks(iw, h, anim.vec2_tracks);
	SaveAnimTracks(iw, h, anim.vec3_tracks);
	SaveAnimTracks(iw, h, anim.vec4_tracks);
	SaveAnimTracks(iw, h, anim.quat_tracks);
	SaveAnimTracks(iw, h, anim.color_tracks);
	SaveAnimTracks(iw, h, anim.string_tracks);

	SaveInstanceAnimTrack(iw, h, anim.instance_anim_track);
}

//
void Read(const Reader &ir, const Handle &h, tVec2<float> &v) {
	Read(ir, h, v.x);
	Read(ir, h, v.y);
}

void Read(const Reader &ir, const Handle &h, Vec3 &v) {
	Read(ir, h, v.x);
	Read(ir, h, v.y);
	Read(ir, h, v.z);
}

void Read(const Reader &ir, const Handle &h, Vec4 &v) {
	Read(ir, h, v.x);
	Read(ir, h, v.y);
	Read(ir, h, v.z);
	Read(ir, h, v.w);
}

void Read(const Reader &ir, const Handle &h, Quaternion &v) {
	Read(ir, h, v.x);
	Read(ir, h, v.y);
	Read(ir, h, v.z);
	Read(ir, h, v.w);
}

void Read(const Reader &ir, const Handle &h, Color &v) {
	Read(ir, h, v.r);
	Read(ir, h, v.g);
	Read(ir, h, v.b);
	Read(ir, h, v.a);
}

template <typename T> void LoadAnimKey(const Reader &ir, const Handle &h, AnimKeyT<T> &key) {
	Read(ir, h, key.t);
	Read(ir, h, key.v);
}

template <typename T> void LoadAnimKey(const Reader &ir, const Handle &h, AnimKeyHermiteT<T> &key) {
	Read(ir, h, key.t);
	Read(ir, h, key.v);
	Read(ir, h, key.tension);
	Read(ir, h, key.bias);
}

template <typename Track> void LoadAnimTrack(const Reader &ir, const Handle &h, Track &track) {
	Read(ir, h, track.target);

	uint32_t count;
	Read(ir, h, count);
	track.keys.resize(count);
	for (uint32_t i = 0; i < count; ++i)
		LoadAnimKey(ir, h, track.keys[i]);

	SortAnimTrackKeys(track);
}

template <typename Track> void LoadAnimTracks(const Reader &ir, const Handle &h, std::vector<Track> &tracks) {
	uint32_t count;
	Read(ir, h, count);
	tracks.resize(count);
	for (uint32_t i = 0; i < count; ++i)
		LoadAnimTrack(ir, h, tracks[i]);
}

void LoadInstanceAnimTrack(const Reader &ir, const Handle &h, AnimTrackT<InstanceAnimKey> &track) {
	uint32_t count;
	Read(ir, h, count);
	track.keys.resize(count);

	for (uint32_t i = 0; i < count; ++i) {
		auto &key = track.keys[i];
		Read(ir, h, key.t);
		Read(ir, h, key.v.anim_name);
		Read(ir, h, key.v.loop_mode);
		Read(ir, h, key.v.t_scale);
	}
}

void LoadAnimFromBinary(const Reader &ir, const Handle &h, Anim &anim) {
	uint16_t version;
	Read(ir, h, version);

	if (version > 2) {
		warn(format("Unsupported animation format version %1").arg(version));
		return;
	}

	Read(ir, h, anim.t_start);
	Read(ir, h, anim.t_end);
	Read(ir, h, anim.flags);

	LoadAnimTracks(ir, h, anim.bool_tracks);
	LoadAnimTracks(ir, h, anim.int_tracks);
	LoadAnimTracks(ir, h, anim.float_tracks);
	LoadAnimTracks(ir, h, anim.vec2_tracks);
	LoadAnimTracks(ir, h, anim.vec3_tracks);
	LoadAnimTracks(ir, h, anim.vec4_tracks);
	LoadAnimTracks(ir, h, anim.quat_tracks);
	LoadAnimTracks(ir, h, anim.color_tracks);
	LoadAnimTracks(ir, h, anim.string_tracks);

	if (version >= 2)
		LoadInstanceAnimTrack(ir, h, anim.instance_anim_track);

	MigrateLegacyAnimTracks(anim);
}

} // namespace hg
