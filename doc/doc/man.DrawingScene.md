.title Drawing a Scene

Drawing a scene is done by calling [SubmitSceneToPipeline].

This function expects a start view index and may use multiple views to complete its operation. It returns an object mapping pipeline stages to view indices and the next unused view index.

Make sure to read [man.Views] if you are unsure about the view system.

If no explicit [ViewState] is provided the scene current camera is used to compute one.

[ViewState] can be computed using [ComputeOrthographicViewState] or [ComputePerspectiveViewState]. This also can be done from a node with a [Camera] component by calling [Node_ComputeCameraViewState].

## Multiple Points of View

To efficiently draw a scene using the forward pipeline from multiple points of view use the low-level API functions to:

1. Prepare the view-independent data using [PrepareSceneForwardPipelineCommonRenderData].
1. For each view:
	1. Prepare the view-dependent data using [PrepareSceneForwardPipelineViewDependentRenderData].
	2. Submit the scene to the forward pipeline by calling [SubmitSceneToForwardPipeline].

Use the view id returned by a call as the starting view of the next call.

Here is a Python example of the full procedure:

```python
# common, view-independent render data
vid = 0
vid, pass_ids = hg.PrepareSceneForwardPipelineCommonRenderData(vid, scene, render_data, pipeline, res)

# view-dependent render data and submit for view 1
vid, pass_ids = hg.PrepareSceneForwardPipelineViewDependentRenderData(vid, view_state_1, scene, render_data, pipeline, res)
vid, pass_ids = hg.SubmitSceneToForwardPipeline(vid, scene, rect, view_state_1, pipeline, render_data, res)

# view-dependent render data and submit for view 2
vid, pass_ids = hg.PrepareSceneForwardPipelineViewDependentRenderData(vid, view_state_2, scene, render_data, pipeline, res)
vid, pass_ids = hg.SubmitSceneToForwardPipeline(vid, scene, rect, view_state_2, pipeline, render_data, res)

# start rendering
hg.Frame()
```
