// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/ssr.h"

#include "foundation/file_rw_interface.h"
#include "foundation/format.h"
#include "foundation/projection.h"

#include "engine/assets_rw_interface.h"

namespace hg {

bool IsValid(const SSR &ssr) {
	return bgfx::isValid(ssr.prg_ssr) && bgfx::isValid(ssr.u_color) && bgfx::isValid(ssr.u_attr0) && bgfx::isValid(ssr.u_attr1) && bgfx::isValid(ssr.u_noise) &&
		   bgfx::isValid(ssr.u_probe) && bgfx::isValid(ssr.u_hiz) && bgfx::isValid(ssr.u_depthTexInfos);
}

static SSR _CreateSSR(const Reader &ir, const ReadProvider &ip, const char *path) {
	SSR ssr;
	ssr.prg_ssr = LoadProgram(ir, ip, format("%1/shader/ssr").arg(path));
	ssr.u_color = bgfx::createUniform("u_color", bgfx::UniformType::Sampler);
	ssr.u_attr0 = bgfx::createUniform("u_attr0", bgfx::UniformType::Sampler);
	ssr.u_attr1 = bgfx::createUniform("u_attr1", bgfx::UniformType::Sampler);
	ssr.u_noise = bgfx::createUniform("u_noise", bgfx::UniformType::Sampler);
	ssr.u_probe = bgfx::createUniform("u_probe", bgfx::UniformType::Sampler);
	ssr.u_hiz = bgfx::createUniform("u_hiz", bgfx::UniformType::Sampler);
	ssr.u_depthTexInfos = bgfx::createUniform("u_depthTexInfos", bgfx::UniformType::Vec4);

	if (!IsValid(ssr)) {
		DestroySSR(ssr);
	}
	return ssr;
}

SSR CreateSSRFromFile(const char *path) { return _CreateSSR(g_file_reader, g_file_read_provider, path); }
SSR CreateSSRFromAssets(const char *path) { return _CreateSSR(g_assets_reader, g_assets_read_provider, path); }

void DestroySSR(SSR &ssr) {
	bgfx_Destroy(ssr.prg_ssr);
	bgfx_Destroy(ssr.u_color);
	bgfx_Destroy(ssr.u_attr0);
	bgfx_Destroy(ssr.u_attr1);
	bgfx_Destroy(ssr.u_probe);
	bgfx_Destroy(ssr.u_noise);
	bgfx_Destroy(ssr.u_hiz);
	bgfx_Destroy(ssr.u_depthTexInfos);
}

void ComputeSSR(bgfx::ViewId &view_id, const iRect &rect, bgfx::BackbufferRatio::Enum ratio, const Texture &color, const Texture &attr0, const Texture &attr1,
	const Texture &probe, const Texture &noise, const HiZ &hiz, bgfx::FrameBufferHandle output, const SSR &ssr) {
	__ASSERT__(IsValid(ssr));

	bgfx::TransientIndexBuffer idx;
	bgfx::TransientVertexBuffer vtx;
	CreateFullscreenQuad(idx, vtx);

	float ortho[16];
	memcpy(ortho, to_bgfx(Compute2DProjectionMatrix(0.f, 100.f, 1.f, 1.f, false)).data(), sizeof(float[16]));

	bgfx::setViewName(view_id, "SSR");
	bgfx::setViewRect(view_id, rect.sx, rect.sy, GetWidth(rect), GetHeight(rect));
	bgfx::setViewFrameBuffer(view_id, output);
	bgfx::setViewTransform(view_id, NULL, ortho);
	bgfx::setViewClear(view_id, BGFX_CLEAR_NONE, 0, 1.f, UINT8_MAX);
	bgfx::setTexture(0, ssr.u_color, color.handle, uint32_t(color.flags));
	bgfx::setTexture(1, ssr.u_attr0, attr0.handle, uint32_t(attr0.flags));
	bgfx::setTexture(2, ssr.u_attr1, attr1.handle, uint32_t(attr1.flags));
	bgfx::setTexture(3, ssr.u_noise, noise.handle, uint32_t(noise.flags));
	bgfx::setTexture(4, ssr.u_probe, probe.handle, uint32_t(probe.flags));
	bgfx::setTexture(5, ssr.u_hiz, hiz.pyramid.handle, uint32_t(hiz.pyramid.flags));

	float params[4];
	params[0] = float(hiz.pyramid_infos.width);
	params[1] = float(hiz.pyramid_infos.height);
	params[2] = float(ratio - hiz.ratio); // [todo] assert(ratio >= hiz.ratio) ?
	params[3] = float(hiz.pyramid_infos.numMips - 1);
	bgfx::setUniform(ssr.u_depthTexInfos, params);

	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_ALWAYS);
	bgfx::setIndexBuffer(&idx);
	bgfx::setVertexBuffer(0, &vtx);
	bgfx::submit(view_id, ssr.prg_ssr);
	view_id++;
}

} // namespace hg
