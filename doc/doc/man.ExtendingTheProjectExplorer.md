.title Extending the project explorer

## Writing a project explorer plugin

A scene tool plugin must be declared as a class extending the `plugin.IProjectExplorerPlugin` interface.

```python
class Plugin(IProjectExplorerPlugin):
	""" A new project explorer plugin """
```

The following methods must be implemented by the plugin:

* `process_drop_event(dropped_urls, target_url)`: Process a drop event over the project explorer. Return `plugin.InterruptPluginChain` to stop execution at your plugin.
