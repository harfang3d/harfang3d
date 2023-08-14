// HARFANG(R) Copyright (C) 2023 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/render_pipeline.h"
#include "foundation/rect.h"

namespace hg {

struct Dof {
	bgfx::ProgramHandle prg_dof_coc = BGFX_INVALID_HANDLE;

	bgfx::UniformHandle u_color = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_attr0 = BGFX_INVALID_HANDLE; // xyz: normal, w: linear depth
	bgfx::UniformHandle u_params = BGFX_INVALID_HANDLE;
};

Dof CreateDofFromFile(const char *path);
Dof CreateDofFromAssets(const char *path);

void DestroyDof(Dof &dof);

void ApplyDof(bgfx::ViewId &view_id, const iRect &rect, bgfx::BackbufferRatio::Enum ratio, const Texture &color, const Texture &attr0,
	bgfx::FrameBufferHandle output, const Dof &dof, float focus_point, float focus_length);

bool IsValid(const Dof &dof);

} // namespace hg
