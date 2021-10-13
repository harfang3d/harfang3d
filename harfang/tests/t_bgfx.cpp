// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "default_data_path.h"
#include "engine/assets.h"
#include "engine/create_geometry.h"
#include "engine/forward_pipeline.h"
#include "engine/fps_controller.h"
#include "engine/model_builder.h"
#include "engine/render_pipeline.h"
#include "engine/scene.h"
#include "engine/scene_forward_pipeline.h"
#include "foundation/clock.h"
#include "foundation/file_rw_interface.h"
#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/projection.h"
#include "foundation/ref_counted_vector_list.h"
#include "foundation/time_chrono.h"
#include "foundation/vector_list.h"
#include "platform/input_system.h"
#include "platform/window_system.h"
#include "gtest/gtest.h"

#include <thread>

using namespace hg;

TEST(BGFX, InitAndShutdown) {
	auto window = NewWindow(1280, 720);
	EXPECT_TRUE(RenderInit(window));

	RenderShutdown();
	DestroyWindow(window);
}

TEST(Scene, Lifetime_AccessingUninitializedNodeDoesNotCrash) {
	Node node;
	EXPECT_EQ(node.GetTransform().ref.idx, 0xffffffff);
	EXPECT_EQ(node.GetCamera().ref.idx, 0xffffffff);
	EXPECT_EQ(node.GetObject().ref.idx, 0xffffffff);
	EXPECT_EQ(node.GetLight().ref.idx, 0xffffffff);
}

TEST(Scene, Lifetime_AccessingHangingNodeDoesNotCrashAfterSceneDeletion) {
	Node node;
	{
		Scene scene;
		node = scene.NewNode();
		node.SetTransform(scene.NewTransform());
		EXPECT_NE(node.GetTransform().ref.idx, 0xffffffff);
	}
	EXPECT_EQ(node.GetTransform().ref.idx, 0xffffffff);
}

TEST(Scene, Lifetime_AccessingUninitializedComponentDoesNotCrash) {
	Transform trs;
	trs.SetPos({1, 2, 3}); // silently fails
	EXPECT_EQ(trs.GetPos(), Vec3(0, 0, 0));
}

#if 1

TEST(BGFX, LoadProgram) {
	auto window = NewWindow(1280, 720);
	EXPECT_TRUE(RenderInit(window));

	auto program = LoadProgram(g_file_reader, ScopedReadHandle(g_file_read_provider, "d:/bgfx_test/dst/vs_gouraud.bin"),
		ScopedReadHandle(g_file_read_provider, "d:/bgfx_test/dst/fs_gouraud.bin"));

	bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x30ff30ff, 1.0f, 0);

	for (int i = 0; i < 16; ++i) {
		bgfx::setViewRect(0, 0, 0, 1280, 720);
		bgfx::touch(0);
		bgfx::frame();
	}

	RenderShutdown();
	DestroyWindow(window);
}

//
TEST(BGFX, RenderModelNoPipeline) {
	InputInit();

	auto window = NewWindow(1280, 720);
	EXPECT_TRUE(RenderInit(window));

	//
	bgfx::VertexDecl vs_decl;

	vs_decl.begin();
	vs_decl.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
	vs_decl.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float);
	vs_decl.end();

	const auto cube_mdl = CreateCubeModel(vs_decl, 0.3f, 0.3f, 0.3f);

	//
	const auto prg = LoadProgramFromFile("C:/ProgramData/Assemble/assets/shaders/default_mdl.vsb", "C:/ProgramData/Assemble/assets/shaders/default_mdl.fsb");

	//
	reset_clock();

	for (float a = 0.f;; a += time_to_sec_f(tick_clock())) {
		bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
		bgfx::setViewRect(0, 0, 0, 1280, 720);

		//
		const auto cam = TransformationMat4({0.f, 4.f, -15.f}, {0.f, 0.f, 0.f});

		const auto view = InverseFast(cam);
		const auto proj = ComputePerspectiveProjectionMatrix(0.01f, 5000.f, 1.8f, {1280.f / 720.f, 1.f});

		bgfx::setViewTransform(0, to_bgfx(view).data(), to_bgfx(proj).data());

		//
		for (float x = -10; x < 10; x += 1)
			for (float z = -10; z < 10; z += 1)
				RenderModel(0, cube_mdl, prg, [](uint16_t) {}, TransformationMat4({x, 0.f, z}, {0.f, a, 0.f}));

		//
		bgfx::frame();
		UpdateWindow(window);
	}

	RenderShutdown();
	DestroyWindow(window);
}

