.title Engine debugger

Harfang integrates a debugger written in [man.Dearimgui]. The debugger can be used to inspect, debug and profile many systems of the engine at runtime.

Use the [SetEnableDebugger] function to enable and disable the debugger. The debugger interface will overlay itself over your program output before each call to [Renderer_ShowFrame].

## Engine systems

The debugger monitors the following engine systems.

* **Renderer:** Statistics for the current [Renderer].
* **Render system:** Statistics for the current [RenderSystem].
* **Texture cache:** Display the content of the engine texture cache.
* **Geometry cache:** Display the content of the engine geometry cache.
* **Material cache:** Display the content of the engine material cache.
* **Log window:** Display the engine log output.

## Scene debugger

.img("man.scene_debugger.jpg")

The debugger tracks all scene creation and deletion and keeps a list of available scene in the *Scene debugger* menu. You can select a specific scene to monitor or select the *automatic* option from the *Scene debugger* menu to track the last displayed scene.

The scene debugger can display the full scene tree, inspect and modify [Node] and their [man.Component].