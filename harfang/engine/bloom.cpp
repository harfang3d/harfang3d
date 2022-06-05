// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "bloom.h"
#include "assets_rw_interface.h"

#include <foundation/file_rw_interface.h>
#include <foundation/format.h>
#include <foundation/log.h>
#include <foundation/projection.h>
#include <foundation/rw_interface.h>

#include <algorithm>
#include <cmath>

#include <bx/math.h>

namespace hg {

static bool LoadShaders(Bloom &bloom, const Reader &ir, const ReadProvider &ip, const char *path) {
	// uniforms
	bloom.u_source = bgfx::createUniform("u_source", bgfx::UniformType::Sampler);
	bloom.u_input = bgfx::createUniform("u_input", bgfx::UniformType::Sampler);
	bloom.u_params = bgfx::createUniform("u_params", bgfx::UniformType::Vec4);
	bloom.u_source_rect = bgfx::createUniform("u_source_rect", bgfx::UniformType::Vec4);
	if (!(bgfx::isValid(bloom.u_source) && bgfx::isValid(bloom.u_input) && bgfx::isValid(bloom.u_params) && bgfx::isValid(bloom.u_source_rect))) {
		warn("failed to create bloom uniforms.");
		return false;
	}

	// load programs
	bloom.prg_threshold = LoadProgram(ir, ip, format("%1/bloom_threshold").arg(path));
	bloom.prg_downsample = LoadProgram(ir, ip, format("%1/bloom_downsample").arg(path));
	bloom.prg_upsample = LoadProgram(ir, ip, format("%1/bloom_upsample").arg(path));
	bloom.prg_combine = LoadProgram(ir, ip, format("%1/bloom_combine").arg(path));

	if (!(bgfx::isValid(bloom.prg_threshold) && bgfx::isValid(bloom.prg_downsample) && bgfx::isValid(bloom.prg_upsample) && bgfx::isValid(bloom.prg_combine))) {
		warn("failed to load bloom programs.");
		return false;
	}

	return true;
}

static const uint64_t g_bloom_tex_flags = 0 | BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;

bool IsValid(const Bloom &bloom) {
	return bgfx::isValid(bloom.in_fb) && bgfx::isValid(bloom.out_fb) && bgfx::isValid(bloom.u_source) && bgfx::isValid(bloom.u_input) &&
		   bgfx::isValid(bloom.u_params) && bgfx::isValid(bloom.u_source_rect) && bgfx::isValid(bloom.prg_threshold) && bgfx::isValid(bloom.prg_downsample) &&
		   bgfx::isValid(bloom.prg_upsample) && bgfx::isValid(bloom.prg_combine);
}

static Bloom CreateBloom(
	const Reader &ir, const ReadProvider &ip, const char *path, const RenderBufferResourceFactory &rb_factory, bgfx::BackbufferRatio::Enum ratio) {
	Bloom bloom;
	if (!LoadShaders(bloom, ir, ip, path)) {
		DestroyBloom(bloom);
		return bloom;
	}

	bloom.in_fb = rb_factory.create_framebuffer(ratio, bgfx::TextureFormat::RGBA16F, g_bloom_tex_flags);
	bloom.out_fb = rb_factory.create_framebuffer(ratio, bgfx::TextureFormat::RGBA16F, g_bloom_tex_flags);
	if (!(bgfx::isValid(bloom.in_fb) && bgfx::isValid(bloom.out_fb))) {
		DestroyBloom(bloom);
		return bloom;
	}

	bgfx::setName(bloom.in_fb, "Bloom IN FB");
	bgfx::setName(bloom.out_fb, "Bloom OUT FB");
	return bloom;
}

Bloom CreateBloomFromFile(const char *path, const RenderBufferResourceFactory &rb_factory, bgfx::BackbufferRatio::Enum ratio) {
	return CreateBloom(g_file_reader, g_file_read_provider, path, rb_factory, ratio);
}
Bloom CreateBloomFromAssets(const char *path, const RenderBufferResourceFactory &rb_factory, bgfx::BackbufferRatio::Enum ratio) {
	return CreateBloom(g_assets_reader, g_assets_read_provider, path, rb_factory, ratio);
}

void DestroyBloom(Bloom &bloom) {
	bgfx_Destroy(bloom.in_fb);
	bgfx_Destroy(bloom.out_fb);

	bgfx_Destroy(bloom.u_source);
	bgfx_Destroy(bloom.u_input);
	bgfx_Destroy(bloom.u_params);
	bgfx_Destroy(bloom.u_source_rect);

	bgfx_Destroy(bloom.prg_threshold);
	bgfx_Destroy(bloom.prg_downsample);
	bgfx_Destroy(bloom.prg_upsample);
	bgfx_Destroy(bloom.prg_combine);
}

void ApplyBloom(bgfx::ViewId &view_id, const iRect &rect, const hg::Texture &input, bgfx::FrameBufferHandle output, const Bloom &bloom, float threshold,
	float smoothness, float intensity) {
	auto stats = bgfx::getStats();

	const int width = stats->width;
	const int height = stats->height;
	ApplyBloom(view_id, rect, input, hg::iVec2(width, height), output, bloom, threshold, smoothness, intensity);
}

void ApplyBloom(bgfx::ViewId &view_id, const iRect &rect, const hg::Texture &input, const hg::iVec2 &fb_size, bgfx::FrameBufferHandle output,
	const Bloom &bloom, float threshold, float smoothness, float intensity) {
	__ASSERT__(IsValid(bloom));

	const bgfx::Caps *caps = bgfx::getCaps();

	bgfx::TransientIndexBuffer idx;
	bgfx::TransientVertexBuffer vtx;
	CreateFullscreenQuad(idx, vtx);

	const int pass_count = static_cast<int>(std::floor(std::log2(std::min(fb_size.x, fb_size.y)))) / 2;

	//
	float ortho[16];
	memcpy(ortho, to_bgfx(Compute2DProjectionMatrix(-1.f, 1.f, 1.f, 1.f, false)).data(), sizeof(float[16]));

	// threshold
	{
		bgfx::setViewName(view_id, "Bloom threshold");
		bgfx::setViewRect(view_id, rect.sx, rect.sy, GetWidth(rect), GetHeight(rect));
		bgfx::setViewTransform(view_id, NULL, ortho);
		bgfx::setViewFrameBuffer(view_id, bloom.out_fb);
		bgfx::setViewClear(view_id, BGFX_CLEAR_NONE, 1.0f, 0, UINT8_MAX);

		const float params[4] = {threshold, threshold * smoothness + 1e-6f, 0.f, 0.f};
		bgfx::setUniform(bloom.u_params, params, 1);

		bgfx::setTexture(0, bloom.u_source, input.handle, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);

		bgfx::setIndexBuffer(&idx);
		bgfx::setVertexBuffer(0, &vtx);
		bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
		bgfx::submit(view_id, bloom.prg_threshold);

		std::swap(bloom.in_fb, bloom.out_fb);
		view_id++;
	}

	// downsample
	iRect pass_rect = rect;
	{
		std::vector<iRect> pass_rects;
		for (int i = 0; i < pass_count; ++i) {
			pass_rects.push_back(pass_rect);
			const iRect source_rect = pass_rect;

			pass_rect.ex = Floor(GetWidth(pass_rect) / 2) + pass_rect.sx;
			pass_rect.ey = Floor(GetHeight(pass_rect) / 2) + pass_rect.sy;

			bgfx::setViewName(view_id, format("Bloom downsample %1").arg(i).c_str());
			bgfx::setViewRect(view_id, pass_rect.sx, pass_rect.sy, GetWidth(pass_rect), GetHeight(pass_rect));
			bgfx::setViewTransform(view_id, NULL, ortho);
			bgfx::setViewClear(view_id, BGFX_CLEAR_NONE, 1.f, 0, UINT8_MAX);
			bgfx::setViewFrameBuffer(view_id, bloom.out_fb);

			const float u_source_rect[] = {float(source_rect.sx), float(source_rect.sy), float(GetWidth(source_rect)), float(GetHeight(source_rect))};
			bgfx::setUniform(bloom.u_source_rect, u_source_rect, 1);
			bgfx::setTexture(0, bloom.u_source, bgfx::getTexture(bloom.in_fb), BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);

			bgfx::setIndexBuffer(&idx);
			bgfx::setVertexBuffer(0, &vtx);
			bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
			bgfx::submit(view_id, bloom.prg_downsample);

			std::swap(bloom.in_fb, bloom.out_fb);
			view_id++;
		}

		// upsample
		for (int i = 0; i < pass_count; ++i) {
			const iRect source_rect = pass_rect;
			pass_rect = pass_rects.back();
			pass_rects.pop_back();

			bgfx::setViewName(view_id, format("Bloom upsample %1").arg(i).c_str());
			bgfx::setViewRect(view_id, pass_rect.sx, pass_rect.sy, GetWidth(pass_rect), GetHeight(pass_rect));
			bgfx::setViewTransform(view_id, NULL, ortho);
			bgfx::setViewClear(view_id, BGFX_CLEAR_NONE, 1.f, 0, UINT8_MAX);
			bgfx::setViewFrameBuffer(view_id, bloom.out_fb);

			const float params[4] = {0.f, 0.f, intensity, 0.f};
			bgfx::setUniform(bloom.u_params, &params[0], 1);

			const float u_source_rect[] = {float(source_rect.sx), float(source_rect.sy), float(GetWidth(source_rect)), float(GetHeight(source_rect))};
			bgfx::setUniform(bloom.u_source_rect, u_source_rect, 1);
			bgfx::setTexture(0, bloom.u_source, bgfx::getTexture(bloom.in_fb), BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);

			bgfx::setIndexBuffer(&idx);
			bgfx::setVertexBuffer(0, &vtx);
			bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
			bgfx::submit(view_id, bloom.prg_upsample);

			std::swap(bloom.in_fb, bloom.out_fb);
			view_id++;
		}
	}

	// combine
	{
		bgfx::setViewName(view_id, "Bloom combine");
		bgfx::setViewRect(view_id, rect.sx, rect.sy, GetWidth(rect), GetHeight(rect));
		bgfx::setViewTransform(view_id, NULL, ortho);
		bgfx::setViewClear(view_id, BGFX_CLEAR_NONE, 1.0f, 0, UINT8_MAX);
		bgfx::setViewFrameBuffer(view_id, output);

		bgfx::setTexture(0, bloom.u_source, bgfx::getTexture(bloom.in_fb), BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
		bgfx::setTexture(1, bloom.u_input, input.handle, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);

		bgfx::setIndexBuffer(&idx);
		bgfx::setVertexBuffer(0, &vtx);
		bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
		bgfx::submit(view_id, bloom.prg_combine);
		view_id++;
	}
}

} // namespace hg
