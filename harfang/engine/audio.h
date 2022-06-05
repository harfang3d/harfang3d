// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <foundation/matrix4.h>
#include <foundation/time.h>
#include <foundation/vector3.h>

#include <engine/audio_stream_interface.h>

namespace hg {

/// Initialize the audio system.
bool AudioInit();
/// Shutdown the audio system.
void AudioShutdown();
bool IsAudioUp();

//
void SetListener(const Mat4 &world, const Vec3 &velocity);

//
enum SourceRepeat { SR_Once, SR_Loop };

struct StereoSourceState {
	float volume{1.f};
	SourceRepeat repeat{SR_Once};

	float panning{0.f}; // from -1 to 1 with 0 as the center
};

struct SpatializedSourceState {
	Mat4 mtx{Mat4::Identity};

	float volume{1.f};
	SourceRepeat repeat{SR_Once};

	Vec3 vel{}; // for Doppler effect
};

//
using SoundRef = int;
static const SoundRef InvalidSoundRef = -1;

SoundRef LoadWAVSoundFile(const char *path);
SoundRef LoadWAVSoundAsset(const char *name);

SoundRef LoadOGGSoundFile(const char *path);
SoundRef LoadOGGSoundAsset(const char *name);

void UnloadSound(SoundRef snd_ref);

//
using SourceRef = int;
static const SourceRef InvalidSourceRef = -1;

SourceRef PlayStereo(SoundRef snd_ref, const StereoSourceState &state);
SourceRef PlaySpatialized(SoundRef snd_ref, const SpatializedSourceState &state);

SourceRef StreamWAVFileStereo(const char *path, const StereoSourceState &state);
SourceRef StreamWAVAssetStereo(const char *name, const StereoSourceState &state);
SourceRef StreamWAVFileSpatialized(const char *path, const SpatializedSourceState &state);
SourceRef StreamWAVAssetSpatialized(const char *name, const SpatializedSourceState &state);

SourceRef StreamOGGFileStereo(const char *path, const StereoSourceState &state);
SourceRef StreamOGGAssetStereo(const char *name, const StereoSourceState &state);
SourceRef StreamOGGFileSpatialized(const char *path, const SpatializedSourceState &state);
SourceRef StreamOGGAssetSpatialized(const char *name, const SpatializedSourceState &state);

//
time_ns GetSourceDuration(SourceRef src_ref);
time_ns GetSourceTimecode(SourceRef src_ref);
bool SetSourceTimecode(SourceRef src_ref, time_ns t);

//
void SetSourceVolume(SourceRef src_ref, float volume);
void SetSourcePanning(SourceRef src_ref, float panning);
void SetSourceRepeat(SourceRef src_ref, SourceRepeat repeat);
void SetSourceTransform(SourceRef src_ref, const Mat4 &world, const Vec3 &velocity);

//
enum SourceState { SS_Initial, SS_Playing, SS_Paused, SS_Stopped, SS_Invalid };

SourceState GetSourceState(SourceRef src_ref);

void PauseSource(SourceRef src_ref);

void StopSource(SourceRef src_ref);
void StopAllSources();

// [todo] loop point?

} // namespace hg
