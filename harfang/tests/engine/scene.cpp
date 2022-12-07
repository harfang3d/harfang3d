// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

// somewhere along the line minwindef.h is included...
#undef near
#undef far

#include "engine/scene.h"

#include "engine/assets.h"
#include "engine/assets_rw_interface.h"
#include "engine/forward_pipeline.h"
#include "engine/scene_forward_pipeline.h"
#include "engine/scene_lua_vm.h"
#include "engine/scene_systems.h"
#if HG_ENABLE_BULLET3_SCENE_PHYSICS
#include "engine/scene_bullet3_physics.h"
#endif

#include "foundation/data.h"
#include "foundation/data_rw_interface.h"

using namespace hg;

static void test_ComponentGarbageCollection() {
	Scene scene;
	const auto transform = scene.CreateTransform();
	TEST_CHECK(transform.IsValid() == true);
	TEST_CHECK(scene.GarbageCollect() == 1);
	TEST_CHECK(transform.IsValid() == false);
}

static void test_DuplicateNodes() {
	PipelineResources resources;

	Scene scene;

	const auto lgt = CreatePointLight(scene, Mat4::Identity, 3.5f);
	const auto cam = CreateCamera(scene, Mat4::Identity, 0.25f, 1024.f);
	TEST_CHECK(scene.GetNodeCount() == 2);

	const auto out = DuplicateNodes(scene, {lgt.ref, cam.ref}, g_assets_reader, g_assets_read_provider, resources, GetForwardPipelineInfo());
	TEST_CHECK(out.size() == 2);
	TEST_CHECK(scene.GetNodeCount() == 4);

	const auto lgt_copy = scene.GetNode(out[0]);
	TEST_CHECK(lgt_copy.GetLight().GetRadius() == 3.5f);

	const auto cam_copy = scene.GetNode(out[1]);
	TEST_CHECK(cam_copy.GetCamera().GetZFar() == 1024.f);
}

static void test_WalkHierarchy() {
	Scene scene;

	auto root0 = CreatePointLight(scene, Mat4::Identity, 0.f);
	root0.SetName("root0");
	auto a0 = CreatePointLight(scene, Mat4::Identity, 0.f);
	a0.SetName("a");
	a0.GetTransform().SetParent(root0.ref);

	auto root1 = CreatePointLight(scene, Mat4::Identity, 0.f);
	root1.SetName("root1");
	auto a1 = CreatePointLight(scene, Mat4::Identity, 0.f);
	a1.SetName("a");
	a1.GetTransform().SetParent(root1.ref);

	auto a2 = CreatePointLight(scene, Mat4::Identity, 0.f);
	a2.SetName("a");

	auto r = scene.GetNodeEx("a"); // GetNodeEx_ walks the hierarchy and should skip a0 and a1 to return a2
	TEST_CHECK(r == a2);

	r = scene.GetNodeEx("root1/a");
	TEST_CHECK(r == a1);

	r = scene.GetNodeEx("root0/a");
	TEST_CHECK(r == a0);
}

static void test_DisableLightNodes() {
	Scene scene;

	std::vector<Node> lgts;
	for (int i = 0; i < 6; ++i)
		lgts.push_back(CreatePointLight(scene, Mat4::Identity, 1.f));

	std::vector<ForwardPipelineLight> out_lights;

	GetSceneForwardPipelineLights(scene, out_lights);
	TEST_CHECK(out_lights.size() == 6);

	lgts[0].Disable();
	lgts[2].Disable();
	lgts[4].Disable();

	GetSceneForwardPipelineLights(scene, out_lights);
	TEST_CHECK(out_lights.size() == 3);

	for (auto lgt : lgts)
		lgt.Enable();

	GetSceneForwardPipelineLights(scene, out_lights);
	TEST_CHECK(out_lights.size() == 6);

	for (auto lgt : lgts)
		lgt.Disable();

	GetSceneForwardPipelineLights(scene, out_lights);
	TEST_CHECK(out_lights.size() == 0);
}

