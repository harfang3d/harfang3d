// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/sao.h"

#include "foundation/file_rw_interface.h"
#include "foundation/format.h"
#include "foundation/projection.h"
#include "foundation/rw_interface.h"

#include "engine/assets_rw_interface.h"

namespace hg {

bool IsValid(const SAO &sao) {
	return bgfx::isValid(sao.compute_fb) && bgfx::isValid(sao.blur_fb) && bgfx::isValid(sao.prg_compute) && bgfx::isValid(sao.prg_blur) &&
		   bgfx::isValid(sao.u_attr0) && bgfx::isValid(sao.u_attr1) && bgfx::isValid(sao.u_noise) && bgfx::isValid(sao.u_input) &&
		   bgfx::isValid(sao.u_params) && bgfx::isValid(sao.u_projection_infos) && bgfx::isValid(sao.u_sample_count);
}

static void CreateSAOCommon(SAO &sao, const Reader &ir, const ReadProvider &ip, const char *path) {
	sao.u_attr0 = bgfx::createUniform("u_attr0", bgfx::UniformType::Sampler);
	sao.u_attr1 = bgfx::createUniform("u_attr1", bgfx::UniformType::Sampler);
	sao.u_noise = bgfx::createUniform("u_noise", bgfx::UniformType::Sampler);
	sao.u_input = bgfx::createUniform("u_input", bgfx::UniformType::Sampler);
	sao.u_params = bgfx::createUniform("u_params", bgfx::UniformType::Vec4, 2);
	sao.u_projection_infos = bgfx::createUniform("u_projection_infos", bgfx::UniformType::Vec4);

	sao.prg_compute = LoadProgram(ir, ip, format("%1/shader/sao_compute").arg(path));
	sao.prg_blur = LoadProgram(ir, ip, format("%1/shader/sao_blur").arg(path));
}

static SAO _CreateSAO(const Reader &ir, const ReadProvider &ip, const char *path, bgfx::BackbufferRatio::Enum ratio) {
	SAO sao;
	sao.ratio = ratio;

	const uint64_t flags = 0 | BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;

	sao.compute_fb = bgfx::createFrameBuffer(ratio, bgfx::TextureFormat::R8, flags);
	sao.blur_fb = bgfx::createFrameBuffer(ratio, bgfx::TextureFormat::R8, flags);

	CreateSAOCommon(sao, ir, ip, path);

	if (!IsValid(sao)) {
		DestroySAO(sao);
		return sao;
	}

	bgfx::setName(sao.compute_fb, "SAO.compute_fb");
	bgfx::setName(sao.blur_fb, "SAO.blur_fb");
	return sao;
}

SAO CreateSAOFromFile(const char *path, bgfx::BackbufferRatio::Enum ratio) { return _CreateSAO(g_file_reader, g_file_read_provider, path, ratio); }
SAO CreateSAOFromAssets(const char *path, bgfx::BackbufferRatio::Enum ratio) { return _CreateSAO(g_assets_reader, g_assets_read_provider, path, ratio); }

void DestroySAO(SAO &sao) {
	bgfx_Destroy(sao.compute_fb);
	bgfx_Destroy(sao.blur_fb);

	bgfx_Destroy(sao.u_attr0);
	bgfx_Destroy(sao.u_attr1);
	bgfx_Destroy(sao.u_noise);
	bgfx_Destroy(sao.u_input);
	bgfx_Destroy(sao.u_params);
	bgfx_Destroy(sao.u_projection_infos);

	bgfx_Destroy(sao.prg_compute);
	bgfx_Destroy(sao.prg_blur);
}

void ComputeSAO(bgfx::ViewId &view_id, const iRect &rect, const Texture &attr0, const Texture &attr1, const Texture &noise, bgfx::FrameBufferHandle output,
	const SAO &sao, const Mat44 &projection, float bias, float radius, int sample_count, float sharpness) {
	__ASSERT__(IsValid(sao));

	const bgfx::Caps *caps = bgfx::getCaps();

	bgfx::TransientIndexBuffer idx;
	bgfx::TransientVertexBuffer vtx;
	CreateFullscreenQuad(idx, vtx);

	float projection_infos[4] = {1.f / projection.m[0][0], 1.f / projection.m[1][1], projection.m[2][2], projection.m[2][3]};

	float ortho[16];
	memcpy(ortho, to_bgfx(Compute2DProjectionMatrix(0.f, 100.f, 1.f, 1.f, false)).data(), sizeof(float[16]));

	float params[4];
	memset(&params, 0, sizeof(float[4]));
	params[0] = radius;
	params[1] = GetHeight(rect) / (4.f * projection.m[1][1]);
	params[2] = bias;
	params[3] = sample_count;

	// sao compute
	bgfx::setViewName(view_id, "SAO compute");
	bgfx::setViewRect(view_id, rect.sx, rect.sy, GetWidth(rect), GetHeight(rect));
	bgfx::setViewFrameBuffer(view_id, sao.compute_fb);
	bgfx::setViewTransform(view_id, NULL, ortho);
	bgfx::setViewClear(view_id, BGFX_CLEAR_NONE, 0, 1.f, UINT8_MAX);

	bgfx::setUniform(sao.u_params, &params, 1);
	bgfx::setUniform(sao.u_projection_infos, &projection_infos[0], 1);
	bgfx::setTexture(0, sao.u_attr0, attr0.handle, uint32_t(attr0.flags));
	bgfx::setTexture(1, sao.u_attr1, attr1.handle, uint32_t(attr1.flags));
	bgfx::setTexture(2, sao.u_noise, noise.handle, uint32_t(noise.flags));

	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_ALWAYS);
	bgfx::setIndexBuffer(&idx);
	bgfx::setVertexBuffer(0, &vtx);
	bgfx::submit(view_id, sao.prg_compute);
	view_id++;
	/*
		if (sao.blur) {
			// sao blur (horizontal)
			bgfx::setViewName(view_id, "SAO horizontal blur");
			bgfx::setViewRect(view_id, 0, 0, width, height);
			bgfx::setViewFrameBuffer(view_id, sao.blur_fb);
			bgfx::setViewTransform(view_id, NULL, ortho);
			bgfx::setViewClear(view_id, BGFX_CLEAR_NONE, 0, 1.f, UINT8_MAX);
			params.dx = 1.f / (float)width;
			params.dy = 0.f;
			params.spharpness = sharpness;
			bgfx::setUniform(sao.u_params, &params, 2);
			bgfx::setUniform(sao.u_projection_infos, &projection_infos[0], 1);
			bgfx::setTexture(0, sao.u_depth, bgfx::getTexture(sao.compute_fb), BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
			bgfx::setTexture(1, sao.u_input, depth.handle, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
			bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_ALWAYS);
			bgfx::setIndexBuffer(&idx);
			bgfx::setVertexBuffer(0, &vtx);
			bgfx::submit(view_id, sao.prg_blur);
			view_id++;

			// sao blur (vertical)
			bgfx::setViewName(view_id, "SAO vertical blur");
			bgfx::setViewRect(view_id, 0, 0, width, height);
			bgfx::setViewFrameBuffer(view_id, output);
			bgfx::setViewTransform(view_id, NULL, ortho);
			bgfx::setViewClear(view_id, BGFX_CLEAR_NONE, 0, 1.f, UINT8_MAX);
			params.dx = 0.f;
			params.dy = 1.f / (float)height;
			bgfx::setUniform(sao.u_params, &params, 2);
			bgfx::setUniform(sao.u_projection_infos, &projection_infos[0], 1);
			bgfx::setTexture(0, sao.u_depth, bgfx::getTexture(sao.blur_fb), BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
			bgfx::setTexture(1, sao.u_input, depth.handle, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
			bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_ALWAYS);
			bgfx::setIndexBuffer(&idx);
			bgfx::setVertexBuffer(0, &vtx);
			bgfx::submit(view_id, sao.prg_blur);
			view_id++;
		}
	*/
}

} // namespace hg
