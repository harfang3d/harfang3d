// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/camera.h"
#include "engine/decorator.h"
#include "engine/engine.h"
#include "engine/environment.h"
#include "engine/factories.h"
#include "engine/light.h"
#include "engine/lua_system.h"
#include "engine/node.h"
#include "engine/object.h"
#include "engine/raster_font.h"
#include "engine/renderer.h"
#include "engine/render_geometry.h"
#include "engine/render_system.h"
#include "engine/render_system_async.h"
#include "engine/renderable_system.h"
#include "engine/renderer_async.h"
#include "engine/scene.h"
#include "engine/script.h"
#include "engine/shader.h"
#include "engine/terrain.h"
#include "engine/transform.h"
#include "engine/create_geometry.h"
#include "foundation/base_type_reflection.h"
#include "foundation/filesystem.h"
#include "foundation/foundation_reflection.h"
#include "foundation/std_file_driver.h"
#include "foundation/math.h"
#include "foundation/math_to_string.h"
#include "foundation/matrix4.h"
#include "foundation/random.h"
#include "foundation/task_system.h"
#include "foundation/math.h"
#include "platform/input_system.h"
#include "shared.h"
#include "gtest/gtest.h"

using namespace hg;

struct FpsController {
	FpsController() : pos(Vector3::Zero), rot(Vector3::Zero), speed(10), turbo(20) {}

	void ApplyToNode(Node &node) {
		node.GetComponent<Transform>()->SetPosition(pos);
		node.GetComponent<Transform>()->SetRotation(rot);
	}

	void Update(float dt_sec) {
		auto mouse = g_input_system.get().GetDevice("mouse");
		auto keyboard = g_input_system.get().GetDevice("keyboard");

		auto world = Matrix3FromEuler(rot.x, rot.y, rot.z);
		auto right = GetX(world);
		auto front = GetZ(world);

		Vector3 dt(0, 0, 0);

		if (keyboard->IsDown(KeyUp) || keyboard->IsDown(KeyZ) || keyboard->IsDown(KeyW))
			dt += front;
		else if (keyboard->IsDown(KeyDown) || keyboard->IsDown(KeyS))
			dt -= front;

		if (keyboard->IsDown(KeyLeft) || keyboard->IsDown(KeyQ) || keyboard->IsDown(KeyA))
			dt -= right;
		else if (keyboard->IsDown(KeyRight) || keyboard->IsDown(KeyD))
			dt += right;

		float u_speed = speed;
		if (keyboard->IsDown(KeyLShift))
			u_speed *= turbo;

		pos += dt * dt_sec * u_speed;

		if (mouse->IsButtonDown(Button0)) {
			float dx = mouse->GetDelta(InputAxisX),
				  dy = mouse->GetDelta(InputAxisY);

			rot += Vector3(-dy * 0.005f, dx * 0.005f, 0);
			rot.x = Wrap(rot.x, -Pi, Pi);
		}
	}

	void UpdateAndApplyToNode(Node &node, float dt_sec) {
		Update(dt_sec);
		ApplyToNode(node);
	}

	std::shared_ptr<Node> camera;

	Vector3 pos, rot;
	float speed, turbo;
};

//
#if 1

TEST(DrawGeometryPerf, ManyDynamic) {
	g_task_system.get().create_workers();

	g_fs.get().Mount(std::make_shared<StdFileDriver>());
//	g_fs.get().Mount(std::make_shared<StdFileDriver>(core_resource_path), "@core");

	auto win = NewWindow(1280, 720);

	auto gpu = std::make_shared<RendererAsync>(g_renderer_factory.get().Instantiate());
	gpu->Open().wait();

	auto srf = gpu->NewOutputSurface(win).get();
	gpu->SetOutputSurface(srf);

	auto render_system = std::make_shared<RenderSystemAsync>(std::make_shared<RenderSystem>());
	render_system->Initialize(gpu->GetRenderer()).wait();

	auto scene = std::make_shared<Scene>();

	SceneSetupCoreSystemsAndComponents(scene, render_system->GetRenderSystem());

	//
	auto scene_env = std::make_shared<Environment>();
	scene_env->SetBackgroundColor(hg::Color::Red);
	scene->AddComponent(scene_env);

	//
	auto camera_node = std::make_shared<Node>();
	camera_node->AddComponent(std::make_shared<Transform>());
	camera_node->AddComponent(std::make_shared<Camera>());
	scene->AddNode(camera_node);
	scene->SetCurrentCamera(camera_node);

	FpsController fps;
	fps.camera = camera_node;
	fps.pos = {0, 0, 0};

	//
	auto geo = CreateCube(1, 1, 1);
	auto tree = render_system->CreateGeometry(geo); // render_system->LoadGeometry("D:/gs-experiments/python/pong/res/arcade_room/cube_test_0.geo");

	// 360K
	std::vector<std::shared_ptr<Node>> nodes;
	nodes.reserve(600 * 600);

	for (float z = -300; z < 300; z += 1.f)
		for (float x = -300; x < 300; x += 1.f) {
			auto node = std::make_shared<Node>();
			//node->SetIsStatic(true);
			nodes.push_back(node);

			auto trs = std::make_shared<Transform>();
			trs->SetPosition({x * 2.5f, 0, z * 2.5f});
			node->AddComponent(trs);

			auto obj = std::make_shared<Object>();
			obj->SetGeometry(tree);
			node->AddComponent(obj);

			scene->AddNode(node);
		}

//	RasterFont font("@core/fonts/default.ttf", 12);

	forever {
		fps.UpdateAndApplyToNode(*camera_node, 1.f / 60.f);

		scene->Update();
		scene->WaitUpdate();
		scene->Commit();
		scene->WaitCommit();

//		render_system->DrawRenderSystemStats(font, 15, 400);

		gpu->DrawFrame();
		gpu->ShowFrame();

		UpdateWindow(win);

		g_input_system.get().Update();
	}

	g_fs.get().UnmountAll();
}