static void test_DisableObjectNodes() {
	Model mdl;
	mdl.bounds.push_back(MinMaxFromPositionSize({0, 0, 0}, {1, 1, 1}));
	mdl.lists.push_back({BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE});
	mdl.mats.push_back(0);

	PipelineResources resources;
	const auto mdl_ref = resources.models.Add("mdl", mdl);

	Scene scene;

	std::vector<Node> objs;
	for (int i = 0; i < 6; ++i)
		objs.push_back(CreateObject(scene, Mat4::Identity, mdl_ref, {{}}));

	std::vector<ModelDisplayList> opaque, transparent;
	std::vector<SkinnedModelDisplayList> opaque_skinned, transparent_skinned;

	scene.GetModelDisplayLists(opaque, transparent, opaque_skinned, transparent_skinned, resources);
	TEST_CHECK(opaque.size() == 6);

	objs[0].Disable();
	objs[2].Disable();
	objs[4].Disable();

	scene.GetModelDisplayLists(opaque, transparent, opaque_skinned, transparent_skinned, resources);
	TEST_CHECK(opaque.size() == 3);

	for (auto &obj : objs)
		obj.Enable();

	scene.GetModelDisplayLists(opaque, transparent, opaque_skinned, transparent_skinned, resources);
	TEST_CHECK(opaque.size() == 6);

	for (auto &obj : objs)
		obj.Disable();

	scene.GetModelDisplayLists(opaque, transparent, opaque_skinned, transparent_skinned, resources);
	TEST_CHECK(opaque.size() == 0);
}

static void test_LoadSaveEmptyScene() {
	PipelineResources resources;

	Data data;
	{
		Scene scene;
		TEST_CHECK(scene.GetAllNodeCount() == 0);
		TEST_CHECK(SaveSceneJsonToData(data, scene, resources) == true);
	}

	data.Rewind();

	{
		Scene scene;
		LoadSceneContext ctx;
		TEST_CHECK(LoadSceneJsonFromData(data, "data", scene, g_assets_reader, g_assets_read_provider, resources, GetForwardPipelineInfo(), ctx) == true);
		TEST_CHECK(scene.GetAllNodeCount() == 0);
	}
}


static void test_LoadSaveEmptySceneBinary() {
	PipelineResources resources;

	Data data;
	{
		Scene scene;
		TEST_CHECK(scene.GetAllNodeCount() == 0);
		TEST_CHECK(SaveSceneBinaryToData(data, scene, resources) == true);
	}

	data.Rewind();

	{
		Scene scene;
		LoadSceneContext ctx;
		TEST_CHECK(LoadSceneBinaryFromData(data, "data", scene, g_assets_reader, g_assets_read_provider, resources, GetForwardPipelineInfo(), ctx) == true);
		TEST_CHECK(scene.GetAllNodeCount() == 0);
	}
}

static void test_LoadSaveObject() {
	PipelineResources resources;

	Data data;
	{
		Scene scene;
		for (int i = 0; i < 1000; ++i) {
			Node node = scene.CreateNode("object");
			node.SetObject(scene.CreateObject());
			node.SetTransform(scene.CreateTransform());
		}
		TEST_CHECK(scene.GetNodesWithComponent(NCI_Object).size() == 1000);
		TEST_CHECK(SaveSceneJsonToData(data, scene, resources) == true);
	}

	data.Rewind();

	{
		Scene scene;
		LoadSceneContext ctx;
		TEST_CHECK(LoadSceneJsonFromData(data, "data", scene, g_assets_reader, g_assets_read_provider, resources, GetForwardPipelineInfo(), ctx) == true);
		TEST_CHECK(scene.GetNodesWithComponent(NCI_Object).size() == 1000);
	}
}

static void test_LoadSaveObjectBinary() {
	PipelineResources resources;

	Data data;
	{
		Scene scene;
		for (int i = 0; i < 1000; ++i) {
			auto node = scene.CreateNode("object");
			node.SetObject(scene.CreateObject());
			node.SetTransform(scene.CreateTransform());
		}
		TEST_CHECK(scene.GetNodesWithComponent(NCI_Object).size() == 1000);
		TEST_CHECK(SaveSceneBinaryToData(data, scene, resources) == true);
	}

	data.Rewind();

	{
		Scene scene;
		LoadSceneContext ctx;
		TEST_CHECK(LoadSceneBinaryFromData(data, "data", scene, g_assets_reader, g_assets_read_provider, resources, GetForwardPipelineInfo(), ctx) == true);
		TEST_CHECK(scene.GetNodesWithComponent(NCI_Object).size() == 1000);
	}
}

