// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/render_pipeline.h"
#include "foundation/rect.h"

namespace hg {

struct HiZ {
	bgfx::BackbufferRatio::Enum ratio;
	bgfx::TextureInfo pyramid_infos;
	hg::Texture pyramid;

	bgfx::ProgramHandle prg_copy = BGFX_INVALID_HANDLE;
	bgfx::ProgramHandle prg_compute = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_depth = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_projection = BGFX_INVALID_HANDLE;
};

HiZ CreateHiZFromFile(const char *path, bgfx::BackbufferRatio::Enum ratio);
HiZ CreateHiZFromAssets(const char *path, bgfx::BackbufferRatio::Enum ratio);

void DestroyHiZ(HiZ &hiz);

/// @note input depth buffer must be in linear depth
void ComputeHiZ(bgfx::ViewId &view_id, const iRect &rect, const Mat44 &proj, const Texture &attr0, const HiZ &hiz);

bool IsValid(const HiZ &hiz);

} // namespace hg
