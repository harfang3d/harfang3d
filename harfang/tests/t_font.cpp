// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/font.h"
#include "engine/render_pipeline.h"

#include "platform/input_system.h"
#include "platform/window_system.h"

#include "foundation/file_rw_interface.h"
#include "foundation/matrix4.h"
#include "foundation/projection.h"
#include "foundation/unit.h"

#include <bx/math.h>

#include "shared.h"
#include <gtest/gtest.h>

using namespace hg;

TEST(Font, LoadFromFile) {
	auto win = NewWindow(64, 64);
	EXPECT_TRUE(RenderInit(win));

	auto valid = LoadFontFromFile(GetResPath("ttf/Cabin-Regular.ttf").c_str());
	EXPECT_FALSE(valid.pages.empty());
	auto invalid = LoadFontFromFile(GetResPath("ttf/invalid.ttf").c_str());
	EXPECT_TRUE(invalid.pages.empty());

	RenderShutdown();
	DestroyWindow(win);
}

TEST(Font, Write) {
	int width = 1280;
	int height = 720;
	bgfx::ViewId view_id = 0;

	auto win = NewWindow(width, height);
	EXPECT_TRUE(RenderInit(win));
	bgfx::reset(width, height, BGFX_RESET_VSYNC);

	auto font = LoadFontFromFile(GetResPath("ttf/Cabin-Regular.ttf").c_str(), 24.f);

	bgfx::ProgramHandle program = LoadProgramFromFile(GetResPath("gpu/shader/font").c_str());
	
	std::string text = "1973 � Adam Heller Proposed the lithium thionyl chloride battery, still used in implanted medical devices and in defense systems where "
					   "greater than a "
		"20-year shelf life, high energy density, or extreme operating temperatures are encountered.[33]\n"
		"1977 � Samar Basu demonstrated electrochemical intercalation of lithium in graphite at the University of Pennsylvania.[34][35] This led to the "
		"development of a workable lithium intercalated graphite electrode at Bell Labs (LiC\n"
		"6)[36] to provide an alternative to the lithium metal electrode battery.\n"
		"Invention and development\n"
		"1979 � Working in separate groups, Ned A. Godshall et al.,[37][38][39] and the following year in 1980 and John Goodenough and Koichi Mizushima, both "
		"demonstrated a rechargeable lithium cell with voltage in the 4 V range using lithium cobalt dioxide (LiCoO\n"
		"2) as the positive electrode and lithium metal as the negative electrode.[40][41] This innovation provided the positive electrode material that "
		"enabled early commercial lithium batteries. LiCoO\n"
		"2 is a stable positive electrode material which acts as a donor of lithium ions, which means that it can be used with a negative electrode material "
		"other than lithium metal.[42] By enabling the use of stable and easy-to-handle negative electrode materials, LiCoO\n"
		"2 enabled novel rechargeable battery systems. Godshall et al. further identified the similar value of ternary compound lithium-transition "
		"metal-oxides such as the spinel LiMn2O4, Li2MnO3, LiMnO2, LiFeO2, LiFe5O8, and LiFe5O4 (and later lithium-copper-oxide and lithium-nickel-oxide "
		"cathode materials in 1985)[43][43]\n"
		"1980 � Rachid Yazami demonstrated the reversible electrochemical intercalation of lithium in graphite.[44][45] The organic electrolytes available at "
		"the time would decompose during charging with a graphite negative electrode. Yazami used a solid electrolyte to demonstrate that lithium could be "
		"reversibly intercalated in graphite through an electrochemical mechanism. (As of 2011, Yazami's graphite electrode was the most commonly used "
		"electrode in commercial Lion batteries).\n"
		"The negative electrode has its origins in PAS (polyacenic semiconductive material) discovered by Tokio Yamabe and later by Shjzukuni Yata in the "
		"early 1980s.[46][47][48][49] The seed of this technology was the discovery of conductive polymers by Professor Hideki Shirakawa and his group, and it "
		"could also be seen as having started from the polyacetylene lithium ion battery developed by Alan MacDiarmid and Alan J. Heeger et al.[50]\n"
		"1982 � Godshall et al. were awarded U.S. Patent 4,340,652[51] for the use of LiCoO2 as cathodes in lithium batteries, based on Godshall's Stanford "
		"University Ph.D. dissertation and 1979 publications.\n"
		"1983 � Michael M. Thackeray, Peter Bruce, William David, and John B. Goodenough developed a manganese spinel as a commercially relevant charged "
		"cathode material for lithium-ion batteries.[52]\n"
		"1985 � Akira Yoshino assembled a prototype cell using carbonaceous material into which lithium ions could be inserted as one electrode, and lithium "
		"cobalt oxide (LiCoO\n"
		"2) as the other.[53] This dramatically improved safety. LiCoO\n"
		"2 enabled industrial-scale production and enabled the commercial lithium-ion battery.\n"
		"1989 � Goodenough and Arumugam Manthiram showed that positive electrodes containing polyanions, e.g., sulfates, produce higher voltages than oxides "
		"due to the induction effect of the polyanion.[54]\n"
		"1996 � Akshaya Padhi, KS Nanjundawamy and Goodenough identified LiFePO4 (LFP) as a cathode material.[55]\n"
		"1998 � C. S. Johnson, J. T. Vaughey, M. M. Thackeray, T. E. Bofinger, and S. A. Hackney report the discovery of the high capacity high voltage "
		"lithium-rich NMC cathode materials.[56]\n"
		"2005 � Y Song, PY Zavalij, and M. Stanley Whittingham report a new two-electron vanadium phosphate cathode material with high energy density "
		"[57][58]\n"
		"2012 � John Goodenough, Rachid Yazami and Akira Yoshino received the 2012 IEEE Medal for Environmental and Safety Technologies for developing the "
		"lithium ion battery.[10]\n"
		"2016 � Z. Qi, and Gary Koenig reported a scalable method to produce sub-micrometer sized LiCoO\n"
		"2 using a template-based approach.[59]\n"
		"Commercial production\n"
		"The performance and capacity of lithium-ion batteries increased as development progressed.";

	auto caps = bgfx::getCaps();
	auto render_state = hg::ComputeRenderState(BM_Alpha, DT_Always, FC_Clockwise, false);
	auto mtx = hg::TransformationMat4({0.f, 32.f, 0.f}, {0.f, 0.f, Deg(0 * 0.01f)}, {1.f, 1.f, 1.f});

	for (int i = 0; i < 160; i++) {
		bgfx::setViewName(view_id, "Font test");
		bgfx::setViewMode(view_id, bgfx::ViewMode::Sequential);

		hg::SetView2D(view_id, 0, 0, width, height, -10.f, 10.f, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, hg::ColorFromABGR32(0x303030ff));

		hg::DrawText(view_id, font, text.c_str(), program, "u_tex", 0, mtx, {0.f, 0.f, 0.f}, DTHA_Left, DTVA_Top,
			{ hg::MakeUniformSetValue("u_color", hg::Vec4(0.f, 0.f, 1.f, 1.f)) }, {}, render_state);

		bgfx::touch(view_id);
		bgfx::frame();

		const auto rect = ComputeTextRect(font, text.c_str());

		hg::UpdateWindow(win);
	}

	hg::RenderShutdown();
	hg::DestroyWindow(win);
}