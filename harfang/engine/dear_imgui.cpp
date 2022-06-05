// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/dear_imgui.h"
#include "engine/render_pipeline.h"

#include "foundation/log.h"

#include "platform/input_system.h"

#include <bx/allocator.h>
#include <bx/math.h>
#include <bx/timer.h>

#include <cinttypes>
#include <memory>

namespace hg {

#define IMGUI_FLAGS_NONE UINT8_C(0x00)
#define IMGUI_FLAGS_ALPHA_BLEND UINT8_C(0x01)

static std::string input_buffer;
static Signal<void(const char *)>::Connection on_text_input_connection;

typedef union {
	ImTextureID ptr;
	struct {
		bgfx::TextureHandle handle;
		uint8_t flags;
		uint8_t mip;
	} s;
} TextureBridge;

static bool checkAvailTransientBuffers(uint32_t _numVertices, const bgfx::VertexLayout &_decl, uint32_t _numIndices) {
	return _numVertices == bgfx::getAvailTransientVertexBuffer(_numVertices, _decl) &&
		   (0 == _numIndices || _numIndices == bgfx::getAvailTransientIndexBuffer(_numIndices));
}

//
static std::unique_ptr<DearImguiContext> g_context;

DearImguiContext *GetGlobalContext() { return g_context.get(); }

static bool CheckImGuiIsInitialized() {
	if (!g_context) {
		error("ImGui is not initialized, call ImGuiInit to initialize it");
		return false;
	}
	return true;
}

//
static void ImGuiRender(const DearImguiContext &ctx, bgfx::ViewId m_viewId, ImDrawData *_drawData) {
#if 0
	const ImGuiIO &io = ImGui::GetIO();
	const float width = io.DisplaySize.x;
	const float height = io.DisplaySize.y;

	bgfx::setViewName(viewId, "ImGui");
	bgfx::setViewMode(viewId, bgfx::ViewMode::Sequential);

	const bgfx::Caps *caps = bgfx::getCaps();
	{
		float ortho[16];
		bx::mtxOrtho(ortho, 0.0f, width, height, 0.0f, 0.0f, 1000.0f, 0.0f, caps->homogeneousDepth);
		bgfx::setViewTransform(viewId, NULL, ortho);
		bgfx::setViewRect(viewId, 0, 0, uint16_t(width), uint16_t(height));
	}

	// Render command lists
	for (int32_t ii = 0, num = _drawData->CmdListsCount; ii < num; ++ii) {
		bgfx::TransientVertexBuffer tvb;
		bgfx::TransientIndexBuffer tib;

		const ImDrawList *drawList = _drawData->CmdLists[ii];
		uint32_t numVertices = (uint32_t)drawList->VtxBuffer.size();
		uint32_t numIndices = (uint32_t)drawList->IdxBuffer.size();

		if (!checkAvailTransientBuffers(numVertices, ctx.m_decl, numIndices)) {
			// not enough space in transient buffer just quit drawing the rest...
			break;
		}

		bgfx::allocTransientVertexBuffer(&tvb, numVertices, ctx.m_decl);
		bgfx::allocTransientIndexBuffer(&tib, numIndices);

		ImDrawVert *verts = (ImDrawVert *)tvb.data;
		bx::memCopy(verts, drawList->VtxBuffer.begin(), numVertices * sizeof(ImDrawVert));

		ImDrawIdx *indices = (ImDrawIdx *)tib.data;
		bx::memCopy(indices, drawList->IdxBuffer.begin(), numIndices * sizeof(ImDrawIdx));

		uint32_t offset = 0;
		for (const ImDrawCmd *cmd = drawList->CmdBuffer.begin(), *cmdEnd = drawList->CmdBuffer.end(); cmd != cmdEnd; ++cmd) {
			if (cmd->UserCallback) {
				cmd->UserCallback(drawList, cmd);
			} else if (0 != cmd->ElemCount) {
				uint64_t state = 0 | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA;

				bgfx::TextureHandle th = ctx.m_texture;
				bgfx::ProgramHandle program = ctx.m_program;

				if (NULL != cmd->TextureId) {
					TextureBridge texture = {cmd->TextureId};
					state |= 0 != (IMGUI_FLAGS_ALPHA_BLEND & texture.s.flags)
								 ? BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA)
								 : BGFX_STATE_NONE;
					th = texture.s.handle;
					if (0 != texture.s.mip) {
						const float lodEnabled[4] = {float(texture.s.mip), 1.0f, 0.0f, 0.0f};
						bgfx::setUniform(ctx.u_imageLodEnabled, lodEnabled);
						program = ctx.m_imageProgram;
					}
				} else {
					state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
				}

				const uint16_t xx = uint16_t(bx::max(cmd->ClipRect.x, 0.0f));
				const uint16_t yy = uint16_t(bx::max(cmd->ClipRect.y, 0.0f));
				bgfx::setScissor(xx, yy, uint16_t(bx::min(cmd->ClipRect.z, 65535.0f) - xx), uint16_t(bx::min(cmd->ClipRect.w, 65535.0f) - yy));

				bgfx::setState(state);
				bgfx::setTexture(0, ctx.s_tex, th);
				bgfx::setVertexBuffer(0, &tvb, 0, numVertices);
				bgfx::setIndexBuffer(&tib, offset, cmd->ElemCount);
				bgfx::submit(viewId, program);
			}

			offset += cmd->ElemCount;
		}
	}
#else
	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	int fb_width = (int)(_drawData->DisplaySize.x * _drawData->FramebufferScale.x);
	int fb_height = (int)(_drawData->DisplaySize.y * _drawData->FramebufferScale.y);
	if (fb_width <= 0 || fb_height <= 0)
		return;

