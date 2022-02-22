// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#ifndef VIDEO_STREAM
#define VIDEO_STREAM

#include "video_stream_interface.h"

#include "platform/shared_library.h"

#include "engine/render_pipeline.h"

namespace hg {

IVideoStreamer MakeVideoStreamer(const SharedLib &h);
IVideoStreamer MakeVideoStreamer(const char *module_path);

bool IsValid(IVideoStreamer &streamer);

bool UpdateTexture(IVideoStreamer &streamer, VideoStreamHandle &handle, Texture &texture, hg::iVec2 &size, bgfx::TextureFormat::Enum &format, bool destroy=false);

} // namespace hg

#endif // VIDEO_STREAM
