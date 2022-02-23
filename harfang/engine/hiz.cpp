// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/hiz.h"

#include "foundation/file_rw_interface.h"
#include "foundation/format.h"
#include "foundation/projection.h"

#include "engine/assets_rw_interface.h"

namespace hg {

bool IsValid(const HiZ &hiz) {
	return bgfx::isValid(hiz.pyramid.handle) && bgfx::isValid(hiz.prg_copy) && bgfx::isValid(hiz.prg_compute) && bgfx::isValid(hiz.u_depth) &&
		   bgfx::isValid(hiz.u_projection) && bgfx::isValid(hiz.u_zThickness);
}

static HiZ _CreateHiZ(
	const Reader &ir, const ReadProvider &ip, const char *path, const RenderBufferResourceFactory &rb_factory, bgfx::BackbufferRatio::Enum ratio) {
	HiZ hiz;

	hiz.u_depth = bgfx::createUniform("u_depth", bgfx::UniformType::Sampler);
	hiz.u_projection = bgfx::createUniform("u_projection", bgfx::UniformType::Mat4);
	hiz.u_zThickness = bgfx::createUniform("u_zThickness", bgfx::UniformType::Vec4);

	static const uint64_t flags =
		0 | BGFX_TEXTURE_RT | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;

	hiz.ratio = ratio;

	hiz.pyramid_infos.depth = 0;
	hiz.pyramid_infos.cubeMap = false;
	hiz.pyramid_infos.numLayers = 1;
	hiz.pyramid_infos.numMips = 1; // exact number of mips will be computed in ComputeHiZ
	hiz.pyramid_infos.format = bgfx::TextureFormat::RG32F;

	bgfx::TextureHandle handle = rb_factory.create_texture2d(
		ratio, hiz.pyramid_infos.numMips > 0,
		hiz.pyramid_infos.numLayers, hiz.pyramid_infos.format, BGFX_TEXTURE_COMPUTE_WRITE | flags);

	hiz.pyramid = MakeTexture(handle, BGFX_TEXTURE_COMPUTE_WRITE | flags);
	hiz.prg_copy = LoadComputeProgram(ir, ip, format("%1/shader/hiz_copy_cs.sc").arg(path));
	hiz.prg_compute = LoadComputeProgram(ir, ip, format("%1/shader/hiz_compute_cs.sc").arg(path));

	if (!IsValid(hiz)) {
		DestroyHiZ(hiz);
		return hiz;
	}

	bgfx::setName(handle, "hiz.pyramid");
	return hiz;
}

HiZ CreateHiZFromFile(const char *path, const RenderBufferResourceFactory &rb_factory, bgfx::BackbufferRatio::Enum ratio) {
	return _CreateHiZ(g_file_reader, g_file_read_provider, path, rb_factory, ratio);
}
HiZ CreateHiZFromAssets(const char *path, const RenderBufferResourceFactory &rb_factory, bgfx::BackbufferRatio::Enum ratio) {
	return _CreateHiZ(g_assets_reader, g_assets_read_provider, path, rb_factory, ratio);
}

void DestroyHiZ(HiZ &hiz) {
	bgfx_Destroy(hiz.pyramid.handle);
	bgfx_Destroy(hiz.prg_copy);
	bgfx_Destroy(hiz.prg_compute);
	bgfx_Destroy(hiz.u_depth);
	bgfx_Destroy(hiz.u_projection);
}

void ComputeHiZ(
	bgfx::ViewId &view_id, const hg::iVec2 &fb_size, const iRect &rect, const Mat44 &proj, float z_thickness, const Texture &depth, HiZ &hiz) {
	__ASSERT__(IsValid(hiz));


	int div = 1;
	switch (hiz.ratio) {
		case bgfx::BackbufferRatio::Half:
			div = 2;
			break;
		case bgfx::BackbufferRatio::Quarter:
			div = 4;
			break;
		case bgfx::BackbufferRatio::Eighth:
			div = 8;
			break;
		case bgfx::BackbufferRatio::Sixteenth:
			div = 16;
			break;
		default:
			div = 1;
			break;
	};
	auto width = fb_size.x;
	auto height = fb_size.y;
	bgfx::calcTextureSize(hiz.pyramid_infos, width / div, height / div, hiz.pyramid_infos.depth, hiz.pyramid_infos.cubeMap, hiz.pyramid_infos.numMips,
		hiz.pyramid_infos.numLayers, hiz.pyramid_infos.format);

	bgfx::setUniform(hiz.u_projection, to_bgfx(proj).data());

	float params[4] = {z_thickness, 0.f, 0.f, 0.f};
	bgfx::setUniform(hiz.u_zThickness, params);

	bgfx::setViewName(view_id, "HiZ copy");
	bgfx::setViewRect(view_id, rect.sx, rect.sy, GetWidth(rect), GetHeight(rect));
	bgfx::setTexture(0, hiz.u_depth, depth.handle);
	bgfx::setImage(1, hiz.pyramid.handle, 0, bgfx::Access::Write);

	bgfx::dispatch(view_id, hiz.prg_copy, (hiz.pyramid_infos.width + 15) / 16, (hiz.pyramid_infos.height + 15) / 16);
	view_id++;

	for (int lod = 1; lod < hiz.pyramid_infos.numMips; lod++) {
		bgfx::setViewName(view_id, format("HiZ level %1").arg(lod).c_str());
		bgfx::setImage(0, hiz.pyramid.handle, lod - 1, bgfx::Access::Read);
		bgfx::setImage(1, hiz.pyramid.handle, lod, bgfx::Access::Write);

		int w = hiz.pyramid_infos.width >> lod;
		int h = hiz.pyramid_infos.height >> lod;
		bgfx::dispatch(view_id, hiz.prg_compute, (w + 15) / 16, (h + 15) / 16);
		view_id++;
	}
}

} // namespace hg