#endif

#if 0

static void SpawnLayerDecorators(const Picture &heightmap, Decorator &decorator, float dt, float threshold) {
	log("Spawning decorator layer...");

	for (float z = 0; z < 31235.f; z += dt) {
		for (float x = 0; x < 31235.f; x += dt) {
			float _z = z + FRand(dt);
			float _x = x + FRand(dt);

			int u = (_x / 31235.f) * heightmap.GetWidth();
			int v = (_z / 31235.f) * heightmap.GetHeight();

			float y = heightmap.GetPixelRGBA(u, v).x / 65536.f * 3093.33f;
			float y_h = heightmap.GetPixelRGBA(u + 1, v).x / 65536.f * 3093.33f;
			float y_v = heightmap.GetPixelRGBA(u, v + 1).x / 65536.f * 3093.33f;

			Vector3 a(1, y_h - y, 0), b(0, y_v - y, -1);
			Vector3 n = Cross(a.Normalized(), b.Normalized());

			if (n.y < threshold)
				continue;

			float scale = FRand(0.2f) + 0.8f;
			float rot_y = FRand(2.f * Pi);
			Matrix4 world = Matrix4::TransformationMatrix(Vector3(_x - 31235.f / 2.f, y, _z - 31235.f / 2.f), Vector3(FRand(0.2f) - 0.1f, rot_y, FRand(0.2f) - 0.1f), Vector3(scale, scale, scale));
			decorator.AddInstance(world);
		}
	}

	log("Decorator layer complete.");
}

