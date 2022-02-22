// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/motion_blur.h"

#include "foundation/file_rw_interface.h"
#include "foundation/format.h"
#include "foundation/projection.h"

#include "engine/assets_rw_interface.h"

namespace hg {

bool IsValid(const MotionBlur &motion_blur) {
	return bgfx::isValid(motion_blur.prg_motion_blur) && bgfx::isValid(motion_blur.u_color) && bgfx::isValid(motion_blur.u_attr0) &&
		   bgfx::isValid(motion_blur.u_attr1) && bgfx::isValid(motion_blur.u_noise);
}

static MotionBlur _CreateMotionBlur(const Reader &ir, const ReadProvider &ip, const char *path) {
	MotionBlur motion_blur;

	motion_blur.prg_motion_blur = LoadProgram(ir, ip, format("%1/shader/motion_blur").arg(path));
	motion_blur.u_color = bgfx::createUniform("u_color", bgfx::UniformType::Sampler);
	motion_blur.u_attr0 = bgfx::createUniform("u_attr0", bgfx::UniformType::Sampler);
	motion_blur.u_attr1 = bgfx::createUniform("u_attr1", bgfx::UniformType::Sampler);
	motion_blur.u_noise = bgfx::createUniform("u_noise", bgfx::UniformType::Sampler);

	if (!IsValid(motion_blur)) {
		DestroyMotionBlur(motion_blur);
	}
	return motion_blur;
}

MotionBlur CreateMotionBlurFromFile(const char *path) { return _CreateMotionBlur(g_file_reader, g_file_read_provider, path); }
MotionBlur CreateMotionBlurFromAssets(const char *path) { return _CreateMotionBlur(g_assets_reader, g_assets_read_provider, path); }

void DestroyMotionBlur(MotionBlur &motion_blur) {
	bgfx_Destroy(motion_blur.prg_motion_blur);
	bgfx_Destroy(motion_blur.u_color);
	bgfx_Destroy(motion_blur.u_attr0);
	bgfx_Destroy(motion_blur.u_attr1);
	bgfx_Destroy(motion_blur.u_noise);
}

void ApplyMotionBlur(bgfx::ViewId &view_id, const iRect &rect, const Texture &color, const Texture &attr0, const Texture &attr1, const Texture &noise,
	bgfx::FrameBufferHandle output, const MotionBlur &motion_blur) {
	__ASSERT__(IsValid(motion_blur));

	const bgfx::Caps *caps = bgfx::getCaps();

	bgfx::TransientIndexBuffer idx;
	bgfx::TransientVertexBuffer vtx;
	CreateFullscreenQuad(idx, vtx);

	float ortho[16];
	memcpy(ortho, to_bgfx(Compute2DProjectionMatrix(0.f, 100.f, 1.f, 1.f, false)).data(), sizeof(float[16]));

	bgfx::setViewName(view_id, "Motion Blur");
	bgfx::setViewRect(view_id, rect.sx, rect.sy, GetWidth(rect), GetHeight(rect));
	bgfx::setViewFrameBuffer(view_id, output);
	bgfx::setViewTransform(view_id, NULL, ortho);
	bgfx::setViewClear(view_id, BGFX_CLEAR_NONE, 0, 1.f, UINT8_MAX);
	bgfx::setTexture(0, motion_blur.u_color, color.handle, uint32_t(color.flags));
	bgfx::setTexture(1, motion_blur.u_attr0, attr0.handle, uint32_t(attr0.flags));
	bgfx::setTexture(2, motion_blur.u_attr1, attr1.handle, uint32_t(attr1.flags));
	bgfx::setTexture(3, motion_blur.u_noise, noise.handle, uint32_t(noise.flags));

	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_ALWAYS);
	bgfx::setIndexBuffer(&idx);
	bgfx::setVertexBuffer(0, &vtx);
	bgfx::submit(view_id, motion_blur.prg_motion_blur);
	view_id++;
}

} // namespace hg
