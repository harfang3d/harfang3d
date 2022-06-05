// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/cubemap.h"
#include "foundation/matrix3.h"

namespace hg {

static Mat4 GetFaceMatrix(CubemapFaces face) {
	if (face == CF_XP)
		return (Mat4)RotationMatY(Deg(90.f));
	if (face == CF_XN)
		return (Mat4)RotationMatY(Deg(-90.f));
	if (face == CF_YP)
		return (Mat4)RotationMatX(-Deg(90.f));
	if (face == CF_YN)
		return (Mat4)RotationMatX(Deg(90.f));
	if (face == CF_ZP)
		return (Mat4)RotationMatY(Deg(180.f));
	if (face == CF_ZN)
		return (Mat4)RotationMatY(Deg(0.f));

	__ASSERT__("Invalid cubemap face index");
	return Mat4::Identity;
}

void RenderCubemapFace(bgfx::ViewId &view_id, const bgfx::FrameBufferHandle frame_buffer, const Scene &scene, const Mat4 &world, CubemapFaces face,
	uint16_t res, int frame, ForwardPipeline &pipeline, const PipelineResources &resources, ForwardPipelineAAA &aaa, const ForwardPipelineAAAConfig &aaa_config,
	float znear, float zfar) {
	const auto view_state = ComputePerspectiveViewState(world * GetFaceMatrix(face), Deg(90.f), znear, zfar, Vec2::One);

	bgfx::setViewFrameBuffer(view_id, frame_buffer);

	SceneForwardPipelinePassViewId views;
	SceneForwardPipelineRenderData render_data;
	PrepareSceneForwardPipelineCommonRenderData(view_id, scene, render_data, pipeline, resources, views, "RenderCubemapFace AAA");
	PrepareSceneForwardPipelineViewDependentRenderData(view_id, view_state, scene, render_data, pipeline, resources, views, "RenderCubemapFace AAA");
	SubmitSceneToForwardPipeline(view_id, scene, iRect(0, 0, res, res), view_state, pipeline, render_data, resources, views, aaa, aaa_config, frame, res, res,
		frame_buffer, "RenderCubemapFace AAA");
	++view_id;

	aaa.Flip(view_state);
}

void CaptureCubemapFace(bgfx::ViewId &view_id, const bgfx::FrameBufferHandle frame_buffer, bgfx::TextureHandle tgt, uint16_t res) {
	bgfx::touch(view_id);
	bgfx::blit(view_id, tgt, 0, 0, bgfx::getTexture(frame_buffer, 0), 0, 0, res, res);
	++view_id;
}

} // namespace hg
