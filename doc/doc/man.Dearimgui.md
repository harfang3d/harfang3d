.title DearImGui

Harfang embeds the [dear imGui](https://github.com/ocornut/imgui) library.

Dear imgui is an immediate GUI library designed to quickly build debugging/profiling user interface.

It is available at all time through the ImGui* function and only requires a [Renderer] instance to work. Only a single instance of the library is available and its output is displayed right before executing a [Renderer_ShowFrame] call.

Accessing ImGui from multiple threads require synchronization using the [ImGuiLock] and [ImGuiUnlock] functions.

### Minimal sample code showing a window

```python
import harfang as hg

hg.LoadPlugins()

renderer = hg.CreateRenderer()
renderer.Open()

win = hg.NewWindow(640, 480)

surface = renderer.NewOutputSurface(win)
renderer.SetOutputSurface(surface)

hg.ImGuiSetOutputSurface(surface)

while True:
	hg.ImGuiBegin("window"):
	hg.ImGuiEnd()

	renderer.Clear(hg.Color.Red)
	renderer.ShowFrame()

	hg.UpdateWindow(win)
	hg.EndFrame()

renderer.DestroyOutputSurface(surface)
hg.DestroyWindow(win)
renderer.Close()
```