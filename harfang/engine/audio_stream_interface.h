// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#ifndef AUDIO_STREAM_INTERFACE
#define AUDIO_STREAM_INTERFACE

#include <stdint.h>
#include <stddef.h>

//
enum AudioFrameFormat {
	AFF_Unsupported,
	AFF_LPCM_44KHZ_S16_Mono,
	AFF_LPCM_48KHZ_S16_Mono,
	AFF_LPCM_44KHZ_S16_Stereo,
	AFF_LPCM_48KHZ_S16_Stereo,
	AFF_Count,
};

static const size_t AFF_Frequency[AFF_Count] = {0, 44100, 48000, 44100, 48000};
static const size_t AFF_Resolution[AFF_Count] = {0, 16, 16, 16, 16};
static const size_t AFF_ChannelCount[AFF_Count] = {0, 1, 1, 2, 2};

//
typedef int64_t AudioTimestamp;

static AudioTimestamp ByteToTimestamp(AudioFrameFormat fmt, size_t bytes) {
	return (bytes * 8LL * 1000000000LL) / (AFF_ChannelCount[fmt] * AFF_Resolution[fmt] * AFF_Frequency[fmt]);
}

static size_t TimestampToByte(AudioFrameFormat fmt, AudioTimestamp t) {
	return (t * AFF_ChannelCount[fmt] * AFF_Resolution[fmt] * AFF_Frequency[fmt]) / (8LL * 1000000000LL);
}

//
typedef uint32_t AudioStreamRef;
static const AudioStreamRef InvalidAudioStreamRef = 0xffffffff;

struct IAudioStreamer {
	int (*Startup)();
	void (*Shutdown)();

	AudioStreamRef (*Open)(const char *name);
	int (*Close)(AudioStreamRef ref);

	int (*Seek)(AudioStreamRef ref, AudioTimestamp t);

	AudioTimestamp (*GetDuration)(AudioStreamRef ref);
	AudioTimestamp (*GetTimeStamp)(AudioStreamRef ref);
	int (*IsEnded)(AudioStreamRef ref);

	int (*GetFrame)(AudioStreamRef ref, uintptr_t *data, int *size, AudioFrameFormat *format);
};

#endif // AUDIO_STREAM_INTERFACE
