// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "fabgen.h"

#include "engine/assets.h"
#include "engine/assets_rw_interface.h"
#include "engine/create_geometry.h"
#include "engine/forward_pipeline.h"
#include "engine/fps_controller.h"
#include "engine/meta.h"
#include "engine/physics.h"
#include "engine/scene.h"
#include "engine/scene_forward_pipeline.h"
#include "engine/scene_lua_vm.h"
#include "engine/scene_systems.h"
#if HG_ENABLE_BULLET3_SCENE_PHYSICS
#include "engine/scene_bullet3_physics.h"
#endif

#include "gtest/gtest.h"

#include "bind_Lua.h"

#include "foundation/clock.h"
#include "foundation/data.h"
#include "foundation/data_rw_interface.h"
#include "foundation/file.h"
#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/rand.h"

#include "platform/input_system.h"
#include "platform/window_system.h"
#include <json/json.hpp>

#include <list>
#include <vector>

using namespace hg;

TEST(Scene, SceneRef) {
	Transform trs;

	{
		Scene scene;

		auto node = scene.CreateNode();
		trs = node.GetTransform();

		auto parent = trs.GetParentNode();
		auto parent_trs = parent.GetTransform();

		auto parent_pos = parent_trs.GetPos();
	}

	auto p = trs.GetPos();
}

#if 0
TEST(Scene, RenderBuffers) {
	InputInit();
	WindowSystemInit();

	int res_x = 940;
	int res_y = 720;

	auto win = RenderInit("Capture Scene Forward Pipeline Stages", res_x, res_y, BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X8);

	AddAssetsFolder("d:/tutorials-hg2/resources_compiled");

	// create pipeline
	auto pipeline = CreateForwardPipeline();
	PipelineResources res;

	// load the scene to draw to a texture
	Scene scene;
	LoadSceneContext ctx;
	LoadSceneFromAssets("materials/materials.scn", scene, res, GetForwardPipelineInfo(), ctx);

	// create a forward pipeline render buffers object to capture the output of each pipeline stage
	auto copy_prg = LoadProgramFromAssets("core/shader/copy");
	SceneForwardPipelineRenderBuffers render_buffers(
		res, "RenderBuffers", copy_prg, true, true, true, false, false, false, bgfx::TextureFormat::RGBA8, bgfx::TextureFormat::D32, SFPRBAA_MSAA4x);

	auto pon = LoadTextureFromAssets("textures/squares.png", 0, res);

	//
	auto quad_prg = LoadProgramFromAssets("shaders/texture");
	auto quad_vtx_layout = VertexLayoutPosFloatTexCoord0UInt8();

	Indices quad_idxs = {0, 2, 1, 0, 3, 2};

	Vertices quad_vtxs(quad_vtx_layout, 4);
	quad_vtxs.Begin(0).SetPos(Vec3(-1, -1, 0)).SetTexCoord0(Vec2(0, 0)).End();
	quad_vtxs.Begin(1).SetPos(Vec3(1, -1, 0)).SetTexCoord0(Vec2(1, 0)).End();
	quad_vtxs.Begin(2).SetPos(Vec3(1, 1, 0)).SetTexCoord0(Vec2(1, 1)).End();
	quad_vtxs.Begin(3).SetPos(Vec3(-1, 1, 0)).SetTexCoord0(Vec2(0, 1)).End();

	// main loop
	while (!ReadKeyboard().key[K_Escape]) {
		const auto dt = tick_clock();
		scene.Update(dt);

		bgfx::ViewId view_id = 0;
		SceneForwardPipelinePassViewId view_ids;
		SubmitSceneToPipeline(view_id, scene, iRect(0, 0, res_x, res_y), true, pipeline, res, view_ids, BGFX_INVALID_HANDLE, &render_buffers);

		//
		SetView2D(view_id, res_x, res_y);
		SetTransform(TransformationMat4(Vec3(470, 360, 0), Vec3::Zero, Vec3(100, 100, 1)));

		DrawTriangles(view_id, quad_idxs, quad_vtxs, quad_prg, {MakeUniformSetValue("color", Vec4(1, 1, 1, 1))},
			{MakeUniformSetTexture("s_tex", res.textures.Get(render_buffers.opaque_ref), 0)});

		bgfx::frame();
		UpdateWindow(win);
	}

	RenderShutdown();
	WindowSystemShutdown();
}
#endif

TEST(Scene, MemoryUsage) { DumpSceneMemoryFootprint(); }

TEST(Scene, DisableLightNodes) {
	Scene scene;

	std::vector<Node> lgts;
	for (int i = 0; i < 6; ++i)
		lgts.push_back(CreatePointLight(scene, Mat4::Identity, 1.f));

	std::vector<ForwardPipelineLight> out_lights;

	GetSceneForwardPipelineLights(scene, out_lights);
	EXPECT_EQ(out_lights.size(), 6);

	lgts[0].Disable();
	lgts[2].Disable();
	lgts[4].Disable();

	GetSceneForwardPipelineLights(scene, out_lights);
	EXPECT_EQ(out_lights.size(), 3);

	for (auto lgt : lgts)
		lgt.Enable();

	GetSceneForwardPipelineLights(scene, out_lights);
	EXPECT_EQ(out_lights.size(), 6);

	for (auto lgt : lgts)
		lgt.Disable();

	GetSceneForwardPipelineLights(scene, out_lights);
	EXPECT_EQ(out_lights.size(), 0);
}

