// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/audio.h"

#include "foundation/cext.h"
#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/timer.h"

#include "engine/ogg_audio_stream.h"
#include "engine/wav_audio_stream.h"

#include <AL/alc.h>
#include <AL/alext.h>

#include <bx/bx.h>

#include <cmath>
#include <future>

namespace hg {

static const size_t max_source = 64;
static const time_ns audio_mixer_period = time_from_ms(20);

//
struct ALStream {
	std::mutex lock;

	IAudioStreamer streamer{};
	AudioStreamRef ref{InvalidAudioStreamRef};

	std::vector<ALuint> buffers;
	std::vector<time_ns> buffers_timestamp;
	std::vector<AudioFrameFormat> buffers_format;

	time_ns timestamp;
	size_t free_buffer_count{}, get{}, put{};

	bool loop{false};
};

struct ALMixer {
	std::mutex lock;
	timer_handle update;

	ALCdevice *device{nullptr};
	ALCcontext *context{nullptr};

	ALuint sources[max_source];
	ALStream streams[max_source];
};

static ALMixer al_mixer;

//
static bool CheckALSuccess(const char *file = "unknown", ALuint line = 0) {
	switch (alGetError()) {
		case AL_NO_ERROR:
			return true;
		case AL_INVALID_NAME:
			warn(format("AL invalid name (%1:%2)").arg(file).arg(line));
			break;
		case AL_INVALID_ENUM:
			warn(format("AL invalid enum (%1:%2)").arg(file).arg(line));
			break;
		case AL_INVALID_VALUE:
			warn(format("AL invalid value (%1:%2)").arg(file).arg(line));
			break;
		case AL_INVALID_OPERATION:
			warn(format("AL invalid operation (%1:%2)").arg(file).arg(line));
			break;
		case AL_OUT_OF_MEMORY:
			warn(format("AL out of memory (%1:%2)").arg(file).arg(line));
			break;
		default:
			warn(format("AL error (%1:%2)").arg(file).arg(line));
			break;
	}
	return false;
}

#define __AL_OK CheckALSuccess(__FILE__, __LINE__)

#define __AL_CALL(_C_)                                                                                                                                         \
	do {                                                                                                                                                       \
		_C_;                                                                                                                                                   \
		(void)CheckALSuccess(__FILE__, __LINE__);                                                                                                              \
	} while (0);

#define __AL_CALL_RET(_C_)                                                                                                                                     \
	do {                                                                                                                                                       \
		_C_;                                                                                                                                                   \
		if (!CheckALSuccess(__FILE__, __LINE__)) {                                                                                                             \
			return false;                                                                                                                                      \
		}                                                                                                                                                      \
	} while (0);

//
static inline int AFF_ALFormat(AudioFrameFormat fmt) {
	switch (fmt) {
		case AFF_LPCM_44KHZ_S16_Mono:
		case AFF_LPCM_48KHZ_S16_Mono:
			return AL_FORMAT_MONO16;
		case AFF_LPCM_44KHZ_S16_Stereo:
		case AFF_LPCM_48KHZ_S16_Stereo:
			return AL_FORMAT_STEREO16;
		default:
			return 0;
	};
}

//
static void AllocStream(ALStream &stream) {
	__ASSERT__(stream.buffers.empty());

	stream.streamer = {};
	stream.ref = InvalidAudioStreamRef;

	stream.free_buffer_count = 16;
	stream.get = stream.put = 0;

	stream.buffers.resize(stream.free_buffer_count);
	stream.buffers_timestamp.resize(stream.free_buffer_count);
	stream.buffers_format.resize(stream.free_buffer_count);

	stream.loop = false;

	alGenBuffers(numeric_cast<ALsizei>(stream.buffers.size()), stream.buffers.data());
}

static void FreeStream(ALStream &stream) {
	if (stream.ref != InvalidAudioStreamRef)
		stream.streamer.Close(stream.ref);

	stream.streamer = {};
	stream.ref = InvalidAudioStreamRef;

	stream.buffers.clear();
	stream.buffers_timestamp.clear();
	stream.buffers_format.clear();

	stream.free_buffer_count = 0;
	stream.get = stream.put = 0;

	stream.loop = false;

	if (!stream.buffers.empty())
		alDeleteBuffers(numeric_cast<ALsizei>(stream.buffers.size()), stream.buffers.data());
}

//
static bool UpdateSourceStream(SourceRef src_ref) {
	if (src_ref < 0 || src_ref >= max_source)
		return true;

	std::lock_guard<std::mutex> lock(al_mixer.lock);

	const auto src = al_mixer.sources[src_ref];
	auto &stream = al_mixer.streams[src_ref];

	// un-queue processed buffers
	ALint processed;
	__AL_CALL_RET(alGetSourcei(src, AL_BUFFERS_PROCESSED, &processed));
	if (processed < 0 || processed > numeric_cast<ALint>(stream.buffers.size())) {
		warn("Incoherent processed buffer count returned from the OpenAL back-end");
		return false;
	}

	while (processed--) {
		__AL_CALL_RET(alSourceUnqueueBuffers(src, 1, &stream.buffers[stream.get]));
		stream.get = (stream.get + 1) % stream.buffers.size();
		++stream.free_buffer_count;
	}

	// fill all available buffers
	while (stream.free_buffer_count > 0) {
		uint8_t *pcm_buffer;
		int pcm_size;
		AudioFrameFormat pcm_format;

		if (stream.streamer.GetFrame(stream.ref, (uintptr_t *)&pcm_buffer, &pcm_size, &pcm_format)) {
			stream.buffers_timestamp[stream.put] = stream.streamer.GetTimeStamp(stream.ref);
			stream.buffers_format[stream.put] = pcm_format;

			__AL_CALL(
				alBufferData(stream.buffers[stream.put], AFF_ALFormat(pcm_format), pcm_buffer, numeric_cast<ALsizei>(pcm_size), AFF_Frequency[pcm_format]));
			__AL_CALL(alSourceQueueBuffers(src, 1, &stream.buffers[stream.put]));

			stream.put = (stream.put + 1) % stream.buffers.size();
			--stream.free_buffer_count;
		} else {
			if (stream.streamer.IsEnded(stream.ref)) { // stream is EOF
				if (stream.loop) { // handle loop
					if (!stream.streamer.Seek(stream.ref, 0)) // FIXME seek to sample_start
						return false;
				} else {
					break; // EOF
				}
			} else {
				return false; // stream error
			}
		}
	}

	// handle stream starting or stalled
	ALint state;
	__AL_CALL_RET(alGetSourcei(src, AL_SOURCE_STATE, &state));

	if (state == AL_PAUSED)
		return true; // source paused

	if (state == AL_STOPPED)
		return false; // source stopped

	if (state != AL_PLAYING) {
		ALint buffer_queued;
		__AL_CALL_RET(alGetSourcei(src, AL_BUFFERS_QUEUED, &buffer_queued));

		if (buffer_queued > 0) {
			__AL_CALL_RET(alSourcePlay(src)); // stalled!
		} else {
			return false; // no buffer queue, source not playing -> we're done here
		}
	}

	return true;
}

static void UpdateAudio() {
	for (SourceRef src_ref = 0; src_ref < max_source; ++src_ref) {
		if (al_mixer.sources[src_ref] == InvalidSourceRef)
			continue;

		auto &stream = al_mixer.streams[src_ref];

		if (stream.ref != InvalidAudioStreamRef)
			if (!UpdateSourceStream(src_ref))
				StopSource(src_ref);
	}
}

//
bool IsAudioUp() { return al_mixer.device || al_mixer.context; }

bool AudioInit() {
	start_timer();

	if (IsAudioUp())
		return true;

	al_mixer.device = alcOpenDevice(nullptr);

	if (!al_mixer.device) {
		al_mixer.device = alcOpenDevice("Generic Software");

		if (!al_mixer.device) {
			warn("OpenAL initialization failed");
			return false;
		}
	}

	ALCint freq;
	alcGetIntegerv(al_mixer.device, ALC_FREQUENCY, 1, &freq);
	log(format("OpenAL ready on device '%1' - @%2hz").arg(alcGetString(al_mixer.device, ALC_DEVICE_SPECIFIER)).arg(freq));

	al_mixer.context = alcCreateContext(al_mixer.device, nullptr);
	alcMakeContextCurrent(al_mixer.context);
	__AL_CALL_RET(alGenSources(max_source, al_mixer.sources));

	al_mixer.update = run_periodic(UpdateAudio, audio_mixer_period); // start update thread
	return true;
}

void AudioShutdown() {
	cancel_periodic(al_mixer.update);

	StopAllSources();

	if (al_mixer.context) {
		__AL_CALL(alDeleteSources(max_source, al_mixer.sources));
		alcMakeContextCurrent(nullptr);
		alcDestroyContext(al_mixer.context);
		al_mixer.context = nullptr;
	}

	if (al_mixer.device) {
		alcCloseDevice(al_mixer.device);
		al_mixer.device = nullptr;
	}
}

void SetListener(const Mat4 &world, const Vec3 &velocity) {
	const auto T = GetT(world);
	__AL_CALL(alListener3f(AL_POSITION, T.x, T.y, T.z));

	const auto Z = GetZ(world), Y = GetY(world);
	ALfloat o[6] = {Z.x, Z.y, -Z.z, Y.x, Y.y, -Y.z};
	__AL_CALL(alListenerfv(AL_ORIENTATION, o));

	__AL_CALL(alListenerfv(AL_VELOCITY, &velocity.x));
}

//
static SourceRef GetFreeSourceRef() {
	for (SourceRef src_ref = 0; src_ref < max_source; ++src_ref) {
		if (al_mixer.streams[src_ref].ref != InvalidAudioStreamRef)
			continue; // streaming sources are locked FIXME unless stopped or what?

		ALint state;
		ALuint src = al_mixer.sources[src_ref];
		alGetSourcei(src, AL_SOURCE_STATE, &state);

		if (__AL_OK && (state == AL_INITIAL || state == AL_STOPPED)) {
			__AL_CALL(alSourcei(src, AL_BUFFER, 0));
			__AL_CALL(alSourceRewind(src));
			return src_ref;
		}
	}

	return InvalidSourceRef;
}

//
struct ALSound {
	std::vector<ALuint> buffers;
};

static std::vector<ALSound> sounds;

static SoundRef GetFreeSoundRef() {
	SoundRef snd_ref;
	for (snd_ref = 0; snd_ref < sounds.size(); ++snd_ref)
		if (sounds[snd_ref].buffers.empty())
			return snd_ref;

	sounds.resize(sounds.size() + 16);
	return snd_ref;
}

void UnloadSound(SoundRef snd_ref) {
	if (snd_ref < 0 || snd_ref >= sounds.size())
		return;

	auto &sound = sounds[snd_ref];
	alDeleteBuffers(numeric_cast<ALsizei>(sound.buffers.size()), sound.buffers.data());
	sound.buffers.clear();
}

//
static SoundRef LoadSound(IAudioStreamer streamer, const char *path) {
	const auto stream_ref = streamer.Open(path);
	if (stream_ref == InvalidAudioStreamRef)
		return InvalidSoundRef;

	const auto snd_ref = GetFreeSoundRef();
	auto &sound = sounds[snd_ref];

	uintptr_t pcm_buffer;
	int pcm_size;
	AudioFrameFormat pcm_format;

	while (streamer.GetFrame(stream_ref, &pcm_buffer, &pcm_size, &pcm_format)) {
		sound.buffers.push_back(AL_INVALID_VALUE);
		__AL_CALL(alGenBuffers(1, &sound.buffers.back()));
		__AL_CALL(alBufferData(
			sound.buffers.back(), AFF_ALFormat(pcm_format), (const ALvoid *)pcm_buffer, numeric_cast<ALsizei>(pcm_size), AFF_Frequency[pcm_format]));
	}

	streamer.Close(stream_ref);
	return snd_ref;
}

SoundRef LoadWAVSoundFile(const char *path) { return LoadSound(MakeWAVFileStreamer(), path); }
SoundRef LoadWAVSoundAsset(const char *name) { return LoadSound(MakeWAVAssetStreamer(), name); }

SoundRef LoadOGGSoundFile(const char *path) { return LoadSound(MakeOGGFileStreamer(), path); }
SoundRef LoadOGGSoundAsset(const char *name) { return LoadSound(MakeOGGAssetStreamer(), name); }

//
static void ALChannelSetState(ALint src, const StereoSourceState &state, bool stream) {
	__AL_CALL(alSourcef(src, AL_GAIN, state.volume));
	// alSourcei(sourceID, AL_SOURCE_RELATIVE, AL_FALSE);

	__AL_CALL(alDistanceModel(AL_NONE));
	__AL_CALL(alSourcei(src, AL_LOOPING, !stream && state.repeat == SR_Loop ? AL_TRUE : AL_FALSE)); // [EJ20190512] looping is handled manually with streaming

	__AL_CALL(alSource3f(src, AL_POSITION, state.panning, 0.f, sqrtf(1.f - state.panning * state.panning)));
}

static void ALChannelSetState(ALint src, const SpatializedSourceState &state, bool stream) {
	__AL_CALL(alSourcef(src, AL_GAIN, state.volume));
	// alSourcei(sourceID, AL_SOURCE_RELATIVE, AL_FALSE);

	__AL_CALL(alDistanceModel(AL_NONE));
	__AL_CALL(alSourcei(src, AL_LOOPING, !stream && state.repeat == SR_Loop ? AL_TRUE : AL_FALSE)); // [EJ20190512] looping is handled manually with streaming

	const auto T = GetT(state.mtx), Z = GetZ(state.mtx), Y = GetY(state.mtx);
	const ALfloat o[6] = {Z.x, Z.y, -Z.z, Y.x, Y.y, -Y.z};
	__AL_CALL(alSourcefv(src, AL_POSITION, &T.x));
	__AL_CALL(alSourcefv(src, AL_ORIENTATION, o));
	__AL_CALL(alSourcefv(src, AL_VELOCITY, &state.vel.x));
}

//
template <typename State> SourceRef Play(SoundRef snd_ref, const State &state) {
	std::lock_guard<std::mutex> lock(al_mixer.lock);

	if (snd_ref < 0 || snd_ref >= sounds.size())
		return InvalidSourceRef;

	const auto &snd = sounds[snd_ref];

	const auto src_ref = GetFreeSourceRef();
	if (src_ref == InvalidSourceRef)
		return InvalidSourceRef;

	const auto src = al_mixer.sources[src_ref];
	ALChannelSetState(src, state, false);
	__AL_CALL(alSourceQueueBuffers(src, numeric_cast<ALsizei>(snd.buffers.size()), snd.buffers.data()));
	__AL_CALL(alSourcePlay(src));

	return src_ref;
}

SourceRef PlayStereo(SoundRef snd_ref, const StereoSourceState &state) { return Play(snd_ref, state); }
SourceRef PlaySpatialized(SoundRef snd_ref, const SpatializedSourceState &state) { return Play(snd_ref, state); }

//
template <typename State> SourceRef Stream(IAudioStreamer streamer, const char *path, const State &state) {
	std::lock_guard<std::mutex> lock(al_mixer.lock);

	const auto src_ref = GetFreeSourceRef();
	if (src_ref == InvalidSourceRef)
		return InvalidSourceRef;

	ALChannelSetState(al_mixer.sources[src_ref], state, true);

	const auto stream_ref = streamer.Open(path);
	if (stream_ref == InvalidAudioStreamRef)
		return InvalidSourceRef;

	ALStream &stream = al_mixer.streams[src_ref];
	AllocStream(stream);
	stream.streamer = streamer;
	stream.ref = stream_ref;
	stream.loop = state.repeat == SR_Loop;

	return src_ref;
}

SourceRef StreamWAVFileStereo(const char *path, const StereoSourceState &state) { return Stream(MakeWAVFileStreamer(), path, state); }
SourceRef StreamWAVAssetStereo(const char *path, const StereoSourceState &state) { return Stream(MakeWAVAssetStreamer(), path, state); }
SourceRef StreamWAVFileSpatialized(const char *path, const SpatializedSourceState &state) { return Stream(MakeWAVFileStreamer(), path, state); }
SourceRef StreamWAVAssetSpatialized(const char *path, const SpatializedSourceState &state) { return Stream(MakeWAVAssetStreamer(), path, state); }

SourceRef StreamOGGFileStereo(const char *path, const StereoSourceState &state) { return Stream(MakeOGGFileStreamer(), path, state); }
SourceRef StreamOGGAssetStereo(const char *path, const StereoSourceState &state) { return Stream(MakeOGGAssetStreamer(), path, state); }
SourceRef StreamOGGFileSpatialized(const char *path, const SpatializedSourceState &state) { return Stream(MakeOGGFileStreamer(), path, state); }
SourceRef StreamOGGAssetSpatialized(const char *path, const SpatializedSourceState &state) { return Stream(MakeOGGAssetStreamer(), path, state); }

//
time_ns GetSourceTimecode(SourceRef src_ref) {
	if (src_ref < 0 || src_ref >= max_source)
		return 0;

	std::lock_guard<std::mutex> mixer_lock(al_mixer.lock);
	const auto src = al_mixer.sources[src_ref];
	auto &stream = al_mixer.streams[src_ref];

	ALint byte_offset;
	__AL_CALL(alGetSourcei(src, AL_BYTE_OFFSET, &byte_offset));

	if (stream.ref != InvalidAudioStreamRef) {
		std::lock_guard<std::mutex> stream_lock(stream.lock);

		const auto pcm_format = stream.buffers_format[stream.get];
		const auto current_pos_us =
			(int64_t(byte_offset) * 1000000LL) / (AFF_Frequency[pcm_format] * AFF_ChannelCount[pcm_format] * AFF_Resolution[pcm_format] / 8);

		stream.timestamp = stream.buffers_timestamp[stream.get] + time_from_us(current_pos_us);
	}
	return stream.timestamp;
}

time_ns GetSourceDuration(SourceRef src_ref) {
	std::lock_guard<std::mutex> mixer_lock(al_mixer.lock);
	if (src_ref < 0 || src_ref >= max_source)
		return 0;

	ALStream &stream = al_mixer.streams[src_ref];
	if (stream.ref == InvalidAudioStreamRef)
		return 0;

	std::lock_guard<std::mutex> stream_lock(stream.lock);
	return stream.streamer.GetDuration(stream.ref);
}

bool SetSourceTimecode(SourceRef src_ref, time_ns t) {
	std::lock_guard<std::mutex> mixer_lock(al_mixer.lock);
	if (src_ref < 0 || src_ref >= max_source)
		return false;

	ALStream &stream = al_mixer.streams[src_ref];
	if (stream.ref == InvalidAudioStreamRef)
		return 0;

	if (stream.streamer.Seek(stream.ref, t) == 0) {
		return false;
	}

	return true;
}

void SetSourceVolume(SourceRef src_ref, float volume) {
	if (src_ref < 0 || src_ref >= max_source)
		return;

	std::lock_guard<std::mutex> lock(al_mixer.lock);
	__AL_CALL(alSourcef(al_mixer.sources[src_ref], AL_GAIN, volume));
}

void SetSourcePanning(SourceRef src_ref, float panning) {
	if (src_ref < 0 || src_ref >= max_source)
		return;

	std::lock_guard<std::mutex> lock(al_mixer.lock);
	const auto src = al_mixer.sources[src_ref];

	__AL_CALL(alDistanceModel(AL_NONE));
	__AL_CALL(alSource3f(src, AL_POSITION, panning, 0.f, sqrtf(1.f - panning * panning)));
}

void SetSourceRepeat(SourceRef src_ref, SourceRepeat repeat) {
	if (src_ref < 0 || src_ref >= max_source)
		return;

	std::lock_guard<std::mutex> lock(al_mixer.lock);
	const auto src = al_mixer.sources[src_ref];

	auto &stream = al_mixer.streams[src_ref];

	if (stream.ref == InvalidAudioStreamRef)
		alSourcei(src, AL_LOOPING, repeat == SR_Loop ? AL_TRUE : AL_FALSE);
	else
		stream.loop = repeat == SR_Loop;
}

void SetSourceTransform(SourceRef src_ref, const Mat4 &world, const Vec3 &velocity) {
	if (src_ref < 0 || src_ref >= max_source)
		return;

	std::lock_guard<std::mutex> lock(al_mixer.lock);
	const auto src = al_mixer.sources[src_ref];

	const auto T = GetTranslation(world);
	const auto z = GetZ(world), y = GetY(world);

	ALfloat o[6] = {z.x, z.y, -z.z, y.x, y.y, -y.z};
	__AL_CALL(alSourcefv(src, AL_POSITION, &T.x));
	__AL_CALL(alSourcefv(src, AL_ORIENTATION, o));
	__AL_CALL(alSourcefv(src, AL_VELOCITY, &velocity.x));
}

//
SourceState GetSourceState(SourceRef src_ref) {
	if (src_ref < 0 || src_ref >= max_source)
		return SS_Invalid;

	std::lock_guard<std::mutex> lock(al_mixer.lock);
	const auto src = al_mixer.sources[src_ref];

	ALint state;
	__AL_CALL(alGetSourcei(src, AL_SOURCE_STATE, &state));

	if (state == AL_INITIAL)
		return SS_Initial;
	if (state == AL_PLAYING)
		return SS_Playing;
	if (state == AL_PAUSED)
		return SS_Paused;
	if (state == AL_STOPPED)
		return SS_Stopped;

	return SS_Invalid;
}

//
void PauseSource(SourceRef src_ref) {
	if (src_ref < 0 || src_ref >= max_source)
		return;

	std::lock_guard<std::mutex> lock(al_mixer.lock);
	const auto src = al_mixer.sources[src_ref];

	__AL_CALL(alSourcePause(src));
}

void StopSource(SourceRef src_ref) {
	if (src_ref < 0 || src_ref >= max_source)
		return;

	std::lock_guard<std::mutex> mixer_lock(al_mixer.lock);
	const auto src = al_mixer.sources[src_ref];

	__AL_CALL(alSourceStop(src));
	__AL_CALL(alSourcei(src, AL_BUFFER, 0));

	auto &stream = al_mixer.streams[src_ref];

	if (stream.ref != InvalidAudioStreamRef) {
		stream.timestamp = stream.streamer.GetDuration(stream.ref);
		FreeStream(stream);
	}
}

void StopAllSources() {
	for (SourceRef src_ref = 0; src_ref < max_source; ++src_ref)
		StopSource(src_ref);
}

} // namespace hg
