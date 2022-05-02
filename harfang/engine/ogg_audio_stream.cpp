// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include <cstring>
#include <vector>

#include "ogg_audio_stream.h"

#include "foundation/cext.h"
#include "foundation/file_rw_interface.h"
#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/math.h"
#include "foundation/rw_interface.h"

#include "engine/assets_rw_interface.h"

#include <stb_vorbis/stb_vorbis.c>

namespace hg {

struct OGGStream {
	const Reader *reader{};
	const ReadProvider *provider{};
	Handle handle;

	AudioFrameFormat fmt{AFF_Unsupported};

	stb_vorbis *vorbis{NULL};
	AudioStreamRef *ref{NULL};

	std::vector<uint16_t> buffer;
};

static std::vector<OGGStream> g_streams;

static bool IsValid(AudioStreamRef ref) {
	if (ref == InvalidAudioStreamRef) {
		return false;
	}
	if (ref >= g_streams.size()) {
		return false;
	}
	const OGGStream &stream = g_streams[ref];
	return stream.reader && stream.reader->is_valid(stream.handle);
}

static void ogg_io_close(void *user) {
	AudioStreamRef *ref = reinterpret_cast<AudioStreamRef *>(user);
	if (!(ref && IsValid(*ref))) {
		return;
	}
	OGGStream &stream = g_streams[*ref];
	stream.provider->close(stream.handle);
}

static int ogg_io_read(void *user, char *data, int size) {
	AudioStreamRef *ref = reinterpret_cast<AudioStreamRef *>(user);
	if (!(ref && IsValid(*ref))) {
		return 0;
	}
	OGGStream &stream = g_streams[*ref];
	return stream.reader->read(stream.handle, data, size);
}

static int ogg_io_seek(void *user, int offset, enum STBVorbisIOSeekMode mode) {
	AudioStreamRef *ref = reinterpret_cast<AudioStreamRef *>(user);
	if (!(ref && IsValid(*ref))) {
		return 1;
	}
	OGGStream &stream = g_streams[*ref];
	SeekMode whence = SM_Start;
	switch (mode) {
		case VORBIS_IO_Start:
			whence = SM_Start;
			break;
		case VORBIS_IO_Current:
			whence = SM_Current;
			break;
		case VORBIS_IO_End:
			whence = SM_End;
			break;
	}
	return stream.reader->seek(stream.handle, offset, whence) ? 0 : 1;
}

static int ogg_io_tell(void *user) {
	AudioStreamRef *ref = reinterpret_cast<AudioStreamRef *>(user);
	if (!(ref && IsValid(*ref))) {
		return 0;
	}
	OGGStream &stream = g_streams[*ref];
	return stream.reader->tell(stream.handle);
}

static int ogg_io_eof(void *user) {
	AudioStreamRef *ref = reinterpret_cast<AudioStreamRef *>(user);
	if (!(ref && IsValid(*ref))) {
		return 1;
	}
	OGGStream &stream = g_streams[*ref];
	return stream.reader->is_eof(stream.handle);
}

static int OGGAudioStreamStartup() { return 1; }

static void OGGAudioStreamShutdown() {
	for (auto &stream : g_streams) {
		stb_vorbis_close(stream.vorbis);
		if (stream.ref) {
			delete stream.ref;
		}
	}
	g_streams.clear();
}

static bool OpenOGG(OGGStream &stream) {
	const auto &reader = *stream.reader;
	const auto handle = stream.handle;

	if (!reader.is_valid(handle))
		return false;

	stbv_io_callbacks callbacks = {ogg_io_close, ogg_io_read, ogg_io_seek, ogg_io_tell, ogg_io_eof};

	int error = VORBIS__no_error;
	stream.vorbis = stb_vorbis_open_from_callbacks(&callbacks, stream.ref, &error, NULL);
	if (error != VORBIS__no_error) {
		return false;
	}

	stb_vorbis_info info = stb_vorbis_get_info(stream.vorbis);

	stream.fmt = AFF_Unsupported;
	if (info.sample_rate == 48000) {
		stream.fmt = (info.channels == 2) ? AFF_LPCM_48KHZ_S16_Stereo : AFF_LPCM_48KHZ_S16_Mono;
	} else if (info.sample_rate == 44100) {
		stream.fmt = (info.channels == 2) ? AFF_LPCM_44KHZ_S16_Stereo : AFF_LPCM_44KHZ_S16_Mono;
	} else {
		warn(format("Unsupported OGG sample rate %1 or channel count %2").arg(info.sample_rate).arg(info.channels));
		return false;
	}
	stream.buffer.resize(16384);
	return true;
}

static AudioStreamRef OGGAudioStreamOpen(const ReadProvider *read_provider, const Reader *reader, const char *name) {
	// find a suitable spot
	AudioStreamRef ref = g_streams.size();
	for (ref = 0; ref < g_streams.size(); ++ref) {
		if (!IsValid(ref)) {
			break;
		}
	}

	if (ref == g_streams.size()) {
		g_streams.resize(ref + 1);
	}

	OGGStream &ogg = g_streams[ref];

	ogg.reader = reader;
	ogg.provider = read_provider;
	ogg.handle = read_provider->open(name, false);
	ogg.ref = new AudioStreamRef(ref);

	if (!OpenOGG(ogg)) {
		if (ogg.provider) {
			ogg.provider->close(ogg.handle);
		}
		delete ogg.ref;
		return InvalidAudioStreamRef;
	}

	return ref;
}

static AudioStreamRef OGGAudioStreamOpenFile(const char *path) { return OGGAudioStreamOpen(&g_file_read_provider, &g_file_reader, path); }
static AudioStreamRef OGGAudioStreamOpenAsset(const char *name) { return OGGAudioStreamOpen(&g_assets_read_provider, &g_assets_reader, name); }

static int OGGAudioStreamClose(AudioStreamRef ref) {
	if (!IsValid(ref))
		return 0;

	auto &stream = g_streams[ref];
	stb_vorbis_close(stream.vorbis);
	stream.vorbis = NULL;
	if (stream.ref) {
		delete stream.ref;
		stream.ref = nullptr;
	}
	return 1;
}

static AudioTimestamp OGGAudioStreamGetDuration(AudioStreamRef ref) {
	if (!IsValid(ref))
		return 0;
	const auto &stream = g_streams[ref];
	return AudioTimestamp(Floor(double(stb_vorbis_stream_length_in_seconds(stream.vorbis)) * 1000000000.0));
}

static int OGGAudioStreamSeek(AudioStreamRef ref, AudioTimestamp t) {
	if (!IsValid(ref))
		return 0;

	const auto &stream = g_streams[ref];
	stb_vorbis_info info = stb_vorbis_get_info(stream.vorbis);
	return stb_vorbis_seek(stream.vorbis, t * info.sample_rate / 1000000000LL);
}

static AudioTimestamp OGGAudioStreamGetTimeStamp(AudioStreamRef ref) {
	if (!IsValid(ref))
		return 0;
	const auto &stream = g_streams[ref];
	stb_vorbis_info info = stb_vorbis_get_info(stream.vorbis);
	return stb_vorbis_get_sample_offset(stream.vorbis) * 1000000000LL / info.sample_rate;
}

static int OGGAudioStreamIsEnded(AudioStreamRef ref) {
	if (!IsValid(ref))
		return 1;

	const auto &stream = g_streams[ref];
	int loc = stb_vorbis_get_sample_offset(stream.vorbis);
	int n = stb_vorbis_stream_length_in_samples(stream.vorbis);
	return (loc < n);
}

static int OGGAudioStreamGetFrame(AudioStreamRef ref, uintptr_t *data, int *size, AudioFrameFormat *format) {
	if (!IsValid(ref))
		return 0;

	auto &stream = g_streams[ref];
	stb_vorbis_info info = stb_vorbis_get_info(stream.vorbis);
	int n = stb_vorbis_get_frame_short_interleaved(stream.vorbis, info.channels, (short *)&stream.buffer[0], stream.buffer.size());
	int error = stb_vorbis_get_error(stream.vorbis);
	if (n == 0) {
		return 0;
	}

	*data = (uintptr_t)stream.buffer.data();
	*size = n * info.channels * 2;
	*format = stream.fmt;
	return 1;
}

static IAudioStreamer InitializeOGGAudioStreamCommon(AudioStreamRef (*open)(const char *path)) {
	IAudioStreamer streamer;

	streamer.Startup = OGGAudioStreamStartup;
	streamer.Shutdown = OGGAudioStreamShutdown;

	streamer.Open = open;
	streamer.Close = OGGAudioStreamClose;

	streamer.Seek = OGGAudioStreamSeek;

	streamer.GetDuration = OGGAudioStreamGetDuration;
	streamer.GetTimeStamp = OGGAudioStreamGetTimeStamp;
	streamer.IsEnded = OGGAudioStreamIsEnded;

	streamer.GetFrame = OGGAudioStreamGetFrame;

	return streamer;
}

IAudioStreamer MakeOGGFileStreamer() { return InitializeOGGAudioStreamCommon(OGGAudioStreamOpenFile); }
IAudioStreamer MakeOGGAssetStreamer() { return InitializeOGGAudioStreamCommon(OGGAudioStreamOpenAsset); }

} // namespace hg
