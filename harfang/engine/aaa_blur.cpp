// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/aaa_blur.h"

#include "foundation/file_rw_interface.h"
#include "foundation/format.h"
#include "foundation/projection.h"

#include "engine/assets_rw_interface.h"

#define MAX_A_TROUS_ITERATIONS 3

namespace hg {

static AAABlur _CreateAAABlur(const Reader &ir, const ReadProvider &ip, const char *path) {
	AAABlur aaa_blur;
	aaa_blur.compute = LoadProgram(ir, ip, format("%1/shader/a_trous").arg(path));
	aaa_blur.u_dir = bgfx::createUniform("u_dir", bgfx::UniformType::Vec4);
	aaa_blur.u_sigma = bgfx::createUniform("u_sigma", bgfx::UniformType::Vec4);
	aaa_blur.u_input = bgfx::createUniform("u_input", bgfx::UniformType::Sampler);
	aaa_blur.u_attr0 = bgfx::createUniform("u_attr0", bgfx::UniformType::Sampler);
	if (!IsValid(aaa_blur)) {
		DestroyAAABlur(aaa_blur);
	}
	return aaa_blur;
}

AAABlur CreateAAABlurFromFile(const char *path) { return _CreateAAABlur(g_file_reader, g_file_read_provider, path); }
AAABlur CreateAAABlurFromAssets(const char *path) { return _CreateAAABlur(g_assets_reader, g_assets_read_provider, path); }

bool IsValid(const AAABlur &aaa_blur) {
	return bgfx::isValid(aaa_blur.compute) && bgfx::isValid(aaa_blur.u_dir) && bgfx::isValid(aaa_blur.u_sigma) && bgfx::isValid(aaa_blur.u_input) &&
		   bgfx::isValid(aaa_blur.u_attr0);
}

void DestroyAAABlur(AAABlur &aaa_blur) {
	bgfx_Destroy(aaa_blur.compute);
	bgfx_Destroy(aaa_blur.u_dir);
	bgfx_Destroy(aaa_blur.u_sigma);
	bgfx_Destroy(aaa_blur.u_input);
	bgfx_Destroy(aaa_blur.u_attr0);
}

void ComputeAAABlur(
	bgfx::ViewId &view_id, const iRect &rect, const Texture &attr0, bgfx::FrameBufferHandle fb0, bgfx::FrameBufferHandle fb1, const AAABlur &aaa_blur) {
	__ASSERT__(IsValid(aaa_blur));
	bgfx::TransientIndexBuffer idx;
	bgfx::TransientVertexBuffer vtx;
	CreateFullscreenQuad(idx, vtx);

	float ortho[16];
	memcpy(ortho, to_bgfx(Compute2DProjectionMatrix(0.f, 100.f, 1.f, 1.f, false)).data(), sizeof(float[16]));

	float direction[5] = {0.f, 1.f, 0.f, 0.f, 0.f};

	static const uint64_t texture_flags = 0 | BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;

	bgfx::FrameBufferHandle source = fb0;
	bgfx::FrameBufferHandle destination = fb1;

	float sigma[4] = {0.3f * (12.f - 1.f) + 0.8f, 32.0f, 1.1f, 0.1f};
	bgfx::setUniform(aaa_blur.u_sigma, sigma);

	for (int j = 0; j < MAX_A_TROUS_ITERATIONS; j++) {
		for (int i = 0; i < 2; i++) {
			bgfx::setViewName(view_id, "AAA blur");
			bgfx::setViewRect(view_id, rect.sx, rect.sy, GetWidth(rect), GetHeight(rect));
			bgfx::setViewFrameBuffer(view_id, destination);
			bgfx::setViewTransform(view_id, NULL, ortho);
			bgfx::setViewClear(view_id, BGFX_CLEAR_NONE, 0, 1.f, UINT8_MAX);
			bgfx::setTexture(0, aaa_blur.u_input, bgfx::getTexture(source), uint32_t(texture_flags));
			bgfx::setTexture(1, aaa_blur.u_attr0, attr0.handle, uint32_t(attr0.flags));

			bgfx::setUniform(aaa_blur.u_dir, &direction[i ^ 1]);

			bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_ALWAYS);
			bgfx::setIndexBuffer(&idx);
			bgfx::setVertexBuffer(0, &vtx);
			bgfx::submit(view_id, aaa_blur.compute);
			view_id++;

			bgfx::FrameBufferHandle tmp = source;
			source = destination;
			destination = tmp;
		}
		direction[1] *= 2.f;
	}
}

} // namespace hg
