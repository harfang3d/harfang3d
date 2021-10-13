// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include <math.h>

#include <engine/assets.h>
#include <engine/bloom.h>
#include <engine/create_geometry.h>
#include <engine/dear_imgui.h>
#include <engine/forward_pipeline.h>
#include <engine/fps_controller.h>
#include <engine/render_pipeline.h>
#include <engine/sao.h>
#include <engine/scene.h>
#include <engine/scene_forward_pipeline.h>

#include <foundation/clock.h>
#include <foundation/format.h>
#include <foundation/log.h>
#include <foundation/matrix3.h>
#include <foundation/projection.h>
#include <foundation/unit.h>

#include <platform/input_system.h>
#include <platform/window_system.h>

#include "shared.h"
#include <gtest/gtest.h>

/*
TEST(Filters, Bloom) {
	int width = 1280;
	int height = 720;
	
	hg::InputInit();

	auto win = hg::NewWindow(width, height);
	EXPECT_TRUE(hg::RenderInit(win));
	bgfx::reset(width, height, BGFX_RESET_VSYNC);

	hg::Texture input = hg::LoadTextureFromFile(GetResPath("pic/triangle.png").c_str(), BGFX_SAMPLER_NONE);

	hg::Bloom bloom = hg::CreateBloomFromFile(GetResPath("gpu/shader").c_str(), width, height);

	float threshold = 0.5f;
	float smoothness = 1.0f;
	float intensity = 1.0f;

#if 0
	hg::Keyboard keyboard;
	while (!keyboard.Pressed(hg::K_Escape)) {
		keyboard.Update();
#else
	for (int i = 0; i < 240; i++) {
#endif
		bgfx::ViewId view_id = 0;

		bgfx::setViewClear(view_id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x8030c0ff, 1.0f, 0);
		bgfx::touch(view_id);

		hg::ApplyBloom(view_id, hg::MakeRectFromWidthHeight(384, 104, 512, 512), input, BGFX_INVALID_HANDLE, bloom, threshold,
			smoothness, intensity);

		bgfx::frame();

		hg::UpdateWindow(win);
	}

	hg::DestroyBloom(bloom);

	hg::RenderShutdown();
	hg::DestroyWindow(win);
}

TEST(Filters, BloomBackBufferRatio) {
	int width = 1280;
	int height = 720;

	auto win = hg::NewWindow(width, height);
	EXPECT_TRUE(hg::RenderInit(win));
	bgfx::reset(width, height, BGFX_RESET_VSYNC);

	hg::Texture input = hg::LoadTextureFromFile(GetResPath("pic/owl.jpg").c_str(), BGFX_SAMPLER_NONE);

	hg::Bloom bloom = hg::CreateBloomFromFile(GetResPath("gpu/shader").c_str(), bgfx::BackbufferRatio::Half);

	float threshold = 0.6f;
	float smoothness = 0.8f;
	float intensity = 1.0f;

	for (int i = 0; i < 240; i++) {
		bgfx::ViewId view_id = 0;

		bgfx::setViewClear(view_id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
		bgfx::touch(view_id);

		hg::ApplyBloom(view_id, hg::MakeRectFromWidthHeight(164, 80, 260, 240), input, BGFX_INVALID_HANDLE, bloom, threshold, smoothness, intensity);

		bgfx::frame();

		hg::UpdateWindow(win);
	}

	hg::DestroyBloom(bloom);

	hg::RenderShutdown();
	hg::DestroyWindow(win);
}
*/