//
TEST(BGFX, SubmitModelToForwardPipeline) {
	InputInit();

	auto window = NewWindow(1280, 720);
	EXPECT_TRUE(RenderInit(window));

	//
	bgfx::VertexDecl vs_decl;

	vs_decl.begin();
	vs_decl.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
	vs_decl.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float);
	vs_decl.end();

	const auto cube_mdl = CreateCubeModel(vs_decl, 0.3f, 0.3f, 0.3f);
	const auto cube_prg = LoadPipelineProgramVariantFromFile("e:/hg_bgfx_lua/compiled/default", "forward");

	//
	ForwardPipeline pipeline;

	//
	reset_clock();

	bgfx::UniformHandle u_diffuse_color = bgfx::createUniform("uDiffuseColor", bgfx::UniformType::Vec4),
						u_specular_color = bgfx::createUniform("uSpecularColor", bgfx::UniformType::Vec4);

	for (float a = 0.f;; a += time_to_sec_f(tick_clock())) {
		bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
		bgfx::setViewRect(0, 0, 0, 1280, 720);

		const auto cam = TransformationMat4({0.f, 4.f, -15.f}, {Deg(20.f), 0.f, 0.f});

		const auto view = InverseFast(cam);
		const auto proj = ComputePerspectiveProjectionMatrix(0.01f, 5000.f, 1.8f, {1280.f / 720.f, 1.f});

		bgfx::setViewTransform(0, to_bgfx(view).data(), to_bgfx(proj).data());

		const auto lights = PrepareForwardPipelineLights({
			MakeForwardPipelinePointLight(TranslationMat4({sin(a) * 5.f, 1.f, cos(a) * 5.f}), {0.f, 0.8f, 1.f}, {0.f, 0.8f, 1.f}, 8.f),
			MakeForwardPipelinePointLight(
				TranslationMat4({sin(a + Deg(120.f)) * 5.f, 1.f, cos(a + Deg(120.f)) * 5.f}), {1.f, 0.8f, 0.f}, {1.f, 0.8f, 0.f}, 8.f),
			MakeForwardPipelinePointLight(
				TranslationMat4({sin(a + Deg(240.f)) * 5.f, 1.f, cos(a + Deg(240.f)) * 5.f}), {1.f, 0.2f, 0.8f}, {1.f, 0.2f, 0.8f}, 8.f),
		});

		for (float x = -10; x <= 10; x += 1)
			for (float z = -10; z <= 10; z += 1)
				SubmitModelToForwardPipeline(0, cube_mdl, pipeline, cube_prg, 0, Color::Black, lights, {0, 0, Color::Black},
					[&](int) {
						const float v[4] = {0.5f, 0.5f, 0.5f, 1.f};
						bgfx::setUniform(u_diffuse_color, v);
						const float w[4] = {1.f, 1.f, 1.f, 0.3f};
						bgfx::setUniform(u_specular_color, w);
					},
					TransformationMat4({x, 0.f, z}, {0.f, 0.f, 0.f}));

		bgfx::frame();
		UpdateWindow(window);
	}

	RenderShutdown();
	DestroyWindow(window);
}

