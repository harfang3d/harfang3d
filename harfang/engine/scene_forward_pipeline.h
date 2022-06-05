// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/aaa_blur.h"
#include "engine/bloom.h"
#include "engine/downsample.h"
#include "engine/forward_pipeline.h"
#include "engine/hiz.h"
#include "engine/motion_blur.h"
#include "engine/render_pipeline.h"
#include "engine/ssgi.h"
#include "engine/ssr.h"
#include "engine/taa.h"
#include "engine/temporal_accumulation.h"
#include "engine/upsample.h"

#if 0
#include "engine/sao.h"
#endif

#include "foundation/frustum.h"
#include "foundation/matrix44.h"
#include "foundation/rect.h"

namespace hg {

class Scene;
struct ViewState;

static const bgfx::FrameBufferHandle InvalidFrameBufferHandle = BGFX_INVALID_HANDLE;

//
enum ForwardPipelineAAADebugBuffer { FPAAADB_None, FPAAADB_SSGI, FPAAADB_SSR };

struct ForwardPipelineAAAConfig {
	float temporal_aa_weight = 0.1f;

	int sample_count = 2; // SSGI/SSR sample count
	float max_distance = 100.f; // SSGI/SSR max exploration
	float z_thickness = 0.1f;

	float bloom_threshold = 5.f, bloom_bias = 0.5f, bloom_intensity = 0.1f;
	float motion_blur = 1.f;
	float exposure = 1.f, gamma = 2.2f;
	float sharpen = 0.1f;

	bool use_tonemapping = true;
	float specular_weight = 1.f;

	ForwardPipelineAAADebugBuffer debug_buffer = FPAAADB_None;
};

bool LoadForwardPipelineAAAConfigFromFile(const char *path, ForwardPipelineAAAConfig &config);
bool LoadForwardPipelineAAAConfigFromAssets(const char *name, ForwardPipelineAAAConfig &config);
bool SaveForwardPipelineAAAConfigToFile(const char *path, const ForwardPipelineAAAConfig &config);

//
struct ForwardPipelineAAA {
	std::array<Texture, 64> noise;

	bgfx::BackbufferRatio::Enum ssgi_ratio = bgfx::BackbufferRatio::Half;
	bgfx::BackbufferRatio::Enum ssr_ratio = bgfx::BackbufferRatio::Half;

	//
	Texture depth, attr0, attr1;
	bgfx::FrameBufferHandle attributes_fb = BGFX_INVALID_HANDLE;

	//
	Downsample downsample;

	//
	Upsample upsample;

	//
#if 0
	SAO sao;
	int sao_sample_count{16};
	float sao_bias{0.08f}, sao_radius{5.f}, sao_sharpness{0.1f};

	Texture sao_output;
	bgfx::FrameBufferHandle sao_output_fb = BGFX_INVALID_HANDLE;
#endif

	//
	Texture work[2];
	bgfx::FrameBufferHandle work_fb[2] = {BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE};

	//
	SSGI ssgi;

	Texture ssgi_history[2];
	bgfx::FrameBufferHandle ssgi_history_fb[2] = {BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE};

	Texture ssgi_output;
	bgfx::FrameBufferHandle ssgi_output_fb = BGFX_INVALID_HANDLE;

	//
	SSR ssr;

	Texture ssr_history[2];
	bgfx::FrameBufferHandle ssr_history_fb[2] = {BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE};

	Texture ssr_output;
	bgfx::FrameBufferHandle ssr_output_fb = BGFX_INVALID_HANDLE;

	//
	TemporalAccumulation temporal_acc;

	//
	HiZ hiz;

	//
	TAA taa;

	//
	AAABlur blur;

	//
	MotionBlur motion_blur;

	//
	Texture frame_hdr;
	bgfx::FrameBufferHandle frame_hdr_fb = BGFX_INVALID_HANDLE;
	bgfx::FrameBufferHandle work_frame_hdr_fb = BGFX_INVALID_HANDLE;
	bgfx::FrameBufferHandle prv_frame_hdr_fb = BGFX_INVALID_HANDLE;
	bgfx::FrameBufferHandle next_frame_hdr_fb = BGFX_INVALID_HANDLE;

