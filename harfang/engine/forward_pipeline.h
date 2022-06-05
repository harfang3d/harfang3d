// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/render_pipeline.h"
#include "foundation/color.h"
#include "foundation/matrix4.h"
#include "foundation/matrix44.h"
#include "foundation/rect.h"
#include "foundation/unit.h"
#include "foundation/vector4.h"

#include <bgfx/bgfx.h>

#include <array>
#include <map>
#include <string>
#include <vector>

namespace hg {

static const int forward_light_count = 8;

enum ForwardPipelineLightType { FPLT_None, FPLT_Point, FPLT_Spot, FPLT_Linear };
enum ForwardPipelineShadowType { FPST_None, FPST_Map };

/*!
	Single light for the forward pipeline.
	@note The complete lighting rig is passed as a ForwardPipelineLights.
	@see PrepareForwardPipelineLights.
*/
struct ForwardPipelineLight { // 112B
	ForwardPipelineLightType type;
	ForwardPipelineShadowType shadow_type;

	Mat4 world; // 48B

	Color diffuse, specular;

	float radius;
	float inner_angle, outer_angle;
	Vec4 pssm_split;
	float priority;
	float shadow_bias;
};

ForwardPipelineLight MakeForwardPipelinePointLight(const Mat4 &world, const Color &diffuse, const Color &specular, float radius = 0.f, float priority = 0.f,
	ForwardPipelineShadowType shadow_type = FPST_None, float shadow_bias = default_shadow_bias);
ForwardPipelineLight MakeForwardPipelineSpotLight(const Mat4 &world, const Color &diffuse, const Color &specular, float radius = 0.f,
	float inner_angle = Deg(40.f), float outer_angle = Deg(45.f), float priority = 0.f, ForwardPipelineShadowType shadow_type = FPST_None,
	float shadow_bias = default_shadow_bias);
ForwardPipelineLight MakeForwardPipelineLinearLight(const Mat4 &world, const Color &diffuse, const Color &specular,
	const Vec4 &pssm_split = {10.f, 50.f, 100.f, 500.f}, float priority = 0.f, ForwardPipelineShadowType shadow_type = FPST_None,
	float shadow_bias = default_shadow_bias);

//
struct ForwardPipelineLights {
	std::array<Vec4, forward_light_count> pos, dir, diff, spec; // shader uniforms
	std::array<ForwardPipelineLight, forward_light_count> lights; // lights that were used to fill uniform values
};

ForwardPipelineLights PrepareForwardPipelineLights(const std::vector<ForwardPipelineLight> &lights);

struct ForwardPipelineShadowData {
	Mat44 linear_shadow_mtx[4]; // slot 0: 4 split PSSM linear light
	Vec4 linear_shadow_slice; // slot 0: PSSM slice distances linear light
	Mat44 spot_shadow_mtx; // slot 1: spot light
};

/// Fog properties for the forward pipeline.
struct ForwardPipelineFog {
	float near{}, far{};
	Color color{};
};

/*!
	Rendering pipeline implementing a forward rendering strategy.

	The main characteristics of this pipeline are:
	- Render in two passes: opaque display lists then transparent ones.
	- Fixed 8 light slots supporting 1 linear light with PSSM shadow mapping, 1 spot with shadow mapping and up to 6 point lights with no shadow mapping.
*/
struct ForwardPipeline : Pipeline {
	int shadow_map_resolution{1024};
};

/// Create a forward pipeline and its resources.
/// @see DestroyForwardPipeline.
ForwardPipeline CreateForwardPipeline(int shadow_map_resolution = 1024, bool spot_16bit_shadow_map = true);
/// Destroy a forward pipeline object.
inline void DestroyForwardPipeline(ForwardPipeline &pipeline) { DestroyPipeline(pipeline); }

//
const PipelineInfo &GetForwardPipelineInfo();

//
enum ForwardPipelineStage { FPS_AttributeBuffers, FPS_Basic, FPS_Advanced, FPS_DepthOnly };

void SubmitModelToForwardPipeline(bgfx::ViewId view_id, const Model &mdl, const ForwardPipeline &pipeline, const PipelineProgram &prg, uint32_t prg_variant,
	uint8_t pipeline_config_idx, const Color &ambient, const ForwardPipelineLights &lights, const ForwardPipelineFog &fog, const Mat4 &mtx);

//
enum ForwardPipelineShadowPass { FPSP_Slot0LinearSplit0, FPSP_Slot0LinearSplit1, FPSP_Slot0LinearSplit2, FPSP_Slot0LinearSplit3, FPSP_Slot1Spot, FPSP_Count };

using ForwardPipelineShadowPassViewId = std::array<bgfx::ViewId, FPSP_Count>;

void GenerateLinearShadowMapForForwardPipeline(bgfx::ViewId &view_id, const ViewState &view_state, const std::vector<ModelDisplayList> &display_lists,
	const std::vector<SkinnedModelDisplayList> &skinned_display_lists, const std::vector<Mat4> &mtxs, const ForwardPipelineLights &lights,
	const ForwardPipeline &pipeline, const PipelineResources &resources, ForwardPipelineShadowPassViewId &views, ForwardPipelineShadowData &shadow_data,
	const char *debug_name = nullptr);

void GenerateSpotShadowMapForForwardPipeline(bgfx::ViewId &view_id, const std::vector<ModelDisplayList> &display_lists,
	const std::vector<SkinnedModelDisplayList> &skinned_display_lists, const std::vector<Mat4> &mtxs, const ForwardPipelineLights &lights,
	const ForwardPipeline &pipeline, const PipelineResources &resources, ForwardPipelineShadowPassViewId &view, ForwardPipelineShadowData &shadow_data,
	const char *debug_name = nullptr);

} // namespace hg
