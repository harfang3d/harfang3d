#include "engine/video_stream_interface.h"

#include "foundation/log.h"
#include "foundation/format.h"

static uint8_t *g_buffer;
static int g_width = 1080;
static int g_height = 720;

VIDEO_STREAM_API int Startup() { 
	g_buffer = new uint8_t[g_width * g_height * 3];
	for (int j = 0; j < g_height; j++) {
		for (int i = 0; i < g_width; i++) {
			int offset = (i + j * g_width) * 3;
			g_buffer[offset] = g_buffer[offset + 1] = g_buffer[offset + 2] = (((i >> 4) ^ (j >> 4)) & 1) * 255;
		}
	}
	return 1;
}

VIDEO_STREAM_API void Shutdown() { delete[] g_buffer; }

VIDEO_STREAM_API VideoStreamHandle Open(const char *name) { 
	hg::log(hg::format("opening %1").arg(name).c_str());
	return 1;
}
VIDEO_STREAM_API int Play(VideoStreamHandle h) { return 1; }
VIDEO_STREAM_API int Pause(VideoStreamHandle h) { return 1; }
VIDEO_STREAM_API int Close(VideoStreamHandle h) { return 1; }

VIDEO_STREAM_API int Seek(VideoStreamHandle h, time_ns t) { return 1; }

VIDEO_STREAM_API time_ns GetDuration(VideoStreamHandle h) { return 120; }
VIDEO_STREAM_API time_ns GetTimeStamp(VideoStreamHandle h) { return 90; }
VIDEO_STREAM_API int IsEnded(VideoStreamHandle h) { return 0; }

VIDEO_STREAM_API int GetFrame(VideoStreamHandle h, const void **data, int *width, int *height, int *pitch, VideoFrameFormat *format) { 
	*data = g_buffer;
	*width = g_width;
	*height = g_height;
	*pitch = 0;
	*format = VFF_RGB24;
	return 1;
}
VIDEO_STREAM_API int FreeFrame(VideoStreamHandle h, int frame) { return 1; }
