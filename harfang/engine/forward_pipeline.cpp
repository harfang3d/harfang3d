// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/forward_pipeline.h"
#include "engine/assets_rw_interface.h"
#include "engine/render_pipeline.h"
#include "engine/scene.h"
#include "engine/taa.h"

#include "foundation/clock.h"
#include "foundation/file_rw_interface.h"
#include "foundation/format.h"
#include "foundation/math.h"
#include "foundation/path_tools.h"
#include "foundation/projection.h"

#include <bgfx/bgfx.h>
#include <math.h>
#include <vector>

namespace hg {

enum ForwardPipelineUniformValue {
	UV_Clock,

	UV_FogColor,
	UV_FogState,

	UV_AmbientColor,

	UV_LightPos,
	UV_LightDir,
	UV_LightDiffuse,
	UV_LightSpecular,

	UV_LinearShadowMatrix,
	UV_LinearShadowSlice,
	UV_SpotShadowMatrix,
	UV_ShadowState,

	UV_Resolution,
	UV_Projection,
	UV_MainProjection,
	UV_MainInvProjection,

	UV_PreviousViewProjection,
	UV_ViewProjUnjittered, // for TAA
	UV_AAAParams, // [0].x: ssgi ratio, [0].y: ssr ratio, [0].z: temporal AA weight, [0].w: motion blur strength
				  // [1].x: exposure,   [1].y: 1/gamma,   [1].z: sample count,       [1].w: screen-space ray max len
				  // [2].x: spec weight [2].y: sharpen

	UV_MainInvView,
	UV_ProbeMatrix,
	UV_InvProbeMatrix,
	UV_ProbeData, // x: type (0: sphere 1: cube) y: parallax

	UV_Count
};

enum ForwardPipelineUniformTexture {
	UT_IrradianceMap,
	UT_RadianceMap,
	UT_SSIrradianceMap,
	UT_SSRadianceMap,
	UT_BrdfMap,
	UT_NoiseMap,

	UT_AmbientOcclusion,
	UT_LinearShadowMap,
	UT_SpotShadowMap,