//
TEST(BGFX, AddModelToSceneAndSubmitToForwardPipeline) {
	InputInit();

	auto window = NewWindow(1280, 720);
	EXPECT_TRUE(RenderInit(window));

	//
	bgfx::VertexDecl vs_decl;
	vs_decl.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float).end();

	//
	PipelineResources resources;

	const auto cube_mdl = CreateCubeModel(vs_decl, 0.3f, 0.3f, 0.3f);
	const auto cube_prg = LoadPipelineProgramVariantFromFile("d:/hg_bgfx_lua/compiled/default", "forward");

	Material mat = {resources.programs.Add("cube", cube_prg)};
	SetMaterialValue(mat, "uDiffuseColor", 0.5f, 0.5f, 0.5f, 1.f);
	SetMaterialValue(mat, "uSpecularColor", 1.f, 1.f, 1.f, 1.f);

	const auto cube_ref = resources.models.Add("cube", cube_mdl);

	//
	Scene scene;

	const auto cam_mtx = hg::TranslationMat4({0, 0.f, -5.f});
	const auto cam = AddCamera(scene, cam_mtx, 0.01f, 5000.f);
	const auto obj_mtx = hg::TranslationMat4({0.f, 0.f, 0.f});
	const auto obj = AddObject(scene, obj_mtx, cube_ref, {mat});
	const auto lgt_mtx = hg::TranslationMat4({0.f, 3.f, -3.f});
	const auto lgt = AddPointLight(scene, lgt_mtx, 0.f);

	scene.SetCurrentCamera(cam);

	//
	ForwardPipeline pipeline;

	for (reset_clock();;) {
		scene.Update(tick_clock());

		bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
		bgfx::setViewRect(0, 0, 0, 1280, 720);

		SubmitSceneToPipeline(0, scene, ComputeAspectRatioX(1280, 720), pipeline, resources);

		bgfx::frame();
		UpdateWindow(window);
	}

	RenderShutdown();
	DestroyWindow(window);
}

