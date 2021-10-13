Capture a texture content to a [Picture]. Return the frame counter at which the capture will be complete.

A [Picture] object can be accessed by the CPU.

This function is asynchronous and its result will not be available until the returned frame counter is equal or greater to the frame counter returned by [Frame].