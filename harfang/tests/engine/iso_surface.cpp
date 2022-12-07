// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/assets.h"
#include "engine/iso_surface.h"
#include "engine/model_builder.h"
#include "engine/render_pipeline.h"
#include "foundation/clock.h"
#include "foundation/math.h"
#include "foundation/projection.h"
#include "foundation/unit.h"
#include "platform/window_system.h"
#include "shared.h"
#include "gtest/gtest.h"

using namespace hg;

TEST(IsoSurface, RenderModelNoPipeline) {
	auto window = NewWindow(1280, 720);
	EXPECT_TRUE(RenderInit(window));

	//
	const auto prg = LoadProgramFromFile(GetResPath("gpu/shader/default_mdl.vsb").c_str(), GetResPath("gpu/shader/default_mdl.fsb").c_str());

	//
	bgfx::VertexLayout vtx_layout;

	vtx_layout.begin();
	vtx_layout.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
	vtx_layout.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float);
	vtx_layout.end();

	//
	static const int cell_count = 64;
	auto surface = NewIsoSurface(cell_count, cell_count, cell_count);

	//
	reset_clock();

	for (float a = 0.f; a < 4.f; a += time_to_sec_f(tick_clock())) {
		//
		const auto cam = TransformationMat4({cell_count / 2.f, cell_count / 2.f, -cell_count / 2.f}, {0.f, 0.f, 0.f});
		hg::SetViewPerspective(0, 0, 0, 1280, 720, cam, 0.01f, 5000.f, 1.8f, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, hg::ColorFromABGR32(0x303030ff));

		//
		std::fill(std::begin(surface), std::end(surface), 0.f);

		for (int i = 0; i < 16; ++i) {
			float k = float(i) * Deg(12.f) + a;

			float x = (Sin(k * 0.4f) * Cos(k * -1.5f) * 0.75f + 1.f) * cell_count / 2.f;
			float y = (Cos(k * -1.2f) * Cos(k * 2.f) * 0.75f + 1.f) * cell_count / 2.f;
			float z = (Sin(k * 1.7f) * Cos(k * -0.8f) * 0.75f + 1.f) * cell_count / 2.f;

			IsoSurfaceSphere(surface, cell_count, cell_count, cell_count, x, y, z, (Sin(k * 3.f + float(i) * Deg(8.f)) * 0.5f + 1.f) * 8.f);
		}

		surface = GaussianBlurIsoSurface(surface, cell_count, cell_count, cell_count);

		//
		ModelBuilder builder;

		if (IsoSurfaceToModel(builder, surface, cell_count, cell_count, cell_count)) {
			auto mdl = builder.MakeModel(vtx_layout);
			DrawModel(0, mdl, prg, {}, {}, &Mat4::Identity);
			Destroy(mdl);
		}

		//
		bgfx::frame();
		UpdateWindow(window);
	}

	RenderShutdown();
	DestroyWindow(window);
}
