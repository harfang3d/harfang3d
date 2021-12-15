.title Virtual Reality

Virtual reality is supported through the OpenVR API on the Windows operating system.

[TOC]

## Prerequisites

- A compatible VR headset (HTC Vive, Occulus Rift or Mixed Reality compatible).
- SteamVR must be installed on your computer.

## OpenVR API

Initialize OpenVR using [OpenVRInit], when done call [OpenVRShutdown]. You can initialize and shutdown OpenVR multiple times throughout your program lifetime.

Create two framebuffer to render both eyes to by calling [OpenVRCreateEyeFrameBuffer]. These buffers can be kept alive between a shutdown/init cycle of OpenVR.

### Main Loop

During your program main loop perform the following steps:

1. Read the current VR state using [OpenVRGetState]. This function takes the transformation of the actor body in the virtual world and returns a state object containing everything required to draw the left and right eye views.
1. Use [OpenVRStateToViewState] function to generate the left and right eye [ViewState].
1. For each eye:
	1. Use [OpenVREyeFrameBuffer_GetHandle] to retrieve a framebuffer handle.
	2. Use [SubmitSceneToPipeline] to draw the scene to the eye framebuffer. To improve performance consider using the low-level APIs to draw the scene, see [man.DrawingScene].
1. Kick-off rendering by calling [Frame].
1. Submit eye framebuffers to the device using [OpenVRSubmitFrame].

The following Python code implements the complete render loop to efficiently display a scene in VR.

```python
scene.Update(hg.TickClock())

vr_state = hg.OpenVRGetState(cam.GetTransform().GetWorld(), cam.GetCamera().GetZNear(), cam.GetCamera().GetZFar())
left, right = hg.OpenVRStateToViewState(vr_state)
vr_eye_rect = hg.IntRect(0, 0, vr_state.width, vr_state.height)

# common, view-independent render data
vid = 0
vid, pass_ids = hg.PrepareSceneForwardPipelineCommonRenderData(vid, scene, render_data, pipeline, res)

# view-dependent render data and submit
vid, pass_ids = hg.PrepareSceneForwardPipelineViewDependentRenderData(vid, left, scene, render_data, pipeline, res)
vid, pass_ids = hg.SubmitSceneToForwardPipeline(vid, scene, vr_eye_rect, left, pipeline, render_data, res, vr_left_fb.GetHandle())

vid, pass_ids = hg.PrepareSceneForwardPipelineViewDependentRenderData(vid, right, scene, render_data, pipeline, res)
vid, pass_ids = hg.SubmitSceneToForwardPipeline(vid, scene, vr_eye_rect, right, pipeline, render_data, res, vr_right_fb.GetHandle())

# start rendering
hg.Frame()

# submit to VR device
hg.OpenVRSubmitFrame(vr_left_fb, vr_right_fb)
```

### Reading Input

Reading input from the VR controllers or trackers is done using the [ReadVRController] and [ReadVRGenericTracker] functions. See [man.Input] for more information on reading inputs in general.

## Performance Notes

VR is extremely costly in terms of performance. Depending on the headset, the minumum required display refresh rate can vary between 90Hz and 120Hz.

At least two different renders, one for both eyes, are required. You might occasionally need a third render to monitor the simulation from an external point of view.

As a result of these requirements, keep in mind the following points:

- You should minimize the amount of work required per scene render. See [man.DrawingScene] to learn how to use the low-level API to efficiently render a scene for multiple observers.
- Linear light shadow mapping is extremely costly. As they are view-dependent, PSSM splits must be drawn from scratch for each viewpoint. Consider using spot light shadow mapping which is not view-dependent and can be performed once for all viewpoints.
- Given the number of scene pass required, the triangle budget must kept under control and will usually be significantly lower than usual.