//
TEST(BGFX, CreateGeometryFromCode) {
	InputInit();

	auto window = NewWindow(1280, 720);
	EXPECT_TRUE(RenderInit(window));

	//
	PipelineResources res;

	//
	bgfx::VertexDecl vs_decl;

	vs_decl.begin();
	vs_decl.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
	vs_decl.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float);
	vs_decl.end();

	ResourceRef cube_model = res.models.Add("cube", CreateCubeModel(vs_decl, 0.1f, 0.1f, 0.1f));

	//
	Material default_mat = {
		res.programs.Add("missing",
			LoadPipelineProgramVariantFromFile("C:/ProgramData/Assemble/projects/d6d59c08-d806-47d2-f54c-6e65cd489188/default/core/shader/missing", "forward")),
	};

	//
	auto pipeline_uniforms = CreateForwardPipelineUniforms();

	//
	Vec3 pos{}, rot{};

	std::vector<ModelDisplayList> display_lists;
	std::vector<Mat4> worlds;

	Mouse mouse;
	Keyboard keyboard;

	time_ns t = time_now();
	for (float a = 0.f;;) {
		time_ns t_now = time_now();
		time_ns dt = t_now - t;
		t = t_now;

		//
		bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
		bgfx::setViewRect(0, 0, 0, 1280, 720);

		//
		mouse.Update();
		keyboard.Update();

		FpsController(keyboard, mouse, pos, rot, 1.f, dt);

		auto cam = TransformationMat4(pos, rot);

		auto view = InverseFast(cam);
		auto _view = to_bgfx(view);

		auto proj = ComputePerspectiveProjectionMatrix(0.01f, 5000.f, 1.8f, {1280.f / 720.f, 1.f});
		auto _proj = to_bgfx(proj);

		bgfx::setViewTransform(0, _view.data(), _proj.data());

		//
		const auto frustum = MakeFrustum(proj, cam);

		//
		std::vector<ForwardPipelineLight> lights(forward_light_count);

		lights[0].world = TransformationMat4({0, 0, 0}, {Deg(70.f), 0, 0});
		lights[0].diffuse = {1, 0, 0.2f};
		lights[0].specular = {1, 0, 0.2f};

		lights[1].world = TransformationMat4({0, 0, 5.f}, {0, 0, 0});
		lights[1].diffuse = {0, 0, 0.5f};
		lights[1].specular = {0.1f, 0.2f, 0.75f};

		lights[2].world = TransformationMat4({0, -2, 5.f}, {0, 0, 0});
		lights[2].diffuse = {1, 0.75f, 0};
		lights[2].specular = {1, 0.75f, 0};
		lights[2].radius = 2.f;

		lights[3].world = TransformationMat4({0, 0.25f, 0}, {0, 0, 0});
		lights[3].diffuse = {0, 0.6f, 0};
		lights[3].specular = {0, 1, 0};
		lights[3].inner_angle = Deg(10.f);
		lights[3].outer_angle = Deg(20.f);

		const auto pipe_lights = PrepareForwardPipelineLights(lights);

		//
#if 1
		display_lists.reserve(360000);
		display_lists.clear();

		worlds.reserve(360000);
		worlds.clear();

		for (float x = -300; x < 300; x += 1)
			for (float z = -300; z < 300; z += 1) {
				worlds.push_back(TransformationMat4({x * 1, 0, z * 1}, {0, a, 0}));
				display_lists.push_back({&default_mat, uint32_t(worlds.size() - 1), uint16_t(cube_model.idx), 0});
			}

		auto t_start = time_now();
		CullModelDisplayLists(frustum, display_lists, worlds, res);

		log(format("Cull timing: %1, COUNT: %2, memory_usage(display_lists): %3")
				.arg(time_to_ms(time_now() - t_start))
				.arg(display_lists.size())
				.arg(display_lists.size() * sizeof(ModelDisplayList)));

		DrawModelDisplayLists(0, display_lists, 0,
			[&](uint16_t) {
				UpdateForwardPipelineUniforms(pipeline_uniforms, Color::Black, pipe_lights, {5, 50, {0.1f, 0.1f, 0.1f}});
			},
			worlds, res);
#else
		for (float x = -10; x < 10; x += 1) {
			for (float z = -10; z < 10; z += 1) {
				auto _mdl = to_bgfx(TransformationMat4({x * 10 + 0.f, -4.f, z * 10 + 8.f}, {0, a, 0}));
				RenderDisplayLists(0, model.lists.data(), model.lists.size(), bound_mat, _mdl.data(), 1);
			}
		}
#endif

		//
		bgfx::frame();

		UpdateWindow(window);

		a += time_to_sec_f(dt) * 0.25f;
	}

	DestroyUniforms(pipeline_uniforms);

	RenderShutdown();
	DestroyWindow(window);
}

