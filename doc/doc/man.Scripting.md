.title Scripting

Scripts can be used to extend the behavior of nodes and scenes.

[TOC]

## Host vs. Embedded VM

When using Harfang from a scripting language it can be difficult to differentiate between parts of your program running on you main script VM and parts of your program running on one of the supported embedded VMs.

We differentiate between those VMs by using the term *host VM* and *embedded VM*. For example, you may write a program in CPython which declares a scene extended using Lua scripts. In this case, CPython is the *host VM* and Lua is the *embedded VM*.

## Declaring Scripts

Create a [Script] component and assign it to a node or scene using [Node_SetScript] or [Scene_SetScript]. Set the path to the script source using [Script_SetPath].

## Creating & Evaluating Scripts

Create a backend such as [SceneLuaVM] and call [SceneLuaVM_SceneCreateScriptsFromAssets] to create the script states corresponding to the scene declaration. A [Script] component can be assigned to multiple nodes in which case they will all share the same execution environment.

You should then use the [SceneUpdateSystems] function to update both the scene and its systems. This function will update all the systems you pass to it and implement a default behavior that dispatch common events to script using a set of specific callbacks.

## Script Environment

The following symbols are defined when creating the environment for a script.

Symbol | Description
------ | -----------
G      | Table shared by all script component created by the same scene.
hg     | Access to the Harfang API.
scene  | Scene object this component belongs to.

### Default Events & Callbacks

Node events reported by the default update behavior:

- *OnAttachToNode(Node node, int slot_index)*: A script component was attached to a node slot as a result of calling [Node_SetScript].
- *OnDetachFromNode(Node node, int slot_index)*: A script component was detached from a node slot as a result of calling [Node_RemoveScript].
- *OnDestroy()*: A script component is about to be destroyed and its memory released.
- *OnUpdate(Node node, time_ns dt)*: Called during a scene update for each node a script component is attached to.
- *OnCollision(Node a, Node b)*: Called when two node collide.

Scene events reported by the default update behavior:

- *OnDestroy()*: A script component is about to be destroyed and its memory released.
- *OnAttachToScene(scene, slot_idx)*: A script component was attached to a Scene slot as a result of calling [Scene_SetScript].
- *OnDetachFromScene(scene, slot_idx)*: A script component was detached from a Scene slot as a result of calling [Scene_RemoveScript].
- *OnUpdate(Scene scene, time_ns dt)*: Called during a scene update.
- *OnSubmitSceneToForwardPipeline(ViewId base_view_id, Scene scene, Rect rect, ViewState view_state, ForwardPipeline pipeline, PipelineResources, FramebufferHandle fb)*: Called at the end of a scene submission to the forward pipeline.

### Communicating with Scripts

Depending on the host and embedded VM used you may be able to access a script component environment directly (eg. [SceneLuaVM_GetScriptEnv]).

If this is not possible however this can be done using the backend get and set value methods from any host VM. In some cases the transfer will be automatic while in other cases you may need to explicitely marshall the outgoing/incoming values.

Source VM/Target VM | CPython | Lua
--------------|---------|-----
CPython       | N/A     | `LuaObject.Pack`/`LuaObject.Unpack`
Lua           | N/A     | -

Sending a value from a host CPython VM to an embedded Lua VM and back.

```python
# from host Python to embedded Lua
lua_vm.SetScriptValue('target_node', lua_vm.MakeLuaObject().Pack(node))

# from embedded Lua to host Python
target_node = lua_vm.GetScriptValue('target_node').Unpack()
```

Sending a value from a host Lua VM to an embedded Lua VM and back.

```lua
-- from host Lua to embedded Lua
lua_vm.SetScriptValue('target_node', node)

-- from embedded Lua to host Lua
node = lua_vm.GetScriptValue('target_node')
```

## Keeping the System Synchronized

Call the script system garbage collect method (eg. [SceneLuaVM_GarbageCollect]) on each update to ensure that destroyed nodes or components are properly removed. If you know that no node or component was destroyed during a particular update, not calling the garbage collector will save on performance.