#if 0
TEST(Filters, SAOBackBufferRatio) {
	int width = 1280;
	int height = 760;

	hg::InputInit();

	auto resources = hg::PipelineResources();

	auto win = hg::NewWindow(width, height);
	EXPECT_TRUE(hg::RenderInit(win));
	bgfx::reset(width, height, BGFX_RESET_VSYNC);

	hg::ImGuiInit(16, hg::LoadProgramFromFile(GetResPath("gpu/shader/imgui").c_str()), hg::LoadProgramFromFile(GetResPath("gpu/shader/imgui_image").c_str()));

	bgfx::VertexLayout vs_decl;
	vs_decl.begin();
	{
		vs_decl.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
		vs_decl.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Uint8, true, true);
	}
	vs_decl.end();

	hg::Model cube_mdl = hg::CreateCubeModel(vs_decl, 1.f, 1.f, 1.f);
	hg::ModelRef cube_ref = resources.models.Add("cube", cube_mdl);
	hg::Model ground_mdl = hg::CreateCubeModel(vs_decl, 50.f, 0.01f, 50.0f);
	hg::ModelRef ground_ref = resources.models.Add("ground", ground_mdl);

	hg::PipelineProgram prg = hg::LoadPipelineProgramFromFile(GetResPath("gpu/shader/default.hps").c_str(), resources, hg::GetForwardPipelineInfo());
	hg::PipelineProgramRef prg_ref = resources.programs.Add("default shader", prg);

	hg::Material mat_cube, mat_ground;
	hg::SetMaterialProgram(mat_cube, prg_ref);
	hg::SetMaterialValue(mat_cube, "uDiffuseColor", hg::Vec4(0.65f, 0.25f, 0.35f, 1.f));
	hg::SetMaterialValue(mat_cube, "uSpecularColor", hg::Vec4(0.2f, 0.2f, 0.2f, 1.f));
	hg::SetMaterialValue(mat_cube, "uSelfColor", hg::Vec4(0.f, 0.f, 0.f, 1.f));

	hg::SetMaterialProgram(mat_ground, prg_ref);
	hg::SetMaterialValue(mat_ground, "uDiffuseColor", hg::Vec4(0.5f, 0.5f, 0.5f, 1.f));
	hg::SetMaterialValue(mat_ground, "uSpecularColor", hg::Vec4(0.1f, 0.1f, 0.1f, 1.f));
	hg::SetMaterialValue(mat_ground, "uSelfColor", hg::Vec4(0.f, 0.f, 0.f, 1.f));

	hg::Scene scene;
	scene.canvas.color = hg::Color(0.13f, 0.16f, 0.16f, 1.f);

	hg::Node cam = hg::CreateCamera(scene, hg::TransformationMat4(hg::Vec3(0.f, 1.f, -10.f), hg::Vec3(hg::Deg(30.f), 0.f, 0.f)), 0.1f, 100.f);
	scene.SetCurrentCamera(cam);

	hg::Node lgt = hg::CreatePointLight(scene, hg::TranslationMat4(hg::Vec3(2.0f, 2.f, -2.f)), 0.f);

	std::vector<hg::Material> mat_list(1);
	mat_list[0] = mat_cube;
	hg::CreateObject(scene, hg::TranslationMat4(hg::Vec3(0.f, 0.5f, 0.f)), cube_ref, mat_list);
	hg::CreateObject(
		scene, hg::TransformationMat4(hg::Vec3(1.2f, 0.25f, 0.f), hg::Vec3(0.f, hg::Deg(15.f), 0.f), hg::Vec3(0.5f, 0.5f, 0.5f)), cube_ref, mat_list);
	mat_list[0] = mat_ground;
	hg::CreateObject(scene, hg::TranslationMat4(hg::Vec3::Zero), ground_ref, mat_list);
	mat_list.clear();

	hg::ForwardPipeline pipeline = hg::CreateForwardPipeline();

	hg::Vec3 cam_pos(2.f, 1.f, -4.f);
	hg::Vec3 cam_rot(0.f, 0.f, 0.f);

	bgfx::FrameBufferHandle fb =
		bgfx::createFrameBuffer(width, height, bgfx::TextureFormat::RGBA8, 0 | BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);

	bgfx::ProgramHandle copy_prg = hg::LoadProgramFromFile(GetResPath("gpu/shader/copy").c_str());

	hg::SceneForwardPipelineRenderBuffers render_buffers(resources, "depth capture", copy_prg, true);

	hg::Keyboard keyboard;
	hg::Mouse mouse;

	hg::Vec2 aspect_ratio = hg::ComputeAspectRatioX((float)width, (float)height);

	float bias = 0.08f;
	float radius = 5.0f;

	int sample_count = 20;
	float sharpness = 0.10f;

	hg::SAO sao = hg::CreateSAO(GetResPath("gpu/shader").c_str());

	hg::reset_clock();

#if 0
	while (!keyboard.Pressed(hg::K_Escape))
#else
	for (int i = 0; i < 240; i++)
#endif
	{
		keyboard.Update();
		mouse.Update();

		hg::time_ns dt = hg::tick_clock();

		hg::ImGuiBeginFrame(width, height, hg::time_from_ms(16), mouse.GetState(), keyboard.GetState());
		{
			ImGui::SetNextWindowPosCenter(ImGuiCond_Appearing);
			ImGui::Begin("SAO parameters");
			ImGui::DragInt("sample count", &sample_count, 1);
			ImGui::DragFloat("radius", &radius, 0.1f);
			ImGui::DragFloat("bias", &bias, 0.1f);
			ImGui::DragFloat("sharpness", &sharpness, 0.1f);
			ImGui::End();
		}
		hg::ImGuiEndFrame();

		if (!ImGui::GetIO().WantCaptureMouse)
			hg::FpsController(keyboard, mouse, cam_pos, cam_rot, 1, dt);

		cam.GetTransform().SetPos(cam_pos);
		cam.GetTransform().SetRot(cam_rot);

		scene.ReadyWorldMatrices();
		scene.ComputeWorldMatrices();

		hg::ViewState view_state = scene.ComputeCurrentCameraViewState(aspect_ratio);

		bgfx::ViewId view_id = 0;

		bgfx::setViewClear(view_id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
		bgfx::touch(view_id);

		hg::SceneForwardPipelinePassViewId views;
		hg::SubmitSceneToPipeline(view_id, scene, hg::MakeRectFromWidthHeight(0, 0, width, height), true, pipeline, resources, views, fb, &render_buffers);

		scene.ReadyWorldMatrices();
		scene.ComputeWorldMatrices();

		hg::ApplySAO(view_id, render_buffers.internal_depth, BGFX_INVALID_HANDLE, sao, view_state.proj, bias, radius, sample_count, sharpness);

		bgfx::frame();

		hg::UpdateWindow(win);
	}

	hg::DestroySAO(sao);

	hg::DestroyForwardPipeline(pipeline);

	hg::RenderShutdown();
	hg::DestroyWindow(win);
}
#endif