//
TEST(BGFX, PBRTest) {
	static const int w = 1024 * 2, h = 1024 * 2;

	InputInit();

	auto window = NewWindow(w, h);
	EXPECT_TRUE(RenderInit(window));
	bgfx::reset(w, h, BGFX_RESET_MSAA_X8);

	//
	PipelineResources res;

	//
	bgfx::VertexDecl vs_decl;

	vs_decl.begin();
	vs_decl.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
	vs_decl.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float);
	vs_decl.end();

	const auto model = res.models.Add("sphere", CreateSphereModel(vs_decl, 0.45f, 48, 64));

	//
	Scene scene;

	scene.ReserveNodes(100);
	scene.ReserveTransforms(100);
	scene.ReserveObjects(100);

	for (float x = 0; x < 7; ++x)
		for (float y = 0; y < 7; ++y) {
			auto node = scene.NewNode("sphere");
			node.SetTransform(scene.NewTransform({x - 3.f, y - 3.f, 0.f}));

			Material mat;
			SetMaterialProgram(mat, LoadPipelineProgramVariantFromFile(
										"C:/ProgramData/Assemble/projects/d6d59c08-d806-47d2-f54c-6e65cd489188/default/core/shader/default_", res, "forward"));
			SetMaterialValue(mat, "uDiffuseColor", 0.5f, 0.5f, 0.5f, y / 6.f);
			SetMaterialValue(mat, "uSpecularColor", 0.5f, 0.5f, 0.5f, x / 6.f);

			auto object = scene.NewObject(model, {mat});
			node.SetObject(object);
		}

	auto cam = scene.NewNode("camera");
	cam.SetTransform(scene.NewTransform({0.f, 0.f, -10.f}));
	cam.SetCamera(scene.NewCamera(0.01, 1000.f, Deg(45.f)));

	scene.SetCurrentCamera(cam.ref);

	scene.environment.irradiance_map = LoadTextureFromFile("d:/cliffDiffuseHDR.dds", BGFX_SAMPLER_NONE, res);
	scene.environment.radiance_map = LoadTextureFromFile("d:/cliffSpecularHDR.dds", BGFX_SAMPLER_MIN_ANISOTROPIC | BGFX_SAMPLER_MAG_ANISOTROPIC, res);
	scene.environment.brdf_map = LoadTextureFromFile("d:/cliffBrdf.dds", BGFX_SAMPLER_NONE, res);

	//
	auto pipeline_uniforms = CreateForwardPipelineUniforms();

	//
	std::vector<ModelDisplayList> display_lists;

	std::vector<ForwardPipelineLight> lights = {
		{ForwardPipelineLight::Model::Point, TranslationMat4({-15, 15, -10.f}), {1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}},
	};

	//
	Mouse mouse;
	Keyboard keyboard;

	reset_clock();

	while (true) {
		time_ns dt = tick_clock();

		auto TRS = cam.GetTransform().GetTRS();

		mouse.Update();
		keyboard.Update();
		FpsController(keyboard, mouse, TRS.pos, TRS.rot, 5.f, dt);

		cam.GetTransform().SetTRS(TRS);

		//
		scene.Update(0);

		bgfx::setViewRect(0, 0, 0, w, h);
		bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x505050ff, 1.0f, 0);

		//		void GetSceneForwardPipelineLights(const Scene &scene, const ViewState &view_state, std::vector<ForwardPipelineLight> &out_lights);

		const auto view_state = scene.ComputeCurrentCameraViewState(ComputeAspectRatioX(w, h));
		SubmitSceneToForwardPipeline(0, scene, view_state, pipeline_uniforms, display_lists, lights, res);

		bgfx::touch(0);

		bgfx::frame();

		UpdateWindow(window);
	}

	DestroyUniforms(pipeline_uniforms);

	RenderShutdown();
	DestroyWindow(window);
}

