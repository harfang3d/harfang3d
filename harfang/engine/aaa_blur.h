// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/render_pipeline.h"
#include "foundation/rect.h"

namespace hg {

struct AAABlur {
	bgfx::ProgramHandle compute = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_dir = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_sigma = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_input = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_attr0 = BGFX_INVALID_HANDLE; // xyz: normal, w: linear depth
};

AAABlur CreateAAABlurFromFile(const char *path);
AAABlur CreateAAABlurFromAssets(const char *path);

void DestroyAAABlur(AAABlur &aaa_blur);

void ComputeAAABlur(
	bgfx::ViewId &view_id, const iRect &rect, const Texture &attr0, bgfx::FrameBufferHandle fb0, bgfx::FrameBufferHandle fb1, const AAABlur &aaa_blur);

bool IsValid(const AAABlur &aaa_blur);

} // namespace hg
