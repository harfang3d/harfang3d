// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/taa.h"

#include "foundation/file_rw_interface.h"
#include "foundation/format.h"
#include "foundation/projection.h"

#include "engine/assets_rw_interface.h"

namespace hg {

bool IsValid(const TAA &taa) {
	return bgfx::isValid(taa.u_color) && bgfx::isValid(taa.u_prv_color) && bgfx::isValid(taa.u_attr0) && bgfx::isValid(taa.u_attr1);
}

static TAA _CreateTAA(const Reader &ir, const ReadProvider &ip, const char *path) {
	TAA taa;

	taa.prg_taa = LoadProgram(ir, ip, format("%1/shader/taa").arg(path));
	taa.u_color = bgfx::createUniform("u_color", bgfx::UniformType::Sampler);
	taa.u_prv_color = bgfx::createUniform("u_prv_color", bgfx::UniformType::Sampler);
	taa.u_attr0 = bgfx::createUniform("u_attr0", bgfx::UniformType::Sampler);
	taa.u_attr1 = bgfx::createUniform("u_attr1", bgfx::UniformType::Sampler);

	if (!IsValid(taa)) {
		DestroyTAA(taa);
	}
	return taa;
}

TAA CreateTAAFromFile(const char *path) { return _CreateTAA(g_file_reader, g_file_read_provider, path); }
TAA CreateTAAFromAssets(const char *path) { return _CreateTAA(g_assets_reader, g_assets_read_provider, path); }

void DestroyTAA(TAA &taa) {
	bgfx_Destroy(taa.prg_taa);
	bgfx_Destroy(taa.u_color);
	bgfx_Destroy(taa.u_prv_color);
	bgfx_Destroy(taa.u_attr0);
	bgfx_Destroy(taa.u_attr1);
}

void ApplyTAA(bgfx::ViewId &view_id, const iRect &rect, const Texture &color, const Texture &prv_color, const Texture &attr0, const Texture &attr1,
	bgfx::FrameBufferHandle output, const TAA &taa) {
	__ASSERT__(IsValid(taa));

	bgfx::TransientIndexBuffer idx;
	bgfx::TransientVertexBuffer vtx;
	CreateFullscreenQuad(idx, vtx);

	float ortho[16];
	memcpy(ortho, to_bgfx(Compute2DProjectionMatrix(0.f, 100.f, 1.f, 1.f, false)).data(), sizeof(float[16]));

	bgfx::setViewName(view_id, "TAA");
	bgfx::setViewRect(view_id, rect.sx, rect.sy, GetWidth(rect), GetHeight(rect));
	bgfx::setViewFrameBuffer(view_id, output);
	bgfx::setViewTransform(view_id, NULL, ortho);
	bgfx::setViewClear(view_id, BGFX_CLEAR_NONE, 0, 1.f, UINT8_MAX);
	bgfx::setTexture(0, taa.u_color, color.handle, uint32_t(color.flags));
	bgfx::setTexture(1, taa.u_prv_color, prv_color.handle, uint32_t(prv_color.flags));
	bgfx::setTexture(2, taa.u_attr0, attr0.handle, uint32_t(attr0.flags));
	bgfx::setTexture(3, taa.u_attr1, attr1.handle, uint32_t(attr1.flags));

	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_ALWAYS);
	bgfx::setIndexBuffer(&idx);
	bgfx::setVertexBuffer(0, &vtx);
	bgfx::submit(view_id, taa.prg_taa);
	view_id++;
}

Vec2 TAAProjectionJitter8(int frame) {
	static const Vec2 grid[8] = {Vec2(-7.f, 1.f) / 8.f, Vec2(-5.f, -5.f) / 8.f, Vec2(-1.f, -3.f) / 8.f, Vec2(3.f, -7.f) / 8.f, Vec2(5.f, -1.f) / 8.f,
		Vec2(7.f, 7.f) / 8.f, Vec2(1.f, 3.f) / 8.f, Vec2(-3.f, 5.f) / 8.f};
	return grid[frame & 7];
}

Vec2 TAAProjectionJitter16(int frame) {
	static const Vec2 grid[16] = {Vec2(-8.f, 0.f) / 8.f, Vec2(-6.f, -4.f) / 8.f, Vec2(-3.f, -2.f) / 8.f, Vec2(-2.f, -6.f) / 8.f, Vec2(1.f, -1.f) / 8.f,
		Vec2(2.f, -5.f) / 8.f, Vec2(6.f, -7.f) / 8.f, Vec2(5.f, -3.f) / 8.f, Vec2(4.f, 1.f) / 8.f, Vec2(7.f, 4.f) / 8.f, Vec2(3.f, 5.f) / 8.f,
		Vec2(0.f, 7.f) / 8.f, Vec2(-1.f, 3.f) / 8.f, Vec2(-4.f, 6.f) / 8.f, Vec2(-7.f, 8.f) / 8.f, Vec2(-5.f, 2.f) / 8.f};
	return grid[frame & 15];
}

Vec2 TAAHaltonJitter8(int frame) {
	static const Vec2 grid[8] = {Vec2(0.000000f, -0.333333f), Vec2(-0.500000f, 0.333333f), Vec2(0.500000f, -0.777778f), Vec2(-0.750000f, -0.111111f),
		Vec2(0.250000f, 0.555556f), Vec2(-0.250000f, -0.555556f), Vec2(0.750000f, 0.111111f), Vec2(-0.875000f, 0.777778f)};
	return grid[frame & 7];
}

Vec2 TAAHaltonJitter16(int frame) {
	static const Vec2 grid[16] = {Vec2(0.000000f, -0.333333f), Vec2(-0.500000f, 0.333333f), Vec2(0.500000f, -0.777778f), Vec2(-0.750000f, -0.111111f),
		Vec2(0.250000f, 0.555556f), Vec2(-0.250000f, -0.555556f), Vec2(0.750000f, 0.111111f), Vec2(-0.875000f, 0.777778f), Vec2(0.125000f, -0.925926f),
		Vec2(-0.375000f, -0.259259f), Vec2(0.625000f, 0.407407f), Vec2(-0.625000f, -0.703704f), Vec2(0.375000f, -0.037037f), Vec2(-0.125000f, 0.629630f),
		Vec2(0.875000f, -0.481481f), Vec2(-0.937500f, 0.185185f)};
	return grid[frame & 15];
}

} // namespace hg