	bgfx::setViewName(m_viewId, "ImGui");
	bgfx::setViewMode(m_viewId, bgfx::ViewMode::Sequential);

	const bgfx::Caps *caps = bgfx::getCaps();
	{
		float ortho[16];
		float x = _drawData->DisplayPos.x;
		float y = _drawData->DisplayPos.y;
		float width = _drawData->DisplaySize.x;
		float height = _drawData->DisplaySize.y;

		bx::mtxOrtho(ortho, x, x + width, y + height, y, 0.0f, 1000.0f, 0.0f, caps->homogeneousDepth);
		bgfx::setViewTransform(m_viewId, NULL, ortho);
		bgfx::setViewRect(m_viewId, 0, 0, uint16_t(width), uint16_t(height));
	}

	const ImVec2 clipPos = _drawData->DisplayPos; // (0,0) unless using multi-viewports
	const ImVec2 clipScale = _drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

	// Render command lists
	for (int32_t ii = 0, num = _drawData->CmdListsCount; ii < num; ++ii) {
		bgfx::TransientVertexBuffer tvb;
		bgfx::TransientIndexBuffer tib;

		const ImDrawList *drawList = _drawData->CmdLists[ii];
		uint32_t numVertices = (uint32_t)drawList->VtxBuffer.size();
		uint32_t numIndices = (uint32_t)drawList->IdxBuffer.size();

		if (!checkAvailTransientBuffers(numVertices, ctx.m_decl, numIndices)) {
			// not enough space in transient buffer just quit drawing the rest...
			break;
		}

		bgfx::allocTransientVertexBuffer(&tvb, numVertices, ctx.m_decl);
		bgfx::allocTransientIndexBuffer(&tib, numIndices, sizeof(ImDrawIdx) == 4);

		ImDrawVert *verts = (ImDrawVert *)tvb.data;
		bx::memCopy(verts, drawList->VtxBuffer.begin(), numVertices * sizeof(ImDrawVert));

		ImDrawIdx *indices = (ImDrawIdx *)tib.data;
		bx::memCopy(indices, drawList->IdxBuffer.begin(), numIndices * sizeof(ImDrawIdx));

		bgfx::Encoder *encoder = bgfx::begin();

		for (const ImDrawCmd *cmd = drawList->CmdBuffer.begin(), *cmdEnd = drawList->CmdBuffer.end(); cmd != cmdEnd; ++cmd) {
			if (cmd->UserCallback) {
				cmd->UserCallback(drawList, cmd);
			} else if (0 != cmd->ElemCount) {
				uint64_t state = 0 | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA;

				bgfx::TextureHandle th = ctx.m_texture;
				bgfx::ProgramHandle program = ctx.m_program;

				if (NULL != cmd->TextureId) {
					union {
						ImTextureID ptr;
						struct {
							bgfx::TextureHandle handle;
							uint8_t flags;
							uint8_t mip;
						} s;
					} texture = {cmd->TextureId};
					state |= 0 != (IMGUI_FLAGS_ALPHA_BLEND & texture.s.flags)
								 ? BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA)
								 : BGFX_STATE_NONE;
					th = texture.s.handle;
					if (0 != texture.s.mip) {
						const float lodEnabled[4] = {float(texture.s.mip), 1.0f, 0.0f, 0.0f};
						bgfx::setUniform(ctx.u_imageLodEnabled, lodEnabled);
						program = ctx.m_imageProgram;
					}
				} else {
					state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
				}

				// Project scissor/clipping rectangles into framebuffer space
				ImVec4 clipRect;
				clipRect.x = (cmd->ClipRect.x - clipPos.x) * clipScale.x;
				clipRect.y = (cmd->ClipRect.y - clipPos.y) * clipScale.y;
				clipRect.z = (cmd->ClipRect.z - clipPos.x) * clipScale.x;
				clipRect.w = (cmd->ClipRect.w - clipPos.y) * clipScale.y;

				if (clipRect.x < fb_width && clipRect.y < fb_height && clipRect.z >= 0.0f && clipRect.w >= 0.0f) {
					const uint16_t xx = uint16_t(bx::max(clipRect.x, 0.0f));
					const uint16_t yy = uint16_t(bx::max(clipRect.y, 0.0f));
					encoder->setScissor(xx, yy, uint16_t(bx::min(clipRect.z, 65535.0f) - xx), uint16_t(bx::min(clipRect.w, 65535.0f) - yy));

					encoder->setState(state);
					encoder->setTexture(0, ctx.s_tex, th);
					encoder->setVertexBuffer(0, &tvb, cmd->VtxOffset, numVertices);
					encoder->setIndexBuffer(&tib, cmd->IdxOffset, cmd->ElemCount);
					encoder->submit(m_viewId, program);
				}
			}
		}

