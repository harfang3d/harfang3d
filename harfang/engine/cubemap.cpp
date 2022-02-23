// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/cubemap.h"
#include "engine/forward_pipeline.h"
#include "engine/scene.h"
#include "engine/scene_forward_pipeline.h"
#include "foundation/matrix3.h"

namespace hg {

Picture RenderCubemap(bgfx::ViewId &view_id, const PipelineResources &resources, Scene &scene, const Mat4 &transform, CubemapLayout cube_layout,
	uint16_t tex_size,
	const ForwardPipelineAAAConfig *aaa_config, float znear, float zfar) {
	bgfx::TextureHandle cubemap[hg::CF_Max];
	for (int i = 0; i < hg::CF_Max; i++) {
		cubemap[i] = bgfx::createTexture2D(tex_size, tex_size, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_BLIT_DST | BGFX_TEXTURE_READ_BACK);
	}

	RenderCubemap(view_id, resources, scene, transform, cubemap, tex_size, aaa_config, znear, zfar);

	auto pic = hg::CreatePictureFromCubemap(cubemap, hg::CL_CubeCross, tex_size);

	for (int i = 0; i < hg::CF_Max; i++) {
		bgfx::destroy(cubemap[i]);
	}

	return pic;
}

void RenderCubemap(bgfx::ViewId &view_id, const PipelineResources &resources, Scene &scene, const Mat4 &transform, bgfx::TextureHandle tgt_textures[CF_Max],
	uint16_t tex_size,
	const ForwardPipelineAAAConfig *aaa_config, float znear, float zfar) {

	ForwardPipeline pipeline = CreateForwardPipeline();

	bgfx::TextureHandle texs[2] = {
		bgfx::createTexture2D(tex_size, tex_size, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_BLIT_DST | BGFX_TEXTURE_RT_MSAA_X8),
		bgfx::createTexture2D(tex_size, tex_size, false, 1, bgfx::TextureFormat::D32, BGFX_TEXTURE_RT_WRITE_ONLY | BGFX_TEXTURE_RT_MSAA_X8)};
	const auto frame_buffer = bgfx::createFrameBuffer(2, texs, true);

	hg::ForwardPipelineAAA aaa[CF_Max];
	if (aaa_config != nullptr) {
		for (size_t i = 0; i < CF_Max; i++) {
			aaa[i] = hg::CreateForwardPipelineAAAFromAssets("core", *aaa_config, tex_size, tex_size);
		}
	}

	RenderCubemap(
		view_id, frame_buffer, pipeline, resources, scene, transform, tgt_textures, tex_size, aaa_config != nullptr ? aaa : nullptr, aaa_config, znear, zfar);

	if (aaa_config != nullptr) {
		for (size_t i = 0; i < CF_Max; i++) {
			hg::DestroyForwardPipelineAAA(aaa[i]);
		}
	}

	hg::DestroyForwardPipeline(pipeline);

	bgfx::destroy(frame_buffer);
	bgfx::destroy(texs[0]);
	bgfx::destroy(texs[1]);
}

void RenderCubemap(bgfx::ViewId &view_id, const bgfx::FrameBufferHandle frame_buffer, ForwardPipeline &pipeline, const PipelineResources &resources,
	Scene &scene, const Mat4 &transform,
	bgfx::TextureHandle tgt_textures[CF_Max], uint16_t tex_size, ForwardPipelineAAA *aaa, const ForwardPipelineAAAConfig *aaa_config,
	float znear, float zfar) {

	const auto debug_name = std::string("cubemap");

	auto color_texture = bgfx::getTexture(frame_buffer, 0);

	Mat4 rotations[CF_Max];
	rotations[CF_XP] = (Mat4)RotationMatY(-Deg(90.f));
	rotations[CF_XN] = (Mat4)RotationMatY(Deg(90.f));
	rotations[CF_YP] = (Mat4)RotationMatX(-Deg(90.f)) * (Mat4)RotationMatZ(Deg(180.f));
	rotations[CF_YN] = (Mat4)RotationMatX(Deg(90.f)) * (Mat4)RotationMatZ(Deg(180.f));
	rotations[CF_ZP] = (Mat4)RotationMatY(Deg(0.f));
	rotations[CF_ZN] = (Mat4)RotationMatY(Deg(180.f));

	if (aaa != nullptr) {
		// use more frames if you use TAA, needs more reflection samples, etc
		// Seems like we need at least 2 frames for the rendering to work properly
		const int frames_to_render_per_view = 2;

		scene.Update(hg::time_from_ms_f(0.0f)); // needs prev_mtx for aaa pass

		for (int i = 0; i < CF_Max; i++) {
			const auto view_state = ComputePerspectiveViewState(transform * rotations[i], Deg(90.f), znear, zfar, Vec2::One);

			for (int frame = 0; frame < frames_to_render_per_view; frame++) {
				bgfx::touch(view_id);

				bgfx::setViewFrameBuffer(view_id, frame_buffer);

				SceneForwardPipelinePassViewId views;
				SceneForwardPipelineRenderData render_data;
				PrepareSceneForwardPipelineCommonRenderData(view_id, scene, render_data, pipeline, resources, views, debug_name.c_str());
				PrepareSceneForwardPipelineViewDependentRenderData(view_id, view_state, scene, render_data, pipeline, resources, views, debug_name.c_str());
				SubmitSceneToForwardPipeline(view_id, scene, Rect<int>(0, 0, tex_size, tex_size), view_state, pipeline, render_data, resources, views, aaa[i],
					*aaa_config, frame, tex_size, tex_size, frame_buffer, debug_name.c_str());

				aaa[i].Flip(view_state);

				// TODO: how to get rid of this call?
				bgfx::frame();
				view_id = 0;
				// 

				view_id++;
			}

			view_id++;
			bgfx::touch(view_id);
			bgfx::blit(view_id, tgt_textures[i], 0, 0, color_texture, 0, 0, tex_size, tex_size);
			view_id++;
		}
	} else {
		for (int i = 0; i < CF_Max; i++) {
			bgfx::touch(view_id);
			const auto view_state = ComputePerspectiveViewState(transform * rotations[i], Deg(90.f), znear, zfar, Vec2::One);

			SceneForwardPipelinePassViewId views;
			SceneForwardPipelineRenderData render_data;
			PrepareSceneForwardPipelineCommonRenderData(view_id, scene, render_data, pipeline, resources, views, debug_name.c_str());
			PrepareSceneForwardPipelineViewDependentRenderData(view_id, view_state, scene, render_data, pipeline, resources, views, debug_name.c_str());
			SubmitSceneToForwardPipeline(
				view_id, scene, Rect<int>(0, 0, tex_size, tex_size), view_state, pipeline, render_data, resources, views, frame_buffer, debug_name.c_str());

			view_id++;
			bgfx::touch(view_id);
			bgfx::blit(view_id, tgt_textures[i], 0, 0, color_texture, 0, 0, tex_size, tex_size);
			view_id++;
		}
	}
}


Picture CreatePictureFromCubemap(bgfx::TextureHandle cubemap[CF_Max], CubemapLayout cube_layout, uint16_t tex_size) {

	const auto pic_format = PictureFormat::PF_RGBA32;
	const size_t size_pixel = 4;

	const uint16_t pic_width = 4 * tex_size;
	const uint16_t pic_height = 3 * tex_size;
	const size_t stride = pic_width * size_pixel;
	const size_t pic_buffer_size = pic_width * pic_height * size_pixel;
	Picture pic(pic_width, pic_height, pic_format);

	// init image to black, seems important for cmft to detect this is a cubemap
	memset(pic.GetData(), 0, pic_buffer_size);

	Picture tmp_pic(tex_size, tex_size, pic_format);

	for (int i = 0; i < CF_Max; i++) {

		auto frame_id = bgfx::readTexture(cubemap[i], tmp_pic.GetData());

		auto current_frame = bgfx::frame();
		while (current_frame < frame_id) {
			current_frame = bgfx::frame();
		}

		size_t offset = 0;
		if (i == CF_XP)
			offset = (pic_width * tex_size + 2 * tex_size) * size_pixel;
		if (i == CF_XN)
			offset = pic_width * tex_size * size_pixel;
		if (i == CF_YP)
			offset = 1 * tex_size * size_pixel;
		if (i == CF_YN)
			offset = (2 * pic_width * tex_size + 1 * tex_size) * size_pixel;
		if (i == CF_ZP)
			offset = (pic_width * tex_size + 3 * tex_size) * size_pixel;
		if (i == CF_ZN)
			offset = (pic_width * tex_size + 1 * tex_size) * size_pixel;

		for (size_t y = 0; y < tex_size; y++) {
			memcpy(pic.GetData() + offset + y * stride, tmp_pic.GetData() + y * tex_size * size_pixel, tex_size * size_pixel);
		}
	}

	return pic;
}

} // namespace hg
