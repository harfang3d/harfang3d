// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "wav_audio_stream.h"

#include "foundation/cext.h"
#include "foundation/file_rw_interface.h"
#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/math.h"
#include "foundation/rw_interface.h"

#include "engine/assets_rw_interface.h"

#include <cstring>
#include <vector>

namespace hg {

struct WAVStream {
	const Reader *reader{};
	const ReadProvider *provider{};
	Handle handle;

	AudioFrameFormat fmt{AFF_Unsupported};
	std::vector<uint8_t> frame;

	size_t data_offset{}; // start of data chunk
	size_t data_size{}; // size of data chunk
};

static std::vector<WAVStream> streams;

static bool IsValid(AudioStreamRef ref) {
	if (ref == InvalidAudioStreamRef)
		return false;
	if (ref >= streams.size())
		return false;

	const auto &stream = streams[ref];
	return stream.reader && stream.reader->is_valid(stream.handle);
}

//
static int WavAudioStreamStartup() { return 1; }

static void WavAudioStreamShutdown() {
	for (auto &stream : streams)
		if (stream.provider)
			stream.provider->close(stream.handle);
	streams.clear();
}

//
static bool OpenWav(WAVStream &stream) {
	stream.fmt = AFF_Unsupported;

	__ASSERT__(stream.reader);

	const auto &reader = *stream.reader;
	const auto handle = stream.handle;

	if (!reader.is_valid(handle))
		return false;

	// verify format
	uint8_t header[4];
	if (reader.read(handle, header, 4) != 4)
		return false;

	if (memcmp(header, "RIFF", 4)) {
		debug("Not a RIFF file");
		return false;
	}

	const auto filesize = Read<uint32_t>(reader, handle); // minus header

	if (reader.read(handle, header, 4) != 4)
		return false;

	if (memcmp(header, "WAVE", 4)) {
		debug("Not a WAVE file");
		return false;
	}

	while (!reader.is_eof(handle)) {
		uint8_t chunk[4];
		if (reader.read(handle, chunk, 4) != 4)
			return false;

		const auto size = Read<uint32_t>(reader, handle);
		if (!size)
			return false;

		if (!memcmp(chunk, "fmt ", 4)) {
			const auto format_tag = Read<uint16_t>(reader, handle);

			if (format_tag != 1) {
				warn("Ignoring non-PCM data");
				continue; // ignore non PCM data
			}

			const auto channels = Read<uint16_t>(reader, handle);

			if (channels > 2) {
				warn("Only mono/stereo WAV supported");
				continue; // only mono or stereo
			}

			const auto frequency = Read<uint32_t>(reader, handle);

			reader.seek(handle, 6, SM_Current);
			const auto resolution = Read<uint16_t>(reader, handle);

			if (resolution != 16) {
				warn("Only 16 bit WAV supported");
				continue; // only 16 bits per sample are supported
			}

			if (frequency == 44100) {
				stream.fmt = channels == 2 ? AFF_LPCM_44KHZ_S16_Stereo : AFF_LPCM_44KHZ_S16_Mono;
			} else if (frequency == 48000) {
				stream.fmt = channels == 2 ? AFF_LPCM_48KHZ_S16_Stereo : AFF_LPCM_48KHZ_S16_Mono;
			} else {
				warn(format("Unsupported WAV frequency %1").arg(frequency));
				continue; // unsupported frequency
			}
		} else if (!memcmp(chunk, "data", 4)) {
			stream.data_offset = reader.tell(handle);
			stream.data_size = size;
			stream.frame.reserve(16384);
		} else {
			reader.seek(handle, size, SM_Current);
		}

		if (stream.fmt != AFF_Unsupported && stream.data_offset != 0)
			break; // we only support a single data chunk atm
	}
	return true;
}

static AudioStreamRef WavAudioStreamOpen(const ReadProvider *read_provider, const Reader *reader, const char *name) {
	WAVStream wav;

	wav.reader = reader;
	wav.provider = read_provider;
	wav.handle = read_provider->open(name, false);

	if (!OpenWav(wav))
		return InvalidAudioStreamRef;

	for (AudioStreamRef ref = 0; ref < streams.size(); ++ref)
		if (!IsValid(ref)) {
			streams[ref] = wav;
			return ref;
		}

	streams.push_back(std::move(wav));
	return numeric_cast<AudioStreamRef>(streams.size()) - 1;
}

static AudioStreamRef WavAudioStreamOpenFile(const char *path) { return WavAudioStreamOpen(&g_file_read_provider, &g_file_reader, path); }
static AudioStreamRef WavAudioStreamOpenAsset(const char *name) { return WavAudioStreamOpen(&g_assets_read_provider, &g_assets_reader, name); }

static int WavAudioStreamClose(AudioStreamRef ref) {
	if (!IsValid(ref))
		return 0;

	auto &stream = streams[ref];

	stream.provider->close(stream.handle);

	stream.data_offset = 0;
	stream.data_size = 0;
	stream.frame.clear();

	return 1;
}

static int WavAudioStreamSeek(AudioStreamRef ref, AudioTimestamp t) {
	if (!IsValid(ref))
		return 0;

	const auto &stream = streams[ref];

	const size_t seek_offset = TimestampToByte(stream.fmt, t);
	if (seek_offset > stream.data_size)
		return 0; // jumping out of stream

	return stream.reader->seek(stream.handle, stream.data_offset + seek_offset, SM_Start);
}

static AudioTimestamp WavAudioStreamGetDuration(AudioStreamRef ref) {
	if (!IsValid(ref))
		return 0;

	const auto &stream = streams[ref];
	return ByteToTimestamp(stream.fmt, stream.data_size);
}

static AudioTimestamp WavAudioStreamGetTimeStamp(AudioStreamRef ref) {
	if (!IsValid(ref))
		return 0;

	const auto &stream = streams[ref];
	return ByteToTimestamp(stream.fmt, stream.reader->tell(stream.handle) - stream.data_offset);
}

static int WavAudioStreamIsEnded(AudioStreamRef ref) {
	if (!IsValid(ref))
		return 1;

	const auto &stream = streams[ref];
	return (stream.reader->tell(stream.handle) - stream.data_offset) == stream.data_size;
}

static int WavAudioStreamGetFrame(AudioStreamRef ref, uintptr_t *data, int *size, AudioFrameFormat *format) {
	if (!IsValid(ref))
		return 0;

	auto &stream = streams[ref];

	const auto data_cursor = stream.reader->tell(stream.handle) - stream.data_offset; // position in data chunk
	const auto data_left = stream.data_size - data_cursor; // data left in chunk
	if (data_left == 0) {
		return 0;
	}

	const auto data_to_read = Min(data_left, stream.frame.capacity()); // amount of data to read
	if (stream.reader->read(stream.handle, stream.frame.data(), data_to_read) != data_to_read)
		return 0;

	*data = (uintptr_t)stream.frame.data();
	*size = numeric_cast<int>(data_to_read);
	*format = stream.fmt;
	return 1;
}

static IAudioStreamer InitializeWAVAudioStreamCommon(AudioStreamRef (*open)(const char *path)) {
	IAudioStreamer streamer;

	streamer.Startup = WavAudioStreamStartup;
	streamer.Shutdown = WavAudioStreamShutdown;

	streamer.Open = open;
	streamer.Close = WavAudioStreamClose;

	streamer.Seek = WavAudioStreamSeek;

	streamer.GetDuration = WavAudioStreamGetDuration;
	streamer.GetTimeStamp = WavAudioStreamGetTimeStamp;
	streamer.IsEnded = WavAudioStreamIsEnded;

	streamer.GetFrame = WavAudioStreamGetFrame;

	return streamer;
}

IAudioStreamer MakeWAVFileStreamer() { return InitializeWAVAudioStreamCommon(WavAudioStreamOpenFile); }
IAudioStreamer MakeWAVAssetStreamer() { return InitializeWAVAudioStreamCommon(WavAudioStreamOpenAsset); }

} // namespace hg