TEST(BGFX, CreateScene) {
	DumpSceneMemoryFootprint();

	// init display
	auto window = NewWindow(1280, 720);
	EXPECT_TRUE(RenderInit(window));
	bgfx::reset(1280, 720, BGFX_RESET_MSAA_X8 | BGFX_RESET_VSYNC);

	PipelineResources resources;

	// load model
	ResourceRef model;

	// setup material
	Material mat = {LoadPipelineProgramVariantFromFile(
		"C:/ProgramData/Assemble/projects/7b7a41ad-7dab-4c77-9af1-130a72ed362d/default/core/shader/default_", resources, "forward")};

	// create scene
	Scene scene;

	time_ns t_start = time_now();
#if 1
	t_start = time_now();
	auto object = scene.NewObject(model, {mat});

	static const int K = 100;

	scene.ReserveNodes(K * K * 4);
	scene.ReserveTransforms(K * K * 4);
	scene.ReserveObjects(K * K * 4);

	for (float x = -K; x < K; x += 1)
		for (float z = -K; z < K; z += 1) {
			auto node = scene.NewNode();
			node.SetTransform(scene.NewTransform({x, 0, z}));
			node.SetObject(object);
		}

	//
	auto node = scene.NewNode();
	node.SetTransform(scene.NewTransform({0, 0, 0}, {Deg(70.f), 0, 0}));
	node.SetLight(scene.NewLinearLight(0.5f, {1, 0, 0.2f}, {1, 0, 0.2f}));

	node = scene.NewNode();
	node.SetTransform(scene.NewTransform({0, 0, 5.f}));
	node.SetLight(scene.NewPointLight(0, {0, 0, 0.5f}, {0.1f, 0.2f, 0.75f}));

	node = scene.NewNode();
	node.SetTransform(scene.NewTransform({0, -2.f, 5.f}));
	node.SetLight(scene.NewPointLight(2.f, {1, 0.75f, 0}, {1, 0.75f, 0}));

	node = scene.NewNode();
	node.SetTransform(scene.NewTransform({0, 0.25f, 0}));
	node.SetLight(scene.NewSpotLight(0, Deg(10.f), Deg(20.f), {0, 0.6f, 0}, {0, 1, 0}));

	//
	auto nodes = scene.GetNodes();

	//
	auto camera = scene.NewNode();
	camera.SetTransform(scene.NewTransform());
	camera.SetCamera(scene.NewCamera());
	//	camera.SetLuaScript(0, scene.NewLuaScript("d:/bgfx_test/dst/script.lua"));

	scene.SetCurrentCamera(camera.ref);

	log(format("Create scene from code: %1 ms").arg(time_to_ms(time_now() - t_start)));
#endif

	Data data;
#if 0
	{
		t_start = time_now();
		scene.Save(resources, data);
		log(format("scene.Save: %1 ms").arg(time_to_ms(time_now() - t_start)));

//		g_fs.get().FileSave("d:/360k_nodes.scn", data);
		data.SetCursor(0);

		scene.Clear();

		//	g_fs.get().FileLoad("d:/360k_nodes.scn", data);
		t_start = time_now();
		scene.Load(itf, resources, data);
		log(format("scene.Load: %1 ms").arg(time_to_ms(time_now() - t_start)));
	}
#endif

	//
	ForwardPipeline pipeline;

	//
	Vec3 pos{}, rot{};
	time_ns t = time_now();

	std::vector<ModelDisplayList> display_lists;
	std::vector<ForwardPipelineLight> lights;

	Mouse mouse;
	Keyboard keyboard;

	for (float a = 0.f;;) {
		time_ns t_now = time_now();
		time_ns dt = t_now - t;
		t = t_now;

		//
		auto TRS = camera.GetTransform().GetTRS();

		mouse.Update();
		keyboard.Update();
		FpsController(keyboard, mouse, TRS.pos, TRS.rot, 5.f, dt);

		scene.GetCurrentCamera().GetTransform().SetTRS(TRS);

		//
		/*
				for (auto &node : nodes) {
					auto trs = node.GetTransform();
					auto pos = trs.GetPos();
					pos.y = sin(a + pos.x * 0.1) * cos(a + pos.z * 0.1) * 2.f;
					trs.SetPos(pos);
				}
		*/
		//
		scene.Update(dt);

		auto view_state = scene.ComputeCurrentCameraViewState(ComputeAspectRatioX(1280, 720));
		GetSceneForwardPipelineLights(scene, view_state, lights);

		//
		bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
		bgfx::setViewRect(0, 0, 0, 1280, 720);
		SubmitSceneToForwardPipeline(0, scene, view_state, pipeline.uniforms, display_lists, lights, resources);
		bgfx::touch(0);
		/*
				bgfx::setViewClear(1, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
				bgfx::setViewRect(1, 640, 0, 640, 720);
				ResubmitSceneToForwardPipeline(1, scene, view_state, display_lists, resources);
				bgfx::touch(1);
		*/
		//
		bgfx::frame();

		UpdateWindow(window);

		a += time_to_sec_f(dt) * 0.75f;
	}

	DestroyUniforms(pipeline.uniforms);

	RenderShutdown();
	DestroyWindow(window);
}

