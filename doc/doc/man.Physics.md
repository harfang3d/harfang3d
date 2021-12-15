.title Physics

Use physics to enable physically plausible interactions with your scene content.

Different backends are supported such as Bullets Physics, NVIDIA PhysX and Newton Dynamics.

[TOC]

## Declaring Physics

To turn a node into a rigid body, create a [RigidBody] component and assign it to the node using [Node_SetRigidBody].

To fully declare a rigid body you must attach at least one collision shape to its node. Create a [Collision] component and call [Node_SetCollision] to do so. You can attach multiple collision components to a single node.

A rigid body can be of the following type:

* `RBT_Dynamic`: Responds in a physically plausible manner to interaction with other rigid bodies. Its motion is fully controlled by the physics engine. A dynamic rigid body will automatically go to sleep and stop consuming simulation resources when it stops moving for a long enough period of time.
* `RBT_Kinematic`: Follows the transformation of the node it belongs to. A kinematic body never goes to sleep automatically.

See [RigidBody_SetType].

The rigid body intertia tensor is computed from its collision shape properties. A zero mass rigid body is considered infinitely heavy and cannot be moved.

## Simulating Physics

Create a physics backend such as [SceneNewtonPhysics] and call [SceneBullet3Physics_SceneCreatePhysicsFromAssets] to create the physics states corresponding to the scene declaration.

*Note:* [SceneBullet3Physics_SceneCreatePhysicsFromAssets] means that if setting up the physics states requires access to an external resource, such as a mesh, it should be loaded from the assets system. If you are working from the filesystem, use [SceneBullet3Physics_SceneCreatePhysicsFromFile].

### Running the Simulation

This involves 3 steps on each update:

1. Synchronize physics state with the scene declaration using [SceneBullet3Physics_SceneCreatePhysicsFromAssets]. Alternatively, you can use a more fine-grained approach using [SceneBullet3Physics_NodeCreatePhysicsFromAssets] to improve performance.
2. Step the simulation using [SceneBullet3Physics_StepSimulation].
3. Synchronize the updated physics transformations to the scene using [SceneBullet3Physics_SyncDynamicBodiesToScene].

*Note:* If you are using kinematic bodies you will also need to synchronize them from their node transformation on each update using [SceneBullet3Physics_SyncKinematicBodiesFromScene].

### The Easy Way

While the low-level API offers the most flexibility most projects will use a straightforward implementation which is provided by the [SceneUpdateSystems] function. This function handles all of the above steps and does the same for other systems you may use.

When using a script system, this function will also dispatch collision events to it.

## Keeping the System Synchronized

Call the physics system garbage collect method (eg. [SceneBullet3Physics_GarbageCollect]) on each update to ensure that destroyed nodes or components are properly removed. If you know that no node or component was destroyed during a particular update, not calling the garbage collector will save on performance.

## Reading Physics Transformation

When a node is assigned a dynamic rigid body its transformation matrix is overriden by the physics system. Its transform component however, is not. Transform methods such as [Transform_SetPos] or [Transform_SetRot] will have no effect unless you explicitely synchronize the node to the physics system after using them.

[Transform_GetWorld] will return the transformation matrix as set by the physics system.
