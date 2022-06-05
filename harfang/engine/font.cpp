// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/font.h"
#include "engine/assets.h"
#include "engine/assets_rw_interface.h"

#include "foundation/file_rw_interface.h"
#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/rect_packer.h"
#include "foundation/vector3.h"

#include <stb_truetype/stb_truetype.h>

#include <cmath>
#include <cstring>

namespace hg {

static Font LoadFont(const ReadProvider &ip, const Reader &ir, const char *name, float size, uint16_t resolution, int padding, const char *glyphs) {
	ScopedReadHandle h(ip, name);

	if (!ir.is_valid(h)) {
		warn(format("Failed to open '%1'").arg(name));
		return {};
	}

	const auto data = LoadData(ir, h);

	if (data.Empty()) {
		warn(format("Failed to read data from '%1'").arg(name));
		return {};
	}

	const auto buffer = reinterpret_cast<const unsigned char *>(data.GetData());

	// extract codepoint
	if(!glyphs) {
		glyphs = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789,;:!?./\\+-*()'\"&_={} ";
	}

	std::vector<utf32_cp> cps;
	convert_utf8_to_utf32(glyphs, cps);

	// remove duplicates
	std::sort(cps.begin(), cps.end());
	cps.erase(std::unique(cps.begin(), cps.end()), cps.end());

	//
	stbtt_fontinfo info;
	stbtt_InitFont(&info, buffer, 0);

	const auto scale = stbtt_ScaleForPixelHeight(&info, size);
	int ascent;
	stbtt_GetFontVMetrics(&info, &ascent, 0, 0);

	//
	stbtt_pack_context spc;
	uint8_t *pixels = nullptr;

	Font font;
	font.resolution = resolution;
	font.ascent = float(ascent) * scale;
	font.scale = scale;

	auto new_page = [&]() {
		__ASSERT__(pixels == nullptr);
		pixels = new uint8_t[resolution * resolution];
		stbtt_PackBegin(&spc, pixels, resolution, resolution, 0, padding, nullptr);
	};

	auto commit_page = [&]() {
		__ASSERT__(pixels != nullptr);
		const auto *mem = bgfx::makeRef(pixels, resolution * resolution, [](void *ptr, void *) { delete[] reinterpret_cast<uint8_t *>(ptr); });
		font.pages.push_back(bgfx::createTexture2D(resolution, resolution, false, 1, bgfx::TextureFormat::A8, BGFX_SAMPLER_NONE, mem));
		pixels = nullptr;
		stbtt_PackEnd(&spc);
	};

	// stbtt_PackSetOversampling(&spc, 2, 2);

	for (auto &cp : cps) {
		if (!pixels)
			new_page();

		stbtt_packedchar pc;
		auto &glyph = font.glyphs[cp];
		glyph.page = numeric_cast<uint16_t>(font.pages.size());

		int ret = stbtt_PackFontRange(&spc, buffer, 0, size, cp, 1, &pc);
		glyph.pc = {{pc.x0, pc.y0, pc.x1, pc.y1}, {{pc.xoff, pc.yoff}, {pc.xoff2, pc.yoff2}}, pc.xadvance};
		if (!ret)
			commit_page();
	}

	if (pixels)
		commit_page();

	// transfer kerning
	for (auto i : cps)
		for (auto j : cps)
			if (auto kerning = stbtt_GetCodepointKernAdvance(&info, i, j) * scale)
				font.kerning[i][j] = kerning;

	return font;
}

//
Font LoadFontFromFile(const char *path, float size, uint16_t resolution, int padding, const char *glyphs) {
	return LoadFont(g_file_read_provider, g_file_reader, path, size, resolution, padding, glyphs);
}

Font LoadFontFromAssets(const char *name, float size, uint16_t resolution, int padding, const char *glyphs) {
	return LoadFont(g_assets_read_provider, g_assets_reader, name, size, resolution, padding, glyphs);
}

//
float GetKerning(const Font &font, uint32_t cp0, uint32_t cp1) {
	const auto i = font.kerning.find(cp0);
	if (i == std::end(font.kerning))
		return 0.f;

	const auto j = i->second.find(cp1);
	if (j == std::end(i->second))
		return 0.f;

	return j->second;
}

//
static float ComputeTextWidth(const Font &font, const std::vector<utf32_cp> &cps) {
	float width = 0.f;
	float line_width = 0.f;
	for (size_t i = 0; i < cps.size(); ++i) {
		const auto cp = cps[i];
		if (cp == '\n') {
			width = Max(width, line_width);
			line_width = 0.f;
		}
		const auto j = font.glyphs.find(cp);
		if (j != std::end(font.glyphs)) {
			line_width += j->second.pc.advance;
		}
	}
	return Max(width, line_width);
}

//
static void DrawText(bgfx::ViewId view_id, const Font &font, const std::vector<utf32_cp> &cps, bgfx::ProgramHandle program, const char *page_uniform,
	uint8_t page_stage, const Mat4 *mtxs, size_t mtx_count, Vec3 pos, const std::vector<UniformSetValue> &values,
	const std::vector<UniformSetTexture> &textures, RenderState state, uint32_t depth) {
	const float xline = pos.x;

	// prepare glyphs per page
	bgfx::VertexLayout v_decl;
	v_decl.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float).end();

