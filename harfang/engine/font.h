// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/rect.h"
#include "foundation/utf8.h"
#include "foundation/vector2.h"

#include "engine/render_pipeline.h"

#include <map>
#include <vector>

namespace hg {

/// Font object for realtime rendering.
struct Font {
	struct PackedChar {
		iRect box;
		Vec2 offsets[2];
		float advance;
	};

	struct Glyph {
		uint16_t page;
		PackedChar pc;
	};

	std::vector<bgfx::TextureHandle> pages;
	std::map<utf32_cp, Glyph> glyphs;
	std::map<utf32_cp, std::map<utf32_cp, float>> kerning;
	uint16_t resolution;
	float ascent, scale;
};

Font LoadFontFromFile(const char *path, float size = 16.f, uint16_t resolution = 1024, int padding = 1, const char *glyphs = nullptr);
Font LoadFontFromAssets(const char *name, float size = 16.f, uint16_t resolution = 1024, int padding = 1, const char *glyphs = nullptr);

//
enum DrawTextHAlign { DTHA_Left, DTHA_Center, DTHA_Right };
enum DrawTextVAlign { DTVA_Top, DTVA_Center, DTVA_Bottom };

/// Write text to the specified view using the provided shader program and uniform values.
void DrawText(bgfx::ViewId view_id, const Font &font, const char *text, bgfx::ProgramHandle program, const char *page_uniform, uint8_t page_stage,
	const Mat4 *mtxs, size_t mtx_count, Vec3 pos = {}, DrawTextHAlign halign = DTHA_Left, DrawTextVAlign valign = DTVA_Top,
	const std::vector<UniformSetValue> &values = {}, const std::vector<UniformSetTexture> &textures = {}, RenderState state = {}, uint32_t depth = 0);

void DrawText(bgfx::ViewId view_id, const Font &font, const char *text, bgfx::ProgramHandle program, const char *page_uniform, uint8_t page_stage, const Mat4 &mtx,
	Vec3 pos = {}, DrawTextHAlign halign = DTHA_Left, DrawTextVAlign valign = DTVA_Top, const std::vector<UniformSetValue> &values = {},
	const std::vector<UniformSetTexture> &textures = {}, RenderState state = {}, uint32_t depth = 0);

//
float GetKerning(const Font &font, uint32_t cp0, uint32_t cp1);

/// Compute the width and height of a text string.
fRect ComputeTextRect(const Font &font, const char *text, float xpos = 0.f, float ypos = 0.f);
/// Compute the height of a text string.
float ComputeTextHeight(const Font &font, const char *text);

} // namespace hg