	UT_Count
};

//
ForwardPipelineLight MakeForwardPipelinePointLight(
	const Mat4 &world, const Color &diffuse, const Color &specular, float radius, float priority, ForwardPipelineShadowType shadow_type, float shadow_bias) {
	return {FPLT_Point, shadow_type, world, diffuse, specular, radius, 0.f, 0.f, Vec4::Zero, priority, shadow_bias};
}

ForwardPipelineLight MakeForwardPipelineSpotLight(const Mat4 &world, const Color &diffuse, const Color &specular, float radius, float inner_angle,
	float outer_angle, float priority, ForwardPipelineShadowType shadow_type, float shadow_bias) {
	return {FPLT_Spot, shadow_type, world, diffuse, specular, radius, inner_angle, outer_angle, Vec4::Zero, priority, shadow_bias};
}

ForwardPipelineLight MakeForwardPipelineLinearLight(const Mat4 &world, const Color &diffuse, const Color &specular, const Vec4 &pssm_split, float priority,
	ForwardPipelineShadowType shadow_type, float shadow_bias) {
	return {FPLT_Linear, shadow_type, world, diffuse, specular, 0.f, 0.f, 0.f, pssm_split, priority, shadow_bias};
}

//
ForwardPipelineLights PrepareForwardPipelineLights(const std::vector<ForwardPipelineLight> &lights) {
	ForwardPipelineLights ls{};

	// separate lights per type
	std::vector<int> i_linear, i_point;
	i_linear.reserve(8);
	i_point.reserve(64);

	for (size_t i = 0; i < lights.size(); ++i)
		if (lights[i].type == FPLT_Linear)
			i_linear.push_back(int(i));
		else
			i_point.push_back(int(i));

	// slot 0: single linear light
	std::sort(std::begin(i_linear), std::end(i_linear), [&](const auto &a, const auto &b) { return lights[a].priority > lights[b].priority; });

	if (!i_linear.empty()) {
		const auto &l = lights[i_linear.front()];

		ls.pos[0] = Vec4(GetT(l.world), 0.f);
		ls.dir[0] = Vec4(GetZ(l.world), 0.f);
		ls.diff[0] = {l.diffuse.r, l.diffuse.g, l.diffuse.b, 0.f};
		ls.spec[0] = {l.specular.r, l.specular.g, l.specular.b, 0.f};
		ls.lights[0] = l;
	}

	// slot 1-n: point/spot
	std::sort(std::begin(i_point), std::end(i_point), [&](const auto &a, const auto &b) { return lights[a].priority > lights[b].priority; });

	size_t slot_idx = 1;
	for (auto i : i_point) {
		const auto &l = lights[i];

		ls.pos[slot_idx] = Vec4(GetT(l.world), l.radius ? 1.f / l.radius : 0.f);
		ls.dir[slot_idx] = Vec4(GetZ(l.world), l.type == FPLT_Spot ? Cos(l.inner_angle) : 0.f);
		ls.diff[slot_idx] = {l.diffuse.r, l.diffuse.g, l.diffuse.b, l.type == FPLT_Spot ? Cos(l.outer_angle) : 0.f};
		ls.spec[slot_idx] = {l.specular.r, l.specular.g, l.specular.b, 0.f};
		ls.lights[slot_idx] = l;

		if (++slot_idx == forward_light_count)
			break; // all slots filled
	}

	return ls;
}

//
void UpdateForwardPipeline(ForwardPipeline &pipeline, const ForwardPipelineShadowData &shadow_data, const Color &ambient, const ForwardPipelineLights &lights,
	const ForwardPipelineFog &fog, const hg::iVec2 &fb_size) {
	pipeline.uniform_values[UV_Clock].value[0] = time_to_sec_f(get_clock());

	pipeline.uniform_values[UV_FogColor].value = {fog.color.r, fog.color.g, fog.color.b, fog.color.a};
	pipeline.uniform_values[UV_FogState].value = {fog.near, (fog.far - fog.near) ? 1.f / (fog.far - fog.near) : 0.f, 0.f, 0.f};

	pipeline.uniform_values[UV_AmbientColor].value = {ambient.r, ambient.g, ambient.b, ambient.a};

	for (size_t i = 0; i < forward_light_count; ++i) {
		memcpy(pipeline.uniform_values[UV_LightPos].value.data() + i * 4, &lights.pos[i].x, sizeof(float) * 4);
		memcpy(pipeline.uniform_values[UV_LightDir].value.data() + i * 4, &lights.dir[i].x, sizeof(float) * 4);
		memcpy(pipeline.uniform_values[UV_LightDiffuse].value.data() + i * 4, &lights.diff[i].x, sizeof(float) * 4);
		memcpy(pipeline.uniform_values[UV_LightSpecular].value.data() + i * 4, &lights.spec[i].x, sizeof(float) * 4);
	}

	for (size_t i = 0; i < 4; ++i)
		memcpy(pipeline.uniform_values[UV_LinearShadowMatrix].value.data() + i * 16, to_bgfx(shadow_data.linear_shadow_mtx[i]).data(), sizeof(float) * 16);
	memcpy(pipeline.uniform_values[UV_LinearShadowSlice].value.data(), &shadow_data.linear_shadow_slice.x, sizeof(float) * 4);

	memcpy(pipeline.uniform_values[UV_SpotShadowMatrix].value.data(), to_bgfx(shadow_data.spot_shadow_mtx).data(), sizeof(float) * 16);

	pipeline.uniform_values[UV_ShadowState].value = {
		1.f / pipeline.shadow_map_resolution, 1.f / pipeline.shadow_map_resolution, lights.lights[0].shadow_bias, lights.lights[1].shadow_bias};

	pipeline.uniform_values[UV_Resolution].value = {float(fb_size.x), float(fb_size.y), -1, -1};
}

void UpdateForwardPipelineProbe(
	ForwardPipeline &pipeline, Texture irradiance, Texture radiance, Texture brdf, ProbeType type, const Mat4 &world, float parallax) {
	pipeline.uniform_textures[UT_IrradianceMap].texture = irradiance;
	pipeline.uniform_textures[UT_RadianceMap].texture = radiance;
	pipeline.uniform_textures[UT_BrdfMap].texture = brdf;

	memcpy(pipeline.uniform_values[UV_ProbeMatrix].value.data(), to_bgfx(world).data(), sizeof(float) * 16);
	memcpy(pipeline.uniform_values[UV_InvProbeMatrix].value.data(), to_bgfx(InverseFast(world)).data(), sizeof(float) * 16);
	pipeline.uniform_values[UV_ProbeData].value = {float(type), parallax, -1, -1};
}

void UpdateForwardPipelineNoise(ForwardPipeline &pipeline, Texture noise) { pipeline.uniform_textures[UT_NoiseMap].texture = noise; }
void UpdateForwardPipelineAO(ForwardPipeline &pipeline, Texture ao) { pipeline.uniform_textures[UT_AmbientOcclusion].texture = ao; }

//
static float backbuffer_ratio[bgfx::BackbufferRatio::Count] = {1.f, 2.f, 4.f, 8.f, 16.f, 0.5f};

void UpdateForwardPipelineAAA(ForwardPipeline &pipeline, const iRect &rect, const Mat4 &view, const Mat44 &proj, const Mat4 &prv_view, const Mat44 &prv_proj,
	const Vec2 &jitter, bgfx::BackbufferRatio::Enum ssgi_ratio, bgfx::BackbufferRatio::Enum ssr_ratio, float temporal_aa_weight, float motion_blur_strength,
	float exposure, float gamma, int sample_count, float max_distance, float specular_weight, float sharpen) {
	pipeline.uniform_values[UV_Projection].value = {1.f / proj.m[0][0], 1.f / proj.m[1][1], proj.m[2][2], proj.m[2][3]};

	memcpy(pipeline.uniform_values[UV_ViewProjUnjittered].value.data(), to_bgfx(proj * view).data(), sizeof(float) * 16);
	memcpy(pipeline.uniform_values[UV_PreviousViewProjection].value.data(), to_bgfx(prv_proj * prv_view).data(), sizeof(float) * 16);

	memcpy(pipeline.uniform_values[UV_MainProjection].value.data(), to_bgfx(proj).data(), sizeof(float) * 16);
	memcpy(pipeline.uniform_values[UV_MainInvProjection].value.data(), to_bgfx(Inverse(proj)).data(), sizeof(float) * 16);

	pipeline.uniform_values[UV_AAAParams].value = {backbuffer_ratio[ssgi_ratio], backbuffer_ratio[ssr_ratio], temporal_aa_weight, motion_blur_strength,
		exposure, 1.f / gamma, float(sample_count), max_distance, specular_weight, sharpen};

	memcpy(pipeline.uniform_values[UV_MainInvView].value.data(), to_bgfx(InverseFast(view)).data(), sizeof(float) * 16);
}

void UpdateForwardPipelineAAA(ForwardPipeline &pipeline, Texture ssgi, Texture ssr) {
	pipeline.uniform_textures[UT_SSIrradianceMap].texture = ssgi;
	pipeline.uniform_textures[UT_SSRadianceMap].texture = ssr;
}

//
void SubmitModelToForwardPipeline(bgfx::ViewId view_id, const Model &mdl, const ForwardPipeline &pipeline, const PipelineProgram &prg, uint32_t prg_variant,
	uint8_t pipeline_stage, const Color &ambient, const ForwardPipelineLights &lights, const ForwardPipelineFog &fog, const Mat4 &mtx) {
	const auto _mtx = to_bgfx(mtx);
	/*
		const auto pipeline_set_draw_env = [&](int mat_idx) {
			UpdateForwardPipelineUniforms(pipeline.uniforms, ambient, lights, fog);
			set_draw_env(mat_idx);
		};

		RenderModel(view_id, mdl, prg.variants[prg_variant][pipeline_stage], pipeline_set_draw_env, mtx);
	*/
}

static Mat4 ComputeCropMatrix() {
	const bgfx::Caps *caps = bgfx::getCaps();
	const float sy = caps->originBottomLeft ? 0.5f : -0.5f;
	const float sz = caps->homogeneousDepth ? 0.5f : 1.0f;
	const float tz = caps->homogeneousDepth ? 0.5f : 0.0f;
	return TranslationMat4({0.5, 0.5, tz}) * ScaleMat4({0.5, sy, sz});
}

static const Vec4 frustum_corners[8] = {{-1.f, 1.f, 0.f, 1.f}, {1.f, 1.f, 0.f, 1.f}, {1.f, -1.f, 0.f, 1.f}, {-1.f, -1.f, 0.f, 1.f}, {-1.f, 1.f, 1.f, 1.f},
	{1.f, 1.f, 1.f, 1.f}, {1.f, -1.f, 1.f, 1.f}, {-1.f, -1.f, 1.f, 1.f}};

void GenerateLinearShadowMapForForwardPipeline(bgfx::ViewId &view_id, const ViewState &view_state, const std::vector<ModelDisplayList> &display_lists,
	const std::vector<SkinnedModelDisplayList> &skinned_display_lists, const std::vector<Mat4> &mtxs, const ForwardPipelineLights &lights,
	const ForwardPipeline &pipeline, const PipelineResources &res, ForwardPipelineShadowPassViewId &views, ForwardPipelineShadowData &shadow_data,
	const char *debug_name) {
	const bgfx::Caps *caps = bgfx::getCaps();

	std::fill(std::begin(views), std::end(views), 65535);
	const auto &framebuffers_end = std::end(pipeline.framebuffers);

	const auto &light = lights.lights[0];
	if (light.type == FPLT_None)
		return;

	if (light.shadow_type == FPST_None)
		return;

	// SLOT 0: linear light
	const auto &linear_buffer = pipeline.framebuffers.find("linear_shadow_map");
	if (linear_buffer != framebuffers_end) {
		const int resolution = pipeline.shadow_map_resolution;

		float near, far;
		ExtractZRangeFromProjectionMatrix(view_state.proj, near, far);

		Mat4 offset = {0.5f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};

		const Mat4 world = InverseFast(view_state.view);
		bool inv_result;
		const Mat44 inv_proj = Inverse(view_state.proj, inv_result);

		if (inv_result) {
			Vec3 view_dir = GetZ(world);
			Vec3 view_orig = GetT(world);
			Vec3 light_up = GetY(light.world);
			Vec3 light_dir = GetZ(light.world);
			Mat4 inv_light_base = InverseFast(light.world);

			// project frustum corners back to view space.
			Vec3 corners[8];
			for (int i = 0; i < 8; i++) {
				Vec4 tmp = inv_proj * frustum_corners[i];
				corners[i] = MakeVec3(tmp) / tmp.w;
			}
			// adjust to shadow range
			float scale = light.pssm_split[3] / (corners[4].z - corners[0].z);
			for (int i = 0; i < 4; i++)
				corners[i + 4] = (corners[i + 4] - corners[i]) * scale;

			// project back to world space
			for (int i = 0; i < 8; i++)
				corners[i] = world * corners[i];

			const bgfx::Caps *caps = bgfx::getCaps();

			float c0 = near;
			for (int i = 0; i < 4; i++) {
				const float c1 = light.pssm_split[i];

				// compute split frustum and its center
				Vec3 s[8], center = Vec3::Zero;
				for (int j = 0; j < 4; j++) {
					s[j] = corners[j] + (corners[j + 4] - corners[j]) * (c0 - near) / (light.pssm_split[3] - near);
					s[j + 4] = corners[j] + (corners[j + 4] - corners[j]) * (c1 - near) / (light.pssm_split[3] - near);
					center += s[j] + s[j + 4];
				}
				center /= 8.f;

				// compute split radius
				float radius = 0.f;
				for (const auto v : s)
					radius = Max(radius, Dist(center, v));

				// [todo] find the most distant shadow caster for this slice
				const float backup_dist = Mtr(200.f); // 4.f * radius;

				Vec3 light_position = inv_light_base * (center - backup_dist * light_dir);

				// stabilization (snap to shadow map texels)
				light_position.x -= fmod(light_position.x, 2.f * radius / (float)resolution);
				light_position.y -= fmod(light_position.y, 2.f * radius / (float)resolution);

				Mat4 light_view = light.world;
				SetTranslation(light_view, light.world * light_position);
				const Mat4 inv_light = InverseFast(light_view);
				const Mat44 light_projection = ComputeOrthographicProjectionMatrix(0.f, backup_dist + radius, 2.f * radius, Vec2::One);
				const Mat4 crop_mtx = ComputeCropMatrix();

				const Mat44 light_viewproj = light_projection * inv_light;

				// select which part of the shadow map we will render to
				offset.m[0][3] = (i & 1) ? 0.5f : 0.0f;
				offset.m[1][3] = (i & 2) ? 0.5f : 0.0f;
				shadow_data.linear_shadow_mtx[i] = offset * crop_mtx * light_viewproj;
				shadow_data.linear_shadow_slice[i] = c1;
				const auto frustum = MakeFrustum(light_projection, light_view);

				c0 = c1;

				if (debug_name)
					bgfx::setViewName(view_id, format("Shadow map slot 0 (slice %1) for %2").arg(i).arg(debug_name));

				uint16_t view_x = (i & 1) * resolution;
				uint16_t view_y = ((i >> 1) & 1) * resolution;

				if (caps->originBottomLeft)
					view_y = resolution - view_y;

				bgfx::touch(view_id);
				bgfx::setViewMode(bgfx::ViewMode::Default);
				bgfx::setViewRect(view_id, view_x, view_y, resolution, resolution);
				bgfx::setViewClear(view_id, BGFX_CLEAR_DEPTH, 0x0, 1.f, 0);
				bgfx::setViewTransform(view_id, to_bgfx(inv_light).data(), to_bgfx(light_projection).data());
				bgfx::setViewFrameBuffer(view_id, linear_buffer->second);

				std::vector<ModelDisplayList> culled_display_lists = display_lists;
				CullModelDisplayLists(frustum, culled_display_lists, mtxs, res);

				DrawModelDisplayLists(
					view_id, culled_display_lists, 9, pipeline.uniform_values, pipeline.uniform_textures, mtxs, res); // config idx is 9 for FPS_DepthOnly

				// FIXME cull skinned models!
				DrawSkinnedModelDisplayLists(
					view_id, skinned_display_lists, 9, pipeline.uniform_values, pipeline.uniform_textures, mtxs, res); // config idx is 9 for FPS_DepthOnly

				views[FPSP_Slot0LinearSplit0 + i] = view_id++;
			}
		}
	}
}

//
void GenerateSpotShadowMapForForwardPipeline(bgfx::ViewId &view_id, const std::vector<ModelDisplayList> &display_lists,
	const std::vector<SkinnedModelDisplayList> &skinned_display_lists, const std::vector<Mat4> &mtxs, const ForwardPipelineLights &lights,
	const ForwardPipeline &pipeline, const PipelineResources &res, ForwardPipelineShadowPassViewId &views, ForwardPipelineShadowData &shadow_data,
	const char *debug_name) {
	const bgfx::Caps *caps = bgfx::getCaps();

	std::fill(std::begin(views), std::end(views), 65535);
	const auto &framebuffers_end = std::end(pipeline.framebuffers);

	const auto &light = lights.lights[1];
	if (light.type == FPLT_None)
		return;

	// SLOT 1: spot light
	const auto &spot_buffer = pipeline.framebuffers.find("spot_shadow_map");
	if (spot_buffer != framebuffers_end) {
		const auto view = InverseFast(light.world);
		const auto proj = ComputePerspectiveProjectionMatrix(0.1f, 100.f, FovToZoomFactor(2.f * light.outer_angle), {1.f, 1.f});
		const auto frustum = MakeFrustum(proj, light.world);
		const auto crop_mtx = ComputeCropMatrix();
		shadow_data.spot_shadow_mtx = crop_mtx * (proj * view);

		if (debug_name)
			bgfx::setViewName(view_id, format("Shadow map slot 1 for %1").arg(debug_name));

		bgfx::touch(view_id);
		bgfx::setViewMode(bgfx::ViewMode::Default);
		bgfx::setViewRect(view_id, 0, 0, pipeline.shadow_map_resolution, pipeline.shadow_map_resolution);
		bgfx::setViewClear(view_id, BGFX_CLEAR_DEPTH, 0x0, 1.f, 0);
		bgfx::setViewTransform(view_id, to_bgfx(view).data(), to_bgfx(proj).data());
		bgfx::setViewFrameBuffer(view_id, spot_buffer->second);

		std::vector<ModelDisplayList> culled_display_lists = display_lists;
		CullModelDisplayLists(frustum, culled_display_lists, mtxs, res);

		DrawModelDisplayLists(
			view_id, culled_display_lists, 9, pipeline.uniform_values, pipeline.uniform_textures, mtxs, res); // config idx is 9 for FPS_DepthOnly

		// FIXME cull skinned models!
		DrawSkinnedModelDisplayLists(
			view_id, skinned_display_lists, 9, pipeline.uniform_values, pipeline.uniform_textures, mtxs, res); // config idx is 9 for FPS_DepthOnly

		views[FPSP_Slot1Spot] = view_id++;
	}
}

// Forward pipeline configurations
static const PipelineInfo forward_pipeline_info = {
	"forward",
	{
		// AAA rendering prepass
		{"FORWARD_PIPELINE_AAA_PREPASS=1", "FORWARD_PIPELINE_AAA=1", "FORWARD_PIPELINE=1"},

		// basic rendering
		{"FORWARD_PIPELINE=1"},
		{"FORWARD_PIPELINE=1", "SLOT0_SHADOWS=1"},
		{"FORWARD_PIPELINE=1", "SLOT1_SHADOWS=1"},
		{"FORWARD_PIPELINE=1", "SLOT0_SHADOWS=1", "SLOT1_SHADOWS=1"},

		// AAA rendering
		{"FORWARD_PIPELINE_AAA=1", "FORWARD_PIPELINE=1"},
		{"FORWARD_PIPELINE_AAA=1", "FORWARD_PIPELINE=1", "SLOT0_SHADOWS=1"},
		{"FORWARD_PIPELINE_AAA=1", "FORWARD_PIPELINE=1", "SLOT1_SHADOWS=1"},
		{"FORWARD_PIPELINE_AAA=1", "FORWARD_PIPELINE=1", "SLOT0_SHADOWS=1", "SLOT1_SHADOWS=1"},

		// depth only
		{"DEPTH_ONLY=1"},
	},
};

const PipelineInfo &GetForwardPipelineInfo() { return forward_pipeline_info; }

//
ForwardPipeline CreateForwardPipeline(int shadow_map_resolution, bool spot_16bit_shadow_map) {
	ForwardPipeline pipeline;

	pipeline.shadow_map_resolution = shadow_map_resolution;
	pipeline.textures = {
		{"linear_shadow_map", bgfx::createTexture2D(pipeline.shadow_map_resolution * 2, pipeline.shadow_map_resolution * 2, false, 1, bgfx::TextureFormat::D32,
								  BGFX_TEXTURE_RT | BGFX_SAMPLER_COMPARE_LEQUAL)}, // this map is an atlas of 4 splits
		{"spot_shadow_map", bgfx::createTexture2D(pipeline.shadow_map_resolution, pipeline.shadow_map_resolution, false, 1,
								spot_16bit_shadow_map ? bgfx::TextureFormat::D16 : bgfx::TextureFormat::D32, BGFX_TEXTURE_RT | BGFX_SAMPLER_COMPARE_LEQUAL)},
	};

	for (auto &i : pipeline.textures)
		bgfx::setName(i.second, i.first.c_str());

	bgfx::TextureHandle linear_shadow_map_texs[] = {pipeline.textures["linear_shadow_map"]};
	bgfx::TextureHandle spot_shadow_map_texs[] = {pipeline.textures["spot_shadow_map"]};

	pipeline.framebuffers = {
		{"linear_shadow_map", bgfx::createFrameBuffer(1, linear_shadow_map_texs)},
		{"spot_shadow_map", bgfx::createFrameBuffer(1, spot_shadow_map_texs)},
	};

	for (auto &i : pipeline.framebuffers)
		bgfx::setName(i.second, i.first.c_str());

	pipeline.uniform_values = {
		MakeUniformSetValue("uClock", Vec4{}),

		MakeUniformSetValue("uFogColor", Vec4{}),
		MakeUniformSetValue("uFogState", Vec4{}),

		MakeUniformSetValue("uAmbientColor", Vec4{}),

		MakeUniformSetValue("uLightPos", Vec4{}, 8),
		MakeUniformSetValue("uLightDir", Vec4{}, 8),
		MakeUniformSetValue("uLightDiffuse", Vec4{}, 8),
		MakeUniformSetValue("uLightSpecular", Vec4{}, 8),

		MakeUniformSetValue("uLinearShadowMatrix", Mat4{}, 4),
		MakeUniformSetValue("uLinearShadowSlice", Vec4{}),
		MakeUniformSetValue("uSpotShadowMatrix", Mat4{}),
		MakeUniformSetValue("uShadowState", Vec4{}),

		// advanced
		MakeUniformSetValue("uResolution", Vec4{}),
		MakeUniformSetValue("uProjection", Vec4{}),
		MakeUniformSetValue("uMainProjection", Mat4{}),
		MakeUniformSetValue("uMainInvProjection", Mat4{}),

		MakeUniformSetValue("uPreviousViewProjection", Mat4{}),
		MakeUniformSetValue("uViewProjUnjittered", Mat4{}),
		MakeUniformSetValue("uAAAParams", Vec4{}, 3), // [3]

		MakeUniformSetValue("uMainInvView", Mat4{}),
		MakeUniformSetValue("uProbeMatrix", Mat4{}),
		MakeUniformSetValue("uInvProbeMatrix", Mat4{}),
		MakeUniformSetValue("uProbeData", Vec4{}),
	};

	__ASSERT__(pipeline.uniform_values.size() == UV_Count);

	pipeline.uniform_textures = {
		MakeUniformSetTexture("uIrradianceMap", {}, 7),
		MakeUniformSetTexture("uRadianceMap", {}, 8),
		MakeUniformSetTexture("uSSIrradianceMap", {}, 9),
		MakeUniformSetTexture("uSSRadianceMap", {}, 10),
		MakeUniformSetTexture("uBrdfMap", {}, 11),
		MakeUniformSetTexture("uNoiseMap", {}, 12),
		MakeUniformSetTexture("uAmbientOcclusion", {}, 13),
		MakeUniformSetTexture("uLinearShadowMap", {BGFX_TEXTURE_RT | BGFX_SAMPLER_COMPARE_LEQUAL, pipeline.textures["linear_shadow_map"]}, 14),
		MakeUniformSetTexture("uSpotShadowMap", {BGFX_TEXTURE_RT | BGFX_SAMPLER_COMPARE_LEQUAL, pipeline.textures["spot_shadow_map"]}, 15),
	};

	__ASSERT__(pipeline.uniform_textures.size() == UT_Count);

	return pipeline;
}

} // namespace hg
