// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/meta.h"
#include "engine/picture.h"
#include "engine/resource_cache.h"

#include "foundation/cext.h"
#include "foundation/color.h"
#include "foundation/data.h"
#include "foundation/frustum.h"
#include "foundation/generational_vector_list.h"
#include "foundation/matrix4.h"
#include "foundation/matrix44.h"
#include "foundation/minmax.h"
#include "foundation/rw_interface.h"
#include "foundation/time.h"
#include "foundation/vector2.h"

#include <bgfx/bgfx.h>
#include <bimg/bimg.h>

#include <deque>
#include <functional>
#include <map>
#include <string>
#include <vector>

#if WIN32
#undef GetObject // stop Windows.h from polluting global namespace
#endif

namespace hg {

class Data;

static constexpr int max_skinned_model_matrix_count = 32; // make sure this stays in sync with bgfx_shader.sh

using bgfxMatrix3 = std::array<float, 9>;
using bgfxMatrix4 = std::array<float, 16>;

bgfxMatrix3 to_bgfx(const Mat3 &m);
bgfxMatrix4 to_bgfx(const Mat4 &m);
bgfxMatrix4 to_bgfx(const Mat44 &m);

#define bgfx_Destroy(V)                                                                                                                                        \
	{                                                                                                                                                          \
		if (bgfx::isValid(V))                                                                                                                                  \
			bgfx::destroy(V);                                                                                                                                  \
		V = BGFX_INVALID_HANDLE;                                                                                                                               \
	}

//
struct ViewState {
	Frustum frustum;
	Mat44 proj;
	Mat4 view;
};

ViewState ComputeOrthographicViewState(const Mat4 &world, float size, float znear, float zfar, const Vec2 &aspect_ratio, const Vec2 &offset = {});
ViewState ComputePerspectiveViewState(const Mat4 &world, float fov, float znear, float zfar, const Vec2 &aspect_ratio, const Vec2 &offset = {});

Mat4 ComputeBillboardMat4(const Vec3 &pos, const ViewState &view_state, const Vec3 &scale = {1, 1, 1});

//
struct Window;

bool RenderInit(Window *window, bgfx::RendererType::Enum type, bgfx::CallbackI *callback = nullptr);
bool RenderInit(Window *window, bgfx::CallbackI *callback = nullptr);

Window *RenderInit(int width, int height, bgfx::RendererType::Enum type, uint32_t reset_flags = 0,
	bgfx::TextureFormat::Enum format = bgfx::TextureFormat::Count, uint32_t debug_flags = 0, bgfx::CallbackI *callback = nullptr);
Window *RenderInit(int width, int height, uint32_t reset_flags = 0, bgfx::TextureFormat::Enum format = bgfx::TextureFormat::Count, uint32_t debug_flags = 0,
	bgfx::CallbackI *callback = nullptr);
Window *RenderInit(const char *window_title, int width, int height, bgfx::RendererType::Enum type, uint32_t reset_flags = 0,
	bgfx::TextureFormat::Enum format = bgfx::TextureFormat::Count, uint32_t debug_flags = 0, bgfx::CallbackI *callback = nullptr);
Window *RenderInit(const char *window_title, int width, int height, uint32_t reset_flags = 0, bgfx::TextureFormat::Enum format = bgfx::TextureFormat::Count,
	uint32_t debug_flags = 0, bgfx::CallbackI *callback = nullptr);

void RenderShutdown();

bool IsRenderUp();

/// Fit the backbuffer to the specified window client area dimensions, return true if resizing was carried out.
bool RenderResetToWindow(Window *win, int &width, int &height, uint32_t reset_flags = 0);

void SetView2D(bgfx::ViewId id, int x, int y, int res_x, int res_y, float znear = -1.f, float zfar = 1.f,
	uint16_t clear_flags = BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, const Color &clear_color = Color::Black, float depth = 1.f, uint8_t stencil = 0,
	bool y_up = false);
void SetViewPerspective(bgfx::ViewId id, int x, int y, int res_x, int res_y, const Mat4 &world, float znear = 0.01f, float zfar = 1000.f,
	float zoom_factor = 1.8, uint16_t clear_flags = BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, const Color &clear_color = Color::Black, float depth = 1.f,
	uint8_t stencil = 0, const Vec2 &aspect_ratio = {});
void SetViewOrthographic(bgfx::ViewId id, int x, int y, int res_x, int res_y, const Mat4 &world, float znear = 0.01f, float zfar = 1000.f, float size = 1.f,
	uint16_t clear_flags = BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, const Color &clear_color = Color::Black, float depth = 1.f, uint8_t stencil = 0,
	const Vec2 &aspect_ratio = {});

//
bgfx::ProgramHandle LoadProgram(const Reader &ir, const ReadProvider &ip, const char *vs_name, const char *fs_name, bool silent = false);
bgfx::ProgramHandle LoadProgram(const Reader &ir, const ReadProvider &ip, const char *name, bool silent = false);
bgfx::ProgramHandle LoadComputeProgram(const Reader &ir, const ReadProvider &ip, const char *cs_name, bool silent = false);

bgfx::ProgramHandle LoadProgramFromFile(const char *vs_path, const char *fs_path, bool silent = false);
bgfx::ProgramHandle LoadProgramFromAssets(const char *vs_name, const char *fs_name, bool silent = false);
bgfx::ProgramHandle LoadProgramFromFile(const char *path, bool silent = false);
bgfx::ProgramHandle LoadProgramFromAssets(const char *name, bool silent = false);
bgfx::ProgramHandle LoadComputeProgramFromFile(const char *cs_path, bool silent = false);
bgfx::ProgramHandle LoadComputeProgramFromAssets(const char *cs_name, bool silent = false);

//
std::vector<bgfx::ShaderHandle> GetProgramShaders(bgfx::ProgramHandle prg_h);

//
json LoadResourceMeta(const Reader &ir, const ReadProvider &ip, const std::string &name);
json LoadResourceMetaFromFile(const std::string &path);
json LoadResourceMetaFromAssets(const std::string &name);

bool SaveResourceMetaToFile(const std::string &path, const json &meta);

//
struct PipelineInfo {
	std::string name;
	std::vector<std::vector<std::string>> configs;
};

struct PipelineResources;

//
enum PipelineProgramFeature {
	OptionalBaseColorOpacityMap, // USE_BASE_COLOR_OPACITY_MAP
	OptionalOcclusionRoughnessMetalnessMap, // USE_OCCLUSION_ROUGHNESS_METALNESS_MAP