static void test_LoadSaveCamera() {
	PipelineResources resources;

	Data data;
	{
		Scene scene;
		for (int i = 0; i < 1000; ++i) {
			auto node = scene.CreateNode("camera");
			node.SetCamera(scene.CreateCamera());
			node.SetTransform(scene.CreateTransform());
		}
		TEST_CHECK(scene.GetNodesWithComponent(NCI_Camera).size() == 1000);
		TEST_CHECK(SaveSceneJsonToData(data, scene, resources) == true);
	}

	data.Rewind();

	{
		Scene scene;
		LoadSceneContext ctx;
		TEST_CHECK(LoadSceneJsonFromData(data, "data", scene, g_assets_reader, g_assets_read_provider, resources, GetForwardPipelineInfo(), ctx) == true);
		TEST_CHECK(scene.GetNodesWithComponent(NCI_Camera).size() == 1000);
	}
}

static void test_LoadSaveCameraBinary() {
	PipelineResources resources;

	Data data;
	{
		Scene scene;
		for (int i = 0; i < 1000; ++i) {
			auto node = scene.CreateNode("camera");
			node.SetCamera(scene.CreateCamera());
			node.SetTransform(scene.CreateTransform());
		}
		TEST_CHECK(scene.GetNodesWithComponent(NCI_Camera).size() == 1000);
		TEST_CHECK(SaveSceneBinaryToData(data, scene, resources) == true);
	}

	data.Rewind();

	{
		Scene scene;
		LoadSceneContext ctx;
		TEST_CHECK(LoadSceneBinaryFromData(data, "data", scene, g_assets_reader, g_assets_read_provider, resources, GetForwardPipelineInfo(), ctx) ==  true);
		TEST_CHECK(scene.GetNodesWithComponent(NCI_Camera).size() == 1000);
	}
}

static void test_LoadSaveLight() {
	PipelineResources resources;

	Data data;
	{
		Scene scene;
		for (int i = 0; i < 1000; ++i) {
			auto node = scene.CreateNode("light");
			node.SetLight(scene.CreateLight());
			node.SetTransform(scene.CreateTransform());
		}
		TEST_CHECK(scene.GetNodesWithComponent(NCI_Light).size() == 1000);
		TEST_CHECK(SaveSceneJsonToData(data, scene, resources) == true);
	}

	data.Rewind();

	{
		Scene scene;
		LoadSceneContext ctx;
		TEST_CHECK(LoadSceneJsonFromData(data, "data", scene, g_assets_reader, g_assets_read_provider, resources, GetForwardPipelineInfo(), ctx) == true);
		TEST_CHECK(scene.GetNodesWithComponent(NCI_Light).size() == 1000);
	}
}

static void test_LoadSaveLightBinary() {
	PipelineResources resources;

	Data data;
	{
		Scene scene;
		for (int i = 0; i < 1000; ++i) {
			auto node = scene.CreateNode("light");
			node.SetLight(scene.CreateLight());
			node.SetTransform(scene.CreateTransform());
		}
		TEST_CHECK(scene.GetNodesWithComponent(NCI_Light).size() == 1000);
		TEST_CHECK(SaveSceneBinaryToData(data, scene, resources) == true);
	}

	data.Rewind();

	{
		Scene scene;
		LoadSceneContext ctx;
		TEST_CHECK(LoadSceneBinaryFromData(data, "data", scene, g_assets_reader, g_assets_read_provider, resources, GetForwardPipelineInfo(), ctx) == true);
		TEST_CHECK(scene.GetNodesWithComponent(NCI_Light).size() == 1000);
	}
}

static void test_SceneLuaVM() {
	Scene scene;
	const auto node = CreateScript(scene);

	SceneLuaVM vm;
	vm.OverrideScriptSource(node.GetScript(0).ref, "print('Hello SceneLuaVM!')");
	SceneSyncToSystemsFromAssets(scene, vm);

	TEST_CHECK(vm.GetScriptCount() == 1);
	TEST_CHECK(vm.GarbageCollect(scene).empty() == true);

	scene.DestroyNode(node);
	TEST_CHECK(scene.GarbageCollect() > 0);

	const auto script_garbage = vm.GarbageCollect(scene);
	TEST_CHECK(script_garbage.empty() == false);

	vm.DestroyScripts(script_garbage);
	TEST_CHECK(vm.GetScriptCount() == 0);
}