	struct Vert {
		float pos[3];
		float uv[2];
	};

	std::vector<std::vector<Vert>> pages_quads(font.pages.size());
	for (auto &page_quads : pages_quads)
		page_quads.reserve(256);

	for (size_t i = 0; i < cps.size(); ++i) {
		const auto cp = cps[i];

		if (cp == '\n') {
			pos.x = xline;
			pos.y += font.ascent;
		}

		const auto j = font.glyphs.find(cp);
		if (j == std::end(font.glyphs))
			continue;

		const auto &glyph = j->second;

		stbtt_aligned_quad quad;
		stbtt_packedchar pc = {glyph.pc.box.sx, glyph.pc.box.sy, glyph.pc.box.ex, glyph.pc.box.ey, glyph.pc.offsets[0].x, glyph.pc.offsets[0].y,
			glyph.pc.advance, glyph.pc.offsets[1].x, glyph.pc.offsets[1].y};
		stbtt_GetPackedQuad(&pc, font.resolution, font.resolution, 0, &pos.x, &pos.y, &quad, 1);

		if ((i + 1) < cps.size())
			pos.x += GetKerning(font, cp, cps[i + 1]);

		auto &page_quads = pages_quads[glyph.page];

		page_quads.push_back({quad.x0, quad.y0, pos.z, quad.s0, quad.t0});
		page_quads.push_back({quad.x0, quad.y1, pos.z, quad.s0, quad.t1});
		page_quads.push_back({quad.x1, quad.y1, pos.z, quad.s1, quad.t1});
		page_quads.push_back({quad.x1, quad.y0, pos.z, quad.s1, quad.t0});
	}

	const auto u_page = bgfx::createUniform(page_uniform, bgfx::UniformType::Sampler);

	std::vector<bgfxMatrix4> bgfx_mtx(mtx_count);
	for (size_t i = 0; i < mtx_count; ++i)
		bgfx_mtx[i] = to_bgfx(mtxs[i]);

	const auto i_mtx = bgfx::setTransform(bgfx_mtx.data(), uint16_t(mtx_count));

	for (size_t i = 0; i < pages_quads.size(); ++i) {
		const auto &page_quads = pages_quads[i];
		if (page_quads.empty())
			continue;

		const uint32_t num_vertices = numeric_cast<uint32_t>(page_quads.size());
		const uint32_t quad_count = num_vertices / 4;
		const uint32_t num_indices = quad_count * 6;

		if (num_vertices != bgfx::getAvailTransientVertexBuffer(num_vertices, v_decl) || num_indices != bgfx::getAvailTransientIndexBuffer(num_indices)) {
			debug("Out of transient buffer space while rendering font, reduce the amount of text displayed in one call");
			break;
		}

		bgfx::TransientVertexBuffer tvb;
		bgfx::TransientIndexBuffer tib;

		bgfx::allocTransientVertexBuffer(&tvb, num_vertices, v_decl);
		bgfx::allocTransientIndexBuffer(&tib, num_indices);

		auto vtx = reinterpret_cast<Vert *>(tvb.data);
		std::copy(std::begin(page_quads), std::end(page_quads), vtx);

		auto idx = reinterpret_cast<uint16_t *>(tib.data);
		for (uint32_t j = 0; j < num_vertices; j += 4) {
			uint16_t k = (uint16_t)j;

			*idx++ = k;
			*idx++ = k + 1;
			*idx++ = k + 2;

			*idx++ = k + 2;
			*idx++ = k + 3;
			*idx++ = k;
		}

		bgfx::setTransform(i_mtx);

		bgfx::setTexture(page_stage, u_page, font.pages[i]);

		SetUniforms(values, textures);

		bgfx::setVertexBuffer(0, &tvb);
		bgfx::setIndexBuffer(&tib);

		bgfx::setState(state.state, state.rgba);

		bgfx::submit(view_id, program, depth);
	}