		bgfx::end(encoder);
	}
#endif
}

void ImGuiEndFrame(const DearImguiContext &ctx, bgfx::ViewId view_id) {
	ImGui::SetCurrentContext(ctx.m_imgui);
	ImGui::Render();
	ImGuiRender(ctx, view_id, ImGui::GetDrawData());
}

void ImGuiEndFrame(bgfx::ViewId view_id) {
	if (CheckImGuiIsInitialized()) {
		ImGui::Render();
		ImGuiRender(*g_context, view_id, ImGui::GetDrawData());
	}
}

void ImGuiBeginFrame(DearImguiContext &ctx, int width, int height, time_ns dt_clock, const MouseState &mouse, const KeyboardState &keyboard) {
	ImGui::SetCurrentContext(ctx.m_imgui);
	auto &io = ImGui::GetIO();
	io.DisplaySize = {float(width), float(height)};

	io.DeltaTime = time_to_sec_f(dt_clock);

	// send mouse state
	io.MousePos = {float(mouse.x), float(height - mouse.y)};
	io.MouseDown[0] = mouse.button[MB_0];
	io.MouseDown[1] = mouse.button[MB_1];
	io.MouseDown[2] = mouse.button[MB_2];

	io.MouseWheel = float(mouse.wheel);

	// send keyboard state
	for (auto i = 0; i < K_Last; ++i)
		io.KeysDown[i] = keyboard.key[i];

	io.KeyCtrl = keyboard.key[K_LCtrl] || keyboard.key[K_RCtrl];
	io.KeyShift = keyboard.key[K_LShift] || keyboard.key[K_RShift];
	io.KeyAlt = keyboard.key[K_LAlt] || keyboard.key[K_RAlt];
	io.KeySuper = keyboard.key[K_LWin] || keyboard.key[K_RWin];

	if (!input_buffer.empty()) {
		io.AddInputCharactersUTF8(input_buffer.c_str());
		if (g_context)
			input_buffer.clear();
	}

	ImGui::NewFrame();
}