static void test_LuaScriptSceneOnUpdateEventCallback() {
	SceneLuaVM vm;
	TEST_CHECK(Execute(vm.GetL(), "G.on_update = 0;", "bootstrap") == true);

	Scene scene;
	auto script = scene.CreateScript();
	vm.OverrideScriptSource(script.ref, "\
function OnUpdate(scene)\n\
G.on_update = G.on_update + 1\n\
end\n\
");
	scene.SetScript(0, script);

	SceneSyncToSystemsFromAssets(scene, vm);

	SceneClocks clocks;
	for (int i = 0; i < 32; ++i)
		SceneUpdateSystems(scene, clocks, 0, vm);

	TEST_CHECK(LuaObjValue(Get(vm.GetG(), "on_update"), -1) == 32);
}


static void test_LuaScriptSceneOnCreateOnDestroyEventCallback() {
	SceneLuaVM vm;
	TEST_CHECK(Execute(vm.GetL(), "G.on_attach = 0; G.on_destroy = 0;", "bootstrap") == true);

	Scene scene;
	auto script = scene.CreateScript();
	scene.SetScript(0, script);
	vm.OverrideScriptSource(script.ref, "\
function OnAttachToScene(scene)\n\
	G.on_attach = G.on_attach + 1\n\
end\n\
\n\
function OnDestroy()\n\
	G.on_destroy = G.on_destroy + 1\n\
end\n\
");

	SceneSyncToSystemsFromAssets(scene, vm);
	TEST_CHECK(LuaObjValue(Get(vm.GetG(), "on_attach"), -1) == 1);
	scene.SetScript(0, {});
	SceneGarbageCollectSystems(scene, vm);
	TEST_CHECK(LuaObjValue(Get(vm.GetG(), "on_destroy"), -1) == 1);
}

static void test_LuaScriptNodeOnAttachOnDetachEventCallback() {
	SceneLuaVM vm;
	TEST_CHECK(Execute(vm.GetL(), "G.on_attach = 0; G.on_destroy = 0;", "bootstrap") == true);

	Scene scene;
	auto node = scene.CreateNode();
	auto script = scene.CreateScript();
	node.SetScript(0, script);

	vm.OverrideScriptSource(script.ref, "\
function OnAttachToNode(node)\n\
	G.on_attach = G.on_attach + 1\n\
end\n\
\n\
function OnDestroy(node)\n\
	G.on_destroy = G.on_destroy + 1\n\
end\n\
");

	SceneSyncToSystemsFromAssets(scene, vm);
	TEST_CHECK(LuaObjValue(Get(vm.GetG(), "on_attach"), -1) == 1);
	scene.DestroyNode(node);
	SceneGarbageCollectSystems(scene, vm);
	TEST_CHECK(LuaObjValue(Get(vm.GetG(), "on_destroy"), -1) == 1);
}

static void test_LuaScriptNodeOnUpdateEventCallback() {
	Scene scene;
	auto script = scene.CreateScript();
	scene.CreateNode().SetScript(0, script);

	SceneLuaVM vm;
	TEST_CHECK(Execute(vm.GetL(), "G.on_update = 0;", "bootstrap") == true);
	vm.OverrideScriptSource(script.ref, "\
function OnUpdate(node)\n\
	G.on_update = G.on_update + 1\n\
end\n\
");

	SceneSyncToSystemsFromAssets(scene, vm);

	SceneClocks clocks;
	for (int i = 0; i < 32; ++i)
		SceneUpdateSystems(scene, clocks, 0, vm);

	TEST_CHECK(LuaObjValue(Get(vm.GetG(), "on_update"), -1) == 32);
}

static void test_LuaScriptOnDestroyCalledBySceneClear() {
	SceneLuaVM vm;
	TEST_CHECK(Execute(vm.GetL(), "G.on_destroy = 0;", "bootstrap") == true);

	Scene scene;
	auto script = scene.CreateScript();
	vm.OverrideScriptSource(script.ref, "\
function OnDestroy()\n\
	G.on_destroy = G.on_destroy + 1;\n\
end\n\
");

	SceneSyncToSystemsFromAssets(scene, vm);
	SceneClearSystems(scene, vm);

	TEST_CHECK(LuaObjValue(Get(vm.GetG(), "on_destroy"), -1) == 1);
}

static void test_LuaScriptWriteToG() {
	Scene scene;
	const auto script = scene.CreateScript();

	SceneLuaVM vm;
	vm.OverrideScriptSource(script.ref, "G.value = 127");
	SceneSyncToSystemsFromAssets(scene, vm);

	TEST_CHECK(LuaObjValue(Get(vm.GetG(), "value"), -1) == 127);
}

