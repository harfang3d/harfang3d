// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/render_pipeline.h"
#include "foundation/rect.h"

namespace hg {

struct SAO {
	bgfx::BackbufferRatio::Enum ratio = bgfx::BackbufferRatio::Half;

	bgfx::FrameBufferHandle compute_fb = BGFX_INVALID_HANDLE;
	bgfx::FrameBufferHandle blur_fb = BGFX_INVALID_HANDLE;

	bgfx::ProgramHandle prg_compute = BGFX_INVALID_HANDLE;
	bgfx::ProgramHandle prg_blur = BGFX_INVALID_HANDLE;

	bgfx::UniformHandle u_attr0 = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_attr1 = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_noise = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_input = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_params = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_projection_infos = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_sample_count = BGFX_INVALID_HANDLE;
};

SAO CreateSAOFromFile(const char *path, bgfx::BackbufferRatio::Enum ratio);
SAO CreateSAOFromAssets(const char *path, bgfx::BackbufferRatio::Enum ratio);
SAO CreateSAOFromFile(const char *path, bgfx::BackbufferRatio::Enum ratio);
SAO CreateSAOFromAssets(const char *path, bgfx::BackbufferRatio::Enum ratio);

/// Destroy an ambient occlusion post process object and its resources.
void DestroySAO(SAO &sao);

/// @note input depth buffer must be in linear depth
void ComputeSAO(bgfx::ViewId &view_id, const iRect &rect, const Texture &attr0, const Texture &attr1, const Texture &noise, bgfx::FrameBufferHandle output,
	const SAO &sao, const Mat44 &projection, float bias, float radius, int sample_count, float sharpness);

bool IsValid(const SAO &sao);

} // namespace hg