void ImGuiClearInputBuffer() { input_buffer.clear(); }

void ImGuiBeginFrame(int width, int height, time_ns dt_clock, const MouseState &mouse, const KeyboardState &keyboard) {
	if (CheckImGuiIsInitialized())
		ImGuiBeginFrame(*g_context, width, height, dt_clock, mouse, keyboard);
}

static void *__alloc(size_t size, void *userdata) { return malloc(size); }
static void __free(void *ptr, void *userdata) { return free(ptr); }

void ImGuiCreateFontTexture(DearImguiContext &ctx) {
	auto &io = ImGui::GetIO();

	uint8_t *data;
	int32_t width, height;
	io.Fonts->GetTexDataAsRGBA32(&data, &width, &height);

	if (bgfx::isValid(ctx.m_texture)) {
		bgfx::destroy(ctx.m_texture);
	}

	ctx.m_texture = bgfx::createTexture2D((uint16_t)width, (uint16_t)height, false, 1, bgfx::TextureFormat::BGRA8, 0, bgfx::copy(data, width * height * 4));

	// commented out because we don't want to destroy imgui mouse cursors
	// io.Fonts->ClearInputData();
	io.Fonts->ClearTexData();
}

void ImGuiCreateFontTexture() { ImGuiCreateFontTexture(*g_context); }

