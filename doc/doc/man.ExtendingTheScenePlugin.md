.title Extending the scene plugin

## Writing a scene tool plugin

A scene tool plugin must be declared as a class extending the `plugin.ISceneToolPlugin` interface.

```python
class Plugin(ISceneToolPlugin):
	""" New Scene plugin """
```

The following methods must be implemented by the plugin:

* `on_selection_changed(selection)`: Called by the scene plugin whenever the selection is changed. The complete new selection is passed to the plugin.
* `on_node_selection_changed(node_selection)`: Same as above but a list of the scene nodes in the new selection is passed to the plugin.
* `on_frame_complete()`: This function is called when the current frame is complete. The plugin is given a chance to draw additional content at this point.
**Note:** This call is done from the rendering thread. The synchronous renderer object [^1] can be used from this location. The renderer matrix stack is automatically saved and restored around this call.
* `on_mouse_event(event, mouse, dt_frame)`: Called whenever a mouse event happens over the viewport.

[^1]: Available through the `engine.renderer` symbol.

## Mouse events

TODO
