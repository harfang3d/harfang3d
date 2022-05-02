# [3.2.1] - 2022-04-10

This minor release brings both code and submodules maintenance, several fixes in the toolchain, in the scenegraph and physics interchange and in the rendering pipeline.

### Source code maintenance
- Updated GLFW (3.3.6).
- Updated BGFX to the latest version.
- Updated OpenAL (v.1.6.10b).
- Updated OpenVR (v1.16.8).
- Fixed the debug build with Visual Studio Build Tools 2022.
- Fixed the installation of PDB files.
- CMake misc. fixes and updates.

### Toolchain

- FBX Importer: changed the unit of command line argument `max smoothing angle` to degrees and fixed the internal vertex/tangent computation.
- GLTF Importer: improved the way non-Windows-compliant filenames are handled.

### Engine

- Fixed an issue in the Wavefront OBJ export that flipped the model on the X axis.
- Improve the error message issued by OpenAssets in case of missing file.
- Fixed a crash when parsing a corrupted .HPS file.
- Added `hg::Picture::SaveBC7` and `hg::Picture::SaveBC6H` functions.
- Switched to a time limit based resource queue processing
- Return the size on disk of a folder
- Implemented a _ComputeNodeWorldMatrix_ to compute a node world matrix on the fly (`ComputeWorld`, `GetWorld`, `SetWorld`). :warning: This function is slow but useful when scene matrices are not yet up-to-date
- Improved the way unicode paths are handled on Windows
- Preliminary support for scene properties animation (`AmbientColor`, `FogNear`, `FogFar`, `FogColor`) while preserving the legacy file format.
- Add an `LSSF_Silent` scene flag, add many boolean flags to silence errors in the rendering IO.
- Allow 32 bit indices in geometries (In the future we might introduce a _force 16 bit indice flag_ if required).
- Validate index and vertex buffer handles when creating geometries.
- Flag instantiated nodes as `NF_InstanceDisabled` if the host node is disabled.
- Added file filter descriptions to the file selector dialogs (`OpenFileDialog`, `SaveFileDialog`). 

### Rendering

 - Added a _depth only_ pass to render shadow maps (`DEPTH_ONLY=1`).
 - Set a higher probe resolution by default (from 64 to 512, using the `--glossScale 20` `--glossBias 0` parameters sent to CMFT).
 - Load/save AAA config (`LoadForwardPipelineAAAConfigFromFile`, `LoadForwardPipelineAAAConfigFromAssets`, `SaveForwardPipelineAAAConfigToFile`)
 - Tweaked the default light values (_default_shadow_bias_ set to 0.0001, _pssm_split_ of a the linear light set to 200m).
 - Switched the jitter sequence to Halton (2,3).
 - Added a function to retrieve the size of the OpenVR framebuffers
 - Added a `RGBA32` pixel format to improve the performance when playing a video stream.

### Physics

- Fixed an issue in synchronizing state from scene to kinematic body

### Documentation

- Added an explanation of why using integer representation for time is important.
- Documented the profiler API.
- Added a HARFANG logo in the Doxygen documentation

# [3.2.0] - 2022-02-21

This minor release brings several fixes, performance improvements and new features to the rendering, physics and audio APIs.

### Source code maintenance
- Cleanup sources, remove spurious hg namespace specifiers and run clang format on affected sources.
- Updated the GLFW CMake to improve the resolution of the library path on Linux.
- Updated ImGui to v1.87.

### Engine
- Implement model load queuing (see `ProcessModelLoadQueue`, `ProcessLoadQueues`, `LSSF_QueueTextureLoads`, `LSSF_QueueModelLoad`).
- Support replay and streaming of OGG files (see `LoadOGGSoundFile`, `LoadOGGSoundAsset`, `StreamOGGFileStereo`, `StreamOGGAssetStereo`, `StreamOGGFileSpatialized`, `StreamOGGAssetSpatialized`).
- Added a `Mat4` copy constructor.
- Added missing declarations for `LoadImage*` functions.
- Added a flag to prevent changing the current camera when loading a scene, if the current camera points to a valid node (see `LSSF_DoNotChangeCurrentCameraIfValid`).
- Added an `is_file` field to the structure returned by `GetFileInfo`.
- Simplify scene binary loader, removed versioning code:
  - Removed unused members from the `RigidBody_` struct.
  - Reduced memory footprint of `RigidBody_` to 6 bytes.