DearImguiContext *ImGuiInitContext(float font_size, bgfx::ProgramHandle imgui_program, bgfx::ProgramHandle imgui_image_program,
	std::function<void(ImGuiIO &io)> font_setup, bx::AllocatorI *allocator) {
	auto ctx = new DearImguiContext;

	ctx->m_allocator = allocator;

	if (allocator == nullptr) {
		static bx::DefaultAllocator allocator;
		ctx->m_allocator = &allocator;
	}

	// ImGui::SetAllocatorFunctions(memAlloc, memFree, NULL);
	ctx->m_imgui = ImGui::CreateContext();
	ImGui::SetCurrentContext(ctx->m_imgui);

	ImGuiIO &io = ImGui::GetIO();

	//	io.ConfigFlags |= /*ImGuiConfigFlags_DpiEnableScaleFonts |*/ ImGuiConfigFlags_DpiEnableScaleViewports;

	io.DisplaySize = {1280.0f, 720.0f};
	io.DeltaTime = 1.0f / 60.0f;
	io.IniFilename = nullptr;

	io.KeyMap[ImGuiKey_Tab] = K_Tab;
	io.KeyMap[ImGuiKey_LeftArrow] = K_Left;
	io.KeyMap[ImGuiKey_RightArrow] = K_Right;
	io.KeyMap[ImGuiKey_UpArrow] = K_Up;
	io.KeyMap[ImGuiKey_DownArrow] = K_Down;
	io.KeyMap[ImGuiKey_PageUp] = K_PageUp;
	io.KeyMap[ImGuiKey_PageDown] = K_PageDown;
	io.KeyMap[ImGuiKey_Home] = K_Home;
	io.KeyMap[ImGuiKey_End] = K_End;
	io.KeyMap[ImGuiKey_Insert] = K_Insert;
	io.KeyMap[ImGuiKey_Delete] = K_Suppr;
	io.KeyMap[ImGuiKey_Backspace] = K_Backspace;
	io.KeyMap[ImGuiKey_Space] = K_Space;
	io.KeyMap[ImGuiKey_Enter] = K_Return;
	io.KeyMap[ImGuiKey_KeypadEnter] = K_Enter;
	io.KeyMap[ImGuiKey_Escape] = K_Escape;
	io.KeyMap[ImGuiKey_A] = K_A;
	io.KeyMap[ImGuiKey_C] = K_C;
	io.KeyMap[ImGuiKey_V] = K_V;
	io.KeyMap[ImGuiKey_X] = K_X;
	io.KeyMap[ImGuiKey_Y] = K_Y;
	io.KeyMap[ImGuiKey_Z] = K_Z;

#if USE_ENTRY
	io.ConfigFlags |= 0 | ImGuiConfigFlags_NavEnableGamepad | ImGuiConfigFlags_NavEnableKeyboard;

	io.NavInputs[ImGuiNavInput_Activate] = (int)entry::Key::GamepadA;
	io.NavInputs[ImGuiNavInput_Cancel] = (int)entry::Key::GamepadB;
	//		io.NavInputs[ImGuiNavInput_Input]       = (int)entry::Key::;
	//		io.NavInputs[ImGuiNavInput_Menu]        = (int)entry::Key::;
	io.NavInputs[ImGuiNavInput_DpadLeft] = (int)entry::Key::GamepadLeft;
	io.NavInputs[ImGuiNavInput_DpadRight] = (int)entry::Key::GamepadRight;
	io.NavInputs[ImGuiNavInput_DpadUp] = (int)entry::Key::GamepadUp;
	io.NavInputs[ImGuiNavInput_DpadDown] = (int)entry::Key::GamepadDown;
//		io.NavInputs[ImGuiNavInput_LStickLeft]  = (int)entry::Key::;
//		io.NavInputs[ImGuiNavInput_LStickRight] = (int)entry::Key::;
//		io.NavInputs[ImGuiNavInput_LStickUp]    = (int)entry::Key::;
//		io.NavInputs[ImGuiNavInput_LStickDown]  = (int)entry::Key::;
//		io.NavInputs[ImGuiNavInput_FocusPrev]   = (int)entry::Key::;
//		io.NavInputs[ImGuiNavInput_FocusNext]   = (int)entry::Key::;
//		io.NavInputs[ImGuiNavInput_TweakSlow]   = (int)entry::Key::;
//		io.NavInputs[ImGuiNavInput_TweakFast]   = (int)entry::Key::;
#endif // USE_ENTRY

	ctx->m_program = imgui_program;

	ctx->u_imageLodEnabled = bgfx::createUniform("u_imageLodEnabled", bgfx::UniformType::Vec4);
	ctx->m_imageProgram = imgui_image_program;

	ctx->m_decl.begin()
		.add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
		.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
		.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
		.end();

	ctx->s_tex = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

	if (font_setup)
		font_setup(io);
	else
		io.Fonts->AddFontDefault();

	ImGuiCreateFontTexture(*ctx);

	//
	auto &style = ImGui::GetStyle();
	ImGui::StyleColorsDark(&style);

	style.TabRounding = 0.f;
	style.TabBorderSize = 0.f;

	style.FramePadding = {4.f, 4.f};
	style.FrameRounding = 0.f;

	style.WindowRounding = 0.f;
	style.WindowBorderSize = 0.f;
	//	style.WindowPadding = {0.f, 0.f};

	style.ScrollbarRounding = 0.f;

	// ImGuiCol_Text
	// ImGuiCol_TextDisabled

	style.Colors[ImGuiCol_WindowBg] = ImColor(45, 45, 48);
	style.Colors[ImGuiCol_ChildBg] = ImColor(37, 37, 38);
	style.Colors[ImGuiCol_PopupBg] = ImColor(30, 30, 30);

	style.Colors[ImGuiCol_Border] = ImColor(0, 120, 200);
	style.Colors[ImGuiCol_BorderShadow] = ImColor(255, 0, 0);

	style.Colors[ImGuiCol_FrameBg] = ImColor(51, 51, 55);
	style.Colors[ImGuiCol_FrameBgHovered] = ImColor(63, 63, 70);
	style.Colors[ImGuiCol_FrameBgActive] = ImColor(63, 63, 70);

	style.Colors[ImGuiCol_TitleBg] = ImColor(51, 51, 55);
	style.Colors[ImGuiCol_TitleBgActive] = ImColor(63, 63, 70);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImColor(63, 63, 70);

	style.Colors[ImGuiCol_DockingEmptyBg] = ImColor(45, 45, 48);

	style.Colors[ImGuiCol_Separator] = ImColor(51, 51, 55);
	style.Colors[ImGuiCol_SeparatorHovered] = ImColor(63, 63, 70);
	style.Colors[ImGuiCol_SeparatorActive] = ImColor(63, 63, 70);

	style.Colors[ImGuiCol_Button] = ImColor(45, 45, 48);
	style.Colors[ImGuiCol_ButtonHovered] = ImColor(63, 63, 70);
	style.Colors[ImGuiCol_ButtonActive] = ImColor(63, 63, 70);

	style.Colors[ImGuiCol_Tab] = ImColor(45, 45, 48);
	style.Colors[ImGuiCol_TabHovered] = ImColor(0, 122, 204);
	style.Colors[ImGuiCol_TabActive] = ImColor(0, 122, 204);
	style.Colors[ImGuiCol_TabUnfocused] = ImColor(45, 45, 48);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImColor(63, 63, 70);

	style.Colors[ImGuiCol_MenuBarBg] = ImColor(45, 45, 48);

	style.Colors[ImGuiCol_ScrollbarBg] = ImColor(62, 62, 66);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImColor(104, 104, 104);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImColor(140, 140, 140);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImColor(200, 200, 200);

	style.Colors[ImGuiCol_ModalWindowDimBg] = ImColor(0, 0, 0, 128);

	/*
	ImGuiCol_CheckMark,
	ImGuiCol_SliderGrab,
	ImGuiCol_SliderGrabActive,
	ImGuiCol_Header,
	ImGuiCol_HeaderHovered,
	ImGuiCol_HeaderActive,
	ImGuiCol_ResizeGrip,
	ImGuiCol_ResizeGripHovered,
	ImGuiCol_ResizeGripActive,
	ImGuiCol_DockingPreview,
	ImGuiCol_PlotLines,
	ImGuiCol_PlotLinesHovered,
	ImGuiCol_PlotHistogram,
	ImGuiCol_PlotHistogramHovered,
	ImGuiCol_TextSelectedBg,
	ImGuiCol_DragDropTarget,
	ImGuiCol_NavHighlight,          // Gamepad/keyboard: current highlighted item
	ImGuiCol_NavWindowingHighlight, // Highlight window when using CTRL+TAB
	ImGuiCol_NavWindowingDimBg,     // Darken/colorize entire screen behind the CTRL+TAB window list, when active
	ImGuiCol_ModalWindowDimBg,      // Darken/colorize entire screen behind a modal window, when one is active
	*/

	//
	on_text_input_connection = on_text_input.Connect([=](const char *utf8) { input_buffer += utf8; });

	return ctx;
}

