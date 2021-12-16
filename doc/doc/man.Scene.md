.title Working with Scene

A scene is a 3d world populated with [Node].

Nodes are container objects taking meaning through the use of components.

## Node & Components

Calling [Scene_CreateNode] returns an empty node with no component attached. In this state, it serves little to no purpose as it will not be drawn or implement any concrete behavior.

### Object

In order to be drawn a node must provide two essential informations:

- *A transformation:* This is done using [Node_SetTransform] to assign it a [Transform] component.
- *A visual representation:* This is done using [Node_SetObject] to assign it an [Object] component.

The object component accepts a [ModelRef] to a [Model] and holds a local list of [Material] used to draw the model. Each object component may hold different material definition for the same model.

The same components can be assigned to multiple nodes.

### Camera

Assign the [Camera] component to nodes to turn them into observers into the scene.

For more information on how this integrates with drawing a scene, see [man.DrawingScene].

### Light

Assign the [Light] component to nodes to turn them into light sources.

### Instance

Scenes can be instantiated in one another. This is useful to create multiple complex parts with their own animations or scripts from which you compose a larger world.

To instantiate a scene use [Node_SetInstance] to assign an [Instance] component to a node. To perform explicit instantiation use [Node_SetupInstanceFromAssets] or [Node_SetupInstanceFromFile].

*Note:* Instances are automatically setup when loading a scene.

After instantiation, the instance content is held in the host scene. [Node_GetInstanceSceneView] can be used to access it in isolation from the host content via the returned [SceneView] object.

## Managing Scene Resources

Most scene resources are returned by value as generational references (see [man.Ownership]) wrapped into helper classes such as [Node], [Camera], [Light] or [Object].

The scene has strong ownership of the resources it manages.

Nodes are explicitely destroyed using [Scene_DestroyNode] and components are implicitely destroyed using [Scene_GarbageCollect].