TEST(Scene, DisableObjectNodes) {
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
	EXPECT_EQ(opaque.size(), 6);

	objs[0].Disable();
	objs[2].Disable();
	objs[4].Disable();

	scene.GetModelDisplayLists(opaque, transparent, opaque_skinned, transparent_skinned, resources);
	EXPECT_EQ(opaque.size(), 3);

	for (auto &obj : objs)
		obj.Enable();

	scene.GetModelDisplayLists(opaque, transparent, opaque_skinned, transparent_skinned, resources);
	EXPECT_EQ(opaque.size(), 6);

	for (auto &obj : objs)
		obj.Disable();

	scene.GetModelDisplayLists(opaque, transparent, opaque_skinned, transparent_skinned, resources);
	EXPECT_EQ(opaque.size(), 0);
}

#if 0

TEST(Scene, TestJSONSave) {
	auto window = NewWindow(16, 16);
	EXPECT_TRUE(RenderInit(window));

	PipelineResources resources;

	Scene scene;
	LoadSceneFromFile("D:/newproject/tournevis/tournevis_low_ok.scn", scene, resources, "forward");

	json js;
	scene.Save_json(js, resources);

	StringToFile("d:/scene.json", js.dump(1, '\t').c_str());

	const auto v_bson = json::to_bson(js);
	const auto v_cbor = json::to_cbor(js);
	const auto v_msgpack = json::to_msgpack(js);
	const auto v_ubjson = json::to_ubjson(js);

	{
		auto f = fopen("d:/scene.bson", "wb");
		fwrite(v_cbor.data(), 1, v_cbor.size(), f);
		fclose(f);
	}

	{
		auto f = fopen("d:/scene.cbor", "wb");
		fwrite(v_msgpack.data(), 1, v_msgpack.size(), f);
		fclose(f);
	}

	{
		auto f = fopen("d:/scene.msgpck", "wb");
		fwrite(v_msgpack.data(), 1, v_msgpack.size(), f);
		fclose(f);
	}

	{
		auto f = fopen("d:/scene.ubjson", "wb");
		fwrite(v_ubjson.data(), 1, v_ubjson.size(), f);
		fclose(f);
	}

	RenderShutdown();
}

#endif

/* FIXME [EJ07012020] ditch std::set/std::map from BinarySave as it absolutely butchered performance!
TEST(Scene, LoadSaveBench) {
	PipelineResources resources;
	Scene scene;

	auto obj = scene.CreateObject();

	for (int i = 0; i < 360000; ++i) {
		auto node = scene.CreateNode();
		node.SetTransform(scene.CreateTransform());
		node.SetObject(obj);
	}

	Data data;
	data.Reserve(5000000);

	//
	{
		const auto t_start = time_now();
		for (int i = 0; i < 1; ++i) {
			data.Rewind();
			SaveSceneBinaryToData(data, scene, resources);
		}
		std::cout << "Saving scene of 360k object nodes to binary took " << time_to_ms(time_now() - t_start)
				  << "ms, size: " << FormatMemorySize(data.GetSize()) << std::endl;
	}

	//
	{
		const auto t_start = time_now();
		for (int i = 0; i < 1; ++i) {
			data.Rewind();
			SaveSceneJsonToData(data, scene, resources);
		}
		std::cout << "Saving scene of 360k object nodes to json took " << time_to_ms(time_now() - t_start)
				  << "ms, size: " << FormatMemorySize(data.GetSize()) << std::endl;
	}
}
*/

//
#if 0
TEST(Scene, LoadAsync) {
	InputInit();

	auto window = NewWindow(1920, 1080);
	EXPECT_TRUE(RenderInit(window));
	bgfx::reset(1920, 1080, BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X8);

	//
	EXPECT_TRUE(AddAssetsFolder("D:/assemble-demo-assets/cyber_city/assets"));

	{
		PipelineResources resources;

		Scene scene;
		LoadSceneContext ctx;

		const auto now = time_now();
		EXPECT_TRUE(LoadSceneFromAssets("preview_5.scn", scene, resources, GetForwardPipelineInfo(), ctx, LSSF_All | LSSF_QueueResourceLoads));
		std::cout << "Load scenes: " << time_to_ms(time_now() - now) << "ms" << std::endl;

		{
			auto now = time_now();
			while (ProcessLoadQueues(resources) > 0) {
				const auto _now = time_now();
				std::cout << "Process load queues: " << time_to_ms(_now - now) << "ms" << std::endl;
				now = _now;
				bgfx::frame();
				UpdateWindow(window);
			}
		}

		resources.DestroyAll();
	}

	bgfx::frame();

	RenderShutdown();
	InputShutdown();
	DestroyWindow(window);
}
#endif