void ImGuiInit(float font_size, bgfx::ProgramHandle imgui_program, bgfx::ProgramHandle imgui_image_program, std::function<void(ImGuiIO &io)> font_setup,
	bx::AllocatorI *allocator) {
	g_context.reset(ImGuiInitContext(font_size, imgui_program, imgui_image_program, font_setup, allocator));
}

void ImGuiShutdown(DearImguiContext &ctx) {
	on_text_input.Disconnect(on_text_input_connection);

	ImGui::DestroyContext(ctx.m_imgui);

	if (bgfx::isValid(ctx.s_tex))
		bgfx::destroy(ctx.s_tex);
	if (bgfx::isValid(ctx.m_texture))
		bgfx::destroy(ctx.m_texture);

	if (bgfx::isValid(ctx.u_imageLodEnabled))
		bgfx::destroy(ctx.u_imageLodEnabled);
	if (bgfx::isValid(ctx.m_imageProgram))
		bgfx::destroy(ctx.m_imageProgram);
	if (bgfx::isValid(ctx.m_program))
		bgfx::destroy(ctx.m_program);

	ctx.m_allocator = nullptr;
}

void ImGuiShutdown() {
	if (CheckImGuiIsInitialized()) {
		ImGuiShutdown(*g_context);
		g_context.reset();
	}
}

} // namespace hg

