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
		hg::error("failed to create bloom uniforms.");
		return false;
	}

	// load programs
	bloom.prg_threshold = hg::LoadProgram(ir, ip, hg::format("%1/bloom_threshold").arg(path));
	bloom.prg_downsample = hg::LoadProgram(ir, ip, hg::format("%1/bloom_downsample").arg(path));
	bloom.prg_upsample = hg::LoadProgram(ir, ip, hg::format("%1/bloom_upsample").arg(path));
	bloom.prg_combine = hg::LoadProgram(ir, ip, hg::format("%1/bloom_combine").arg(path));

	if (!(bgfx::isValid(bloom.prg_threshold) && bgfx::isValid(bloom.prg_downsample) && bgfx::isValid(bloom.prg_upsample) && bgfx::isValid(bloom.prg_combine))) {
		hg::error("failed to load bloom programs.");
		return false;
	}

	return true;
}

static const uint64_t g_bloom_tex_flags = 0 | BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;

static Bloom CreateBloom(const Reader &ir, const ReadProvider &ip, const char *path, bgfx::BackbufferRatio::Enum ratio) {
	Bloom bloom;

	bloom.in_fb = bgfx::createFrameBuffer(ratio, bgfx::TextureFormat::RGBA16F, g_bloom_tex_flags);
	bgfx::setName(bloom.in_fb, "Bloom IN FB");
	bloom.out_fb = bgfx::createFrameBuffer(ratio, bgfx::TextureFormat::RGBA16F, g_bloom_tex_flags);
	bgfx::setName(bloom.out_fb, "Bloom OUT FB");

	LoadShaders(bloom, ir, ip, path);
	return bloom;
}

Bloom CreateBloomFromFile(const char *path, bgfx::BackbufferRatio::Enum ratio) { return CreateBloom(g_file_reader, g_file_read_provider, path, ratio); }
Bloom CreateBloomFromAssets(const char *path, bgfx::BackbufferRatio::Enum ratio) { return CreateBloom(g_assets_reader, g_assets_read_provider, path, ratio); }

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
	const bgfx::Caps *caps = bgfx::getCaps();

	bgfx::TransientIndexBuffer idx;
	bgfx::TransientVertexBuffer vtx;
	CreateFullscreenQuad(idx, vtx);

	//
	auto stats = bgfx::getStats();

	const int width = stats->width;
	const int height = stats->height;

	const int pass_count = static_cast<int>(std::floor(std::log2(std::min(width, height)))) / 2;

	//
	float ortho[16];
	memcpy(ortho, hg::to_bgfx(hg::Compute2DProjectionMatrix(-1.f, 1.f, 1.f, 1.f, false)).data(), sizeof(float[16]));

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

			pass_rect.ex = GetWidth(pass_rect) / 2 + pass_rect.sx;
			pass_rect.ey = GetHeight(pass_rect) / 2 + pass_rect.sy;

			bgfx::setViewName(view_id, hg::format("Bloom downsample %1").arg(i).c_str());
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
			const iRect pass_rect = pass_rects.back();
			pass_rects.pop_back();

			bgfx::setViewName(view_id, hg::format("Bloom upsample %1").arg(i).c_str());
			bgfx::setViewRect(view_id, pass_rect.sx, pass_rect.sy, GetWidth(pass_rect), GetHeight(pass_rect));
			bgfx::setViewTransform(view_id, NULL, ortho);
			bgfx::setViewClear(view_id, BGFX_CLEAR_NONE, 1.f, 0, UINT8_MAX);
			bgfx::setViewFrameBuffer(view_id, bloom.out_fb);

			const float params[4] = {0.f, 0.f, intensity, 0.f};
			bgfx::setUniform(bloom.u_params, &params[0], 1);
			const float u_source_rect[] = {
				float(pass_rect.sx), float(pass_rect.sy), (pass_rect.ex - pass_rect.sx) / 2.f + pass_rect.sx, (pass_rect.ey - pass_rect.sy) / 2.f + pass_rect.sy};
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
