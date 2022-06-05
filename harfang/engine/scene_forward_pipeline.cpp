// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/scene_forward_pipeline.h"
#include "engine/assets_rw_interface.h"
#include "engine/scene.h"

#include "foundation/file_rw_interface.h"
#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/projection.h"

#include "fabgen.h"

#include <json/json.hpp>

namespace bgfx {
void getTextureSizeFromRatio(BackbufferRatio::Enum _ratio, uint16_t &_width, uint16_t &_height);
} // namespace bgfx

namespace hg {

void UpdateForwardPipeline(ForwardPipeline &pipeline, const ForwardPipelineShadowData &shadow_data, const Color &ambient, const ForwardPipelineLights &lights,
	const ForwardPipelineFog &fog, const hg::iVec2 &fb_size);
void UpdateForwardPipelineNoise(ForwardPipeline &pipeline, Texture noise);
void UpdateForwardPipelineProbe(
	ForwardPipeline &pipeline, Texture irradiance, Texture radiance, Texture brdf, ProbeType type, const Mat4 &world, float parallax);
void UpdateForwardPipelineAO(ForwardPipeline &pipeline, Texture ao);
void UpdateForwardPipelineAAA(ForwardPipeline &pipeline, const iRect &rect, const Mat4 &view, const Mat44 &proj, const Mat4 &prv_view, const Mat44 &prv_proj,
	const Vec2 &jitter, bgfx::BackbufferRatio::Enum ssgi_ratio, bgfx::BackbufferRatio::Enum ssr_ratio, float temporal_aa_weight, float motion_blur_strength,
	float exposure, float gamma, int sample_count, float max_distance, float specular_weight, float sharpen);
void UpdateForwardPipelineAAA(ForwardPipeline &pipeline, Texture ssgi, Texture ssr);

static const uint64_t attribute_texture_flags = 0 | BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;

//
void ForwardPipelineAAA::Flip(const ViewState &view_state) {
	std::swap(prv_frame_hdr_fb, next_frame_hdr_fb);
	prv_view_state = view_state;
}

//
bool LoadForwardPipelineAAAConfig(const json &js, ForwardPipelineAAAConfig &config) {
	config.temporal_aa_weight = js["taa_weight"];

	config.sample_count = js["sample_count"];
	config.max_distance = js["max_distance"];
	config.z_thickness = js["z_thickness"];

	config.bloom_threshold = js["bloom_threshold"];
	config.bloom_bias = js["bloom_bias"];
	config.bloom_intensity = js["bloom_intensity"];

	config.motion_blur = js["motion_blur"];

	config.exposure = js["exposure"];
	config.gamma = js["gamma"];

	return true;
}

void SaveForwardPipelineAAAConfig(json &js, const ForwardPipelineAAAConfig &config) {
	js["taa_weight"] = config.temporal_aa_weight;

	js["sample_count"] = config.sample_count;
	js["max_distance"] = config.max_distance;
	js["z_thickness"] = config.z_thickness;

	js["bloom_threshold"] = config.bloom_threshold;
	js["bloom_bias"] = config.bloom_bias;
	js["bloom_intensity"] = config.bloom_intensity;

	js["motion_blur"] = config.motion_blur;

	js["exposure"] = config.exposure;
	js["gamma"] = config.gamma;
}

bool LoadForwardPipelineAAAConfigFromFile(const char *path, ForwardPipelineAAAConfig &config) {
	bool result;
	const auto js = LoadJsonFromFile(path, &result);
	return result ? LoadForwardPipelineAAAConfig(js, config) : false;
}

bool LoadForwardPipelineAAAConfigFromAssets(const char *name, ForwardPipelineAAAConfig &config) {
	bool result;
	const auto js = LoadJsonFromAssets(name, &result);
	return result ? LoadForwardPipelineAAAConfig(js, config) : false;
}

bool SaveForwardPipelineAAAConfigToFile(const char *path, const ForwardPipelineAAAConfig &config) {
	json js;
	SaveForwardPipelineAAAConfig(js, config);
	return SaveJsonToFile(js, path);
}

//
bool IsValid(const ForwardPipelineAAA &aaa) {
	for (auto &tex : aaa.noise) {
		if (!bgfx::isValid(tex.handle)) {
			return false;
		}
	}
	if (!(bgfx::isValid(aaa.depth.handle) && bgfx::isValid(aaa.attr0.handle) && bgfx::isValid(aaa.attr1.handle))) {
		return false;
	}
	if (!bgfx::isValid(aaa.attributes_fb)) {
		return false;
	}

	if (!(IsValid(aaa.downsample) && IsValid(aaa.upsample) && IsValid(aaa.ssgi) && IsValid(aaa.ssr) && IsValid(aaa.temporal_acc) && IsValid(aaa.hiz) &&
			IsValid(aaa.taa) && IsValid(aaa.blur) && IsValid(aaa.motion_blur) && IsValid(aaa.bloom))) {
		return false;
	}
	for (int i = 0; i < 2; i++) {
		if (!(bgfx::isValid(aaa.ssgi_history[i].handle) && bgfx::isValid(aaa.ssgi_history_fb[i]) && bgfx::isValid(aaa.ssr_history[i].handle) &&
				bgfx::isValid(aaa.ssr_history_fb[i]))) {
			return false;
		}
	}

	if (aaa.ssgi_ratio != bgfx::BackbufferRatio::Equal) {
		if (!(bgfx::isValid(aaa.ssgi_output.handle) && bgfx::isValid(aaa.ssgi_output_fb))) {
			return false;
		}
	}

	if (aaa.ssr_ratio != bgfx::BackbufferRatio::Equal) {
		if (!(bgfx::isValid(aaa.ssr_output.handle) && bgfx::isValid(aaa.ssr_output_fb))) {
			return false;
		}
	}

	if (!(bgfx::isValid(aaa.work[0].handle) && bgfx::isValid(aaa.work_fb[0]))) {
		return false;
	}
	if (aaa.ssgi_ratio != aaa.ssr_ratio) {
		if (!(bgfx::isValid(aaa.work[1].handle) && bgfx::isValid(aaa.work_fb[1]))) {
			return false;
		}
	}

	return bgfx::isValid(aaa.frame_hdr.handle) && bgfx::isValid(aaa.frame_hdr_fb) && bgfx::isValid(aaa.work_frame_hdr_fb) &&
		   bgfx::isValid(aaa.prv_frame_hdr_fb) && bgfx::isValid(aaa.next_frame_hdr_fb) && bgfx::isValid(aaa.u_color) && bgfx::isValid(aaa.u_depth) &&
		   bgfx::isValid(aaa.compositing_prg);
}

static ForwardPipelineAAA _CreateForwardPipelineAAA(const Reader &ir, const ReadProvider &ip, const char *path, const ForwardPipelineAAAConfig &config,
	const RenderBufferResourceFactory &rb_factory, bgfx::BackbufferRatio::Enum ssgi_ratio, bgfx::BackbufferRatio::Enum ssr_ratio) {
	ForwardPipelineAAA aaa;

	const uint64_t flags = attribute_texture_flags;

	for (size_t i = 0; i < aaa.noise.size(); ++i)
		aaa.noise[i] = LoadTexture(ir, ip, format("%1/noise/LDR_RGBA_%2.png").arg(path).arg(i), 0);

	{
		// depth texture
		aaa.depth = {flags, rb_factory.create_texture2d(bgfx::BackbufferRatio::Equal, false, 1, bgfx::TextureFormat::D24, flags)};
		// w: linear depth, xyz: view normal
		aaa.attr0 = {flags, rb_factory.create_texture2d(bgfx::BackbufferRatio::Equal, false, 1, bgfx::TextureFormat::RGBA16F, flags)};
		// yz: velocity
		aaa.attr1 = {flags, rb_factory.create_texture2d(bgfx::BackbufferRatio::Equal, false, 1, bgfx::TextureFormat::RGBA16F, flags)};

		bgfx::TextureHandle texs[] = {aaa.depth.handle, aaa.attr0.handle, aaa.attr1.handle};
		aaa.attributes_fb = bgfx::createFrameBuffer(3, texs, true);
	}

	{
		aaa.work[0] = {flags, rb_factory.create_texture2d(ssgi_ratio, false, 1, bgfx::TextureFormat::RGBA16F, flags)};
		aaa.work_fb[0] = bgfx::createFrameBuffer(1, &aaa.work[0].handle, true);
		if (ssgi_ratio != ssr_ratio) {
			aaa.work[1] = {flags, rb_factory.create_texture2d(ssr_ratio, false, 1, bgfx::TextureFormat::RGBA16F, flags)};
			aaa.work_fb[1] = bgfx::createFrameBuffer(1, &aaa.work[1].handle, true);
		} else {
			aaa.work[1] = aaa.work[0];
			aaa.work_fb[1] = aaa.work_fb[0];
		}
	}

	{
		aaa.ssgi_ratio = ssgi_ratio;
		if (ssgi_ratio != bgfx::BackbufferRatio::Equal) {
			aaa.ssgi_output = {flags, rb_factory.create_texture2d(bgfx::BackbufferRatio::Equal, false, 1, bgfx::TextureFormat::RGBA16F, flags)};
			bgfx::TextureHandle texs[] = {aaa.ssgi_output.handle};
			aaa.ssgi_output_fb = bgfx::createFrameBuffer(1, texs, true);
		} else {
			aaa.ssgi_output_fb = BGFX_INVALID_HANDLE;
			aaa.ssgi_output = {};
		}
		aaa.ssgi = CreateSSGIFromAssets(path);

		aaa.ssgi_history[0] = {flags, rb_factory.create_texture2d(aaa.ssgi_ratio, false, 1, bgfx::TextureFormat::RGBA16F, flags)};
		aaa.ssgi_history[1] = {flags, rb_factory.create_texture2d(aaa.ssgi_ratio, false, 1, bgfx::TextureFormat::RGBA16F, flags)};

		aaa.ssgi_history_fb[0] = bgfx::createFrameBuffer(1, &aaa.ssgi_history[0].handle, true);
		aaa.ssgi_history_fb[1] = bgfx::createFrameBuffer(1, &aaa.ssgi_history[1].handle, true);
	}

	{
		aaa.ssr_ratio = ssr_ratio;
		if (ssr_ratio != bgfx::BackbufferRatio::Equal) {
			aaa.ssr_output = {flags, rb_factory.create_texture2d(bgfx::BackbufferRatio::Equal, false, 1, bgfx::TextureFormat::RGBA16F, flags)};
			bgfx::TextureHandle texs[] = {aaa.ssr_output.handle};
			aaa.ssr_output_fb = bgfx::createFrameBuffer(1, texs, true);
		} else {
			aaa.ssr_output_fb = BGFX_INVALID_HANDLE;
			aaa.ssr_output = {};
		}
		aaa.ssr = CreateSSRFromAssets(path);

		aaa.ssr_history[0] = {flags, rb_factory.create_texture2d(aaa.ssr_ratio, false, 1, bgfx::TextureFormat::RGBA16F, flags)};

		aaa.ssr_history[1] = {flags, rb_factory.create_texture2d(aaa.ssr_ratio, false, 1, bgfx::TextureFormat::RGBA16F, flags)};

		aaa.ssr_history_fb[0] = bgfx::createFrameBuffer(1, &aaa.ssr_history[0].handle, true);
		aaa.ssr_history_fb[1] = bgfx::createFrameBuffer(1, &aaa.ssr_history[1].handle, true);
	}

	{ aaa.blur = CreateAAABlurFromAssets(path); }

	{ aaa.hiz = CreateHiZFromAssets(path, rb_factory, hg::Min(ssgi_ratio, ssr_ratio)); }

	{ aaa.downsample = CreateDownsampleFromAssets(path, rb_factory); }

	{ aaa.upsample = CreateUpsampleFromAssets(path); }

	{ aaa.temporal_acc = CreateTemporalAccumulationFromAssets(path); }

	{ aaa.motion_blur = CreateMotionBlurFromAssets(path); }

	{
		// RGBA16F
		aaa.frame_hdr = {flags, rb_factory.create_texture2d(bgfx::BackbufferRatio::Equal, false, 1, bgfx::TextureFormat::RGBA16F, flags)};

		bgfx::TextureHandle texs[] = {aaa.depth.handle, aaa.frame_hdr.handle};
		aaa.frame_hdr_fb = bgfx::createFrameBuffer(2, texs, false);

		aaa.work_frame_hdr_fb = rb_factory.create_framebuffer(bgfx::BackbufferRatio::Equal, bgfx::TextureFormat::RGBA16F, flags);

		aaa.prv_frame_hdr_fb =
			rb_factory.create_framebuffer(bgfx::BackbufferRatio::Equal, bgfx::TextureFormat::RGBA16F, flags); // (reprojected) input to SSR/SSGI
		aaa.next_frame_hdr_fb = rb_factory.create_framebuffer(bgfx::BackbufferRatio::Equal, bgfx::TextureFormat::RGBA16F, flags); // current frame with SSR/SSGI
	}

	{
		aaa.compositing_prg = LoadProgram(ir, ip, format("%1/shader/compositing").arg(path).c_str());
		aaa.u_color = bgfx::createUniform("u_color", bgfx::UniformType::Sampler);
		aaa.u_depth = bgfx::createUniform("u_depth", bgfx::UniformType::Sampler);
	}

	{
		aaa.copy_prg = LoadProgram(ir, ip, format("%1/shader/copy").arg(path).c_str());
		aaa.u_copyColor = bgfx::createUniform("u_copyColor", bgfx::UniformType::Sampler);
		aaa.u_copyDepth = bgfx::createUniform("u_copyDepth", bgfx::UniformType::Sampler);
	}

	aaa.taa = CreateTAAFromAssets(path);
	aaa.bloom = CreateBloomFromAssets(hg::format("%1/shader").arg(path), rb_factory, bgfx::BackbufferRatio::Equal);

	if (!IsValid(aaa)) {
		DestroyForwardPipelineAAA(aaa);
		return aaa;
	}

	bgfx::setName(aaa.depth.handle, "aaa.depth");
	bgfx::setName(aaa.attr0.handle, "aaa.attr0");
	bgfx::setName(aaa.attr1.handle, "aaa.attr1");
	bgfx::setName(aaa.attributes_fb, "Attributes FB");
	bgfx::setName(aaa.work[0].handle, "aaa.work[0]");
	bgfx::setName(aaa.work_fb[0], "Work FB #0");
	bgfx::setName(aaa.ssgi_history[0].handle, "aaa.ssgi_history_0");
	bgfx::setName(aaa.ssgi_history[1].handle, "aaa.ssgi_history_1");
	bgfx::setName(aaa.ssr_history[0].handle, "aaa.ssr_history_0");
	bgfx::setName(aaa.ssr_history[1].handle, "aaa.ssr_history_1");
	bgfx::setName(aaa.frame_hdr_fb, "Frame HDR FB");
	bgfx::setName(aaa.work_frame_hdr_fb, "Work HDR frame FB");
	bgfx::setName(aaa.prv_frame_hdr_fb, "Previous HDR frame FB");
	bgfx::setName(aaa.next_frame_hdr_fb, "Next HDR frame FB");

	if (ssgi_ratio != bgfx::BackbufferRatio::Equal)
		bgfx::setName(aaa.ssgi_output.handle, "aaa.ssgi_output");
	if (ssr_ratio != bgfx::BackbufferRatio::Equal)
		bgfx::setName(aaa.ssr_output.handle, "aaa.ssr_output");

	if (ssgi_ratio != ssr_ratio) {
		bgfx::setName(aaa.work[1].handle, "aaa.work[1]");
		bgfx::setName(aaa.work_fb[1], "Work FB #1");
	}

	return aaa;
}

ForwardPipelineAAA CreateForwardPipelineAAAFromFile(
	const char *path, const ForwardPipelineAAAConfig &config, bgfx::BackbufferRatio::Enum ssgi_ratio, bgfx::BackbufferRatio::Enum ssr_ratio) {
	auto rb_factory = RenderBufferResourceFactory::Backbuffer();
	return _CreateForwardPipelineAAA(g_file_reader, g_file_read_provider, path, config, rb_factory, ssgi_ratio, ssr_ratio);
}

ForwardPipelineAAA CreateForwardPipelineAAAFromAssets(
	const char *path, const ForwardPipelineAAAConfig &config, bgfx::BackbufferRatio::Enum ssgi_ratio, bgfx::BackbufferRatio::Enum ssr_ratio) {
	auto rb_factory = RenderBufferResourceFactory::Backbuffer();
	return _CreateForwardPipelineAAA(g_assets_reader, g_assets_read_provider, path, config, rb_factory, ssgi_ratio, ssr_ratio);
}

ForwardPipelineAAA CreateForwardPipelineAAAFromFile(const char *path, const ForwardPipelineAAAConfig &config, uint16_t rb_width, uint16_t rb_height,
	bgfx::BackbufferRatio::Enum ssgi_ratio, bgfx::BackbufferRatio::Enum ssr_ratio) {
	auto rb_factory = RenderBufferResourceFactory::Custom(rb_width, rb_height);
	return _CreateForwardPipelineAAA(g_file_reader, g_file_read_provider, path, config, rb_factory, ssgi_ratio, ssr_ratio);
}

ForwardPipelineAAA CreateForwardPipelineAAAFromAssets(const char *path, const ForwardPipelineAAAConfig &config, uint16_t rb_width, uint16_t rb_height,
	bgfx::BackbufferRatio::Enum ssgi_ratio, bgfx::BackbufferRatio::Enum ssr_ratio) {
	auto rb_factory = RenderBufferResourceFactory::Custom(rb_width, rb_height);
	return _CreateForwardPipelineAAA(g_assets_reader, g_assets_read_provider, path, config, rb_factory, ssgi_ratio, ssr_ratio);
}

void DestroyForwardPipelineAAA(ForwardPipelineAAA &aaa) {
	for (size_t i = 0; i < aaa.noise.size(); ++i)
		Destroy(aaa.noise[i]);

	//
	if (aaa.work_fb[1].idx != aaa.work_fb[0].idx) {
		bgfx_Destroy(aaa.work_fb[1]);
	}
	aaa.work_fb[1] = BGFX_INVALID_HANDLE;
	bgfx_Destroy(aaa.work_fb[0])

		//
		bgfx_Destroy(aaa.attributes_fb); // destroys depth, attr0, attr1

	//
	bgfx_Destroy(aaa.ssgi_output_fb);
	DestroySSGI(aaa.ssgi);
	bgfx_Destroy(aaa.ssr_output_fb);
	DestroySSR(aaa.ssr);

	for (int i = 0; i < 2; i++) {
		bgfx_Destroy(aaa.ssgi_history_fb[i]);
		bgfx_Destroy(aaa.ssr_history_fb[i]);
	}

	//
	DestroyAAABlur(aaa.blur);

	//
#if 0
	DestroySAO(aaa.sao);
#endif
	DestroyMotionBlur(aaa.motion_blur);

	//
	Destroy(aaa.frame_hdr);

	bgfx_Destroy(aaa.frame_hdr_fb);
	bgfx_Destroy(aaa.work_frame_hdr_fb);
	bgfx_Destroy(aaa.prv_frame_hdr_fb);
	bgfx_Destroy(aaa.next_frame_hdr_fb);

	//
	bgfx_Destroy(aaa.compositing_prg);
	bgfx_Destroy(aaa.u_color);
	bgfx_Destroy(aaa.u_depth);

	//
	bgfx_Destroy(aaa.copy_prg);
	bgfx_Destroy(aaa.u_copyColor);
	bgfx_Destroy(aaa.u_copyDepth);

	//
	DestroyTAA(aaa.taa);
	DestroyBloom(aaa.bloom);
	DestroyHiZ(aaa.hiz);

	//
	DestroyDownsample(aaa.downsample);

	DestroyUpsample(aaa.upsample);

	DestroyTemporalAccumulation(aaa.temporal_acc);
}

//
void GetSceneForwardPipelineLights(const Scene &scene, std::vector<ForwardPipelineLight> &out_lights) {
	out_lights.clear();

	const auto lights = scene.GetLights();

	out_lights.reserve(lights.size());

	for (const auto &light : lights) {
		if (!light.IsEnabled())
			continue;

		const auto trs = light.GetTransform();
		const auto lgt = light.GetLight();

		ForwardPipelineLight lgt_;

		lgt_.world = scene.GetTransformWorldMatrix(trs.ref.idx);
		lgt_.diffuse = lgt.GetDiffuseColor() * lgt.GetDiffuseIntensity();
		lgt_.specular = lgt.GetSpecularColor() * lgt.GetSpecularIntensity();

		const auto light_type = lgt.GetType();

		if (light_type == LT_Linear) {
			lgt_.type = FPLT_Linear;
			lgt_.pssm_split = lgt.GetPSSMSplit();
			lgt_.radius = 0.f;
			lgt_.inner_angle = 0.f;
			lgt_.outer_angle = 0.f;
		} else if (light_type == LT_Spot) {
			lgt_.type = FPLT_Spot;
			lgt_.pssm_split = Vec4::Zero;
			lgt_.radius = lgt.GetRadius();
			lgt_.inner_angle = lgt.GetInnerAngle();
			lgt_.outer_angle = lgt.GetOuterAngle();
		} else { // fall back to point
			lgt_.type = FPLT_Point;
			lgt_.pssm_split = Vec4::Zero;
			lgt_.radius = lgt.GetRadius();
			lgt_.inner_angle = 0.f;
			lgt_.outer_angle = 0.f;
		}

		const auto shadow_type = lgt.GetShadowType();

		if (shadow_type == LST_Map)
			lgt_.shadow_type = FPST_Map;
		else
			lgt_.shadow_type = FPST_None;

		lgt_.priority = lgt.GetPriority();
		lgt_.shadow_bias = lgt.GetShadowBias();

		out_lights.push_back(lgt_);
	}
}

//
std::vector<uint32_t> ComputeModelDisplayListSortKeys(
	const Scene &scene, const ViewState &view_state, const std::vector<ModelDisplayList> &dls, const PipelineResources &res) {
	std::vector<uint32_t> sort_keys;
	sort_keys.reserve(dls.size());

	const auto &mtxs = scene.GetTransformWorldMatrices();

	for (const auto &dl : dls) {
		const auto &mdl_mtx = mtxs[dl.mtx_idx]; // model matrix
		const auto &mdl_view_mtx = view_state.view * mdl_mtx; // TODO cache based on dl.mtx_idx
		const auto &mdl = res.models.Get_unsafe_(dl.mdl_idx);

#if 0
		const auto &dl_center = GetCenter(mdl.bounds[dl.lst_idx]); // display list bounding volume center
		const auto &dl_center_in_view = dl_center * mdl_view_mtx;
		sort_keys.push_back(uint32_t(dl_center_in_view.z * 1000.f)); // sort to mm precision
#else
		const auto &bound = mdl.bounds[dl.lst_idx];

		const Vec3 vtx[8] = {
			{bound.mn.x, bound.mn.y, bound.mn.z},
			{bound.mx.x, bound.mn.y, bound.mn.z},
			{bound.mx.x, bound.mx.y, bound.mn.z},
			{bound.mn.x, bound.mx.y, bound.mn.z},
			{bound.mn.x, bound.mn.y, bound.mx.z},
			{bound.mx.x, bound.mn.y, bound.mx.z},
			{bound.mx.x, bound.mx.y, bound.mx.z},
			{bound.mn.x, bound.mx.y, bound.mx.z},
		};

		float closest = std::numeric_limits<float>::max();

		for (auto &v : vtx) {
			const auto &in_view = mdl_view_mtx * v;
			if (in_view.z < closest)
				closest = in_view.z;
		}

		if (closest < 0.f)
			closest = 0.f;

		sort_keys.push_back(ComputeSortKey(closest)); // sort to mm precision
#endif
	}

	return sort_keys;
}

std::vector<uint32_t> ComputeSkinnedModelDisplayListSortKeys(
	const Scene &scene, const ViewState &view_state, const std::vector<SkinnedModelDisplayList> &dls, const PipelineResources &res) {
	std::vector<uint32_t> sort_keys;
	sort_keys.reserve(dls.size());

	const auto &mtxs = scene.GetTransformWorldMatrices();

	for (const auto &dl : dls) {
		// TODO
		sort_keys.push_back(0);
	}

	return sort_keys;
}

static int ComputeForwardPipelineConfigurationIdx(ForwardPipelineStage pipeline_stage, const ForwardPipelineLights &lights) {
	if (pipeline_stage == FPS_AttributeBuffers)
		return 0;

	if (pipeline_stage == FPS_DepthOnly)
		return 9;

	int light_config_idx = 0;

	if (lights.lights[0].shadow_type == FPST_Map)
		light_config_idx |= 0b01;
	if (lights.lights[1].shadow_type == FPST_Map)
		light_config_idx |= 0b10;

	if (pipeline_stage == FPS_Basic)
		return light_config_idx + 1;

	if (pipeline_stage == FPS_Advanced)
		return 4 + light_config_idx + 1;

	return -1;
}

//
void PrepareSceneForwardPipelineCommonRenderData(bgfx::ViewId &view_id, const Scene &scene, SceneForwardPipelineRenderData &render_data,
	const ForwardPipeline &pipeline, const PipelineResources &resources, SceneForwardPipelinePassViewId &views, const char *debug_name) {
	scene.GetModelDisplayLists(
		render_data.all_opaque, render_data.all_transparent, render_data.all_opaque_skinned, render_data.all_transparent_skinned, resources);

	std::vector<ForwardPipelineLight> lights;
	GetSceneForwardPipelineLights(scene, lights);
	render_data.pipe_lights = PrepareForwardPipelineLights(lights);

	ForwardPipelineShadowPassViewId sp_views;
	GenerateSpotShadowMapForForwardPipeline(view_id, render_data.all_opaque, render_data.all_opaque_skinned, scene.GetTransformWorldMatrices(),
		render_data.pipe_lights, pipeline, resources, sp_views, render_data.shadow_data, debug_name);

	views[FPSP_Slot1Spot] = sp_views[FPSP_Slot1Spot];
}

//
void PrepareSceneForwardPipelineViewDependentRenderData(bgfx::ViewId &view_id, const ViewState &view_state, const Scene &scene,
	SceneForwardPipelineRenderData &render_data, const ForwardPipeline &pipeline, const PipelineResources &resources, SceneForwardPipelinePassViewId &views,
	const char *debug_name) {

	ForwardPipelineShadowPassViewId sp_views;
	GenerateLinearShadowMapForForwardPipeline(view_id, view_state, render_data.all_opaque, render_data.all_opaque_skinned, scene.GetTransformWorldMatrices(),
		render_data.pipe_lights, pipeline, resources, sp_views, render_data.shadow_data, debug_name);

	views[SFPP_Slot0LinearSplit0] = sp_views[FPSP_Slot0LinearSplit0];
	views[SFPP_Slot0LinearSplit1] = sp_views[FPSP_Slot0LinearSplit1];
	views[SFPP_Slot0LinearSplit2] = sp_views[FPSP_Slot0LinearSplit2];
	views[SFPP_Slot0LinearSplit3] = sp_views[FPSP_Slot0LinearSplit3];

	render_data.view_opaque = render_data.all_opaque;
	CullModelDisplayLists(view_state.frustum, render_data.view_opaque, scene.GetTransformWorldMatrices(), resources);
	render_data.view_transparent = render_data.all_transparent;
	CullModelDisplayLists(view_state.frustum, render_data.view_transparent, scene.GetTransformWorldMatrices(), resources);

	// FIXME cull skinned models !!!
	render_data.view_opaque_skinned = render_data.all_opaque_skinned;
	render_data.view_transparent_skinned = render_data.all_transparent_skinned;

	render_data.fog = GetSceneForwardPipelineFog(scene);
}

//
ForwardPipelineFog GetSceneForwardPipelineFog(const Scene &scene) {
	return {scene.environment.fog_near, scene.environment.fog_far, scene.environment.fog_color};
}

//
static iRect ScreenSpaceRect(const iRect &rect, bgfx::BackbufferRatio::Enum ratio) {
	if (ratio == bgfx::BackbufferRatio::Half)
		return rect / 2;
	if (ratio == bgfx::BackbufferRatio::Quarter)
		return rect / 4;
	if (ratio == bgfx::BackbufferRatio::Eighth)
		return rect / 8;
	if (ratio == bgfx::BackbufferRatio::Sixteenth)
		return rect / 16;
	if (ratio == bgfx::BackbufferRatio::Double)
		return rect * 2;
	return rect;
}

static void UpdateForwardPipelineProbe(ForwardPipeline &pipeline, const Probe &probe, TextureRef brdf_map_ref, const PipelineResources &resources) {
	auto &brdf_map = resources.textures.Get(brdf_map_ref);

	auto &irradiance_map = resources.textures.Get(probe.irradiance_map);
	auto &radiance_map = resources.textures.Get(probe.radiance_map);

	Mat4 world;
	if (probe.type == PT_Cube)
		world = TransformationMat4(probe.trs.pos, probe.trs.rot, probe.trs.scl);
	else if (probe.type == PT_Sphere)
		world = TransformationMat4(probe.trs.pos, probe.trs.rot, {probe.trs.scl.x, probe.trs.scl.x, probe.trs.scl.x});

	UpdateForwardPipelineProbe(pipeline, irradiance_map, radiance_map, brdf_map, probe.type, world, unpack_float(probe.parallax));
}

void SubmitSceneToForwardPipeline(bgfx::ViewId &view_id, const Scene &scene, const Rect<int> &rect, const ViewState &view_state, ForwardPipeline &pipeline,
	const SceneForwardPipelineRenderData &render_data, const PipelineResources &resources, SceneForwardPipelinePassViewId &views, ForwardPipelineAAA &aaa,
	const ForwardPipelineAAAConfig &aaa_config, int frame, uint16_t rb_width, uint16_t rb_height, bgfx::FrameBufferHandle fb, const char *debug_name) {
	__ASSERT__(bgfx::getCaps()->supported & BGFX_CAPS_TEXTURE_BLIT);

	hg::iVec2 fb_size(rb_width, rb_width);

	if (fb_size.x <= 0 || fb_size.y <= 0) {
		const bgfx::Stats *stats = bgfx::getStats();
		fb_size.x = stats->width;
		fb_size.y = stats->height;
	}

	// reset pass views
	std::fill(std::begin(views), std::end(views), 65535);

	// update pipeline
	UpdateForwardPipeline(pipeline, render_data.shadow_data, scene.environment.ambient, render_data.pipe_lights, render_data.fog, fb_size);
	UpdateForwardPipelineAAA(pipeline, rect, view_state.view, view_state.proj, aaa.prv_view_state.view, aaa.prv_view_state.proj, TAAHaltonJitter8(frame),
		aaa.ssgi_ratio, aaa.ssr_ratio, aaa_config.temporal_aa_weight, aaa_config.motion_blur, aaa_config.exposure, aaa_config.gamma, aaa_config.sample_count,
		aaa_config.max_distance, aaa_config.specular_weight, aaa_config.sharpen); // [todo] ssgi_ratio/ssr_ratio ([EJ] what is wrong with ssgi_ratio/ssr_ratio)
	UpdateForwardPipelineProbe(pipeline, scene.environment.probe, scene.environment.brdf_map, resources);

	// TAA jittered projection matrix
	const auto jitter = TAAHaltonJitter8(frame) / Vec2(float(GetWidth(rect)), float(GetHeight(rect)));
	auto proj_jittered = to_bgfx(view_state.proj);
	proj_jittered[8] += jitter.x;
	proj_jittered[9] += jitter.y;

	// fill attribute buffers (linear depth/normal/velocity)
	{
		bgfx::touch(view_id);
		bgfx::setViewName(view_id, format("Attribute buffers: %1").arg(debug_name).c_str());
		bgfx::setViewRect(view_id, rect.sx, rect.sy, GetWidth(rect), GetHeight(rect));
		bgfx::setViewFrameBuffer(view_id, aaa.attributes_fb);

		bgfx::setViewClear(view_id, BGFX_CLEAR_DEPTH | BGFX_CLEAR_COLOR, 0, 1.f, 0);

		bgfx::setViewMode(bgfx::ViewMode::Default);
		bgfx::setViewTransform(view_id, to_bgfx(view_state.view).data(), proj_jittered.data());

		const int pipeline_config_idx = ComputeForwardPipelineConfigurationIdx(FPS_AttributeBuffers, render_data.pipe_lights);

		DrawModelDisplayLists(view_id, render_data.view_opaque, pipeline_config_idx, pipeline.uniform_values, pipeline.uniform_textures,
			scene.GetTransformWorldMatrices(), scene.GetPreviousTransformWorldMatrices(), resources);
		DrawSkinnedModelDisplayLists(view_id, render_data.view_opaque_skinned, pipeline_config_idx, pipeline.uniform_values, pipeline.uniform_textures,
			scene.GetTransformWorldMatrices(), scene.GetPreviousTransformWorldMatrices(), resources);

		views[SFPP_DepthPrepass] = view_id++;
	}

	// animated blue noise texture
	const Texture &noise = aaa.noise[frame % aaa.noise.size()];

	UpdateForwardPipelineNoise(pipeline, noise);

	// Downsample
	if ((aaa.ssgi_ratio != bgfx::BackbufferRatio::Equal) || (aaa.ssr_ratio != bgfx::BackbufferRatio::Equal)) {
		ComputeDownsample(view_id, ScreenSpaceRect(rect, bgfx::BackbufferRatio::Half), {attribute_texture_flags, bgfx::getTexture(aaa.prv_frame_hdr_fb)},
			aaa.attr0, aaa.depth, aaa.downsample);
	}

	// HiZ
	{
		if ((aaa.ssgi_ratio != bgfx::BackbufferRatio::Equal) && (aaa.ssr_ratio != bgfx::BackbufferRatio::Equal)) {
			ComputeHiZ(
				view_id, fb_size, ScreenSpaceRect(rect, bgfx::BackbufferRatio::Half), view_state.proj, aaa_config.z_thickness, aaa.downsample.attr0, aaa.hiz);
		} else {
			ComputeHiZ(view_id, fb_size, rect, view_state.proj, aaa_config.z_thickness, aaa.attr0, aaa.hiz);
		}
	}

	int current = frame & 1;
	int previous = current ^ 1;

	Texture ssgi_output;
	Texture ssr_output;

	// SSGI
	{
		auto &probe_map = resources.textures.Get(scene.environment.probe.irradiance_map);
		if (aaa.ssgi_ratio != bgfx::BackbufferRatio::Equal) {
			ComputeSSGI(view_id, ScreenSpaceRect(rect, aaa.ssgi_ratio), aaa.ssgi_ratio, aaa.downsample.color, aaa.downsample.attr0,
				aaa.attr1, // [todo] downsample attr1 ?!
				probe_map, noise, aaa.hiz, aaa.work_fb[0], aaa.ssgi);

			ComputeTemporalAccumulation(view_id, ScreenSpaceRect(rect, aaa.ssgi_ratio), aaa.work[0], aaa.ssgi_history[previous], aaa.attr1,
				aaa.ssgi_history_fb[current], aaa.temporal_acc);

			ComputeAAABlur(view_id, ScreenSpaceRect(rect, aaa.ssgi_ratio), aaa.downsample.attr0, aaa.ssgi_history_fb[current], aaa.work_fb[0], aaa.blur);

			ComputeUpsample(view_id, rect, aaa.ssgi_history[current], aaa.downsample.attr0, aaa.attr0, aaa.ssgi_output_fb, aaa.upsample);
			ssgi_output = aaa.ssgi_output;
		} else {
			ComputeSSGI(view_id, rect, aaa.ssgi_ratio, {attribute_texture_flags, bgfx::getTexture(aaa.prv_frame_hdr_fb)}, aaa.attr0, aaa.attr1, probe_map,
				noise, aaa.hiz, aaa.work_fb[0], aaa.ssgi);

			ComputeTemporalAccumulation(view_id, rect, aaa.work[0], aaa.ssgi_history[previous], aaa.attr1, aaa.ssgi_history_fb[current], aaa.temporal_acc);

			ComputeAAABlur(view_id, ScreenSpaceRect(rect, aaa.ssgi_ratio), aaa.attr0, aaa.ssgi_history_fb[current], aaa.work_fb[0], aaa.blur);

			ssgi_output = aaa.ssgi_history[current];
		}
	}

	// SSR
	{
		auto &probe_map = resources.textures.Get(scene.environment.probe.radiance_map);

		if (aaa.ssr_ratio != bgfx::BackbufferRatio::Equal) {
			ComputeSSR(view_id, ScreenSpaceRect(rect, aaa.ssr_ratio), aaa.ssr_ratio, aaa.downsample.color, aaa.downsample.attr0,
				aaa.attr1, // [todo] downsample attr1 ?!
				probe_map, noise, aaa.hiz, aaa.work_fb[1], aaa.ssr);

			ComputeTemporalAccumulation(view_id, ScreenSpaceRect(rect, aaa.ssr_ratio), aaa.work[1], aaa.ssr_history[previous], aaa.attr1,
				aaa.ssr_history_fb[current], aaa.temporal_acc);

			ComputeUpsample(view_id, rect, aaa.ssr_history[current], aaa.downsample.attr0, aaa.attr0, aaa.ssr_output_fb, aaa.upsample);
			ssr_output = aaa.ssr_output;
		} else {
			ComputeSSR(view_id, rect, aaa.ssr_ratio, {attribute_texture_flags, bgfx::getTexture(aaa.prv_frame_hdr_fb)}, aaa.attr0, aaa.attr1, probe_map, noise,
				aaa.hiz, aaa.work_fb[1], aaa.ssr);
			ComputeTemporalAccumulation(view_id, rect, aaa.work[1], aaa.ssr_history[previous], aaa.attr1, aaa.ssr_history_fb[current], aaa.temporal_acc);
			ssr_output = aaa.ssr_history[current];
		}
	}

#if 0
	// SAO [todo] move to forward pipeline
	{
		ComputeSAO(view_id, ScreenSpaceRect(rect, aaa.ssgi_ratio), aaa.attr0, aaa.attr1, noise, aaa.sao_output_fb, aaa.sao, view_state.proj, aaa.sao_bias, // [todo] aaa.ssgi_ratio : add ratio to SAO struct
			aaa.sao_radius,
		aaa.sao_sample_count, aaa.sao_sharpness);
		UpdateForwardPipelineAO(pipeline, aaa.sao_output);
	}
#endif

	// setup environment for AAA pipeline
	UpdateForwardPipelineAAA(pipeline, ssgi_output, ssr_output);

	// opaque pass
	{
		bgfx::touch(view_id);
		bgfx::setViewName(view_id, format("Opaque pass: %1").arg(debug_name).c_str());
		bgfx::setViewRect(view_id, rect.sx, rect.sy, GetWidth(rect), GetHeight(rect));
		bgfx::setViewFrameBuffer(view_id, aaa.frame_hdr_fb);

		if (scene.canvas.clear_z || scene.canvas.clear_color) {
			const uint16_t flags = (scene.canvas.clear_z ? BGFX_CLEAR_DEPTH : 0) | (scene.canvas.clear_color ? BGFX_CLEAR_COLOR : 0);
			const uint32_t color = ColorToABGR32(scene.canvas.color);
			bgfx::setViewClear(view_id, flags, color, 1.f, 0);
		} else {
			bgfx::setViewClear(view_id, BGFX_CLEAR_NONE, 0, 1.f, UINT8_MAX);
		}

		bgfx::setViewMode(bgfx::ViewMode::Default);
		bgfx::setViewTransform(view_id, to_bgfx(view_state.view).data(), proj_jittered.data());

		const int pipeline_config_idx = ComputeForwardPipelineConfigurationIdx(FPS_Advanced, render_data.pipe_lights);

		DrawModelDisplayLists(view_id, render_data.view_opaque, pipeline_config_idx, pipeline.uniform_values, pipeline.uniform_textures,
			scene.GetTransformWorldMatrices(), resources);
		DrawSkinnedModelDisplayLists(view_id, render_data.view_opaque_skinned, pipeline_config_idx, pipeline.uniform_values, pipeline.uniform_textures,
			scene.GetTransformWorldMatrices(), resources);

		views[SFPP_Opaque] = view_id++;
	}

	// setup environment for non AAA pipeline
	// UpdateForwardPipelineProbe(pipeline, scene.environment.probe, scene.environment.brdf_map, resources);

	// transparent pass
	{
		bgfx::touch(view_id);
		bgfx::setViewName(view_id, format("Transparent pass: %1").arg(debug_name).c_str());
		bgfx::setViewRect(view_id, rect.sx, rect.sy, GetWidth(rect), GetHeight(rect));
		bgfx::setViewFrameBuffer(view_id, aaa.frame_hdr_fb);

		bgfx::setViewClear(view_id, BGFX_CLEAR_NONE, 0, 1.f, UINT8_MAX);
		bgfx::setViewMode(view_id, bgfx::ViewMode::DepthDescending);
		bgfx::setViewTransform(view_id, to_bgfx(view_state.view).data(), proj_jittered.data());

		const int pipeline_config_idx = ComputeForwardPipelineConfigurationIdx(FPS_Basic, render_data.pipe_lights);

		const auto view_transparent_sort_keys = ComputeModelDisplayListSortKeys(scene, view_state, render_data.view_transparent, resources);
		DrawModelDisplayLists(view_id, render_data.view_transparent, view_transparent_sort_keys, pipeline_config_idx, pipeline.uniform_values,
			pipeline.uniform_textures, scene.GetTransformWorldMatrices(), resources);

		const auto view_transparent_skinned_sort_keys =
			ComputeSkinnedModelDisplayListSortKeys(scene, view_state, render_data.view_transparent_skinned, resources);
		DrawSkinnedModelDisplayLists(view_id, render_data.view_transparent_skinned, view_transparent_skinned_sort_keys, pipeline_config_idx,
			pipeline.uniform_values, pipeline.uniform_textures, scene.GetTransformWorldMatrices(), resources);

		views[SFPP_Transparent] = view_id++;
	}

	// TAA
	ApplyTAA(
		view_id, rect, aaa.frame_hdr, {attribute_texture_flags, bgfx::getTexture(aaa.prv_frame_hdr_fb)}, aaa.attr0, aaa.attr1, aaa.next_frame_hdr_fb, aaa.taa);

	// motion blur
	ApplyMotionBlur(
		view_id, rect, {attribute_texture_flags, bgfx::getTexture(aaa.next_frame_hdr_fb)}, aaa.attr0, aaa.attr1, noise, aaa.work_frame_hdr_fb, aaa.motion_blur);

	// bloom
	ApplyBloom(view_id, rect, {attribute_texture_flags, bgfx::getTexture(aaa.work_frame_hdr_fb)}, fb_size, aaa.frame_hdr_fb, aaa.bloom,
		aaa_config.bloom_threshold, aaa_config.bloom_bias, aaa_config.bloom_intensity);

	// final compositing for presentation (exposure/gamma correction)
	if (aaa_config.use_tonemapping) {
		if (bgfx::isValid(fb))
			bgfx::setViewName(view_id, "Final compositing to target framebuffer");
		else
			bgfx::setViewName(view_id, "Final compositing to back buffer");

		bgfx::setViewRect(view_id, 0, 0, fb_size.x, fb_size.y);
		bgfx::setViewFrameBuffer(view_id, fb);

		bgfx::setViewClear(view_id, BGFX_CLEAR_NONE, 0, 1.f, UINT8_MAX);
		bgfx::setViewTransform(view_id, nullptr, nullptr);

		if (aaa_config.debug_buffer == FPAAADB_None)
			bgfx::setTexture(0, aaa.u_color, aaa.frame_hdr.handle, uint32_t(attribute_texture_flags));
		else if (aaa_config.debug_buffer == FPAAADB_SSGI)
			bgfx::setTexture(0, aaa.u_color, ssgi_output.handle, uint32_t(attribute_texture_flags));
		else if (aaa_config.debug_buffer == FPAAADB_SSR)
			bgfx::setTexture(0, aaa.u_color, ssr_output.handle, uint32_t(attribute_texture_flags));
		bgfx::setTexture(1, aaa.u_depth, aaa.depth.handle, uint32_t(aaa.depth.flags));

		bgfx::VertexLayout decl;
		decl.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float).end();

		const float k_y = bgfx::getCaps()->originBottomLeft ? 0.f : 1.f;

		Vertices vtx(decl, 4);
		vtx.Begin(0).SetPos({-1, -1, 0}).SetTexCoord0({0, k_y}).End();
		vtx.Begin(1).SetPos({1, -1, 0}).SetTexCoord0({1, k_y}).End();
		vtx.Begin(2).SetPos({1, 1, 0}).SetTexCoord0({1, 1.f - k_y}).End();
		vtx.Begin(3).SetPos({-1, 1, 0}).SetTexCoord0({0, 1.f - k_y}).End();
		DrawTriangles(view_id, {0, 1, 2, 0, 2, 3}, vtx, aaa.compositing_prg, {}, {}, ComputeRenderState(BM_Opaque, DT_Always));

		++view_id;
	} else {
		bgfx::setViewName(view_id, "Copying to back buffer");

		bgfx::setViewRect(view_id, 0, 0, fb_size.x, fb_size.y);
		bgfx::setViewFrameBuffer(view_id, fb);
		bgfx::setViewClear(view_id, BGFX_CLEAR_NONE, 0, 1.f);
		bgfx::setViewTransform(view_id, nullptr, nullptr);

		bgfx::setTexture(0, aaa.u_copyColor, aaa.frame_hdr.handle, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
		bgfx::setTexture(1, aaa.u_copyDepth, aaa.depth.handle, uint32_t(aaa.depth.flags));

		bgfx::VertexLayout decl;
		decl.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float).end();

		const float k_y = bgfx::getCaps()->originBottomLeft ? 0.f : 1.f;

		Vertices vtx(decl, 4);
		vtx.Begin(0).SetPos({-1, -1, 0}).SetTexCoord0({0, k_y}).End();
		vtx.Begin(1).SetPos({1, -1, 0}).SetTexCoord0({1, k_y}).End();
		vtx.Begin(2).SetPos({1, 1, 0}).SetTexCoord0({1, 1.f - k_y}).End();
		vtx.Begin(3).SetPos({-1, 1, 0}).SetTexCoord0({0, 1.f - k_y}).End();
		DrawTriangles(view_id, {0, 1, 2, 0, 2, 3}, vtx, aaa.copy_prg, {}, {}, ComputeRenderState(BM_Opaque, DT_Always));

		++view_id;
	}
}