TEST(Scene, LoadSaveEmptyJson) {
	{
		PipelineResources resources;

		Data data;
		{
			Scene scene;
			EXPECT_TRUE(SaveSceneJsonToData(data, scene, resources));
		}

		data.Rewind();
		{
			Scene scene;
			LoadSceneContext ctx;
			EXPECT_TRUE(LoadSceneJsonFromData(data, "data", scene, g_assets_reader, g_assets_read_provider, resources, GetForwardPipelineInfo(), ctx));
		}
	}
}

TEST(Scene, LoadSaveCameraJson) {
	{
		PipelineResources resources;

		Data data;
		{
			Scene scene;
			for (int i = 0; i < 1000; ++i) {
				auto node = scene.CreateNode("camera");
				node.SetCamera(scene.CreateCamera());
				node.SetTransform(scene.CreateTransform());
			}
			EXPECT_TRUE(SaveSceneJsonToData(data, scene, resources));
		}

		data.Rewind();
		{
			Scene scene;
			LoadSceneContext ctx;
			EXPECT_TRUE(LoadSceneJsonFromData(data, "data", scene, g_assets_reader, g_assets_read_provider, resources, GetForwardPipelineInfo(), ctx));
			EXPECT_EQ(scene.GetNodesWithComponent(NCI_Camera).size(), 1000);
		}
	}
}

TEST(Scene, LoadSaveLightJson) {
	{
		PipelineResources resources;

		Data data;
		{
			Scene scene;
			for (int i = 0; i < 1000; ++i) {
				auto node = scene.CreateNode("light");
				node.SetLight(scene.CreateLight());
				node.SetTransform(scene.CreateTransform());
			}
			EXPECT_TRUE(SaveSceneJsonToData(data, scene, resources));
		}

		data.Rewind();
		{
			Scene scene;
			LoadSceneContext ctx;
			EXPECT_TRUE(LoadSceneJsonFromData(data, "data", scene, g_assets_reader, g_assets_read_provider, resources, GetForwardPipelineInfo(), ctx));
			EXPECT_EQ(scene.GetNodesWithComponent(NCI_Light).size(), 1000);
		}
	}
}

TEST(Scene, LoadSaveObjectJson) {
	{
		PipelineResources resources;

		Data data;
		{
			Scene scene;
			for (int i = 0; i < 1000; ++i) {
				auto node = scene.CreateNode("object");
				node.SetObject(scene.CreateObject());
				node.SetTransform(scene.CreateTransform());
			}
			EXPECT_TRUE(SaveSceneJsonToData(data, scene, resources));
		}

		data.Rewind();
		{
			Scene scene;
			LoadSceneContext ctx;
			EXPECT_TRUE(LoadSceneJsonFromData(data, "data", scene, g_assets_reader, g_assets_read_provider, resources, GetForwardPipelineInfo(), ctx));
			EXPECT_EQ(scene.GetNodesWithComponent(NCI_Object).size(), 1000);
		}
	}
}

//
TEST(Scene, LoadSaveEmptyBinary) {
	{
		PipelineResources resources;

		Data data;
		{
			Scene scene;
			EXPECT_TRUE(SaveSceneBinaryToData(data, scene, resources));
		}

		data.Rewind();
		{
			Scene scene;
			LoadSceneContext ctx;
			EXPECT_TRUE(LoadSceneBinaryFromData(data, "data", scene, g_assets_reader, g_assets_read_provider, resources, GetForwardPipelineInfo(), ctx));
		}
	}
}

TEST(Scene, LoadSaveCameraBinary) {
	{
		PipelineResources resources;

		Data data;
		{
			Scene scene;
			for (int i = 0; i < 1000; ++i) {
				auto node = scene.CreateNode("camera");
				node.SetCamera(scene.CreateCamera());
				node.SetTransform(scene.CreateTransform());
			}
			EXPECT_TRUE(SaveSceneBinaryToData(data, scene, resources));
		}

		data.Rewind();
		{
			Scene scene;
			LoadSceneContext ctx;
			EXPECT_TRUE(LoadSceneBinaryFromData(data, "data", scene, g_assets_reader, g_assets_read_provider, resources, GetForwardPipelineInfo(), ctx));
			EXPECT_EQ(scene.GetNodesWithComponent(NCI_Camera).size(), 1000);
		}
	}
}

TEST(Scene, LoadSaveLightBinary) {
	{
		PipelineResources resources;

		Data data;
		{
			Scene scene;
			for (int i = 0; i < 1000; ++i) {
				auto node = scene.CreateNode("light");
				node.SetLight(scene.CreateLight());
				node.SetTransform(scene.CreateTransform());
			}
			EXPECT_TRUE(SaveSceneBinaryToData(data, scene, resources));
		}

		data.Rewind();
		{
			Scene scene;
			LoadSceneContext ctx;
			EXPECT_TRUE(LoadSceneBinaryFromData(data, "data", scene, g_assets_reader, g_assets_read_provider, resources, GetForwardPipelineInfo(), ctx));
			EXPECT_EQ(scene.GetNodesWithComponent(NCI_Light).size(), 1000);
		}
	}
}

