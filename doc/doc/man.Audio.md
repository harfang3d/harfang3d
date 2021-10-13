.title Playing Audio

The audio system can play back two kind of audio resource, sound and stream.

* A sound resource is played from memory, this is usually the most efficient resource to play. However, since it must fully be stored in memory longer audio samples are best streamed.
* A stream resource is read from storage in small increments as it is being played back by the audio backend. While it consumes less memory a stream is usually more performance taxing as it requires constant access to storage and CPU to decode.

It is recommended to use sounds for short, ponctual effects and streams for background music or ambience.

## Playing Sound and Stream

The audio system manages a finite number of audio sources. A source can play a single audio sound or stream at a time. When initiating playback a free source is selected, if none is available the request fails and [SRC_Invalid] is returned.

To playback a sound, load it first using [LoadWAVSoundFile] for example, then pass the returned resource reference to [PlayStereo] or [PlaySpatialized], this in turn will return a reference to the audio source playback started on.

To start streaming a sound file, call a streaming function such as [StreamWAVFileStereo].

Each source has a set of independently configurable properties that can be specified using the [StereoSourceState] and [SpatializedSourceState] classes.

When using spatialization all transformations are specified in world coordinates. Use [SetListener] to change the listener position in space.