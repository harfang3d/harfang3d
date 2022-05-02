// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/render_pipeline.h"
#include "foundation/rect.h"

namespace hg {

struct TAA {
	bgfx::ProgramHandle prg_taa = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_color = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_prv_color = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_attr0 = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_attr1 = BGFX_INVALID_HANDLE;
};

TAA CreateTAAFromFile(const char *path);
TAA CreateTAAFromAssets(const char *path);

void DestroyTAA(TAA &taa);

/// @note input depth buffer must be in linear depth
void ApplyTAA(bgfx::ViewId &view_id, const iRect &rect, const Texture &color, const Texture &prv_color, const Texture &attr0, const Texture &attr1,
	bgfx::FrameBufferHandle output, const TAA &taa);

Vec2 TAAProjectionJitter8(int frame);
Vec2 TAAProjectionJitter16(int frame);

Vec2 TAAHaltonJitter8(int frame);
Vec2 TAAHaltonJitter16(int frame);

bool IsValid(const TAA &taa);

} // namespace hg
