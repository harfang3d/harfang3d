// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "render_pipeline.h"

#include <bgfx/bgfx.h>
#include <vector>

#include <foundation/rect.h>

namespace hg {

/// Bloom post-process object holding internal states and resources.
/// Create with CreateBloomFromAssets() or CreateBloomFromFile(), use with ApplyBloom(), finally call DestroyBloom() to dispose of resources when done.
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

// for engine internal use
Bloom CreateBloomFromFile(const char *path, const RenderBufferResourceFactory &rb_factory, bgfx::BackbufferRatio::Enum ratio);
Bloom CreateBloomFromAssets(const char *path, const RenderBufferResourceFactory &rb_factory, bgfx::BackbufferRatio::Enum ratio);

static Bloom CreateBloomFromFile(const char *path, bgfx::BackbufferRatio::Enum ratio) {
	return CreateBloomFromFile(path, RenderBufferResourceFactory::Backbuffer(), ratio);
}
static Bloom CreateBloomFromAssets(const char *path, bgfx::BackbufferRatio::Enum ratio) {
	return CreateBloomFromAssets(path, RenderBufferResourceFactory::Backbuffer(), ratio);
}

/// Destroy a bloom post process object and all associated resources.
void DestroyBloom(Bloom &bloom);

/// Process `input` texture and generate a bloom overlay on top of `output`, input and output must be of the same size.
/// Use CreateBloomFromFile() or CreateBloomFromAssets() to create a Bloom object and DestroyBloom() to destroy its internal resources after usage.
void ApplyBloom(bgfx::ViewId &view_id, const iRect &rect, const hg::Texture &input, const hg::iVec2 &fb_size, bgfx::FrameBufferHandle output,
	const Bloom &bloom, float threshold, float smoothness, float intensity);

void ApplyBloom(bgfx::ViewId &view_id, const iRect &rect, const hg::Texture &input, bgfx::FrameBufferHandle output, const Bloom &bloom, float threshold,
	float smoothness, float intensity);

bool IsValid(const Bloom &bloom);

} // namespace hg