	OptionalDiffuseMap, // USE_DIFFUSE_MAP
	OptionalSpecularMap, // USE_SPECULAR_MAP
	OptionalLightMap, // USE_LIGHT_MAP
	OptionalSelfMap, // USE_SELF_MAP
	OptionalOpacityMap, // USE_OPACITY_MAP
	OptionalAmbientMap, // USE_AMBIENT_MAP
	OptionalReflectionMap, // USE_REFLECTION_MAP
	OptionalNormalMap, // USE_NORMAL_MAP

	NormalMapInWorldSpace, // NORMAL_MAP_IN_WORLD_SPACE

	DiffuseUV1, // DIFFUSE_UV_CHANNEL=N
	SpecularUV1, // SPECULAR_UV_CHANNEL=N
	AmbientUV1, // AMBIENT_UV_CHANNEL=N

	OptionalSkinning, // ENABLE_SKINNING
	OptionalAlphaCut, // ENABLE_ALPHA_CUT

	Count,
};

std::vector<PipelineProgramFeature> LoadPipelineProgramFeatures(const Reader &ir, const ReadProvider &ip, const char *name, bool &success, bool silent = false);
std::vector<PipelineProgramFeature> LoadPipelineProgramFeaturesFromFile(const char *path, bool &success, bool silent = false);
std::vector<PipelineProgramFeature> LoadPipelineProgramFeaturesFromAssets(const char *name, bool &success, bool silent = false);

//
struct PipelineProgram;
struct Material;
struct Texture;
struct Model;

using PipelineProgramRef = ResourceRef<PipelineProgram>;
using MaterialRef = ResourceRef<Material>;
using TextureRef = ResourceRef<Texture>;
using ModelRef = ResourceRef<Model>;

static const PipelineProgramRef InvalidPipelineProgramRef;
static const MaterialRef InvalidMaterialRef;
static const TextureRef InvalidTextureRef;
static const ModelRef InvalidModelRef;

//
struct ProgramHandle {
	bgfx::ProgramHandle handle = BGFX_INVALID_HANDLE;
	bool loaded{false};
};

struct TextureUniform {
	bgfx::UniformHandle handle;
	TextureRef tex_ref;
	uint8_t channel{0xff};
};

struct Vec4Uniform {
	bgfx::UniformHandle handle;
	Vec4 value = {1.f, 1.f, 1.f, 1.f};
	bool is_color = false;
};

struct PipelineProgram {
	std::vector<PipelineProgramFeature> features; // features imply associated uniforms
	std::vector<TextureUniform> texture_uniforms; // naked texture uniforms
	std::vector<Vec4Uniform> vec4_uniforms; // naked vec4/color uniforms
	std::vector<ProgramHandle> programs; // program variants (dependent on feature set)