void SubmitSceneToForwardPipeline(bgfx::ViewId &view_id, const Scene &scene, const Rect<int> &rect, const ViewState &view_state, ForwardPipeline &pipeline,
	const SceneForwardPipelineRenderData &render_data, const PipelineResources &resources, SceneForwardPipelinePassViewId &views, ForwardPipelineAAA &aaa,
	const ForwardPipelineAAAConfig &aaa_config, int frame, bgfx::FrameBufferHandle fb, const char *debug_name) {
	SubmitSceneToForwardPipeline(view_id, scene, rect, view_state, pipeline, render_data, resources, views, aaa, aaa_config, frame, 0, 0, fb, debug_name);
}

// basic rendering path
void SubmitSceneToForwardPipeline(bgfx::ViewId &view_id, const Scene &scene, const Rect<int> &rect, const ViewState &view_state, ForwardPipeline &pipeline,
	const SceneForwardPipelineRenderData &render_data, const PipelineResources &resources, SceneForwardPipelinePassViewId &views, bgfx::FrameBufferHandle fb,
	const char *debug_name) {
	std::fill(std::begin(views), std::end(views), 65535);

	// update pipeline
	UpdateForwardPipeline(
		pipeline, render_data.shadow_data, scene.environment.ambient, render_data.pipe_lights, render_data.fog, hg::iVec2(GetWidth(rect), GetHeight(rect)));
	//	UpdateForwardPipelineAAA(pipeline, rect, view_state.view, view_state.proj, view_state.view, view_state.proj, {});
	UpdateForwardPipelineProbe(pipeline, scene.environment.probe, scene.environment.brdf_map, resources);

	const int pipeline_config_idx = ComputeForwardPipelineConfigurationIdx(FPS_Basic, render_data.pipe_lights);

	// opaque pass
	{
		bgfx::touch(view_id);
		bgfx::setViewName(view_id, format("Opaque pass: %1").arg(debug_name).c_str());
		bgfx::setViewRect(view_id, rect.sx, rect.sy, GetWidth(rect), GetHeight(rect));
		bgfx::setViewFrameBuffer(view_id, fb);

		if (scene.canvas.clear_z || scene.canvas.clear_color) {
			const uint16_t flags = (scene.canvas.clear_z ? BGFX_CLEAR_DEPTH : 0) | (scene.canvas.clear_color ? BGFX_CLEAR_COLOR : 0);
			const uint32_t color = ColorToABGR32(scene.canvas.color);
			bgfx::setViewClear(view_id, flags, color, 1.f, 0);
		} else {
			bgfx::setViewClear(view_id, BGFX_CLEAR_NONE, 0, 1.f, UINT8_MAX);
		}

		bgfx::setViewMode(bgfx::ViewMode::Default);
		bgfx::setViewTransform(view_id, to_bgfx(view_state.view).data(), to_bgfx(view_state.proj).data());

		DrawModelDisplayLists(view_id, render_data.view_opaque, pipeline_config_idx, pipeline.uniform_values, pipeline.uniform_textures,
			scene.GetTransformWorldMatrices(), resources);
		DrawSkinnedModelDisplayLists(view_id, render_data.view_opaque_skinned, pipeline_config_idx, pipeline.uniform_values, pipeline.uniform_textures,
			scene.GetTransformWorldMatrices(), resources);

		views[SFPP_Opaque] = view_id++;
	}

	// transparent pass
	{
		bgfx::touch(view_id);
		bgfx::setViewName(view_id, format("Transparent pass: %1").arg(debug_name).c_str());
		bgfx::setViewRect(view_id, rect.sx, rect.sy, GetWidth(rect), GetHeight(rect));
		bgfx::setViewFrameBuffer(view_id, fb);

		bgfx::setViewClear(view_id, BGFX_CLEAR_NONE, 0, 1.f, UINT8_MAX);
		bgfx::setViewMode(view_id, bgfx::ViewMode::DepthDescending);
		bgfx::setViewTransform(view_id, to_bgfx(view_state.view).data(), to_bgfx(view_state.proj).data());

		const auto view_transparent_sort_keys = ComputeModelDisplayListSortKeys(scene, view_state, render_data.view_transparent, resources);
		DrawModelDisplayLists(view_id, render_data.view_transparent, view_transparent_sort_keys, pipeline_config_idx, pipeline.uniform_values,
			pipeline.uniform_textures, scene.GetTransformWorldMatrices(), resources);

		const auto view_transparent_skinned_sort_keys =
			ComputeSkinnedModelDisplayListSortKeys(scene, view_state, render_data.view_transparent_skinned, resources);
		DrawSkinnedModelDisplayLists(view_id, render_data.view_transparent_skinned, view_transparent_skinned_sort_keys, pipeline_config_idx,
			pipeline.uniform_values, pipeline.uniform_textures, scene.GetTransformWorldMatrices(), resources);

		views[SFPP_Transparent] = view_id++;
	}
}