TEST(BGFX, LoadScene) {
	hg::InputInit();

	const bool stereo = false;
	const int w = 1280, h = 720;

	const int k_w = stereo ? 2 : 1;

	// init display
	auto window = NewWindow(w * k_w, h);
	EXPECT_TRUE(RenderInit(window));
	bgfx::reset(w * k_w, h, BGFX_RESET_MSAA_X8);

	// assets folder
	AddAssetsFolder("C:/ProgramData/Assemble/projects/d6d59c08-d806-47d2-f54c-6e65cd489188/default");

	// declare pipeline
	PipelineResources resources;
	ForwardPipeline pipeline;

	// resources.materials.SetDefault(BindMaterial(g_file_reader, g_file_read_provider, {"d:/bgfx_test/dst/default"}, resources, "forward"));

	// load scene
	Scene scene;
	//	EXPECT_TRUE(LoadSceneFromAssets("bot/quadruperd_bot.scn", scene, resources, "forward"));
	EXPECT_TRUE(LoadSceneFromAssets("cour_V4/cour_V4.scn", scene, resources, "forward"));
	//	EXPECT_TRUE(LoadSceneFromAssets("SunTemple/SunTemple.scn", scene, resources, "forward"));

	DumpSceneMemoryFootprint();

	scene.environment.irradiance_map = LoadTextureFromFile("d:/cliffDiffuseHDR.dds", BGFX_SAMPLER_NONE, resources);
	scene.environment.radiance_map = LoadTextureFromFile("d:/cliffSpecularHDR.dds", BGFX_SAMPLER_MIN_ANISOTROPIC | BGFX_SAMPLER_MAG_ANISOTROPIC, resources);
	scene.environment.brdf_map = LoadTextureFromFile("d:/cliffBrdf.dds", BGFX_SAMPLER_NONE, resources);

	// add default nodes
	auto camera = scene.NewNode();
	camera.SetTransform(scene.NewTransform());
	camera.SetCamera(scene.NewCamera(0.1f, 500.f, Deg(45.f)));
	scene.SetCurrentCamera(camera.ref);

#if 0
	auto light = scene.NewNode();
	light.SetTransform(scene.NewTransform({0, 0, 0}, {Deg(45.f), 0, 0}));
	light.SetLight(scene.NewLinearLight(0.5f));
#endif

#if 1
	for (auto &l : scene.GetNodesWithComponent(Scene::NodeLightIdx)) {
		l.GetLight().SetDiffuseColor({0, 0, 0});
		l.GetLight().SetSpecularColor({0.f, 0.f, 0.f});
	}
#endif

	//
	std::vector<ModelDisplayList> display_lists;
	std::vector<ForwardPipelineLight> lights;

	Mouse mouse;
	Keyboard keyboard;

	hg::reset_clock();

	while (true) {
		const auto dt = hg::tick_clock();

		//
		auto TRS = camera.GetTransform().GetTRS();

		mouse.Update();
		keyboard.Update();
		FpsController(keyboard, mouse, TRS.pos, TRS.rot, 4.f, dt);

		camera.GetTransform().SetTRS(TRS);

		//
		scene.Update(dt);

		const auto view_state = scene.ComputeCurrentCameraViewState(ComputeAspectRatioX(w, h));
		GetSceneForwardPipelineLights(scene, view_state, lights);

		//
		bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
		bgfx::setViewRect(0, 0, 0, w, h);
		SubmitSceneToForwardPipeline(0, scene, view_state, pipeline.uniforms, display_lists, lights, resources);
		bgfx::touch(0);

		if (stereo) {
			bgfx::setViewClear(1, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
			bgfx::setViewRect(1, w, 0, w, h);
			SubmitSceneToForwardPipeline(1, scene, view_state, pipeline.uniforms, display_lists, lights, resources);
			bgfx::touch(1);
		}

		bgfx::frame();

		UpdateWindow(window);
	}

	DestroyUniforms(pipeline.uniforms);

	RenderShutdown();
	DestroyWindow(window);
}

#endif
