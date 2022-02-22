// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/generational_vector_list.h"
#include "foundation/time.h"

#include "engine/physics.h"

#include <vector>

namespace hg {

using ComponentRef = gen_ref;

struct Node;
struct Script;

class Scene;
class SceneLuaVM;
#if HG_ENABLE_BULLET3_SCENE_PHYSICS
class SceneBullet3Physics;
#endif

struct SceneClocks {
	time_ns physics{};
};

/// Create dependent system resource based on the current scene state.
void SceneSyncToSystemsFromFile(Scene &scene, SceneLuaVM &vm);
void SceneSyncToSystemsFromAssets(Scene &scene, SceneLuaVM &vm);
#if HG_ENABLE_BULLET3_SCENE_PHYSICS
void SceneSyncToSystemsFromFile(Scene &scene, SceneBullet3Physics &physics);
void SceneSyncToSystemsFromAssets(Scene &scene, SceneBullet3Physics &physics);
void SceneSyncToSystemsFromFile(Scene &scene, SceneBullet3Physics &physics, SceneLuaVM &vm);
void SceneSyncToSystemsFromAssets(Scene &scene, SceneBullet3Physics &physics, SceneLuaVM &vm);
#endif

/// Update scene, physics and scripts. Script events are called where required.
void SceneUpdateSystems(Scene &scene, SceneClocks &clocks, time_ns dt);
void SceneUpdateSystems(Scene &scene, SceneClocks &clocks, time_ns dt, SceneLuaVM &vm);

#if HG_ENABLE_BULLET3_SCENE_PHYSICS
void SceneUpdateSystems(Scene &scene, SceneClocks &clocks, time_ns dt, SceneBullet3Physics &physics, time_ns physics_step, int max_physics_step);
void SceneUpdateSystems(
	Scene &scene, SceneClocks &clocks, time_ns dt, SceneBullet3Physics &physics, time_ns physics_step, int max_physics_step, SceneLuaVM &vm);

void SceneUpdateSystems(
	Scene &scene, SceneClocks &clocks, time_ns dt, SceneBullet3Physics &physics, NodePairContacts &contacts, time_ns physics_step, int max_physics_step);
void SceneUpdateSystems(Scene &scene, SceneClocks &clocks, time_ns dt, SceneBullet3Physics &physics, NodePairContacts &contacts, time_ns physics_step,
	int max_physics_step, SceneLuaVM &vm);
#endif

/// Collect scene, physics and scripts garbage and destroy them.
size_t SceneGarbageCollectSystems(Scene &scene);
size_t SceneGarbageCollectSystems(Scene &scene, SceneLuaVM &vm);

#if HG_ENABLE_BULLET3_SCENE_PHYSICS
size_t SceneGarbageCollectSystems(Scene &scene, SceneBullet3Physics &physics);
size_t SceneGarbageCollectSystems(Scene &scene, SceneBullet3Physics &physics, SceneLuaVM &vm);
#endif

/// Clear scene, physics and scripts.
void SceneClearSystems(Scene &scene);
void SceneClearSystems(Scene &scene, SceneLuaVM &vm);

#if HG_ENABLE_BULLET3_SCENE_PHYSICS
void SceneClearSystems(Scene &scene, SceneBullet3Physics &physics);
void SceneClearSystems(Scene &scene, SceneBullet3Physics &physics, SceneLuaVM &vm);
#endif

} // namespace hg