TEST(Scene, LoadSaveObjectBinary) {
	{
		PipelineResources resources;

		Data data;
		{
			Scene scene;
			for (int i = 0; i < 1000; ++i) {
				auto node = scene.CreateNode("object");
				node.SetObject(scene.CreateObject());
				node.SetTransform(scene.CreateTransform());
			}
			EXPECT_TRUE(SaveSceneBinaryToData(data, scene, resources));
		}

		data.Rewind();
		{
			Scene scene;
			LoadSceneContext ctx;
			EXPECT_TRUE(LoadSceneBinaryFromData(data, "data", scene, g_assets_reader, g_assets_read_provider, resources, GetForwardPipelineInfo(), ctx));
			EXPECT_EQ(scene.GetNodesWithComponent(NCI_Object).size(), 1000);
		}
	}
}

//
TEST(Scene, SceneLuaVM) {
	Scene scene;
	const auto node = CreateScript(scene);

	SceneLuaVM vm;
	vm.OverrideScriptSource(node.GetScript(0).ref, "print('Hello SceneLuaVM!')");
	SceneSyncToSystemsFromAssets(scene, vm);

	EXPECT_EQ(vm.GetScriptCount(), 1);
	EXPECT_TRUE(vm.GarbageCollect(scene).empty());

	scene.DestroyNode(node);
	EXPECT_GT(scene.GarbageCollect(), 0);

	const auto script_garbage = vm.GarbageCollect(scene);
	EXPECT_FALSE(script_garbage.empty());

	vm.DestroyScripts(script_garbage);
	EXPECT_EQ(vm.GetScriptCount(), 0);
}

TEST(Scene, LuaScriptSceneOnUpdateEventCallback) {
	SceneLuaVM vm;
	EXPECT_TRUE(Execute(vm.GetL(), "G.on_update = 0;", "bootstrap"));

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

	EXPECT_EQ(LuaObjValue(Get(vm.GetG(), "on_update"), -1), 32);
}

TEST(Scene, LuaScriptSceneOnCreateOnDestroyEventCallback) {
	SceneLuaVM vm;
	EXPECT_TRUE(Execute(vm.GetL(), "G.on_attach = 0; G.on_destroy = 0;", "bootstrap"));

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
	EXPECT_EQ(LuaObjValue(Get(vm.GetG(), "on_attach"), -1), 1);
	scene.SetScript(0, {});
	SceneGarbageCollectSystems(scene, vm);
	EXPECT_EQ(LuaObjValue(Get(vm.GetG(), "on_destroy"), -1), 1);
}

TEST(Scene, LuaScriptNodeOnAttachOnDetachEventCallback) {
	SceneLuaVM vm;
	EXPECT_TRUE(Execute(vm.GetL(), "G.on_attach = 0; G.on_destroy = 0;", "bootstrap"));

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
	EXPECT_EQ(LuaObjValue(Get(vm.GetG(), "on_attach"), -1), 1);
	scene.DestroyNode(node);
	SceneGarbageCollectSystems(scene, vm);
	EXPECT_EQ(LuaObjValue(Get(vm.GetG(), "on_destroy"), -1), 1);
}

//
TEST(Scene, LuaScriptNodeOnUpdateEventCallback) {
	Scene scene;
	auto script = scene.CreateScript();
	scene.CreateNode().SetScript(0, script);

	SceneLuaVM vm;
	EXPECT_TRUE(Execute(vm.GetL(), "G.on_update = 0;", "bootstrap"));
	vm.OverrideScriptSource(script.ref, "\
function OnUpdate(node)\n\
	G.on_update = G.on_update + 1\n\
end\n\
");

	SceneSyncToSystemsFromAssets(scene, vm);

	SceneClocks clocks;
	for (int i = 0; i < 32; ++i)
		SceneUpdateSystems(scene, clocks, 0, vm);

	EXPECT_EQ(LuaObjValue(Get(vm.GetG(), "on_update"), -1), 32);
}

//
TEST(Scene, LuaScriptOnDestroyCalledBySceneClear) {
	SceneLuaVM vm;
	EXPECT_TRUE(Execute(vm.GetL(), "G.on_destroy = 0;", "bootstrap"));

	Scene scene;
	auto script = scene.CreateScript();
	vm.OverrideScriptSource(script.ref, "\
function OnDestroy()\n\
	G.on_destroy = G.on_destroy + 1;\n\
end\n\
");

	SceneSyncToSystemsFromAssets(scene, vm);
	SceneClearSystems(scene, vm);

	EXPECT_EQ(LuaObjValue(Get(vm.GetG(), "on_destroy"), -1), 1);
}

//
TEST(Scene, LuaScriptWriteToG) {
	Scene scene;
	const auto script = scene.CreateScript();

	SceneLuaVM vm;
	vm.OverrideScriptSource(script.ref, "G.value = 127");
	SceneSyncToSystemsFromAssets(scene, vm);

	EXPECT_EQ(LuaObjValue(Get(vm.GetG(), "value"), -1), 127);
}