TEST(DrawGeometryPerf, Mountain) {
	g_fs.get().Mount(std::make_shared<StdFileDriver>());
g_fs.get().Mount(std::make_shared<StdFileDriver>("d:/gs-experiments/py-editor"));
g_fs.get().Mount(std::make_shared<StdFileDriver>("d:/gs-experiments/py-editor/forest"));

	auto renderer = std::make_shared<RendererAsync>(g_renderer_factory.get().Instantiate());
	auto render_system = std::make_shared<RenderSystemAsync>(std::make_shared<RenderSystem>());

	renderer->Open();

	auto win = NewWindow(1920, 800);
	auto srf = renderer->NewOutputSurface(win);
	renderer->SetOutputSurface(srf.get());

	render_system->SetAA(8);
	render_system->Initialize(renderer->GetRenderer()).wait();

	auto scene = std::make_shared<Scene>();
	SceneSetupCoreSystemsAndComponents(scene, render_system->GetRenderSystem());

//	scene->renderable_system->SetFrameRenderer(GetFrameRenderer("OpenVR")); // set OpenVR as the global scene renderer

	//
	auto env = std::make_shared<ScriptEngineEnv>(render_system, renderer, nullptr);

	auto lua_system = std::make_shared<LuaSystem>(env);
	lua_system->Open();
	scene->AddSystem(lua_system);

	//
	auto scene_env = std::make_shared<Environment>();
	scene_env->SetFogNear(3000.f);
	scene_env->SetFogFar(15000.f);
	scene_env->SetFogColor(Color(0.8f, 0.9f, 1.f));
	scene->AddComponent(scene_env);

	//
	auto camera_node = std::make_shared<Node>();
	camera_node->AddComponent(std::make_shared<Transform>());
	camera_node->AddComponent(std::make_shared<Camera>());
	camera_node->GetComponent<Camera>()->SetZNear(1);
	scene->AddNode(camera_node);

	FpsController fps;
	fps.camera = camera_node;
	fps.pos = {0.f, 3200.f, 0.f};
	fps.turbo = 150.f;

	auto script = std::make_shared<LogicScript>();
	script->SetPath("@core/lua/sky_lighting.lua");
	script->Set("time_of_day", MakeTypeValue(16.5f));
	script->Set("attenuation", MakeTypeValue(0.75f));
	script->Set("shadow_range", MakeTypeValue(500.f));
	script->Set("shadow_split", MakeTypeValue(Vector4(0.1, 0.15, 0.3, 1.0)));
	scene->AddComponent(script);

	scene->SetCurrentCamera(camera_node);

	//
	auto terrain_node = std::make_shared<Node>();
	terrain_node->AddComponent(std::make_shared<Transform>());
	auto terrain = std::make_shared<Terrain>();

	terrain->SetSize(Vector3(31235.3f, 3093.33f, 31235.3f));

	terrain->SetHeightmap("d:/hero.r16");
	terrain->SetHeightmapResolution(iVector2(8192, 8192));
	terrain->SetPatchSubdivisionThreshold(20);
	terrain->SetHeightmapBpp(16);

	terrain->SetSurfaceShader("d:/terrain.isl");

	terrain_node->AddComponent(terrain);
	scene->AddNode(terrain_node);

	//
#if 1
	//
	auto tree = render_system->LoadGeometry("d:/gs-experiments/py-editor/forest/trees/tanne-7m/tanne-7m.geo");
	auto treeLOD1 = render_system->LoadGeometry("d:/gs-experiments/py-editor/forest/trees/tanne-7m/tanne-7mLOD1.geo");
	auto treeLOD2 = render_system->LoadGeometry("d:/gs-experiments/py-editor/forest/trees/tanne-7m/tanne-7mLOD2.geo");

	tree->lod_proxy = treeLOD1;
	tree->lod_distance = 20;
	treeLOD1->lod_proxy = treeLOD2;
	treeLOD1->lod_distance = 80;

	auto bright_round = render_system->LoadGeometry("d:/gs-experiments/py-editor/forest/trees/bright-roundA/bright-roundA.geo");
	auto bright_roundLOD1 = render_system->LoadGeometry("d:/gs-experiments/py-editor/forest/trees/bright-roundA/bright-roundALOD1.geo");
	auto bright_roundLOD2 = render_system->LoadGeometry("d:/gs-experiments/py-editor/forest/trees/bright-roundA/bright-roundALOD2.geo");
	auto bright_roundLOD3 = render_system->LoadGeometry("d:/gs-experiments/py-editor/forest/trees/bright-roundA/bright-roundALOD3.geo");

	bright_round->lod_proxy = bright_roundLOD1;
	bright_round->lod_distance = 20;
	bright_roundLOD1->lod_proxy = bright_roundLOD2;
	bright_roundLOD1->lod_distance = 40;
	bright_roundLOD2->lod_proxy = bright_roundLOD3;
	bright_roundLOD2->lod_distance = 60;

	auto shiver_green = render_system->LoadGeometry("d:/gs-experiments/py-editor/forest/trees/shiver_green/shiver_green.geo");
	auto shiver_greenLOD1 = render_system->LoadGeometry("d:/gs-experiments/py-editor/forest/trees/shiver_green/shiver_greenLOD1.geo");
	auto shiver_greenLOD2 = render_system->LoadGeometry("d:/gs-experiments/py-editor/forest/trees/shiver_green/shiver_greenLOD2.geo");

	shiver_green->lod_proxy = shiver_greenLOD1;
	shiver_green->lod_distance = 20;
	shiver_greenLOD1->lod_proxy = shiver_greenLOD2;
	shiver_greenLOD1->lod_distance = 40;

	//
#define KK 1.f

	Picture heightmap;
	if (LoadHeightmap(heightmap, "d:/hero.r16", 8192, 8192, 16)) {
		auto tree_decorator = std::make_shared<Decorator>();
		tree_decorator->SetGeometry(tree);
		tree_decorator->SetRange(2000); // 2km
		tree_decorator->SetDecimationRangeStart(500); // decimate from 500m
		SpawnLayerDecorators(heightmap, *tree_decorator, 5.f * KK, 0.96f);

		auto bright_round_decorator = std::make_shared<Decorator>();
		bright_round_decorator->SetGeometry(bright_round);
		bright_round_decorator->SetRange(600);
		bright_round_decorator->SetDecimationRangeStart(300);
		SpawnLayerDecorators(heightmap, *bright_round_decorator, 15.f * KK, 0.9f);

		auto shiver_green_decorator = std::make_shared<Decorator>();
		shiver_green_decorator->SetGeometry(shiver_green);
		shiver_green_decorator->SetRange(800);
		shiver_green_decorator->SetDecimationRangeStart(400);
		SpawnLayerDecorators(heightmap, *shiver_green_decorator, 30.f * KK, 0.8f);

		scene->AddComponent(tree_decorator);
		scene->AddComponent(bright_round_decorator);
		scene->AddComponent(shiver_green_decorator);
	}
#endif

	//
	auto font = std::make_shared<RasterFont>("@core/fonts/default.ttf", 12);

	while (!g_input_system.get().GetDevice("keyboard")->WasPressed(KeyEscape)) {
		fps.UpdateAndApplyToNode(*camera_node, 1.f / 60.f);

		scene->Update();
		scene->WaitUpdate();
		scene->Commit();
		scene->WaitCommit();

//		render_system->DrawRenderSystemStats(font, 15, 400);

		renderer->DrawFrame();
		renderer->ShowFrame();
//		renderer->Commit();

		UpdateWindow(win);

		g_input_system.get().Update();
	}

	g_fs.get().UnmountAll();
}

#endif