	ViewState prv_view_state{};

	void Flip(const ViewState &view_state);

	// compositing
	bgfx::UniformHandle u_color;
	bgfx::UniformHandle u_depth;
	bgfx::ProgramHandle compositing_prg;

	// no compositing
	bgfx::UniformHandle u_copyColor;
	bgfx::UniformHandle u_copyDepth;
	bgfx::ProgramHandle copy_prg;

	//
	Bloom bloom;
};

ForwardPipelineAAA CreateForwardPipelineAAAFromFile(const char *path, const ForwardPipelineAAAConfig &config,
	bgfx::BackbufferRatio::Enum ssgi_ratio = bgfx::BackbufferRatio::Half, bgfx::BackbufferRatio::Enum ssr_ratio = bgfx::BackbufferRatio::Half);
ForwardPipelineAAA CreateForwardPipelineAAAFromAssets(const char *path, const ForwardPipelineAAAConfig &config,
	bgfx::BackbufferRatio::Enum ssgi_ratio = bgfx::BackbufferRatio::Half, bgfx::BackbufferRatio::Enum ssr_ratio = bgfx::BackbufferRatio::Half);

ForwardPipelineAAA CreateForwardPipelineAAAFromFile(const char *path, const ForwardPipelineAAAConfig &config, uint16_t rb_width, uint16_t rb_height,
	bgfx::BackbufferRatio::Enum ssgi_ratio = bgfx::BackbufferRatio::Half, bgfx::BackbufferRatio::Enum ssr_ratio = bgfx::BackbufferRatio::Half);
ForwardPipelineAAA CreateForwardPipelineAAAFromAssets(const char *path, const ForwardPipelineAAAConfig &config, uint16_t rb_width, uint16_t rb_height,
	bgfx::BackbufferRatio::Enum ssgi_ratio = bgfx::BackbufferRatio::Half, bgfx::BackbufferRatio::Enum ssr_ratio = bgfx::BackbufferRatio::Half);

void DestroyForwardPipelineAAA(ForwardPipelineAAA &aaa);

bool IsValid(const ForwardPipelineAAA &aaa);

//
enum SceneForwardPipelinePass {
	SFPP_Opaque,
	SFPP_Transparent,
	SFPP_Slot0LinearSplit0,
	SFPP_Slot0LinearSplit1,
	SFPP_Slot0LinearSplit2,
	SFPP_Slot0LinearSplit3,
	SFPP_Slot1Spot,
	SFPP_DepthPrepass,
	SFPP_Count,
};

using SceneForwardPipelinePassViewId = std::array<bgfx::ViewId, SFPP_Count>;

/// Return the bgfx view id for a scene forward pipeline pass id.
inline bgfx::ViewId GetSceneForwardPipelinePassViewId(const SceneForwardPipelinePassViewId &views, SceneForwardPipelinePass pass) { return views[pass]; }

/// Prepare scene lights for the forward pipeline.
void GetSceneForwardPipelineLights(const Scene &scene, std::vector<ForwardPipelineLight> &out_lights);

/// Return scene fog settings for the forward pipeline.
ForwardPipelineFog GetSceneForwardPipelineFog(const Scene &scene);

//
struct SceneForwardPipelineRenderData {
	std::vector<ModelDisplayList> all_opaque, view_opaque;
	std::vector<ModelDisplayList> all_transparent, view_transparent;

	std::vector<SkinnedModelDisplayList> all_opaque_skinned, view_opaque_skinned;
	std::vector<SkinnedModelDisplayList> all_transparent_skinned, view_transparent_skinned;

