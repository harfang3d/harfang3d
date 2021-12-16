// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/render_pipeline.h"
#include "foundation/rect.h"

namespace hg {

struct MotionBlur {
	bgfx::ProgramHandle prg_motion_blur = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_color = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_attr0 = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_attr1 = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_noise = BGFX_INVALID_HANDLE;
};

MotionBlur CreateMotionBlurFromFile(const char *path);
MotionBlur CreateMotionBlurFromAssets(const char *path);

void DestroyMotionBlur(MotionBlur &motion_blur);

/// @note input depth buffer must be in linear depth
void ApplyMotionBlur(bgfx::ViewId &view_id, const iRect &rect, const Texture &color, const Texture &attr0, const Texture &attr1, const Texture &noise,
	bgfx::FrameBufferHandle output, const MotionBlur &motion_blur);

bool IsValid(const MotionBlur &motion_blur);

} // namespace hg