//
#if HG_ENABLE_BULLET3_SCENE_PHYSICS
TEST(Scene, PhysicDynamicRigidBodyFreefall) {
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
	EXPECT_LT(GetT(sphere.GetTransform().GetWorld()).y, -0.3f);
}

TEST(Scene, PhysicKinematicRigidBodyNoFreefall) {
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

	EXPECT_EQ(GetT(sphere.GetTransform().GetWorld()).y, 0.f);
}

TEST(Scene, PhysicDynamicVsStaticRigidBodyCollisionCallback) {
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

	EXPECT_GT(collision_count, 0);
}

TEST(Scene, PhysicKinematicRigidBodyCollideWorld) {
	Scene scene;
	auto sphere = CreatePhysicSphere(scene, 0.5, TranslationMat4({0, 0, 0}), {}, {}, 1.f);
	sphere.GetRigidBody().SetType(RBT_Kinematic);
	auto ground = CreatePhysicCube(scene, {10, 1, 10}, TranslationMat4({0, -0.5f, 0}), {}, {}, 0.f);

	SceneBullet3Physics physics;
	physics.SceneCreatePhysicsFromAssets(scene);

	const auto collisions = physics.NodeCollideWorld(sphere, sphere.GetTransform().GetWorld());
	EXPECT_GT(collisions.size(), 0);
}

TEST(Scene, PhysicRaycastFirstHit) {
	Scene scene;
	CreatePhysicCube(scene, {1, 1, 1}, Mat4::Identity, InvalidModelRef, {}, 0.f);
	const auto closest_node = CreatePhysicCube(scene, {.1f, .1f, .1f}, TranslationMat4({0, 0, -1.f}), InvalidModelRef, {}, 0.f);
	CreatePhysicCube(scene, {.1f, .1f, .1f}, TranslationMat4({0, 0, 1.f}), InvalidModelRef, {}, 0.f);

	SceneBullet3Physics physics;
	physics.SceneCreatePhysicsFromAssets(scene);

	const auto out = physics.RaycastFirstHit(scene, {0, 0, -10.f}, {0, 0, 10.f});
	EXPECT_TRUE(out.node.IsValid());
	EXPECT_EQ(out.node, closest_node);
}

TEST(Scene, PhysicRaycastFirstHitOutOfReach) {
	Scene scene;
	CreatePhysicCube(scene, {1, 1, 1}, Mat4::Identity, InvalidModelRef, {}, 0.f);
	const auto closest_node = CreatePhysicCube(scene, {.1f, .1f, .1f}, TranslationMat4({0, 0, -1.f}), InvalidModelRef, {}, 0.f);
	CreatePhysicCube(scene, {.1f, .1f, .1f}, TranslationMat4({0, 0, 1.f}), InvalidModelRef, {}, 0.f);

	SceneBullet3Physics physics;
	physics.SceneCreatePhysicsFromAssets(scene);

	const auto out = physics.RaycastFirstHit(scene, {0, 0, -10.f}, {0, 0, -2.f});
	EXPECT_FALSE(out.node.IsValid());
}

TEST(Scene, PhysicRaycastAllHits) {
	Scene scene;
	const auto middle_node = CreatePhysicCube(scene, {1, 1, 1}, Mat4::Identity, InvalidModelRef, {}, 0.f);
	const auto closest_node = CreatePhysicCube(scene, {.1f, .1f, .1f}, TranslationMat4({0, 0, -1.f}), InvalidModelRef, {}, 0.f);
	const auto farthest_node = CreatePhysicCube(scene, {.1f, .1f, .1f}, TranslationMat4({0, 0, 1.f}), InvalidModelRef, {}, 0.f);

	SceneBullet3Physics physics;
	physics.SceneCreatePhysicsFromAssets(scene);

	const auto out = physics.RaycastAllHits(scene, {0, 0, -10.f}, {0, 0, 10.f});
	EXPECT_EQ(out.size(), 3);

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
		EXPECT_EQ(found, j + 1);
	}
}

TEST(Scene, PhysicRaycastAllHitsOutOfReach) {
	Scene scene;
	CreatePhysicCube(scene, {1, 1, 1}, Mat4::Identity, InvalidModelRef, {}, 0.f);
	CreatePhysicCube(scene, {.1f, .1f, .1f}, TranslationMat4({0, 0, -1.f}), InvalidModelRef, {}, 0.f);
	CreatePhysicCube(scene, {.1f, .1f, .1f}, TranslationMat4({0, 0, 1.f}), InvalidModelRef, {}, 0.f);

	SceneBullet3Physics physics;
	physics.SceneCreatePhysicsFromAssets(scene);

	const auto out = physics.RaycastAllHits(scene, {0, 0, -10.f}, {0, 0, -2.f});
	EXPECT_TRUE(out.empty());
}
#endif // HG_ENABLE_BULLET3_SCENE_PHYSICS

//
TEST(Scene, ComponentGarbageCollection) {
	Scene scene;
	const auto transform = scene.CreateTransform();
	EXPECT_TRUE(transform.IsValid());
	EXPECT_EQ(scene.GarbageCollect(), 1);
	EXPECT_FALSE(transform.IsValid());
}

