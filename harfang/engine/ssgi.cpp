// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/ssgi.h"

#include "foundation/file_rw_interface.h"
#include "foundation/format.h"
#include "foundation/projection.h"

#include "engine/assets_rw_interface.h"

namespace hg {

bool IsValid(const SSGI &ssgi) {
	return bgfx::isValid(ssgi.compute) && bgfx::isValid(ssgi.u_color) && bgfx::isValid(ssgi.u_attr0) && bgfx::isValid(ssgi.u_attr1) &&
		   bgfx::isValid(ssgi.u_noise) && bgfx::isValid(ssgi.u_probe) && bgfx::isValid(ssgi.u_depthTex) && bgfx::isValid(ssgi.u_depthTexInfos);
}

static SSGI _CreateSSGI(const Reader &ir, const ReadProvider &ip, const char *path) {
	SSGI ssgi;
	ssgi.compute = LoadProgram(ir, ip, format("%1/shader/ssgi").arg(path));
	ssgi.u_color = bgfx::createUniform("u_color", bgfx::UniformType::Sampler);
	ssgi.u_attr0 = bgfx::createUniform("u_attr0", bgfx::UniformType::Sampler);
	ssgi.u_attr1 = bgfx::createUniform("u_attr1", bgfx::UniformType::Sampler);
	ssgi.u_noise = bgfx::createUniform("u_noise", bgfx::UniformType::Sampler);
	ssgi.u_probe = bgfx::createUniform("u_probe", bgfx::UniformType::Sampler);
	ssgi.u_depthTex = bgfx::createUniform("u_depthTex", bgfx::UniformType::Sampler);
	ssgi.u_depthTexInfos = bgfx::createUniform("u_depthTexInfos", bgfx::UniformType::Vec4);

	if (!IsValid(ssgi)) {
		DestroySSGI(ssgi);
	}
	return ssgi;
}

SSGI CreateSSGIFromFile(const char *path) { return _CreateSSGI(g_file_reader, g_file_read_provider, path); }
SSGI CreateSSGIFromAssets(const char *path) { return _CreateSSGI(g_assets_reader, g_assets_read_provider, path); }

void DestroySSGI(SSGI &ssgi) {
	bgfx_Destroy(ssgi.compute);
	bgfx_Destroy(ssgi.u_color);
	bgfx_Destroy(ssgi.u_attr0);
	bgfx_Destroy(ssgi.u_attr1);
	bgfx_Destroy(ssgi.u_probe);
	bgfx_Destroy(ssgi.u_noise);
	bgfx_Destroy(ssgi.u_depthTex);
	bgfx_Destroy(ssgi.u_depthTexInfos);
}

void ComputeSSGI(bgfx::ViewId &view_id, const iRect &rect, bgfx::BackbufferRatio::Enum ratio, const Texture &color, const Texture &attr0, const Texture &attr1,
	const Texture &probe, const Texture &noise, const HiZ &hiz, bgfx::FrameBufferHandle output, const SSGI &ssgi) {
	__ASSERT__(IsValid(ssgi));

	bgfx::TransientIndexBuffer idx;
	bgfx::TransientVertexBuffer vtx;
	CreateFullscreenQuad(idx, vtx);

	float ortho[16];
	memcpy(ortho, to_bgfx(Compute2DProjectionMatrix(0.f, 100.f, 1.f, 1.f, false)).data(), sizeof(float[16]));

	bgfx::setViewName(view_id, "SSGI");
	bgfx::setViewRect(view_id, rect.sx, rect.sy, GetWidth(rect), GetHeight(rect));
	bgfx::setViewFrameBuffer(view_id, output);
	bgfx::setViewTransform(view_id, NULL, ortho);
	bgfx::setViewClear(view_id, BGFX_CLEAR_NONE, 0, 1.f, UINT8_MAX);
	bgfx::setTexture(0, ssgi.u_color, color.handle, uint32_t(color.flags));
	bgfx::setTexture(1, ssgi.u_attr0, attr0.handle, uint32_t(attr0.flags));
	bgfx::setTexture(2, ssgi.u_attr1, attr1.handle, uint32_t(attr1.flags));
	bgfx::setTexture(3, ssgi.u_noise, noise.handle, uint32_t(noise.flags));
	bgfx::setTexture(4, ssgi.u_probe, probe.handle, uint32_t(probe.flags));
	bgfx::setTexture(5, ssgi.u_depthTex, hiz.pyramid.handle, uint32_t(hiz.pyramid.flags));

	float params[4];
	params[0] = float(hiz.pyramid_infos.width);
	params[1] = float(hiz.pyramid_infos.height);
	params[2] = float(ratio - hiz.ratio); // [todo] assert(ratio >= hiz.ratio) ?
	params[3] = float(hiz.pyramid_infos.numMips - 1);
	bgfx::setUniform(ssgi.u_depthTexInfos, params);

	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_ALWAYS);
	bgfx::setIndexBuffer(&idx);
	bgfx::setVertexBuffer(0, &vtx);
	bgfx::submit(view_id, ssgi.compute);
	view_id++;
}

} // namespace hg