	std::string name;
	PipelineInfo pipeline;
	ReadProvider ip;
	Reader ir;
};

PipelineProgram LoadPipelineProgram(
	const Reader &ir, const ReadProvider &ip, const char *name, PipelineResources &resources, const PipelineInfo &pipeline, bool silent = false);
PipelineProgram LoadPipelineProgramFromFile(const char *path, PipelineResources &resources, const PipelineInfo &pipeline, bool silent = false);
PipelineProgram LoadPipelineProgramFromAssets(const char *name, PipelineResources &resources, const PipelineInfo &pipeline, bool silent = false);

void Destroy(PipelineProgram &pipeline_program);

//
bool LoadPipelineProgramUniforms(const Reader &ir, const ReadProvider &ip, const char *name, std::vector<TextureUniform> &texs, std::vector<Vec4Uniform> &vecs,
	PipelineResources &resources, bool silent = false);
bool LoadPipelineProgramUniformsFromFile(
	const char *path, std::vector<TextureUniform> &texs, std::vector<Vec4Uniform> &vecs, PipelineResources &resources, bool silent = false);
bool LoadPipelineProgramUniformsFromAssets(
	const char *name, std::vector<TextureUniform> &texs, std::vector<Vec4Uniform> &vecs, PipelineResources &resources, bool silent = false);

//
struct DisplayList { // 4B
	bgfx::IndexBufferHandle index_buffer;
	bgfx::VertexBufferHandle vertex_buffer;
	std::vector<uint16_t> bones_table;
};

/// Create an empty texture.
/// @see CreateTextureFromPicture and UpdateTextureFromPicture.
Texture CreateTexture(int width, int height, const char *name, uint64_t flags, bgfx::TextureFormat::Enum format = bgfx::TextureFormat::RGBA8);
/// Create a texture from a picture.
/// @see Picture, CreateTexture and UpdateTextureFromPicture.
Texture CreateTextureFromPicture(const Picture &pic, const char *name, uint64_t flags, bgfx::TextureFormat::Enum format = bgfx::TextureFormat::RGBA8);

void UpdateTextureFromPicture(Texture &tex, const Picture &pic);

uint64_t LoadTextureFlags(const Reader &ir, const ReadProvider &ip, const std::string &name, bool silent = false);
uint64_t LoadTextureFlagsFromFile(const std::string &path, bool silent = false);
uint64_t LoadTextureFlagsFromAssets(const std::string &name, bool silent = false);

Texture LoadTexture(const Reader &ir, const ReadProvider &ip, const char *name, uint64_t flags, bgfx::TextureInfo *info = nullptr,
	bimg::Orientation::Enum *orientation = nullptr, bool silent = false);
Texture LoadTextureFromFile(
	const char *path, uint64_t flags, bgfx::TextureInfo *info = nullptr, bimg::Orientation::Enum *orientation = nullptr, bool silent = false);
Texture LoadTextureFromAssets(
	const char *name, uint64_t flags, bgfx::TextureInfo *info = nullptr, bimg::Orientation::Enum *orientation = nullptr, bool silent = false);

void Destroy(Texture &texture);

struct Texture { // 8B
	uint64_t flags = BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE;
	bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;
};

inline Texture MakeTexture(bgfx::TextureHandle handle, uint64_t flags = BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE) { return {flags, handle}; }

// for engine internal use
struct RenderBufferResourceFactory {
	std::function<bgfx::TextureHandle(bgfx::BackbufferRatio::Enum ratio, bool hasMips, uint16_t numLayers, bgfx::TextureFormat::Enum format, uint64_t flags)>
		create_texture2d;

	std::function<bgfx::FrameBufferHandle(bgfx::BackbufferRatio::Enum ratio, bgfx::TextureFormat::Enum format, uint64_t textureFlags)> create_framebuffer;

	// resources will be created in terms of this custom size
	static RenderBufferResourceFactory Custom(uint16_t width, uint16_t height);

	// resources will be created in terms of the backbuffer's size
	static RenderBufferResourceFactory Backbuffer();
};

//
struct UniformSetValue { // burn it with fire, complete insanity
	UniformSetValue() = default;
	UniformSetValue(const UniformSetValue &v);
	UniformSetValue &operator=(const UniformSetValue &v);
	~UniformSetValue();

	bgfx::UniformHandle uniform = BGFX_INVALID_HANDLE;
	std::vector<float> value;
	uint16_t count{1};
};

UniformSetValue MakeUniformSetValue(const char *name, float v, uint16_t count = 1);
UniformSetValue MakeUniformSetValue(const char *name, const Vec2 &v, uint16_t count = 1);
UniformSetValue MakeUniformSetValue(const char *name, const Vec3 &v, uint16_t count = 1);
UniformSetValue MakeUniformSetValue(const char *name, const Vec4 &v, uint16_t count = 1);
UniformSetValue MakeUniformSetValue(const char *name, const Mat3 &mtx, uint16_t count = 1);
UniformSetValue MakeUniformSetValue(const char *name, const Mat4 &mtx, uint16_t count = 1);
UniformSetValue MakeUniformSetValue(const char *name, const Mat44 &mtx, uint16_t count = 1);

struct UniformSetTexture { // burn it with fire, complete insanity
	UniformSetTexture() = default;
	UniformSetTexture(const UniformSetTexture &v);
	UniformSetTexture &operator=(const UniformSetTexture &v);
	~UniformSetTexture();