- Performance improvements on multiple scene loading (through instances).
- Added a profiling API (see `BeginProfilerSection`, `EndProfilerSection`, `EndProfilerFrame`, `CaptureProfilerFrame`, `PrintProfilerFrame`).
- Added a Videostream plugin interface (see `MakeVideoStreamer`).
- Fixed scene animation garbage collection.
- Fixed a bug with Unicode support in assetc.
- Fixed an issue with trailing slashes on Linux in the assetc command line.

### Rendering

 - Fixed an issue in the viewport computation when upscaling half buffers in the AAA rendering pipeline.
 - Added a `z_thickness` param to the AAA rendering pipeline.
 - Added a series of cubemap render functions.
 - Implemented and documented [all supported pipeline program features](https://www.harfang3d.com/docs/3.2.0/man.pipelineshader/).

### Physics

- New functions to lock translations and rotations in a more consistent way with the Bullet API:
  - `NodeSetLinearLockAxes`, `NodeSetAngularLockAxes` replaced by `NodeSetLinearFactor`, `NodeSetAngularFactor`.
  - `NodeGetLinearLockAxes`, `NodeGetAngularLockAxes` replaced by `NodeGetLinearFactor`, `NodeGetAngularFactor`.
- Added AddTorque/AddTorqueImpulse to physics API:
  - `NodeAddTorque`, `NodeAddTorqueImpulse`
- Simplified the physics collision/contact query code, unified collision query API for `NodeCollideWorld` and `StepSimulation` (see `CollectCollisionEvents`).
- Implemented `NodeTeleport` in Bullet physics.
- Improved the transform synchronization logic, set node world transform using the fast scene path (see `SyncTransformsFromScene`, `SyncTransformsToScene`).
- Added proper motion interpolation in Bullet physics synchronize to scene.
- Added missing functions to create all supported collision shape types.

### Documentation

- Added a missing reference to the requirements page in the main index.
- Improved the `LoadSceneXXX` functions documentation.
- Fixed both Lua and Python code snippets.
- Improved the clarity of functions using bitflags by adding support for constants group in the documentation generator.
- Update of ownership and views manual pages.

# [3.1.1] - 2021-12-31

This minor release brings several fixes, mainly in the Bullet Physics API.

### Engine

- Added a missing texture render target flag (`TF_RenderTarget`).
- Added  `SetSourceStreamTimecode()` to the audio API.

### Physics

- Updated the Physics unit tests.
- Fixed an issue in the `Kinematic` physics matrix.
- `SetRigidBodyAngularDamping()` now takes a `float` instead of a `Vec3`.

### Documentation

- Fixed several typos in the documentation.

# [3.1.0] - 2021-12-13

This minor release brings several improvements and fixes, mainly in the Bullet Physics API.

### Engine

- Added new HLS colorspace functions.
- Improved `CreateInstanceFromFile`/`CreateInstanceFromAssets` by returning a boolean to inform the caller that the hosted scene could be instantiated.
- Fixed various bindings (`CreateInstanceFromFile`, `CreateInstanceFromAssets`, `GetColorTexture`, `GetDepthTexture`, `GetTextures`)
- Added a function to center a window (GLFW). 
- Added the support for Windows refresh callback (GLFW). 
- Bind read back and blit destination texture flags. 
- Simplify the frame buffer API store less content in Harfang objects. 
- Added fog to render data.
- Validate AAA forward pipeline and associated post processes. 
- Fixed an issue where setting up the imgui font destroyed the cursors. 
- Added `time_from_xxx_d` functions and fixed a precision issue with `time_from_string` 

### Physics

- Fixed the initial transform setup code of Bullet rigid bodies. 
- Changed the sync kinematic and dynamic mechanisme.
- Added `ResetWorld`,  `DisableDeactivation`, add a way to lock axes.
- Added a getter/setter for rolling friction.
- Added a raycast functions to return all collisions found between two points. 
- Fixed a raycast issue with geometry hierarchy.
- Added the support for multi collision shapes with matrices

### Toolchain

- Assetc cleanup error messages, do not output unless truly reporting on an error.
- Assetc will now delete the outputs for deleted inputs. A flag was added to opt-out from this mechanism (`-no_clean_removed_inputs`).
- Assetc will now delete the previous compiled lua scripts if a compilation error arises.
- Assimp importer: fixed a mention to the wrong converter. 
- Assimp importer: added a `-merge-mesh` option.
- Assimp importer: updated to Assimp v5.1.2.
- GLTF importer: fixed the import in case of many instances.
- Fixed the toolchain integrity check.

### Documentation

- Documentation improvements and cleanup.
- Fixed a typo in `GetTextures` documentation filename.
- Fixed the reference to `SceneBullet3Physics` in the man Physics page. 
- Added a man.Requirements page.
- Improved the Pypi wheel description ('Quickstart' was lacking a download URL).

### Misc

- Updated the license.
- Fixed some cppcheck issues. 

# [3.0.0] - 2021-10-12

This major release mainly replaces Newton Dynamics with Bullet Dynamics for physics and improves interoperability between supported languages.

### Engine

- Add diffuse and specular intensity to lights.
- Performance and filtering improvements of the AAA rendering pipeline.
- Replace Newton Dynamics with Bullet 3 Dynamics, SceneBullet3Physics uses the same API as SceneNewtonPhysics.
- Add SceneBullet3Physics.NodeWake to wake a sleeping body.
- Implement restitution and friction in rigid bodies.
- Provide support for value transfer between Lua VMs when calling Get/Set/Call on SceneLuaVM.
- Implement full value transfer between Python and Lua VMs.
- Implement Call on SceneLuaVM.
- Light default specular value is now (1.0, 1.0, 1.0).
- Support for specular color in core light models (point, spot, linear) in the PBR shader.
  This is not compliant with a strict definition of a PBR Pipeline, but this approach tends to be a consensus in the industry (Blender, Redshift, ...) and is artist-friendly.
- Add Joystick support (Joystick, JoystickState, ReadJoystick, GetJoystickNames, GetJoystickDeviceNames).
- Improve OpenAsset and LoadResourceMeta performance.
- Improve Vertices.End performance by removing systematic default validation.
- Rename x_aspect_ratio parameter to fov_axis_is_horizontal in SceneSubmitToForwardRenderPipeline.
- SetView2D and SetViewPerspective now takes x,y,w,h instead of w,h.
- Fix redundant loads of texture meta JSON when loading materials.
- Fix ImGui Enter/Return key distinction.
- Fix broken linear texture filtering.
- Fix GLFW window system not sending window signals and breaking IsWindowOpen as a consequence.
- Fix spurious error message when opening asset with several asset folders registered.

### Toolchain

- New AssImp converter.
- GLTF Importer: Add occlusion texture if no metal/roughness texture is declared in a source material.
- FBX, GLTF importer: Fix on the importation of animations in some specific rotations situations.

### Documentation

- Quickstart page improvements.
- Index class members in the search index.

### Misc

- Update STB Truetype.
- Update STB Vorbis.
- Update STB Image.
- Update STB Image Write.
- Update bgfx.
- Update bimg.
- Update tiny process.

# [2.0.111] - 2021-06-21

This release is a rewrite of the Harfang API toward a data-oriented approach.
As a developer, you will find that many of the old classes are now gone and replaced by function calls that you string together in order to implement complex functionalities. The improved API provides greater flexibility and delivers higher performance than the Harfang 1.x API.
Of major interest in this first release of Harfang 2.x are the new resource/asset pipeline (see Resources & Assets) as well as the new AAA renderer beta enabling screen-space global illumation with correct approximation of both the radiance and irradiance terms of the PBR equation.

**NOTE:**

The AAA rendering pipeline is currently a **WORK IN PROGRESS** and has important issues to address.