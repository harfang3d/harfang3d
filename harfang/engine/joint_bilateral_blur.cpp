// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/joint_bilateral_blur.h"

#include "foundation/file_rw_interface.h"
#include "foundation/format.h"
#include "foundation/projection.h"

#include "engine/assets_rw_interface.h"

namespace hg {

static JointBilateralBlur _CreateJointBilateralBlur(const Reader &ir, const ReadProvider &ip, const char *path) {
	JointBilateralBlur blur;
	blur.compute = hg::LoadProgram(ir, ip, hg::format("%1/shader/joint_bilateral_blur").arg(path));
	blur.u_input = bgfx::createUniform("u_input", bgfx::UniformType::Sampler);
	blur.u_attr0 = bgfx::createUniform("u_attr0", bgfx::UniformType::Sampler);
	
	static const uint64_t flags =
		0 | BGFX_TEXTURE_RT | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;

	blur.work = {flags, bgfx::createTexture2D(bgfx::BackbufferRatio::Half, false, 1, bgfx::TextureFormat::RGBA32F, flags)};
	bgfx::setName(blur.work.handle, "blur.work");

	blur.fb = bgfx::createFrameBuffer(1, &blur.work.handle, true);
	bgfx::setName(blur.fb, "Blur FB");

    return blur;
}

JointBilateralBlur CreateJointBilateralBlurFromFile(const char *path) { return _CreateJointBilateralBlur(g_file_reader, g_file_read_provider, path); }
JointBilateralBlur CreateJointBilateralBlurFromAssets(const char *path) { return _CreateJointBilateralBlur(g_assets_reader, g_assets_read_provider, path); }

void DestroyJointBilateralBlur(JointBilateralBlur &blur) {
	bgfx_Destroy(blur.compute);
	bgfx_Destroy(blur.u_input);
	bgfx_Destroy(blur.u_attr0);
	bgfx_Destroy(blur.fb);
	blur.work = {};
}

void ComputeJointBilateralBlur(
	bgfx::ViewId &view_id, const iRect &rect, const Texture &input, const Texture &attr0, const JointBilateralBlur &blur) {
	bgfx::TransientIndexBuffer idx;
	bgfx::TransientVertexBuffer vtx;
	CreateFullscreenQuad(idx, vtx);

	float ortho[16];
	memcpy(ortho, hg::to_bgfx(hg::Compute2DProjectionMatrix(0.f, 100.f, 1.f, 1.f, false)).data(), sizeof(float[16]));

	bgfx::setViewName(view_id, "Blur");
	bgfx::setViewRect(view_id, rect.sx, rect.sy, GetWidth(rect), GetHeight(rect));
	bgfx::setViewFrameBuffer(view_id, blur.fb);
	bgfx::setViewTransform(view_id, NULL, ortho);
	bgfx::setViewClear(view_id, BGFX_CLEAR_NONE, 0, 1.f, UINT8_MAX);
	bgfx::setTexture(0, blur.u_input, input.handle, uint32_t(input.flags));
	bgfx::setTexture(1, blur.u_attr0, attr0.handle, uint32_t(attr0.flags));

	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_ALWAYS);
	bgfx::setIndexBuffer(&idx);
	bgfx::setVertexBuffer(0, &vtx);
	bgfx::submit(view_id, blur.compute);
	view_id++;
}

} // namespace hg