	bgfx::destroy(u_page);
}

//
static fRect ComputeTextRect(const Font &font, const std::vector<utf32_cp> &cps, float xpos, float ypos) {
	float x = xpos;

	fRect rect{};

	rect.sx = xpos;
	rect.sy = ypos;

	rect.ex = xpos;
	rect.ey = ypos + font.ascent;

	for (size_t i = 0; i < cps.size(); ++i) {
		const auto cp = cps[i];
		if (cp == '\n') {
			x = xpos;
			rect.ey += font.ascent;
		} else {
			const auto j = font.glyphs.find(cp);
			if (j != std::end(font.glyphs)) {
				x += j->second.pc.advance;
				rect.ex = Max(x, rect.ex);
			}
		}
	}

	return rect;
}

//
void DrawText(bgfx::ViewId view_id, const Font &font, const char *text, bgfx::ProgramHandle program, const char *page_uniform, uint8_t page_stage,
	const Mat4 *mtxs, size_t mtx_count, Vec3 pos, DrawTextHAlign halign, DrawTextVAlign valign, const std::vector<UniformSetValue> &values,
	const std::vector<UniformSetTexture> &textures, RenderState state, uint32_t depth) {
	const auto xline = pos.x;

	std::vector<utf32_cp> cps;
	cps.reserve(256);

	fRect box = ComputeTextRect(font, text);
	const float width = GetWidth(box);
	const float height = GetHeight(box);

	if (valign == DTVA_Center)
		pos.y -= -height * 0.5f;
	else if (valign == DTVA_Bottom)
		pos.y -= -height;
	else // WVA_Top
		;

	while (*text) {
		while (true) {
			if (*text == '\n') {
				++text;
				break; // EOL
			}

			if (*text == '\0')
				break; // EOS

			utf32_cp cp;
			text += utf8_to_utf32((const utf8_cp *)text, cp);
			cps.push_back(cp);
		}

		if (halign != DTHA_Left) {
			float line_width = ComputeTextWidth(font, cps);
			if (halign == DTHA_Center) {
				pos.x = xline - line_width / 2.f;
			} else { // WHA_Right
				pos.x = xline - line_width;
			}
		} else {
			pos.x = xline;
		}

		if (!cps.empty()) {
			DrawText(view_id, font, cps, program, page_uniform, page_stage, mtxs, mtx_count, pos, values, textures, state, depth);
			cps.clear();
		}

		pos.y += font.ascent;
	}
}

void DrawText(bgfx::ViewId view_id, const Font &font, const char *text, bgfx::ProgramHandle program, const char *page_uniform, uint8_t page_stage, const Mat4 &mtx,
	Vec3 pos, DrawTextHAlign halign, DrawTextVAlign valign, const std::vector<UniformSetValue> &values, const std::vector<UniformSetTexture> &textures,
	RenderState state, uint32_t depth) {
	DrawText(view_id, font, text, program, page_uniform, page_stage, &mtx, 1, pos, halign, valign, values, textures, state, depth);
}

//
fRect ComputeTextRect(const Font &font, const char *text, float xpos, float ypos) {
	std::vector<utf32_cp> cps;
	convert_utf8_to_utf32(text, cps);
	return ComputeTextRect(font, cps, xpos, ypos);
}

float ComputeTextHeight(const Font &font, const char *text) {
	float line_count = 1.f;
	while (*text)
		if (*text++ == '\n')
			line_count += 1.f;
	return line_count * font.ascent;
}

} // namespace hg
