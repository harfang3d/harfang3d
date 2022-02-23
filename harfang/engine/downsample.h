// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/render_pipeline.h"
#include "foundation/rect.h"

namespace hg {

struct Downsample {
	bgfx::ProgramHandle compute = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_color = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_attr0 = BGFX_INVALID_HANDLE; // xyz: normal, w: linear depth
	bgfx::UniformHandle u_depth = BGFX_INVALID_HANDLE;

	bgfx::FrameBufferHandle fb = BGFX_INVALID_HANDLE;
	Texture color, attr0, depth;
};

Downsample CreateDownsampleFromFile(const char *path, const RenderBufferResourceFactory &rb_factory);
Downsample CreateDownsampleFromAssets(const char *path, const RenderBufferResourceFactory &rb_factory);

void DestroyDownsample(Downsample &downsample);

void ComputeDownsample(
	bgfx::ViewId &view_id, const iRect &rect, const Texture &color, const Texture &attr0, const Texture &depth, const Downsample &downsample);

bool IsValid(const Downsample &downsample);

} // namespace hg