#if HG_ENABLE_BULLET3_SCENE_PHYSICS
static void test_PhysicDynamicRigidBodyFreefall() {
	Scene scene;
	auto sphere = CreatePhysicSphere(scene, 0.5, Mat4::Identity, {}, {}, 1.f);

	SceneBullet3Physics physics;
	SceneSyncToSystemsFromAssets(scene, physics);

	SceneClocks clocks;
	time_ns physics_step = time_from_ms(10);
	int max_physics_step = 64;
	for (int i = 0; i < 16; ++i) {
		SceneUpdateSystems(scene, clocks, time_from_ms(16), physics, physics_step, max_physics_step);
	}
	TEST_CHECK(GetT(sphere.GetTransform().GetWorld()).y < -0.3f);
}

static void test_PhysicKinematicRigidBodyNoFreefall() {
	Scene scene;
	auto sphere = CreatePhysicSphere(scene, 0.5, Mat4::Identity, {}, {}, 1.f);
	sphere.GetRigidBody().SetType(RBT_Kinematic);

	SceneBullet3Physics physics;
	physics.SceneCreatePhysicsFromAssets(scene);

	time_ns dt = time_from_ms(10);
	for (int i = 0; i < 16; ++i) {
		scene.ReadyWorldMatrices();
		physics.StepSimulation(dt);
		physics.SyncTransformsToScene(scene);
	}

	TEST_CHECK(GetT(sphere.GetTransform().GetWorld()).y == 0.f);
}

static void test_PhysicDynamicVsStaticRigidBodyCollisionCallback() {
	Scene scene;
	auto sphere = CreatePhysicSphere(scene, 0.5, TranslationMat4({0, 5, 0}), {}, {}, 1.f);
	auto ground = CreatePhysicCube(scene, {10, 1, 10}, TranslationMat4({0, -0.5f, 0}), {}, {}, 0.f);

	SceneBullet3Physics physics;
	physics.SceneCreatePhysicsFromAssets(scene);
	physics.NodeStartTrackingCollisionEvents(sphere.ref);

	time_ns dt = time_from_ms(10);
	size_t collision_count = 0;
	for (int i = 0; i < 256; ++i) {
		physics.StepSimulation(dt);
		NodePairContacts contacts;
		physics.CollectCollisionEvents(scene, contacts);
		collision_count += contacts.size();
	}

	TEST_CHECK(collision_count > 0);
}

static void test_PhysicKinematicRigidBodyCollideWorld() {
	Scene scene;
	auto sphere = CreatePhysicSphere(scene, 0.5, TranslationMat4({0, 0, 0}), {}, {}, 1.f);
	sphere.GetRigidBody().SetType(RBT_Kinematic);
	auto ground = CreatePhysicCube(scene, {10, 1, 10}, TranslationMat4({0, -0.5f, 0}), {}, {}, 0.f);

	SceneBullet3Physics physics;
	physics.SceneCreatePhysicsFromAssets(scene);

	const auto collisions = physics.NodeCollideWorld(sphere, sphere.GetTransform().GetWorld());
	TEST_CHECK(collisions.size() > 0);
}

static void test_PhysicRaycastFirstHit() {
	Scene scene;
	CreatePhysicCube(scene, {1, 1, 1}, Mat4::Identity, InvalidModelRef, {}, 0.f);
	const auto closest_node = CreatePhysicCube(scene, {.1f, .1f, .1f}, TranslationMat4({0, 0, -1.f}), InvalidModelRef, {}, 0.f);
	CreatePhysicCube(scene, {.1f, .1f, .1f}, TranslationMat4({0, 0, 1.f}), InvalidModelRef, {}, 0.f);

	SceneBullet3Physics physics;
	physics.SceneCreatePhysicsFromAssets(scene);

	const auto out = physics.RaycastFirstHit(scene, {0, 0, -10.f}, {0, 0, 10.f});
	TEST_CHECK(out.node.IsValid() == true);
	TEST_CHECK(out.node == closest_node);
}