	bgfx::UniformHandle uniform = BGFX_INVALID_HANDLE;
	Texture texture;
	uint8_t stage{0};
};

UniformSetTexture MakeUniformSetTexture(const char *name, const Texture &texture, uint8_t stage);

//
struct RenderState {
	uint64_t state{BGFX_STATE_DEFAULT};
	uint32_t rgba{0};
};

//
static const int MF_EnableSkinning = 0x01;
static const int MF_DiffuseUV1 = 0x02;
static const int MF_SpecularUV1 = 0x04;
static const int MF_AmbientUV1 = 0x08;
static const int MF_NormalMapInWorldSpace = 0x10;
static const int MF_EnableAlphaCut = 0x20;

struct Material { // 56B
	PipelineProgramRef program;
	uint32_t variant_idx{0};

	struct Value {
		bgfx::UniformType::Enum type{bgfx::UniformType::Vec4};
		std::vector<float> value;
		uint16_t count{1};

		bgfx::UniformHandle uniform = BGFX_INVALID_HANDLE;
	};

	std::map<std::string, Value> values;

	struct Texture {
		TextureRef texture;
		uint8_t channel{0};

		bgfx::UniformHandle uniform = BGFX_INVALID_HANDLE;
	};

	std::map<std::string, Texture> textures;

	RenderState state;

	uint8_t flags{0};
};

Material CreateMaterial(PipelineProgramRef prg);
Material CreateMaterial(PipelineProgramRef prg, const char *value_name, const Vec4 &value);
Material CreateMaterial(PipelineProgramRef prg, const char *value_name_0, const Vec4 &value_0, const char *value_name_1, const Vec4 &value_1);

void SetMaterialProgram(Material &mat, PipelineProgramRef prg);

void SetMaterialValue(Material &mat, const char *name, float v);
void SetMaterialValue(Material &mat, const char *name, const Vec2 &v);
void SetMaterialValue(Material &mat, const char *name, const Vec3 &v);
void SetMaterialValue(Material &mat, const char *name, const Vec4 &v);
void SetMaterialValue(Material &mat, const char *name, const Mat3 &m);
void SetMaterialValue(Material &mat, const char *name, const Mat4 &m);
void SetMaterialValue(Material &mat, const char *name, const Mat44 &m);

void SetMaterialTexture(Material &mat, const char *name, TextureRef tex, uint8_t stage);
bool SetMaterialTextureRef(Material &mat, const char *name, TextureRef tex);

TextureRef GetMaterialTexture(Material &mat, const char *name);
std::vector<std::string> GetMaterialTextures(Material &mat);

std::vector<std::string> GetMaterialValues(Material &mat);

enum FaceCulling { FC_Disabled, FC_Clockwise, FC_CounterClockwise };

FaceCulling GetMaterialFaceCulling(const Material &mat);
void SetMaterialFaceCulling(Material &mat, FaceCulling mode);

enum DepthTest { DT_Less, DT_LessEqual, DT_Equal, DT_GreaterEqual, DT_Greater, DT_NotEqual, DT_Never, DT_Always, DT_Disabled };

DepthTest GetMaterialDepthTest(const Material &mat);
void SetMaterialDepthTest(Material &mat, DepthTest test);

/// Control the compositing mode used to draw primitives.
enum BlendMode { BM_Additive, BM_Alpha, BM_Darken, BM_Lighten, BM_Multiply, BM_Opaque, BM_Screen, BM_LinearBurn, BM_Undefined };

BlendMode GetMaterialBlendMode(const Material &mat);
void SetMaterialBlendMode(Material &mat, BlendMode mode);

void GetMaterialWriteRGBA(const Material &m_, bool &write_r, bool &write_g, bool &write_b, bool &write_a);
void SetMaterialWriteRGBA(Material &m_, bool write_r, bool write_g, bool write_b, bool write_a);

bool GetMaterialWriteZ(const Material &m);
void SetMaterialWriteZ(Material &m, bool enable);

bool GetMaterialNormalMapInWorldSpace(const Material &m);
void SetMaterialNormalMapInWorldSpace(Material &m, bool enable);

bool GetMaterialDiffuseUsesUV1(const Material &m);
void SetMaterialDiffuseUsesUV1(Material &m, bool enable);
bool GetMaterialSpecularUsesUV1(const Material &m);
void SetMaterialSpecularUsesUV1(Material &m, bool enable);
bool GetMaterialAmbientUsesUV1(const Material &m);
void SetMaterialAmbientUsesUV1(Material &m, bool enable);

bool GetMaterialSkinning(const Material &m);
void SetMaterialSkinning(Material &m, bool enable);
bool GetMaterialAlphaCut(const Material &m);
void SetMaterialAlphaCut(Material &m, bool enable);

/// Compute a render state to control subsequent render calls culling mode, blending mode, Z mask, etc... The same render state can be used by different render calls.
/// @see DrawLines, DrawTriangles and DrawModel.
RenderState ComputeRenderState(BlendMode blend, bool write_z, bool write_r = true, bool write_g = true, bool write_b = true, bool write_a = true);
RenderState ComputeRenderState(BlendMode blend, DepthTest test = DT_Less, FaceCulling culling = FC_Clockwise, bool write_z = true, bool write_r = true,
	bool write_g = true, bool write_b = true, bool write_a = true);

//
bgfx::VertexLayout VertexLayoutPosFloatNormFloat();
bgfx::VertexLayout VertexLayoutPosFloatNormUInt8();
bgfx::VertexLayout VertexLayoutPosFloatColorFloat();
bgfx::VertexLayout VertexLayoutPosFloatColorUInt8();
bgfx::VertexLayout VertexLayoutPosFloatTexCoord0UInt8();
bgfx::VertexLayout VertexLayoutPosFloatNormUInt8TexCoord0UInt8();

//
struct Model { // 96B (+heap)
	std::vector<MinMax> bounds; // minmax/list
	std::vector<DisplayList> lists;
	std::vector<uint16_t> mats; // material/list
	std::vector<Mat4> bind_pose; // bind pose matrices
};

struct ModelInfo {
	bgfx::VertexLayout vs_decl{};
	uint32_t tri_count{};
};

Model LoadModel(const Reader &ir, const Handle &h, const char *name, ModelInfo *info = nullptr, bool silent = false);
Model LoadModelFromFile(const char *path, ModelInfo *info = nullptr, bool silent = false);
Model LoadModelFromAssets(const char *name, ModelInfo *info = nullptr, bool silent = false);

size_t GetModelMaterialCount(const Model &model);

//
void Destroy(Model &model);
void Destroy(Material &material);

//
struct TextureLoad {
	Reader ir;
	ReadProvider ip;
	TextureRef ref;
};

struct ModelLoad {
	Reader ir;
	ReadProvider ip;
	ModelRef ref;
};

struct PipelineResources {
	PipelineResources() : programs(Destroy), textures(Destroy), materials(Destroy), models(Destroy) {}
	~PipelineResources() { DestroyAll(); }

