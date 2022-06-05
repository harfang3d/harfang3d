// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/time.h"

#include <bgfx/bgfx.h>
#include <imgui/imgui.h>

#include <functional>
#include <string>
#include <vector>

namespace bx {
struct AllocatorI;
} // namespace bx

namespace hg {

/// Context to render immediate GUI.
struct DearImguiContext {
	ImGuiContext *m_imgui = nullptr;
	bx::AllocatorI *m_allocator = nullptr;
	bgfx::VertexLayout m_decl;
	bgfx::ProgramHandle m_program = BGFX_INVALID_HANDLE;
	bgfx::ProgramHandle m_imageProgram = BGFX_INVALID_HANDLE;
	bgfx::TextureHandle m_texture = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle s_tex = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_imageLodEnabled = BGFX_INVALID_HANDLE;
	std::vector<ImFont *> m_fonts;
};

DearImguiContext *ImGuiInitContext(float font_size, bgfx::ProgramHandle imgui_program, bgfx::ProgramHandle imgui_image_program,
	std::function<void(ImGuiIO &io)> font_setup = {}, bx::AllocatorI *allocator = nullptr);
void ImGuiInit(float font_size, bgfx::ProgramHandle imgui_program, bgfx::ProgramHandle imgui_image_program, std::function<void(ImGuiIO &io)> font_setup = {},
	bx::AllocatorI *allocator = nullptr);
void ImGuiShutdown();

void ImGuiCreateFontTexture(DearImguiContext &ctx);
void ImGuiCreateFontTexture();

struct MouseState;
struct KeyboardState;

void ImGuiBeginFrame(DearImguiContext &ctx, int width, int height, time_ns dt_clock, const MouseState &mouse, const KeyboardState &keyboard);
void ImGuiBeginFrame(int width, int height, time_ns dt_clock, const MouseState &mouse, const KeyboardState &keyboard);
void ImGuiEndFrame(const DearImguiContext &ctx, bgfx::ViewId view_id = 255);
void ImGuiEndFrame(bgfx::ViewId view_id = 255);

void ImGuiClearInputBuffer();

struct Texture;

} // namespace hg

