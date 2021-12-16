// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/render_pipeline.h"
#include "foundation/rect.h"

namespace hg {

struct Upsample {
	bgfx::ProgramHandle compute = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_input = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_attr_lo = BGFX_INVALID_HANDLE; // xyz: normal, w: linear depth
    bgfx::UniformHandle u_attr_hi = BGFX_INVALID_HANDLE; // xyz: normal, w: linear depth
};

Upsample CreateUpsampleFromFile(const char *path);
Upsample CreateUpsampleFromAssets(const char *path);

void DestroyUpsample(Upsample &upsample);

void ComputeUpsample(bgfx::ViewId &view_id, const iRect &rect, const Texture &input, const Texture &attr0_lo, const Texture &attr0_hi, bgfx::FrameBufferHandle output, const Upsample &upsample);

bool IsValid(const Upsample &upsample);

} // namespace hg
