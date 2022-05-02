// HARFANG(R) Copyright (C) 2022 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#ifndef VIDEO_STREAM_INTERFACE
#define VIDEO_STREAM_INTERFACE

#include <stdint.h>

#ifndef VIDEO_STREAM_API
#	ifndef VIDEO_STREAM_API_PREFIX
#		ifdef __cplusplus
#			define VIDEO_STREAM_API_PREFIX extern "C"
#		else
#			define VIDEO_STREAM_API_PREFIX
#		endif
#	endif
#	if WIN32
#		define VIDEO_STREAM_API VIDEO_STREAM_API_PREFIX  __declspec(dllexport)
#	else /* POSIX */
#		define VIDEO_STREAM_API VIDEO_STREAM_API_PREFIX __attribute__((visibility("default")))
#	endif
#endif

/// Video frame formats.
enum VideoFrameFormat { 
	VFF_UNKNOWN = -1,		///< Unknown/Unsupported 
	VFF_YUV422 = 0,			///< 16 bits per pixels. 2 channels per pixel. 8 bits per channel. U and V are shared between two pixels.
	VFF_RGB24,				///< 24 bits per pixels. 8 bits per channel (Red, Green, Blue).
	VFF_RGBA32,				///< 32 bits per pixels. 8 bits per channel (Red, Green, Blue, Alpha).
};

/// Opaque handle for video stream.
/// 0 is considered an invalid handle.
typedef intptr_t VideoStreamHandle;
/// Time value (in nanoseconds).
typedef int64_t time_ns;

/// Video streamer interface.
struct IVideoStreamer {
	/// Initialize video streamer.
	/// For example, it can be driver or hardware setup.
	int (*Startup)();
	/// Release resources used by the video streamer.
	void (*Shutdown)();
	/// Open a new video stream.
	/// @param [in] name Video source. It can be a file path, a device name or a stream url.
	/// @return Video stream handle.
	VideoStreamHandle (*Open)(const char *name);
	/// Start video stream.
	/// @param [in] h Video stream handle.
	/// @return 1 if the stream was succesfuly started or 0 if an error occured.
	int (*Play)(VideoStreamHandle h);
	/// Toggle pause.
	/// @param [in] h Video stream handle.
	/// @return 1 if the stream was succesfuly started or 0 if an error occured.
	int (*Pause)(VideoStreamHandle h);
	/// Close video stream.
	/// @param [in] h Video stream handle.
	/// @return 1 if the stream was succesfuly started or 0 if an error occured.
	int (*Close)(VideoStreamHandle h);
	/// Jump video stream to a specific timestamp.
	/// @param [in] h Video stream handle.
	/// @param [in] t Timestamp.
	/// @return 1 if the stream was succesfuly started or 0 if an error occured.
	int (*Seek)(VideoStreamHandle h, time_ns t);
	/// Return video stream duration.
	/// Note that it is sometimes impossible to get the duration of some video streams.
	/// For instance, network video stream.
	/// @param [in] h Video stream handle.
	/// @return Video stream duration.
	time_ns (*GetDuration)(VideoStreamHandle h);
	/// Get current video timestamp.
	/// @param [in] h Video stream handle.
	/// @return Current timestamp.
	time_ns (*GetTimeStamp)(VideoStreamHandle h);
	/// Is the video stream still running?
	/// @param [in] h Video stream handle.
	/// @return 0 if the video stream is still active, 1 if it is stopped.
	int (*IsEnded)(VideoStreamHandle h);
	/// Retrieve last video frame.
	/// Do not call free or delete on the returned pointer. Call FreeFrame with the id returned by this fonction instead.
	/// The same frame may be returned more than once. This is usually the case when the video framerate is lower than the
	/// replay frequency. 
	/// @param [in] h Video stream handle.
	/// @param [out] data Pointer to the video frame buffer.
	/// @param [out] width Horizontal size of the video frame.
	/// @param [out] height Vertical size of the video frame.
	/// @param [out] pitch Number of bytes
	/// @param [out] format Format of the video frame.
	/// @return Frame id or 0 if an error occured.
	int (*GetFrame)(VideoStreamHandle h, const void **data, int *width, int *height, int *pitch, VideoFrameFormat *format);
	/// Release video frame.
	/// @param [in] h Video stream handle.
	/// @param [in] frame Id returned by GetFrame.
	/// @return 1 upon success or 0 if an error occured.
	int (*FreeFrame)(VideoStreamHandle h, int frame);
};

#endif // VIDEO_STREAM_INTERFACE
