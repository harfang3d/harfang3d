Advance the rendering backend to the next frame, execute all queued rendering commands.
This function returns the backend current frame.

The frame counter is used by asynchronous functions such as [CaptureTexture]. You must wait for the frame counter to reach or exceed the value returned by an asynchronous function before accessing its result.