//
TEST(Scene, DuplicateNodes) {
	PipelineResources resources;

	Scene scene;

	const auto lgt = CreatePointLight(scene, Mat4::Identity, 3.5f);
	const auto cam = CreateCamera(scene, Mat4::Identity, 0.25f, 1024.f);
	EXPECT_EQ(scene.GetNodeCount(), 2);

	const auto out = DuplicateNodes(scene, {lgt.ref, cam.ref}, g_assets_reader, g_assets_read_provider, resources, GetForwardPipelineInfo());
	EXPECT_EQ(out.size(), 2);
	EXPECT_EQ(scene.GetNodeCount(), 4);

	const auto lgt_copy = scene.GetNode(out[0]);
	EXPECT_EQ(lgt_copy.GetLight().GetRadius(), 3.5f);

	const auto cam_copy = scene.GetNode(out[1]);
	EXPECT_EQ(cam_copy.GetCamera().GetZFar(), 1024.f);
}

//
TEST(Scene, GetNodeEx_WalkHierarchy) {
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
	EXPECT_EQ(r, a2);

	r = scene.GetNodeEx("root1/a");
	EXPECT_EQ(r, a1);

	r = scene.GetNodeEx("root0/a");
	EXPECT_EQ(r, a0);
}

#if 0

#include "engine/create_geometry.h"
#include "engine/forward_pipeline.h"
#include "engine/scene_forward_pipeline.h"
#include "foundation/clock.h"
#include "foundation/projection.h"
#include "platform/input_system.h"
#include "platform/window_system.h"

#include <bgfx/bgfx.h>

TEST(SceneVisuals, PhysicsVisual) {
	InputInit();

	auto window = NewWindow(1920, 1080);
	EXPECT_TRUE(RenderInit(window));
	bgfx::reset(1920, 1080, BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X8);

	//
	bgfx::VertexLayout vs_decl;
	vs_decl.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float).end();

	//
	PipelineResources resources;

	const auto sphere_ref = resources.models.Add("sphere", CreateSphereModel(vs_decl, 0.5f, 12, 24));
	const auto cube_ref = resources.models.Add("cube", CreateCubeModel(vs_decl, 0.5f, 0.5f, 0.5f));

	//
	const auto prg = LoadPipelineProgramFromFile("d:/harfang/cppsdk/bin/data/gpu/shader/default.hps", resources, GetForwardPipelineInfo());
	Material mat = {resources.programs.Add("cube", prg)};
	SetMaterialValue(mat, "uDiffuseColor", Vec4(0.5f, 0.5f, 0.5f, 1.f));
	SetMaterialValue(mat, "uSpecularColor", Vec4(1.f, 1.f, 1.f, 1.f));
	UpdateMaterialPipelineProgramVariant(mat, resources);

	//
	Scene scene;

	const auto cam_mtx = TransformationMat4({0, 10.f, -18.f}, {Deg(30.f), 0, 0});
	const auto cam = CreateCamera(scene, cam_mtx, 0.01f, 5000.f);

	const auto lgt_mtx = TranslationMat4({0.f, 30.f, -30.f});
	const auto lgt = CreatePointLight(scene, lgt_mtx, 0.f);

	//
	auto mdl_ref = resources.models.Add("ground", CreateCubeModel(vs_decl, 15, 1, 15));
	CreatePhysicCube(scene, {15, 1, 15}, TranslationMat4({0, -.5f, 0}), mdl_ref, {mat}, 0.f);
	mdl_ref = resources.models.Add("wall", CreateCubeModel(vs_decl, 1, 5.5f, 17));
	CreatePhysicCube(scene, {1, 5.5f, 17}, TranslationMat4({-8.f, -.5f, 0}), mdl_ref, {mat}, 0.f);
	CreatePhysicCube(scene, {1, 5.5f, 17}, TranslationMat4({8.f, -.5f, 0}), mdl_ref, {mat}, 0.f);
	mdl_ref = resources.models.Add("wall2", CreateCubeModel(vs_decl, 17, 5.5f, 1));
	CreatePhysicCube(scene, {17, 5.5f, 1}, TranslationMat4({0, -.5f, -8.f}), mdl_ref, {mat}, 0.f);
	CreatePhysicCube(scene, {17, 5.5f, 1}, TranslationMat4({0, -.5f, 8.f}), mdl_ref, {mat}, 0.f);

	scene.SetCurrentCamera(cam);

	//
	SceneClocks clocks;
	SceneNewtonPhysics physics;
	physics.SceneCreatePhysicsFromFile(scene);

	//
	auto pipeline = CreateForwardPipeline();

	std::list<NodeRef> node_refs;

	auto start = time_now();
	for (reset_clock(); (time_now() - start) < time_from_sec(10000);) {
		const auto state = ReadKeyboard();

		if (state.key[K_S]) {
			for (int i = 0; i < 8; ++i) {
				SetMaterialValue(mat, "uDiffuseColor", Vec4(FRand(), FRand(), FRand(), 1.f));

				Node node;
				if (Rand() % 2)
					node = CreatePhysicCube(scene, {0.5f, 0.5f, 0.5f}, TranslationMat4(RandomVec3({-14, 10, -14}, {14, 10, 14})), cube_ref, {mat});
				else
					node = CreatePhysicSphere(scene, 0.5f, TranslationMat4(RandomVec3({-14, 10, -14}, {14, 10, 14})), sphere_ref, {mat});
				physics.NodeCreatePhysicsFromFile(node);

				// scene.StartTrackingNodeDynamicCollisionEvents(node.ref);
				node_refs.push_back(node.ref);
			}

			log(format("%1 nodes").arg(scene.GetNodes().size()));
		} else if (state.key[K_D]) {
			for (int i = 0; i < 8; ++i)
				if (!node_refs.empty()) {
					const auto ref = node_refs.front();
					node_refs.pop_front();
					scene.DestroyNode(ref);
				}

			const auto destroyed_components = scene.GarbageCollect();
			const auto destroyed_physics = physics.GarbageCollect(scene);

			log(format("Destroyed components: %1, destroyed physics: %2").arg(destroyed_components).arg(destroyed_physics));
		} else if (state.key[K_Escape]) {
			break;
		}

		SceneUpdateSystems(scene, clocks, tick_clock(), physics);

		bgfx::ViewId view_id = 0;
		SceneForwardPipelinePassViewId views;
		SubmitSceneToPipeline(view_id, scene, {0, 0, 1920, 1080}, 1.f, pipeline, resources, views);
		bgfx::frame();

		UpdateWindow(window);
	}

	RenderShutdown();
	InputShutdown();
	DestroyWindow(window);
}