namespace ImGui {

// Helper function for passing bgfx::TextureHandle to ImGui::Image.
void Image(const hg::Texture &tex, uint8_t _flags, uint8_t _mip, const ImVec2 &_size, const ImVec2 &_uv0, const ImVec2 &_uv1, const ImVec4 &_tintCol,
	const ImVec4 &_borderCol) {
	hg::TextureBridge texture;
	texture.s.handle = tex.handle;
	texture.s.flags = _flags;
	texture.s.mip = _mip;
	Image(texture.ptr, _size, {_uv0.x, _uv0.y}, {_uv1.x, _uv1.y}, _tintCol, _borderCol);
}

// Helper function for passing bgfx::TextureHandle to ImGui::Image.
void Image(const hg::Texture &tex, const ImVec2 &_size, const ImVec2 &_uv0, const ImVec2 &_uv1, const ImVec4 &_tintCol, const ImVec4 &_borderCol) {
	Image(tex, IMGUI_FLAGS_ALPHA_BLEND, 0, _size, _uv0, _uv1, _tintCol, _borderCol);
}

// Helper function for passing bgfx::TextureHandle to ImGui::ImageButton.
bool ImageButton(const hg::Texture &tex, uint8_t _flags, uint8_t _mip, const ImVec2 &_size, const ImVec2 &_uv0, const ImVec2 &_uv1, int _framePadding,
	const ImVec4 &_bgCol, const ImVec4 &_tintCol) {
	hg::TextureBridge texture;
	texture.s.handle = tex.handle;
	texture.s.flags = _flags;
	texture.s.mip = _mip;
	return ImageButton(texture.ptr, _size, _uv0, _uv1, _framePadding, _bgCol, _tintCol);
}

// Helper function for passing bgfx::TextureHandle to ImGui::ImageButton.
bool ImageButton(
	const hg::Texture &tex, const ImVec2 &_size, const ImVec2 &_uv0, const ImVec2 &_uv1, int _framePadding, const ImVec4 &_bgCol, const ImVec4 &_tintCol) {
	return ImageButton(tex, IMGUI_FLAGS_ALPHA_BLEND, 0, _size, _uv0, _uv1, _framePadding, _bgCol, _tintCol);
}

inline void NextLine() { SetCursorPosY(GetCursorPosY() + GetTextLineHeightWithSpacing()); }

inline bool TabButton(const char *_text, float _width, bool _active) {
	int32_t count = 1;

	if (_active) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.75f, 0.0f, 0.78f));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
		count = 2;
	} else {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.7f));
	}

	bool retval = ImGui::Button(_text, ImVec2(_width, 20.0f));
	ImGui::PopStyleColor(count);

	return retval;
}

inline bool MouseOverArea() { return false || ImGui::IsAnyItemHovered() || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow); }

bool time_ns_Edit(const char *label, hg::time_ns &v) {
	int hour = hg::time_to_hour(v) % 24, min = hg::time_to_min(v) % 60, sec = hg::time_to_sec(v) % 60, ms = hg::time_to_ms(v) % 1000,
		us = hg::time_to_us(v) % 1000, ns = hg::time_to_ns(v) % 1000;

	char timestamp[14];
	sprintf(timestamp, "%02d:%02d:%02d:%03d", hour, min, sec, ms);

	ImGui::PushItemWidth(84);
	bool res = ImGui::InputText(label, timestamp, 13, ImGuiInputTextFlags_CharsDecimal);
	if (res) {
		if (sscanf(timestamp, "%d:%d:%d:%d", &hour, &min, &sec, &ms) == 4) {
			auto t_hour = hg::time_from_hour(hour % 24);
			auto t_min = hg::time_from_min(min % 60);
			auto t_sec = hg::time_from_sec(sec % 60);
			auto t_ms = hg::time_from_ms(ms % 1000);

			v = t_hour + t_min + t_sec + t_ms;
			res = true;
		} else {
			res = false;
		}
	}
	ImGui::PopItemWidth();

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("%d hour %d min %d sec %d ms %d ns\n%s: %" PRId64 " ns", hour, min, sec, ms, ns, label, v);

	return res;
}