	ResourceCache<PipelineProgram, PipelineProgramRef> programs;
	ResourceCache<Texture, TextureRef> textures;
	ResourceCache<Material, MaterialRef> materials;
	ResourceCache<Model, ModelRef> models;

	std::deque<TextureLoad> texture_loads;
	std::map<gen_ref, bgfx::TextureInfo> texture_infos;

	std::deque<ModelLoad> model_loads;
	std::map<gen_ref, ModelInfo> model_infos;

	void DestroyAll();
};

//
size_t ProcessTextureLoadQueue(PipelineResources &resources, time_ns t_budget = time_from_ms(4), bool silent = false);

TextureRef QueueLoadTexture(const Reader &ir, const ReadProvider &ip, const char *name, uint64_t flags, PipelineResources &resources);
TextureRef QueueLoadTextureFromFile(const char *path, uint64_t flags, PipelineResources &resources);
TextureRef QueueLoadTextureFromAssets(const char *name, uint64_t flags, PipelineResources &resources);

TextureRef SkipLoadOrQueueTextureLoad(
	const Reader &ir, const ReadProvider &ip, const char *path, PipelineResources &resources, bool queue_load, bool do_not_load, bool silent = false);

//
size_t ProcessModelLoadQueue(PipelineResources &resources, time_ns t_budget = time_from_ms(4), bool silent = false);
ModelRef QueueLoadModel(const Reader &ir, const ReadProvider &ip, const char *name, PipelineResources &resources);

ModelRef QueueLoadModelFromFile(const char *path, PipelineResources &resources);
ModelRef QueueLoadModelFromAssets(const char *name, PipelineResources &resources);

ModelRef SkipLoadOrQueueModelLoad(
	const Reader &ir, const ReadProvider &ip, const char *path, PipelineResources &resources, bool queue_load, bool do_not_load, bool silent = false);

//
Material LoadMaterial(const json &js, const Reader &deps_ir, const ReadProvider &deps_ip, PipelineResources &resources, const PipelineInfo &pipeline,
	bool queue_texture_loads, bool do_not_load_resources, bool silent = false);
Material LoadMaterial(const Reader &ir, const Handle &h, const Reader &deps_ir, const ReadProvider &deps_ip, PipelineResources &resources,
	const PipelineInfo &pipeline, bool queue_texture_loads, bool do_not_load_resources, bool silent = false);
Material LoadMaterialFromFile(
	const char *path, PipelineResources &resources, const PipelineInfo &pipeline, bool queue_texture_loads, bool do_not_load_resources, bool silent = false);
Material LoadMaterialFromAssets(
	const char *path, PipelineResources &resources, const PipelineInfo &pipeline, bool queue_texture_loads, bool do_not_load_resources, bool silent = false);

bool SaveMaterial(const Material &mat, json &js, const PipelineResources &resources);
bool SaveMaterial(const Material &mat, const Writer &iw, const Handle &h, const PipelineResources &resources);
bool SaveMaterialToFile(const char *path, const Material &m, const PipelineResources &resources);

//
PipelineProgramRef LoadPipelineProgramRef(
	const Reader &ir, const ReadProvider &ip, const char *name, PipelineResources &resources, const PipelineInfo &pipeline, bool silent = false);
PipelineProgramRef LoadPipelineProgramRefFromFile(const char *path, PipelineResources &resources, const PipelineInfo &pipeline, bool silent = false);
PipelineProgramRef LoadPipelineProgramRefFromAssets(const char *name, PipelineResources &resources, const PipelineInfo &pipeline, bool silent = false);

ModelRef LoadModel(const Reader &ir, const ReadProvider &ip, const char *path, PipelineResources &resources, bool silent = false);
ModelRef LoadModelFromFile(const char *path, PipelineResources &resources, bool silent = false);
ModelRef LoadModelFromAssets(const char *path, PipelineResources &resources, bool silent = false);

struct TextureMeta {
	uint64_t flags{};
};

TextureMeta LoadTextureMeta(const Reader &ir, const ReadProvider &ip, const std::string &name, bool silent = false);
TextureMeta LoadTextureMetaFromFile(const std::string &path, bool silent = false);
TextureMeta LoadTextureMetaFromAssets(const std::string &name, bool silent = false);

TextureRef LoadTexture(const Reader &ir, const ReadProvider &ip, const char *path, uint64_t flags, PipelineResources &resources, bool silent = false);
TextureRef LoadTextureFromFile(const char *path, uint64_t flags, PipelineResources &resources, bool silent = false);
TextureRef LoadTextureFromAssets(const char *path, uint64_t flags, PipelineResources &resources, bool silent = false);

/// Capture a texture content to a Picture. Return the frame counter at which the capture will be complete.
/// A Picture object can be accessed by the CPU.
/// This function is asynchronous and its result will not be available until the returned frame counter is equal or greater to the frame counter returned by Frame.
uint32_t CaptureTexture(const PipelineResources &resources, const TextureRef &t, Picture &pic);

MaterialRef LoadMaterialRef(const Reader &ir, const Handle &h, const char *path, const Reader &deps_ir, const ReadProvider &deps_ip,
	PipelineResources &resources, const PipelineInfo &pipeline, bool queue_texture_loads, bool do_not_load_resources, bool silent = false);
MaterialRef LoadMaterialRefFromFile(
	const char *path, PipelineResources &resources, const PipelineInfo &pipeline, bool queue_texture_loads, bool do_not_load_resources, bool silent = false);
MaterialRef LoadMaterialRefFromAssets(
	const char *path, PipelineResources &resources, const PipelineInfo &pipeline, bool queue_texture_loads, bool do_not_load_resources, bool silent = false);

//
size_t GetQueuedResourceCount(const PipelineResources &res);
size_t ProcessLoadQueues(PipelineResources &res, time_ns t_budget = time_from_ms(4), bool silent = false);

//
std::vector<int> GetMaterialPipelineProgramFeatureStates(const Material &mat, const std::vector<PipelineProgramFeature> &features);
std::string GetPipelineProgramVariantName(const char *name, const std::vector<PipelineProgramFeature> &features, const std::vector<int> &states);

int GetPipelineProgramFeatureStateCount(PipelineProgramFeature feat);

int GetPipelineProgramVariantCount(const std::vector<PipelineProgramFeature> &feats);
int GetPipelineProgramVariantIndex(const std::vector<PipelineProgramFeature> &feats, const std::vector<int> &states);

void UpdateMaterialPipelineProgramVariant(Material &mat, const PipelineResources &resources);

void CreateMissingMaterialProgramValues(Material &mat, PipelineResources &resources, const Reader &ir, const ReadProvider &ip);
void CreateMissingMaterialProgramValuesFromFile(Material &mat, PipelineResources &resources);
void CreateMissingMaterialProgramValuesFromAssets(Material &mat, PipelineResources &resources);

/// Compute a sorting key to control the rendering order of a display list, `view_depth` is expected in view space.
uint32_t ComputeSortKey(float view_depth);

/// Compute a sorting key to control the rendering order of a display list.
uint32_t ComputeSortKeyFromWorld(const Vec3 &T, const Mat4 &view);
uint32_t ComputeSortKeyFromWorld(const Vec3 &T, const Mat4 &view, const Mat4 &model);

//
void DrawDisplayList(bgfx::ViewId view_id, bgfx::IndexBufferHandle idx, bgfx::VertexBufferHandle vtx, bgfx::ProgramHandle prg,
	const std::vector<UniformSetValue> &values = {}, const std::vector<UniformSetTexture> &textures = {}, RenderState state = {}, uint32_t depth = 0);

/// Draw a model to the specified view.
/// @see UniformSetValueList and UniformSetTextureList to pass uniform values to the shader program.
void DrawModel(bgfx::ViewId view_id, const Model &mdl, bgfx::ProgramHandle prg, const std::vector<UniformSetValue> &values,
	const std::vector<UniformSetTexture> &textures, const Mat4 *mtxs, size_t mtx_count = 1, RenderState state = {}, uint32_t depth = 0);

//
struct ModelDisplayList { // 16B
	const Material *mat; // 8
	uint32_t mtx_idx; // 4
	uint16_t mdl_idx; // 2
	uint16_t lst_idx; // 2
};

void CullModelDisplayLists(const Frustum &frustum, std::vector<ModelDisplayList> &display_lists, const std::vector<Mat4> &mtxs, const PipelineResources &res);

void DrawModelDisplayLists(bgfx::ViewId view_id, const std::vector<ModelDisplayList> &display_lists, uint8_t pipeline_config_idx,
	const std::vector<UniformSetValue> &values, const std::vector<UniformSetTexture> &textures, const std::vector<Mat4> &mtxs, const PipelineResources &res);
void DrawModelDisplayLists(bgfx::ViewId view_id, const std::vector<ModelDisplayList> &display_lists, const std::vector<uint32_t> &depths,
	uint8_t pipeline_config_idx, const std::vector<UniformSetValue> &values, const std::vector<UniformSetTexture> &textures, const std::vector<Mat4> &mtxs,
	const PipelineResources &res);
void DrawModelDisplayLists(bgfx::ViewId view_id, const std::vector<ModelDisplayList> &display_lists, uint8_t pipeline_config_idx,
	const std::vector<UniformSetValue> &values, const std::vector<UniformSetTexture> &textures, const std::vector<Mat4> &mtxs,
	const std::vector<Mat4> &prv_mtxs, const PipelineResources &res);

//
struct SkinnedModelDisplayList { // 782B
	const Material *mat; // 8
	uint32_t mtx_idxs[max_skinned_model_matrix_count]; // 384
	uint32_t bones_idxs[max_skinned_model_matrix_count]; // 384
	uint16_t bone_count; // 2
	uint16_t mdl_idx; // 2
	uint16_t lst_idx; // 2
};

void DrawSkinnedModelDisplayLists(bgfx::ViewId view_id, const std::vector<SkinnedModelDisplayList> &display_lists, uint8_t pipeline_config_idx,
	const std::vector<UniformSetValue> &values, const std::vector<UniformSetTexture> &textures, const std::vector<Mat4> &mtxs, const PipelineResources &res);
void DrawSkinnedModelDisplayLists(bgfx::ViewId view_id, const std::vector<SkinnedModelDisplayList> &display_lists, const std::vector<uint32_t> &depths,
	uint8_t pipeline_config_idx, const std::vector<UniformSetValue> &values, const std::vector<UniformSetTexture> &textures, const std::vector<Mat4> &mtxs,
	const PipelineResources &res);
void DrawSkinnedModelDisplayLists(bgfx::ViewId view_id, const std::vector<SkinnedModelDisplayList> &display_lists, uint8_t pipeline_config_idx,
	const std::vector<UniformSetValue> &values, const std::vector<UniformSetTexture> &textures, const std::vector<Mat4> &mtxs,
	const std::vector<Mat4> &prv_mtxs, const PipelineResources &res);

//
using Indices = std::vector<uint16_t>;

struct Vertices {
	Vertices(const bgfx::VertexLayout &decl, size_t count);

