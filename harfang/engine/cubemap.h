// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/picture.h"
#include "engine/scene_forward_pipeline.h"

namespace hg {

enum CubemapFaces { CF_XP, CF_XN, CF_YP, CF_YN, CF_ZP, CF_ZN, CF_Max };

void RenderCubemapFace(bgfx::ViewId &view_id, const bgfx::FrameBufferHandle frame_buffer, const Scene &scene, const Mat4 &world, CubemapFaces face,
	uint16_t res, int frame, ForwardPipeline &pipeline, const PipelineResources &resources, ForwardPipelineAAA &aaa, const ForwardPipelineAAAConfig &aaa_config,
	float znear, float zfar);

void CaptureCubemapFace(bgfx::ViewId &view_id, const bgfx::FrameBufferHandle frame_buffer, bgfx::TextureHandle tgt, uint16_t res);

} // namespace hg
