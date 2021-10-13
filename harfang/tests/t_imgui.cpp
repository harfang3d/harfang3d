// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/dear_imgui.h"
#include "engine/render_pipeline.h"

#include "foundation/file_rw_interface.h"

#include "platform/input_system.h"
#include "platform/window_system.h"

#include "shared.h"
#include <gtest/gtest.h>

using namespace hg;

TEST(ImGui, BasicWindow) {
	auto win = NewWindow(1280, 720);
	EXPECT_TRUE(RenderInit(win));

	ImGuiInit(16, LoadProgramFromFile(GetResPath("gpu/shader/imgui").c_str()), LoadProgramFromFile(GetResPath("gpu/shader/imgui_image").c_str()));

	for (size_t i = 0; i < 160; ++i) {
		ImGuiBeginFrame(1280, 720, time_from_ms(16), {}, {});

		ImGui::SetNextWindowPosCenter(ImGuiCond_Appearing);
		ImGui::SetNextWindowSize({320, 240}, ImGuiCond_Appearing);
		ImGui::Begin("Window");
		ImGui::Text("dear imgui, basic window unit test...");
		ImGui::End();

		ImGuiEndFrame();

		bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
		bgfx::touch(0);
		bgfx::frame();

		UpdateWindow(win);
	}

	ImGuiShutdown();

	RenderShutdown();
	DestroyWindow(win);
}