	const bgfx::VertexLayout &GetDecl() const { return decl; }

	Vertices &Begin(size_t i);
	Vertices &SetPos(const Vec3 &pos);
	Vertices &SetNormal(const Vec3 &normal);
	Vertices &SetTangent(const Vec3 &tangent);
	Vertices &SetBinormal(const Vec3 &binormal);
	Vertices &SetTexCoord0(const Vec2 &uv);
	Vertices &SetTexCoord1(const Vec2 &uv);
	Vertices &SetTexCoord2(const Vec2 &uv);
	Vertices &SetTexCoord3(const Vec2 &uv);
	Vertices &SetTexCoord4(const Vec2 &uv);
	Vertices &SetTexCoord5(const Vec2 &uv);
	Vertices &SetTexCoord6(const Vec2 &uv);
	Vertices &SetTexCoord7(const Vec2 &uv);
	Vertices &SetColor0(const Color &color);
	Vertices &SetColor1(const Color &color);
	Vertices &SetColor2(const Color &color);
	Vertices &SetColor3(const Color &color);
	void End(bool validate = false);

	void Clear();

	void Reserve(size_t count);
	void Resize(size_t count);

	const void *GetData() const { return data.data(); }

	size_t GetSize() const { return data.size(); }
	size_t GetCount() const { return data.size() / decl.getStride(); }
	size_t GetCapacity() const { return data.capacity() / decl.getStride(); }

private:
	bgfx::VertexLayout decl;
	std::vector<char> data;

