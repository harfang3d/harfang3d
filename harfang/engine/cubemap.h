// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <stdint.h>
#include <bgfx/bgfx.h>
#include "engine/picture.h"

namespace bgfx {
struct TextureHandle;
struct FrameBufferHandle;
typedef uint16_t ViewId;
}
namespace hg {

struct PipelineResources;
struct ForwardPipeline;
struct ForwardPipelineAAAConfig;
struct ForwardPipelineAAA;
class Scene;
struct Mat4;

enum CubemapFaces { CF_XP, CF_XN, CF_YP, CF_YN, CF_ZP, CF_ZN, CF_Max };
static_assert(CF_XP == 0, "");

enum CubemapLayout {
	CL_CubeCross,
};

// pass nullptr for aaa_config for forward rendering, and a config for AAA rendering
Picture RenderCubemap(bgfx::ViewId &view_id, const PipelineResources &resources, Scene &scene, const Mat4 &transform, CubemapLayout cube_layout,
	uint16_t tex_size, bgfx::TextureFormat::Enum format, PictureFormat pic_format, const ForwardPipelineAAAConfig *aaa_config = nullptr, float znear = 0.01f,
	float zfar = 1000.f);

void RenderCubemap(bgfx::ViewId &view_id, const PipelineResources &resources, Scene &scene, const Mat4 &transform, bgfx::TextureHandle tgt_textures[CF_Max],
	uint16_t tex_size, bgfx::TextureFormat::Enum format, const ForwardPipelineAAAConfig *aaa_config, float znear, float zfar);

// aaa currently needs 1 pipeline per cubemap as ForwardPipelineAAA stores the previous frame
void RenderCubemap(bgfx::ViewId &view_id, const bgfx::FrameBufferHandle frame_buffer, ForwardPipeline &pipeline, const PipelineResources &resources,
	Scene &scene, const Mat4 &transform, bgfx::TextureHandle tgt_textures[CF_Max], uint16_t tex_size, ForwardPipelineAAA aaa[CF_Max],
	const ForwardPipelineAAAConfig *aaa_config,
	float znear, float zfar);

Picture CreatePictureFromCubemap(bgfx::TextureHandle cubemap[CF_Max], CubemapLayout cube_layout, uint16_t tex_size, PictureFormat pic_format);

} // namespace hg
