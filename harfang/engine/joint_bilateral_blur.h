// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/render_pipeline.h"
#include "foundation/rect.h"

namespace hg {

struct JointBilateralBlur {
	bgfx::ProgramHandle compute = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_input = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_attr0 = BGFX_INVALID_HANDLE; // xyz: normal, w: linear depth

	bgfx::FrameBufferHandle fb = BGFX_INVALID_HANDLE;
	Texture work;
};

JointBilateralBlur CreateJointBilateralBlurFromFile(const char *path);
JointBilateralBlur CreateJointBilateralBlurFromAssets(const char *path);

void DestroyJointBilateralBlur(JointBilateralBlur &blur);

void ComputeJointBilateralBlur(
	bgfx::ViewId &view_id, const iRect &rect, const Texture &input, const Texture &attr0, const JointBilateralBlur &blur);

} // namespace hg
