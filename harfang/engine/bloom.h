// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "render_pipeline.h"

#include <bgfx/bgfx.h>
#include <vector>

#include <foundation/rect.h>

namespace hg {

struct Bloom {
	mutable bgfx::FrameBufferHandle in_fb = BGFX_INVALID_HANDLE;
	mutable bgfx::FrameBufferHandle out_fb = BGFX_INVALID_HANDLE;

	bgfx::ProgramHandle prg_threshold = BGFX_INVALID_HANDLE;
	bgfx::ProgramHandle prg_downsample = BGFX_INVALID_HANDLE;
	bgfx::ProgramHandle prg_upsample = BGFX_INVALID_HANDLE;
	bgfx::ProgramHandle prg_combine = BGFX_INVALID_HANDLE;

	bgfx::UniformHandle u_source = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_input = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_params = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_source_rect = BGFX_INVALID_HANDLE;
};

Bloom CreateBloomFromFile(const char *path, bgfx::BackbufferRatio::Enum ratio);
Bloom CreateBloomFromAssets(const char *path, bgfx::BackbufferRatio::Enum ratio);

void DestroyBloom(Bloom &bloom);

void ApplyBloom(bgfx::ViewId &view_id, const iRect &rect, const hg::Texture &input, bgfx::FrameBufferHandle output, const Bloom &bloom, float threshold,
	float smoothness, float intensity);

bool IsValid(const Bloom &bloom);

} // namespace hg
