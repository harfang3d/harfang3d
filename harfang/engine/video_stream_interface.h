// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#ifndef VIDEO_STREAM_INTERFACE
#define VIDEO_STREAM_INTERFACE

#include "stdint.h"

enum VideoFrameFormat {
	YUV422,
	EXTERNAL_OES, //< GLint id of an external GL OES texture
};

#define VIDEOSTREAM int
#define TIME_NS int64_t

struct IVideoStream {
	int (*Startup)();
	void (*Shutdown)();

	VIDEOSTREAM (*Open)(const char *name);
	int (*Play)(VIDEOSTREAM h);
	int (*Pause)(VIDEOSTREAM h);
	int (*Close)(VIDEOSTREAM h);

	int (*Seek)(VIDEOSTREAM h, TIME_NS t);

	TIME_NS (*GetDuration)(VIDEOSTREAM h);
	TIME_NS (*GetTimeStamp)(VIDEOSTREAM h);
	int (*IsEnded)(VIDEOSTREAM h);

	int (*GetFrame)(VIDEOSTREAM h, const void **data, int *width, int *height, int *pitch, VideoFrameFormat *format);
	int (*FreeFrame)(VIDEOSTREAM h, int frame);
};

#endif // VIDEO_STREAM_INTERFACE
