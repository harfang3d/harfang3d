// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "video_stream.h"

#include <string.h>

#include <map>

#include "foundation/log.h"
#include "foundation/format.h"

namespace hg {

enum VideoStreamBaseFunction {
	VS_BF_Startup = 0,
	VS_BF_Shutdown,
	VS_BF_Open,
	VS_BF_Play,
	VS_BF_Pause,
	VS_BF_Close,
	VS_BF_Seek,
	VS_BF_GetDuration,
	VS_BF_GetTimeStamp,
	VS_BF_IsEnded,
	VS_BF_GetFrame,
	VS_BF_FreeFrame,
	VS_BF_Count
};

IVideoStreamer MakeVideoStreamer(const SharedLib &h) {
	static const char *base_function_names[VS_BF_Count] = { 
		"Startup",
		"Shutdown",
		"Open",
		"Play",
		"Pause",
		"Close",
		"Seek",
		"GetDuration",
		"GetTimeStamp",
		"IsEnded",
		"GetFrame",
		"FreeFrame"
	};
	void *base_function_handles[VS_BF_Count];

 	IVideoStreamer streamer;
	memset(&streamer, 0, sizeof(IVideoStreamer));

	for (int i = 0; i<VS_BF_Count; i++) {
		base_function_handles[i] = GetFunctionPointer(h, base_function_names[i]);
		if (!base_function_handles[i]) {
			warn(format("failed to load %1 video stream function").arg(base_function_names[i]));
			return streamer;
		}
	}

	streamer.Startup = (int (*)())base_function_handles[VS_BF_Startup];
	streamer.Shutdown = (void (*)())base_function_handles[VS_BF_Shutdown];
	streamer.Open = (VideoStreamHandle(*)(const char *))base_function_handles[VS_BF_Open];
	streamer.Play = (int (*)(VideoStreamHandle))base_function_handles[VS_BF_Play];
	streamer.Pause = (int (*)(VideoStreamHandle))base_function_handles[VS_BF_Pause];
	streamer.Close = (int (*)(VideoStreamHandle))base_function_handles[VS_BF_Close];
	streamer.Seek = (int (*)(VideoStreamHandle, time_ns))base_function_handles[VS_BF_Seek];
	streamer.GetDuration = (time_ns(*)(VideoStreamHandle))base_function_handles[VS_BF_GetDuration];
	streamer.GetTimeStamp = (time_ns(*)(VideoStreamHandle))base_function_handles[VS_BF_GetTimeStamp];
	streamer.IsEnded = (int (*)(VideoStreamHandle))base_function_handles[VS_BF_IsEnded];
	streamer.GetFrame = (int (*)(VideoStreamHandle, const void **, int *, int *, int *, VideoFrameFormat *))base_function_handles[VS_BF_GetFrame];
	streamer.FreeFrame = (int (*)(VideoStreamHandle, int))base_function_handles[VS_BF_FreeFrame];

	return streamer;
}

static std::map<std::string, SharedLib> g_module_cache;

IVideoStreamer MakeVideoStreamer(const char *module_path) {
	auto it = g_module_cache.find(module_path);
	SharedLib module;
	if(it == g_module_cache.end()) {
		module = LoadSharedLibrary(module_path);
		g_module_cache[module_path] = module;
	} else {
		module = it->second;
	}

	if(!module) {
		IVideoStreamer empty;
		memset(&empty, 0, sizeof(IVideoStreamer));
		return empty;
	}

	return MakeVideoStreamer(module);
}

bool IsValid(IVideoStreamer &streamer) {
	return (
		   streamer.Startup
		&& streamer.Shutdown
		&& streamer.Open
		&& streamer.Play
		&& streamer.Pause
		&& streamer.Close
		&& streamer.Seek
		&& streamer.GetDuration
		&& streamer.GetTimeStamp
		&& streamer.IsEnded
		&& streamer.GetFrame
		&& streamer.FreeFrame
	);
}

struct VideoStreamMemHelper {
	IVideoStreamer streamer;
	VideoStreamHandle handle;
	int id;
};

static void ReleaseFrame(void *ptr, void *userData) {
	VideoStreamMemHelper *helper = reinterpret_cast<VideoStreamMemHelper*>(userData);
	helper->streamer.FreeFrame(helper->handle, helper->id);
	delete helper;
}

static inline bgfx::TextureFormat::Enum VideoFrameTextureFormat(VideoFrameFormat fmt) {
	switch (fmt) {
		case VFF_YUV422:
			return bgfx::TextureFormat::RG8;
		case VFF_RGB24:
			return bgfx::TextureFormat::RGB8;
		case VFF_RGBA32:
			return bgfx::TextureFormat::RGBA8;
		default:
			return bgfx::TextureFormat::Unknown;
	}
}

bool UpdateTexture(IVideoStreamer &streamer, VideoStreamHandle &handle, hg::Texture &texture, hg::iVec2 &size, bgfx::TextureFormat::Enum &texfmt, bool destroy) {
	if (!IsValid(streamer)) {
		return false;
	}

	const void *data;
	int width, height, pitch;
	VideoFrameFormat video_fmt;

	int id = streamer.GetFrame(handle, &data, &width, &height, &pitch, &video_fmt);
	if (id == 0) {
		warn("Unable to get video frame");
		return false;
	}

	bgfx::TextureFormat::Enum fmt = VideoFrameTextureFormat(video_fmt);
	if (fmt == bgfx::TextureFormat::Unknown) {
		warn(format("Unsupported video frame format (%1)").arg(video_fmt));
		streamer.FreeFrame(handle, id);
		return false;
	}

	bgfx::TextureInfo info;
	bgfx::calcTextureSize(info, width, height, 1, false, false, 1, fmt);

	uint64_t flags = BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE;

	// create a new texture if the size or format does not match.
	if (bgfx::isValid(texture.handle) && ((width != size.x) || (height != size.y) || (fmt != texfmt))) {
		if (destroy) {
			bgfx::destroy(texture.handle);
		}
		texture = BGFX_INVALID_HANDLE;
	}

	if(!bgfx::isValid(texture.handle)) {
		texture = CreateTexture(width, height, hg::format("VideoStream texture %1").arg(id).c_str(), flags, fmt);
		if(!bgfx::isValid(texture.handle)) {
			streamer.FreeFrame(handle, id);
			return false;
		}
	}
	
	size.x = width;
	size.y = height;
	texfmt = fmt;

	VideoStreamMemHelper *helper = new VideoStreamMemHelper;
	helper->streamer = streamer;
	helper->id = id;
	helper->handle = handle;
	pitch = 0;
	const auto *mem = bgfx::makeRef(data, pitch * height, ReleaseFrame, helper);
	bgfx::updateTexture2D(texture.handle, 0, 0, 0, 0, width, height, mem);
	return true;
}


} // namespace hg