void PushTextureID(ImDrawList *draw_list, const hg::Texture &tex, uint8_t _flags, uint8_t _mip) {
	hg::TextureBridge texture;
	texture.s.handle = tex.handle;
	texture.s.flags = _flags;
	texture.s.mip = _mip;
	draw_list->PushTextureID(texture.ptr);
}

void PushTextureID(ImDrawList *draw_list, const hg::Texture &tex) { return PushTextureID(draw_list, tex, IMGUI_FLAGS_ALPHA_BLEND, 0); }

void AddImage(ImDrawList *draw_list, const hg::Texture &tex, uint8_t _flags, uint8_t _mip, const ImVec2 &a, const ImVec2 &b, const ImVec2 &uv_a,
	const ImVec2 &uv_b, ImU32 col) {
	hg::TextureBridge texture;
	texture.s.handle = tex.handle;
	texture.s.flags = _flags;
	texture.s.mip = _mip;
	draw_list->AddImage(texture.ptr, a, b, uv_a, uv_b, col);
}

void AddImage(ImDrawList *draw_list, const hg::Texture &tex, const ImVec2 &a, const ImVec2 &b, const ImVec2 &uv_a, const ImVec2 &uv_b, ImU32 col) {
	AddImage(draw_list, tex, IMGUI_FLAGS_ALPHA_BLEND, 0, a, b, uv_a, uv_b, col);
}

void AddImageQuad(ImDrawList *draw_list, const hg::Texture &tex, uint8_t _flags, uint8_t _mip, const ImVec2 &a, const ImVec2 &b, const ImVec2 &c,
	const ImVec2 &d, const ImVec2 &uv_a, const ImVec2 &uv_b, const ImVec2 &uv_c, const ImVec2 &uv_d, ImU32 col) {
	hg::TextureBridge texture;
	texture.s.handle = tex.handle;
	texture.s.flags = _flags;
	texture.s.mip = _mip;
	draw_list->AddImageQuad(texture.ptr, a, b, c, d, uv_a, uv_b, uv_c, uv_d, col);
}

void AddImageQuad(ImDrawList *draw_list, const hg::Texture &tex, const ImVec2 &a, const ImVec2 &b, const ImVec2 &c, const ImVec2 &d, const ImVec2 &uv_a,
	const ImVec2 &uv_b, const ImVec2 &uv_c, const ImVec2 &uv_d, ImU32 col) {
	return AddImageQuad(draw_list, tex, IMGUI_FLAGS_ALPHA_BLEND, 0, a, b, c, d, uv_a, uv_b, uv_c, uv_d, col);
}

void AddImageRounded(ImDrawList *draw_list, const hg::Texture &tex, uint8_t _flags, uint8_t _mip, const ImVec2 &a, const ImVec2 &b, const ImVec2 &uv_a,
	const ImVec2 &uv_b, ImU32 col, float rounding, int rounding_corners) {
	hg::TextureBridge texture;
	texture.s.handle = tex.handle;
	texture.s.flags = _flags;
	texture.s.mip = _mip;
	draw_list->AddImageRounded(texture.ptr, a, b, uv_a, uv_b, col, rounding, rounding_corners);
}

void AddImageRounded(ImDrawList *draw_list, const hg::Texture &tex, const ImVec2 &a, const ImVec2 &b, const ImVec2 &uv_a, const ImVec2 &uv_b, ImU32 col,
	float rounding, int rounding_corners) {
	return AddImageRounded(draw_list, tex, IMGUI_FLAGS_ALPHA_BLEND, 0, a, b, uv_a, uv_b, col, rounding, rounding_corners);
}

} // namespace ImGui