	int idx{-1};
	uint32_t vtx_attr_flag{};
};

//
void SetTransform(const Mat4 &world);
void SetUniforms(const std::vector<UniformSetValue> &values, const std::vector<UniformSetTexture> &textures);

/// Draw a list of lines to the specified view.
/// @see UniformSetValueList and UniformSetTextureList to pass uniform values to the shader program.
void DrawLines(bgfx::ViewId view_id, const Vertices &vtx, bgfx::ProgramHandle prg, RenderState = {}, uint32_t depth = 0);
void DrawLines(bgfx::ViewId view_id, const Vertices &vtx, bgfx::ProgramHandle prg, const std::vector<UniformSetValue> &values,
	const std::vector<UniformSetTexture> &textures, RenderState = {}, uint32_t depth = 0);
void DrawLines(bgfx::ViewId view_id, const Indices &idx, const Vertices &vtx, bgfx::ProgramHandle prg, const std::vector<UniformSetValue> &values,
	const std::vector<UniformSetTexture> &textures, RenderState = {}, uint32_t depth = 0);
/*!
	Draw a list of sprites to the specified view.
	@see UniformSetValueList and UniformSetTextureList to pass uniform values to the shader program.
	@note This function prepares the sprite on the CPU before submitting them all to the GPU as a single draw call.
*/
void DrawSprites(bgfx::ViewId view_id, const Mat3 &inv_view_R, bgfx::VertexLayout &decl, const std::vector<Vec3> &pos, const Vec2 &size,
	bgfx::ProgramHandle prg, RenderState state = {}, uint32_t depth = 0);

/// Draw a list of triangles to the specified view.
/// @see UniformSetValueList and UniformSetTextureList to pass uniform values to the shader program.
void DrawTriangles(bgfx::ViewId view_id, const Vertices &vtx, bgfx::ProgramHandle prg, RenderState = {}, uint32_t depth = 0);
void DrawTriangles(bgfx::ViewId view_id, const Vertices &vtx, bgfx::ProgramHandle prg, const std::vector<UniformSetValue> &values,
	const std::vector<UniformSetTexture> &textures, RenderState = {}, uint32_t depth = 0);
void DrawTriangles(bgfx::ViewId view_id, const Indices &idx, const Vertices &vtx, bgfx::ProgramHandle prg, const std::vector<UniformSetValue> &values,
	const std::vector<UniformSetTexture> &textures, RenderState = {}, uint32_t depth = 0);
void DrawSprites(bgfx::ViewId view_id, const Mat3 &inv_view_R, bgfx::VertexLayout &decl, const std::vector<Vec3> &pos, const Vec2 &size,
	bgfx::ProgramHandle prg, const std::vector<UniformSetValue> &values, const std::vector<UniformSetTexture> &textures, RenderState state = {},
	uint32_t depth = 0);

//
static const float default_shadow_bias = 0.0001f;
static const Vec4 default_pssm_split = {10.f, 50.f, 100.f, 500.f};

//
struct Pipeline {
	std::map<std::string, bgfx::TextureHandle> textures;
	std::map<std::string, bgfx::FrameBufferHandle> framebuffers;