#endif

#if 0
TEST(SceneVisuals, ManyDynamic) {
	InputInit();

	auto window = NewWindow(1280, 720);
	EXPECT_TRUE(RenderInit(window));
	//bgfx::reset(1280, 720, BGFX_RESET_MSAA_X8);
	bgfx::setDebug(BGFX_DEBUG_STATS);

	//
	bgfx::VertexLayout vs_decl;
	vs_decl.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float).end();

	//
	PipelineResources resources;
	//const auto sphere_ref = resources.models.Add("sphere", CreateSphereModel(vs_decl, 0.5f, 12, 24));
	const auto cube_ref = resources.models.Add("cube", CreateCubeModel(vs_decl, 0.5f, 0.5f, 0.5f));
	const auto program_ref = LoadPipelineProgramRefFromFile("d:/harfang/cppsdk/bin/data/gpu/shader/default.hps", resources, GetForwardPipelineInfo());

	//
	Scene scene;
	scene.canvas.color = Color::Red;

	Material mat = {program_ref};
	SetMaterialValue(mat, "uDiffuseColor", Vec4(0.5f, 0.5f, 0.5f, 1.f));
	SetMaterialValue(mat, "uSpecularColor", Vec4(1.f, 1.f, 1.f, 1.f));
	UpdateMaterialPipelineProgramVariant(mat, resources);

	for (float z = -100; z < 100; z += 1.f)
		for (float x = -100; x < 100; x += 1.f)
			CreateObject(scene, TranslationMat4({x * 1.5f, 0, z * 1.5f}), cube_ref, {mat});

	Vec3 cam_pos{0, 10.f, -18.f}, cam_rot{Deg(30.f), 0, 0};
	const auto cam = CreateCamera(scene, Mat4::Identity, 0.01f, 5000.f);
	scene.SetCurrentCamera(cam);

	const auto lgt_mtx = TranslationMat4({0.f, 30.f, -30.f});
	const auto lgt = CreateLinearLight(scene, lgt_mtx); //, 0.f);

	//
	auto pipeline = CreateForwardPipeline();

	SceneClocks clocks;

	Keyboard keyboard;
	Mouse mouse;

	while (!keyboard.Pressed(K_Escape)) {
		keyboard.Update();
		mouse.Update();

		const auto dt = tick_clock();

		FpsController(keyboard, mouse, cam_pos, cam_rot, keyboard.Down(K_LShift) ? 40.f : 10.f, dt);
		cam.GetTransform().SetPosRot(cam_pos, cam_rot);

		SceneUpdateSystems(scene, clocks, dt);

		bgfx::ViewId view_id = 0;
		SceneForwardPipelinePassViewId views;
		SubmitSceneToPipeline(view_id, scene, {0, 0, 1280, 720}, 1.f, pipeline, resources, views);
		bgfx::frame();

		UpdateWindow(window);
	}

	RenderShutdown();
	InputShutdown();
	DestroyWindow(window);
}
#endif

#if 0

#include "engine/create_geometry.h"
#include "engine/forward_pipeline.h"
#include "engine/scene_forward_pipeline.h"
#include "foundation/clock.h"
#include "foundation/projection.h"
#include "platform/input_system.h"
#include "platform/window_system.h"

#include <bgfx/bgfx.h>