static void test_PhysicRaycastFirstHitOutOfReach() {
	Scene scene;
	CreatePhysicCube(scene, {1, 1, 1}, Mat4::Identity, InvalidModelRef, {}, 0.f);
	const auto closest_node = CreatePhysicCube(scene, {.1f, .1f, .1f}, TranslationMat4({0, 0, -1.f}), InvalidModelRef, {}, 0.f);
	CreatePhysicCube(scene, {.1f, .1f, .1f}, TranslationMat4({0, 0, 1.f}), InvalidModelRef, {}, 0.f);

	SceneBullet3Physics physics;
	physics.SceneCreatePhysicsFromAssets(scene);

	const auto out = physics.RaycastFirstHit(scene, {0, 0, -10.f}, {0, 0, -2.f});
	TEST_CHECK(out.node.IsValid() == false);
}

static void test_PhysicRaycastAllHits() {
	Scene scene;
	const auto middle_node = CreatePhysicCube(scene, {1, 1, 1}, Mat4::Identity, InvalidModelRef, {}, 0.f);
	const auto closest_node = CreatePhysicCube(scene, {.1f, .1f, .1f}, TranslationMat4({0, 0, -1.f}), InvalidModelRef, {}, 0.f);
	const auto farthest_node = CreatePhysicCube(scene, {.1f, .1f, .1f}, TranslationMat4({0, 0, 1.f}), InvalidModelRef, {}, 0.f);

	SceneBullet3Physics physics;
	physics.SceneCreatePhysicsFromAssets(scene);

	const auto out = physics.RaycastAllHits(scene, {0, 0, -10.f}, {0, 0, 10.f});
	TEST_CHECK(out.size() == 3);

	// Nodes are not returned in distance order.
	// Here we just check if there's no duplicates/weird result (it should not).
	int found = 0;
	const hg::Node nodes[] = {middle_node, closest_node, farthest_node};
	for (int j = 0; j < 3; j++) {
		for (int i = 0; i < out.size(); i++) {
			if (out[i].node == nodes[j]) {
				found++;
			}
		}
		TEST_CHECK(found == (j + 1));
	}
}

static void test_PhysicRaycastAllHitsOutOfReach() {
	Scene scene;
	CreatePhysicCube(scene, {1, 1, 1}, Mat4::Identity, InvalidModelRef, {}, 0.f);
	CreatePhysicCube(scene, {.1f, .1f, .1f}, TranslationMat4({0, 0, -1.f}), InvalidModelRef, {}, 0.f);
	CreatePhysicCube(scene, {.1f, .1f, .1f}, TranslationMat4({0, 0, 1.f}), InvalidModelRef, {}, 0.f);

	SceneBullet3Physics physics;
	physics.SceneCreatePhysicsFromAssets(scene);

	const auto out = physics.RaycastAllHits(scene, {0, 0, -10.f}, {0, 0, -2.f});
	TEST_CHECK(out.empty() == true);
}
#endif // HG_ENABLE_BULLET3_SCENE_PHYSICS

void test_scene() {
	test_ComponentGarbageCollection();
	test_DuplicateNodes();
	test_WalkHierarchy();
	test_DisableLightNodes();
	test_DisableObjectNodes();
	test_LoadSaveEmptyScene();
	test_LoadSaveEmptySceneBinary();
	test_LoadSaveObject();
	test_LoadSaveObjectBinary();
	test_LoadSaveCamera();
	test_LoadSaveCameraBinary();
	test_LoadSaveLight();
	test_LoadSaveLightBinary();
	test_SceneLuaVM();
	test_LuaScriptSceneOnUpdateEventCallback();
	test_LuaScriptSceneOnCreateOnDestroyEventCallback();
	test_LuaScriptNodeOnAttachOnDetachEventCallback();
	test_LuaScriptNodeOnUpdateEventCallback();
	test_LuaScriptOnDestroyCalledBySceneClear();
	test_LuaScriptWriteToG();
#if HG_ENABLE_BULLET3_SCENE_PHYSICS
	test_PhysicDynamicRigidBodyFreefall();
	test_PhysicKinematicRigidBodyNoFreefall();
	test_PhysicDynamicVsStaticRigidBodyCollisionCallback();
	test_PhysicKinematicRigidBodyCollideWorld();
	test_PhysicRaycastFirstHit();
	test_PhysicRaycastFirstHitOutOfReach();
	test_PhysicRaycastAllHits();
	test_PhysicRaycastAllHitsOutOfReach();
#endif // HG_ENABLE_BULLET3_SCENE_PHYSICS
}
