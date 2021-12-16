// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/hiz.h"
#include "engine/render_pipeline.h"
#include "foundation/rect.h"

namespace hg {

struct SSR {
	bgfx::ProgramHandle prg_ssr = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_color = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_attr0 = BGFX_INVALID_HANDLE; // x: linear depth, yz: velocity
	bgfx::UniformHandle u_attr1 = BGFX_INVALID_HANDLE; // xyz: view normal
	bgfx::UniformHandle u_noise = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_probe = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_hiz = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_depthTexInfos = BGFX_INVALID_HANDLE; // [todo] rename and/or move to hiz struct ?
};

SSR CreateSSRFromFile(const char *path);
SSR CreateSSRFromAssets(const char *path);

void DestroySSR(SSR &ssr);

/// @note input depth buffer must be in linear depth
void ComputeSSR(bgfx::ViewId &view_id, const iRect &rect, bgfx::BackbufferRatio::Enum ratio, const Texture &color, const Texture &attr0, const Texture &attr1,
	const Texture &probe, const Texture &noise, const HiZ &hiz, bgfx::FrameBufferHandle output, const SSR &ssr);

bool IsValid(const SSR &ssr);

} // namespace hg