//
void SubmitSceneToPipeline(bgfx::ViewId &view_id, const Scene &scene, const Rect<int> &rect, const ViewState &view_state, ForwardPipeline &pipeline,
	const PipelineResources &resources, SceneForwardPipelinePassViewId &views, bgfx::FrameBufferHandle fb, const char *debug_name) {
	SceneForwardPipelineRenderData render_data;
	PrepareSceneForwardPipelineCommonRenderData(view_id, scene, render_data, pipeline, resources, views, debug_name);
	PrepareSceneForwardPipelineViewDependentRenderData(view_id, view_state, scene, render_data, pipeline, resources, views, debug_name);
	SubmitSceneToForwardPipeline(view_id, scene, rect, view_state, pipeline, render_data, resources, views, fb, debug_name);
}

void SubmitSceneToPipeline(bgfx::ViewId &view_id, const Scene &scene, const Rect<int> &rect, bool fov_axis_is_horizontal, ForwardPipeline &pipeline,
	const PipelineResources &resources, SceneForwardPipelinePassViewId &views, bgfx::FrameBufferHandle fb, const char *debug_name) {
	const auto w = float(GetWidth(rect)), h = float(GetHeight(rect));
	const auto ar = fov_axis_is_horizontal ? ComputeAspectRatioX(w, h) : ComputeAspectRatioY(w, h);

	const auto view_state = scene.ComputeCurrentCameraViewState(ar);
	SubmitSceneToPipeline(view_id, scene, rect, view_state, pipeline, resources, views, fb, debug_name);
}