namespace ImGui {

// obsolete functions
static inline void SetNextWindowContentWidth(float width) {
	SetNextWindowContentSize(ImVec2(width, 0.0f));
} // OBSOLETE 1.53+ (nb: original version preserved last Y value set by SetNextWindowContentSize())

static inline void SetNextWindowPosCenter(ImGuiCond cond = 0) {
	SetNextWindowPos(ImVec2(GetIO().DisplaySize.x * 0.5f, GetIO().DisplaySize.y * 0.5f), cond, ImVec2(0.5f, 0.5f));
} // OBSOLETE 1.52+

static inline float GetContentRegionAvailWidth() { return GetContentRegionAvail().x; } // OBSOLETED in 1.70 (from May 2019)

bool time_ns_Edit(const char *label, hg::time_ns &v);

//
void Image(const hg::Texture &tex, uint8_t _flags, uint8_t _mip, const ImVec2 &_size, const ImVec2 &_uv0 = {0.f, 0.f}, const ImVec2 &_uv1 = {1.f, 1.f},
	const ImVec4 &_tintCol = {1.f, 1.f, 1.f, 1.f}, const ImVec4 &_borderCol = {0.f, 0.f, 0.f, 0.f});
void Image(const hg::Texture &tex, const ImVec2 &_size, const ImVec2 &_uv0 = {0.f, 0.f}, const ImVec2 &_uv1 = {1.f, 1.f},
	const ImVec4 &_tintCol = {1.f, 1.f, 1.f, 1.f}, const ImVec4 &_borderCol = {0.f, 0.f, 0.f, 0.f});
bool ImageButton(const hg::Texture &tex, uint8_t _flags, uint8_t _mip, const ImVec2 &_size, const ImVec2 &_uv0 = {0.f, 0.f}, const ImVec2 &_uv1 = {1.f, 1.f},
	int _framePadding = -1, const ImVec4 &_bgCol = {0.f, 0.f, 0.f, 0.f}, const ImVec4 &_tintCol = {1.f, 1.f, 1.f, 1.f});
bool ImageButton(const hg::Texture &tex, const ImVec2 &_size, const ImVec2 &_uv0 = {0.f, 0.f}, const ImVec2 &_uv1 = {1.f, 1.f}, int _framePadding = -1,
	const ImVec4 &_bgCol = {0.f, 0.f, 0.f, 0.f}, const ImVec4 &_tintCol = {1.f, 1.f, 1.f, 1.f});

void PushTextureID(ImDrawList *draw_list, const hg::Texture &tex, uint8_t _flags, uint8_t _mip);
void PushTextureID(ImDrawList *draw_list, const hg::Texture &tex);
void AddImage(ImDrawList *draw_list, const hg::Texture &tex, uint8_t _flags, uint8_t _mip, const ImVec2 &a, const ImVec2 &b, const ImVec2 &uv_a = ImVec2(0, 0),
	const ImVec2 &uv_b = ImVec2(1, 1), ImU32 col = IM_COL32_WHITE);
void AddImage(ImDrawList *draw_list, const hg::Texture &tex, const ImVec2 &a, const ImVec2 &b, const ImVec2 &uv_a = ImVec2(0, 0),
	const ImVec2 &uv_b = ImVec2(1, 1), ImU32 col = IM_COL32_WHITE);
void AddImageQuad(ImDrawList *draw_list, const hg::Texture &tex, uint8_t _flags, uint8_t _mip, const ImVec2 &a, const ImVec2 &b, const ImVec2 &c,
	const ImVec2 &d, const ImVec2 &uv_a = ImVec2(0, 0), const ImVec2 &uv_b = ImVec2(1, 0), const ImVec2 &uv_c = ImVec2(1, 1), const ImVec2 &uv_d = ImVec2(0, 1),
	ImU32 col = IM_COL32_WHITE);
void AddImageQuad(ImDrawList *draw_list, const hg::Texture &tex, const ImVec2 &a, const ImVec2 &b, const ImVec2 &c, const ImVec2 &d,
	const ImVec2 &uv_a = ImVec2(0, 0), const ImVec2 &uv_b = ImVec2(1, 0), const ImVec2 &uv_c = ImVec2(1, 1), const ImVec2 &uv_d = ImVec2(0, 1),
	ImU32 col = IM_COL32_WHITE);
void AddImageRounded(ImDrawList *draw_list, const hg::Texture &tex, uint8_t _flags, uint8_t _mip, const ImVec2 &a, const ImVec2 &b, const ImVec2 &uv_a,
	const ImVec2 &uv_b, ImU32 col, float rounding, int rounding_corners = ImDrawFlags_RoundCornersAll);
void AddImageRounded(ImDrawList *draw_list, const hg::Texture &tex, const ImVec2 &a, const ImVec2 &b, const ImVec2 &uv_a, const ImVec2 &uv_b, ImU32 col,
	float rounding, int rounding_corners = ImDrawFlags_RoundCornersAll);

static inline float GetWindowContentRegionWidth() { return GetWindowContentRegionMax().x - GetWindowContentRegionMin().x; }

} // namespace ImGui

inline ImVec2 operator+(const ImVec2 &a, const ImVec2 &b) { return {a.x + b.x, a.y + b.y}; }
inline ImVec2 operator-(const ImVec2 &a, const ImVec2 &b) { return {a.x - b.x, a.y - b.y}; }
inline ImVec2 operator*(const ImVec2 &a, const ImVec2 &b) { return {a.x * b.x, a.y * b.y}; }
inline ImVec2 operator/(const ImVec2 &a, const ImVec2 &b) { return {a.x / b.x, a.y / b.y}; }
