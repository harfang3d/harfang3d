// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/render_pipeline.h"
#include "foundation/rect.h"

namespace hg {

struct TemporalAccumulation {
	bgfx::ProgramHandle compute = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_previous = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_current = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle u_attr1 = BGFX_INVALID_HANDLE; // xyz: velocity
};

TemporalAccumulation CreateTemporalAccumulationFromFile(const char *path);
TemporalAccumulation CreateTemporalAccumulationFromAssets(const char *path);

void DestroyTemporalAccumulation(TemporalAccumulation &temporal_acc);

void ComputeTemporalAccumulation(bgfx::ViewId &view_id, const iRect &rect, const Texture &current, const Texture &previous, const Texture &attr1,
	bgfx::FrameBufferHandle output, const TemporalAccumulation &temporal_acc);

bool IsValid(const TemporalAccumulation &temporal_acc);

} // namespace hg