//
void SubmitSceneToPipeline(bgfx::ViewId &view_id, const Scene &scene, const Rect<int> &rect, const ViewState &view_state, ForwardPipeline &pipeline,
	const PipelineResources &resources, SceneForwardPipelinePassViewId &views, ForwardPipelineAAA &aaa, const ForwardPipelineAAAConfig &aaa_config, int frame,
	bgfx::FrameBufferHandle fb, const char *debug_name) {
	SceneForwardPipelineRenderData render_data;
	PrepareSceneForwardPipelineCommonRenderData(view_id, scene, render_data, pipeline, resources, views, debug_name);
	PrepareSceneForwardPipelineViewDependentRenderData(view_id, view_state, scene, render_data, pipeline, resources, views, debug_name);
	SubmitSceneToForwardPipeline(view_id, scene, rect, view_state, pipeline, render_data, resources, views, aaa, aaa_config, frame, fb, debug_name);
	aaa.Flip(view_state);
}

void SubmitSceneToPipeline(bgfx::ViewId &view_id, const Scene &scene, const Rect<int> &rect, bool fov_axis_is_horizontal, ForwardPipeline &pipeline,
	const PipelineResources &resources, SceneForwardPipelinePassViewId &views, ForwardPipelineAAA &aaa, const ForwardPipelineAAAConfig &aaa_config, int frame,
	bgfx::FrameBufferHandle fb, const char *debug_name) {
	const auto w = float(GetWidth(rect)), h = float(GetHeight(rect));
	const auto ar = fov_axis_is_horizontal ? ComputeAspectRatioX(w, h) : ComputeAspectRatioY(w, h);

	const auto view_state = scene.ComputeCurrentCameraViewState(ar);
	SubmitSceneToPipeline(view_id, scene, rect, view_state, pipeline, resources, views, aaa, aaa_config, frame, fb, debug_name);
}

} // namespace hg