	std::vector<UniformSetValue> uniform_values;
	std::vector<UniformSetTexture> uniform_textures;
};

void DestroyPipeline(Pipeline &pipeline);

//
struct FrameBuffer {
	bgfx::FrameBufferHandle handle = BGFX_INVALID_HANDLE;
};

FrameBuffer CreateFrameBuffer(const Texture &color, const Texture &depth, const char *name);
FrameBuffer CreateFrameBuffer(bgfx::TextureFormat::Enum color_format, bgfx::TextureFormat::Enum depth_format, int aa, const char *name);
FrameBuffer CreateFrameBuffer(int width, int height, bgfx::TextureFormat::Enum color_format, bgfx::TextureFormat::Enum depth_format, int aa, const char *name);

Texture GetColorTexture(FrameBuffer &frameBuffer);
Texture GetDepthTexture(FrameBuffer &frameBuffer);

/// Destroy a frame buffer and its resources.
void DestroyFrameBuffer(FrameBuffer &frameBuffer);

bool CreateFullscreenQuad(bgfx::TransientIndexBuffer &idx, bgfx::TransientVertexBuffer &vtx);

bimg::ImageContainer *LoadImage(const Reader &ir, const ReadProvider &ip, const char *name);
bimg::ImageContainer *LoadImageFromFile(const char *name);
bimg::ImageContainer *LoadImageFromAssets(const char *name);
void UpdateTextureFromImage(Texture &tex, bimg::ImageContainer *img, bool auto_delete = true);

} // namespace hg
