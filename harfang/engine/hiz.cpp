// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/hiz.h"

#include "foundation/file_rw_interface.h"
#include "foundation/format.h"
#include "foundation/projection.h"

#include "engine/assets_rw_interface.h"

namespace hg {

bool IsValid(const HiZ &hiz) {
	return bgfx::isValid(hiz.pyramid.handle) && bgfx::isValid(hiz.prg_copy) && bgfx::isValid(hiz.prg_compute) && bgfx::isValid(hiz.u_depth) &&
		   bgfx::isValid(hiz.u_projection);
}

static HiZ _CreateHiZ(const Reader &ir, const ReadProvider &ip, const char *path, bgfx::BackbufferRatio::Enum ratio) {
	HiZ hiz;

	hiz.u_depth = bgfx::createUniform("u_depth", bgfx::UniformType::Sampler);
	hiz.u_projection = bgfx::createUniform("u_projection", bgfx::UniformType::Mat4);

	static const uint64_t flags =
		0 | BGFX_TEXTURE_RT | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
		
	hiz.ratio = ratio;
	
	const bgfx::Stats *stats = bgfx::getStats();
	int div = 1;
	switch (ratio) {
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
	bgfx::calcTextureSize(hiz.pyramid_infos, stats->width / div, stats->height / div, 0, false, true, 1, bgfx::TextureFormat::RG32F);

	bgfx::TextureHandle handle = bgfx::createTexture2D(hiz.pyramid_infos.width, hiz.pyramid_infos.height, hiz.pyramid_infos.numMips,
		hiz.pyramid_infos.numLayers, hiz.pyramid_infos.format, BGFX_TEXTURE_COMPUTE_WRITE | flags);
	
	hiz.pyramid = hg::MakeTexture(handle, BGFX_TEXTURE_COMPUTE_WRITE | flags);
	hiz.prg_copy = hg::LoadComputeProgram(ir, ip, hg::format("%1/shader/hiz_copy_cs.sc").arg(path));
	hiz.prg_compute = hg::LoadComputeProgram(ir, ip, hg::format("%1/shader/hiz_compute_cs.sc").arg(path));
	
	if (!IsValid(hiz)) {
		DestroyHiZ(hiz);
		return hiz;
	}

	bgfx::setName(handle, "hiz.pyramid");
	return hiz;
}

HiZ CreateHiZFromFile(const char *path, bgfx::BackbufferRatio::Enum ratio) { return _CreateHiZ(g_file_reader, g_file_read_provider, path, ratio); }
HiZ CreateHiZFromAssets(const char *path, bgfx::BackbufferRatio::Enum ratio) { return _CreateHiZ(g_assets_reader, g_assets_read_provider, path, ratio); }

void DestroyHiZ(HiZ &hiz) {
	bgfx_Destroy(hiz.pyramid.handle);
	bgfx_Destroy(hiz.prg_copy);
	bgfx_Destroy(hiz.prg_compute);
	bgfx_Destroy(hiz.u_depth);
	bgfx_Destroy(hiz.u_projection);
}

void ComputeHiZ(bgfx::ViewId &view_id, const iRect &rect, const Mat44& proj, const Texture &depth, const HiZ &hiz) {
	__ASSERT__(IsValid(hiz));

	bgfx::setUniform(hiz.u_projection, to_bgfx(proj).data());

	bgfx::setViewName(view_id, "HiZ copy");
	bgfx::setViewRect(view_id, rect.sx, rect.sy, GetWidth(rect), GetHeight(rect));
	bgfx::setTexture(0, hiz.u_depth, depth.handle);
	bgfx::setImage(1, hiz.pyramid.handle, 0, bgfx::Access::Write);

	bgfx::dispatch(view_id, hiz.prg_copy, (hiz.pyramid_infos.width + 15) / 16, (hiz.pyramid_infos.height + 15) / 16);
	view_id++;

	for (int lod = 1; lod < hiz.pyramid_infos.numMips; lod++) {
		bgfx::setViewName(view_id, hg::format("HiZ level %1").arg(lod).c_str());
		bgfx::setImage(0, hiz.pyramid.handle, lod - 1, bgfx::Access::Read);
		bgfx::setImage(1, hiz.pyramid.handle, lod, bgfx::Access::Write);

		int w = hiz.pyramid_infos.width >> lod;
		int h = hiz.pyramid_infos.height >> lod;
		bgfx::dispatch(view_id, hiz.prg_compute, (w + 15) / 16, (h + 15) / 16);
		view_id++;
	}
}

} // namespace hg
