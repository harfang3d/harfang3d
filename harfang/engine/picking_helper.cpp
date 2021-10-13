// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/picking_helper.h"

namespace hg {

void PickingHelper::Create(int w, int h) {
	if (width == w && height == h)
		return;

	Destroy();

	color = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::RGBA8,
		BGFX_TEXTURE_RT | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
	color_read = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::RGBA8,
		BGFX_TEXTURE_BLIT_DST | BGFX_TEXTURE_READ_BACK | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_U_CLAMP |
			BGFX_SAMPLER_V_CLAMP);

	depth = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::D24S8,
		BGFX_TEXTURE_RT | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
	depth_read = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::D24S8,
		BGFX_TEXTURE_BLIT_DST | BGFX_TEXTURE_READ_BACK | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_U_CLAMP |
			BGFX_SAMPLER_V_CLAMP);

	bgfx::TextureHandle texs[2] = {color, depth};
	fb = bgfx::createFrameBuffer(2, texs, false);

	width = w;
	height = h;
}

void PickingHelper::Destroy() {
	if (!bgfx::isValid(fb))
		return;

	bgfx::destroy(color);
	bgfx::destroy(color_read);
	color = BGFX_INVALID_HANDLE;
	color_read = BGFX_INVALID_HANDLE;

	bgfx::destroy(depth);
	bgfx::destroy(depth_read);
	depth = BGFX_INVALID_HANDLE;
	depth_read = BGFX_INVALID_HANDLE;

	bgfx::destroy(fb);
	fb = BGFX_INVALID_HANDLE;
}

void PickingHelper::Readback(bgfx::ViewId &view_id, PickingResults &res) const {
	res.width = width;
	res.height = height;

	res.color_data.resize(width * height * 4);
	res.depth_data.resize(width * height * 4);

	bgfx::touch(view_id);

	bgfx::blit(view_id, color_read, 0, 0, color);
	res.result_frame = bgfx::readTexture(color_read, res.color_data.data());

	++view_id;

	bgfx::blit(view_id, depth_read, 0, 0, depth);
	res.result_frame = bgfx::readTexture(depth_read, res.depth_data.data());
}

PickingHelper::~PickingHelper() { Destroy(); }

} // namespace hg