	ForwardPipelineLights pipe_lights;
	ForwardPipelineShadowData shadow_data;
	ForwardPipelineFog fog;
};

/// Prepare common scene render data for a submission to the forward pipeline by calling SubmitSceneToForwardPipeline.
void PrepareSceneForwardPipelineCommonRenderData(bgfx::ViewId &view_id, const Scene &scene, SceneForwardPipelineRenderData &render_data,
	const ForwardPipeline &pipeline, const PipelineResources &resources, SceneForwardPipelinePassViewId &views, const char *debug_name = "scene");

void PrepareSceneForwardPipelineViewDependentRenderData(bgfx::ViewId &view_id, const ViewState &view_state, const Scene &scene,
	SceneForwardPipelineRenderData &render_data, const ForwardPipeline &pipeline, const PipelineResources &resources, SceneForwardPipelinePassViewId &views,
	const char *debug_name = "scene");

/// Submit a scene to the forward pipeline.
/// Scene render data must be prepared beforehand by calling PrepareSceneForwardPipelineCommonRenderData and PrepareSceneForwardPipelineViewDependentRenderData.
void SubmitSceneToForwardPipeline(bgfx::ViewId &view_id, const Scene &scene, const Rect<int> &rect, const ViewState &view_state, ForwardPipeline &pipeline,
	const SceneForwardPipelineRenderData &render_data, const PipelineResources &resources, SceneForwardPipelinePassViewId &views,
	bgfx::FrameBufferHandle fb = BGFX_INVALID_HANDLE, const char *debug_name = "scene");

void SubmitSceneToForwardPipeline(bgfx::ViewId &view_id, const Scene &scene, const Rect<int> &rect, const ViewState &view_state, ForwardPipeline &pipeline,
	const SceneForwardPipelineRenderData &render_data, const PipelineResources &resources, SceneForwardPipelinePassViewId &views, ForwardPipelineAAA &aaa,
	const ForwardPipelineAAAConfig &aaa_config, int frame, uint16_t rb_width, uint16_t rb_height, bgfx::FrameBufferHandle fb = BGFX_INVALID_HANDLE,
	const char *debug_name = "scene");

void SubmitSceneToForwardPipeline(bgfx::ViewId &view_id, const Scene &scene, const Rect<int> &rect, const ViewState &view_state, ForwardPipeline &pipeline,
	const SceneForwardPipelineRenderData &render_data, const PipelineResources &resources, SceneForwardPipelinePassViewId &views, ForwardPipelineAAA &aaa,
	const ForwardPipelineAAAConfig &aaa_config, int frame, bgfx::FrameBufferHandle fb = BGFX_INVALID_HANDLE, const char *debug_name = "scene");

/// High-level submit scene to forward pipeline using a custom view state.
void SubmitSceneToPipeline(bgfx::ViewId &view_id, const Scene &scene, const Rect<int> &rect, const ViewState &view_state, ForwardPipeline &pipeline,
	const PipelineResources &resources, SceneForwardPipelinePassViewId &views, bgfx::FrameBufferHandle fb = BGFX_INVALID_HANDLE,
	const char *debug_name = "scene");

/// High-level submit scene to forward pipeline using the current scene camera as view state.
void SubmitSceneToPipeline(bgfx::ViewId &view_id, const Scene &scene, const Rect<int> &rect, bool fov_axis_is_horizontal, ForwardPipeline &pipeline,
	const PipelineResources &resources, SceneForwardPipelinePassViewId &views, bgfx::FrameBufferHandle fb = BGFX_INVALID_HANDLE,
	const char *debug_name = "scene");

/// High-level submit scene to AAA forward pipeline using a custom view state.
void SubmitSceneToPipeline(bgfx::ViewId &view_id, const Scene &scene, const Rect<int> &rect, const ViewState &view_state, ForwardPipeline &pipeline,
	const PipelineResources &resources, SceneForwardPipelinePassViewId &views, ForwardPipelineAAA &aaa, const ForwardPipelineAAAConfig &aaa_config, int frame,
	bgfx::FrameBufferHandle fb = BGFX_INVALID_HANDLE, const char *debug_name = "scene");

/// High-level submit scene to AAA forward pipeline using the current scene camera as view state.
void SubmitSceneToPipeline(bgfx::ViewId &view_id, const Scene &scene, const Rect<int> &rect, bool fov_axis_is_horizontal, ForwardPipeline &pipeline,
	const PipelineResources &resources, SceneForwardPipelinePassViewId &views, ForwardPipelineAAA &aaa, const ForwardPipelineAAAConfig &aaa_config, int frame,
	bgfx::FrameBufferHandle fb = BGFX_INVALID_HANDLE, const char *debug_name = "scene");

} // namespace hg
