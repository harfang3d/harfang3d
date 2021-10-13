.title Feature List

### General

* Cross-platform
* Lightweight code base
* Small memory footprint

### Interoperability

* Command line FBX converter (extensive support including geometry skinning)
* Native support for many image file formats (PSD, JPG, PNG, TGA, ...)
* Native support for many sound file formats (OGG, WAV, AIFF, XM, S3M, ...)

### Framework

* Flexible file system abstraction (local, archive, network components with chaining)
* HID abstraction to access machine devices (DirectInput, XInput, ...)
* Data format abstraction (XML/JSON/Binary back-ends)
* 2D Vector graphics engine based on the Anti Grain Geometry library

### Multi-threading

* Task-based multi-threading
* Asynchronous interfaces to control key API objects from any thread or language

### Audio

* Audio API abstraction layer (OpenAL back-end)
* Any supported audio format can be streamed or loaded as a sound
* 3D audio support

### Rendering

* GPU-accelerated
* Graphic API abstraction layer (OpenGL 3.3/ES 2.0 & DirectX 11 back-ends)
* Shader-based rendering
* Draw TTF text to screen

### Scene

* Complete scene management
* Component/system architecture
* GPU skinning
* Light component with shadow mapping
* Post-processing (motion blur, depth of field, ...)
* Bullet/PhysX 3 physics system
* Recast/Detour navigation system
* Create new component using Lua scripts
* Multiple Lua scripts can run in parallel
* Each scene system executes tasks in parallel using a lock-free stepping algorithm