TEST(SceneVisuals, PhysicsVisual) {
	hg::set_log_level(LL_Normal);

	InputInit();

	auto window = NewWindow(1920, 1080);
	EXPECT_TRUE(RenderInit(window));
	bgfx::reset(1920, 1080, BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X8);

	//
	bgfx::VertexDecl vs_decl;
	vs_decl.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float).end();

	//
	PipelineResources resources;

	const auto sphere_ref = resources.models.Add("sphere", CreateSphereModel(vs_decl, 0.5f, 12.f, 24.f));
	const auto cube_ref = resources.models.Add("cube", CreateCubeModel(vs_decl, 0.5f, 0.5f, 0.5f));

	//
	const auto prg = LoadPipelineProgramFromFile("d:/hg_lua_samples/compiled/default.hps", GetForwardPipelineInfo());
	Material mat = {resources.programs.Add("cube", prg)};
	SetMaterialValue(mat, "uDiffuseColor", Vec4(0.5f, 0.5f, 0.5f, 1.f));
	SetMaterialValue(mat, "uSpecularColor", Vec4(1.f, 1.f, 1.f, 1.f));
	UpdateMaterialPipelineProgramVariant(mat, resources);

	//
	Scene scene;

	const auto cam_mtx = TransformationMat4({0, 20.f, -30.f}, {Deg(30.f), 0, 0});
	const auto cam = CreateCamera(scene, cam_mtx, 0.01f, 5000.f);

	const auto lgt_mtx = TransformationMat4({0.f, 30.f, 0.f}, {Deg(90.f), 0, 0});
	const auto lgt = CreateSpotLight(scene, lgt_mtx, 0.f, Deg(30.f), Deg(35.f), {1, 1, 1}, {1, 1, 1}, 0.f, LST_Map);

	//
	auto mdl_ref = resources.models.Add("ground", CreateCubeModel(vs_decl, 15, 1, 15));
	CreatePhysicCube(scene, {15, 1, 15}, TranslationMat4({0, -.5f, 0}), mdl_ref, {mat}, 0.f);
	mdl_ref = resources.models.Add("wall", CreateCubeModel(vs_decl, 1, 5.5f, 17));
	CreatePhysicCube(scene, {1, 5.5f, 17}, TranslationMat4({-15.5f, -.5f, 0}), mdl_ref, {mat}, 0.f);
	CreatePhysicCube(scene, {1, 5.5f, 17}, TranslationMat4({15.5f, -.5f, 0}), mdl_ref, {mat}, 0.f);
	mdl_ref = resources.models.Add("wall2", CreateCubeModel(vs_decl, 17, 5.5f, 1));
	CreatePhysicCube(scene, {17, 5.5f, 1}, TranslationMat4({0, -.5f, -15.5f}), mdl_ref, {mat}, 0.f);
	CreatePhysicCube(scene, {17, 5.5f, 1}, TranslationMat4({0, -.5f, 15.5f}), mdl_ref, {mat}, 0.f);

	scene.SetCurrentCamera(cam);

	//
	SceneForwardPipelinePassViewId offsets;
	SceneForwardPipelineRenderBuffers render_buffers(LoadProgramFromFile("d:/hg_lua_samples/compiled/copy"), true, true, true, SFPRBAA_MSAA8x);

	//
	auto pipeline = CreateForwardPipeline();

	std::list<NodeRef> node_refs;

	auto start = time_now();
	for (reset_clock(); (time_now() - start) < time_from_sec(10000);) {
		const auto state = ReadKeyboard();

		if (state.key[K_S]) {
			for (int i = 0; i < 8; ++i) {
				SetMaterialValue(mat, "uDiffuseColor", Vec4(FRand(), FRand(), FRand(), 1.f));

				Node node;
				if (Rand() % 2)
					node = CreatePhysicCube(scene, {0.5f, 0.5f, 0.5f}, TranslationMat4(RandomVec3({-14, 10, -14}, {14, 10, 14})), cube_ref, {mat});
				else
					node = CreatePhysicSphere(scene, 0.5f, TranslationMat4(RandomVec3({-14, 10, -14}, {14, 10, 14})), sphere_ref, {mat});

				scene.StartTrackingNodeDynamicCollisionEvents(node.ref);
				node_refs.push_back(node.ref);
			}

			log(format("%1 nodes").arg(scene.GetNodes().size()));
		} else if (state.key[K_D]) {
			for (int i = 0; i < 8; ++i)
				if (!node_refs.empty()) {
					const auto ref = node_refs.front();
					node_refs.pop_front();
					scene.DestroyNode(ref);
				}

			const auto removed_component_count = scene.GarbageCollect();
			log(format("Removed component count: %1").arg(removed_component_count));
		} else if (state.key[K_Escape]) {
			break;
		}

		scene.Update(tick_clock());

		bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
		bgfx::setViewRect(0, 0, 0, 1920, 1080);

		bgfx::ViewId view_id = 0;
		SubmitSceneToPipeline(view_id, scene, {0, 0, 1920, 1080}, true, pipeline, resources, offsets, BGFX_INVALID_HANDLE, &render_buffers, "physic test");

		bgfx::frame();
		UpdateWindow(window);
	}

	RenderShutdown();
	InputShutdown();
	DestroyWindow(window);
}

#endif
