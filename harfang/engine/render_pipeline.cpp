// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/render_pipeline.h"
#include "engine/assets_rw_interface.h"
#include "engine/file_format.h"
#include "engine/meta.h"

#include "foundation/file.h"
#include "foundation/file_rw_interface.h"
#include "foundation/format.h"
#include "foundation/kv_store.h"
#include "foundation/log.h"
#include "foundation/matrix3.h"
#include "foundation/matrix4.h"
#include "foundation/matrix44.h"
#include "foundation/path_tools.h"
#include "foundation/profiler.h"
#include "foundation/projection.h"
#include "foundation/time.h"

#include "platform/window_system.h"

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bimg/decode.h>
#include <bx/allocator.h>
#include <bx/bx.h>

#include <json.hpp>

#include <set>

using json = nlohmann::json;

namespace bgfx {

NLOHMANN_JSON_SERIALIZE_ENUM(UniformType::Enum, {
													{UniformType::Enum::Sampler, "sampler"},
													{UniformType::Enum::End, "end"},
													{UniformType::Enum::Vec4, "vec4"},
													{UniformType::Enum::Mat3, "mat3"},
													{UniformType::Enum::Mat4, "mat4"},
												});

void getTextureSizeFromRatio(BackbufferRatio::Enum _ratio, uint16_t &_width, uint16_t &_height);
} // namespace bgfx

namespace hg {

static bool bgfx_is_up = false;

static bgfx::UniformHandle u_previous_model = BGFX_INVALID_HANDLE;

bgfxMatrix4 to_bgfx(const Mat44 &m) {
	return {m.m[0][0], m.m[1][0], m.m[2][0], m.m[3][0], m.m[0][1], m.m[1][1], m.m[2][1], m.m[3][1], m.m[0][2], m.m[1][2], m.m[2][2], m.m[3][2], m.m[0][3],
		m.m[1][3], m.m[2][3], m.m[3][3]};
}

bgfxMatrix4 to_bgfx(const Mat4 &m) {
	return {
		m.m[0][0], m.m[1][0], m.m[2][0], 0.f, m.m[0][1], m.m[1][1], m.m[2][1], 0.f, m.m[0][2], m.m[1][2], m.m[2][2], 0.f, m.m[0][3], m.m[1][3], m.m[2][3], 1.f};
}

bgfxMatrix3 to_bgfx(const Mat3 &m) { return {m.m[0][0], m.m[1][0], m.m[2][0], m.m[0][1], m.m[1][1], m.m[2][1], m.m[0][2], m.m[1][2], m.m[2][2]}; }

//
ViewState ComputeOrthographicViewState(const Mat4 &world, float size, float znear, float zfar, const Vec2 &aspect_ratio, const Vec2 &offset) {
	const bgfx::Caps *caps = bgfx::getCaps();
	const auto view = InverseFast(world);
	const auto proj = ComputeOrthographicProjectionMatrix(znear, zfar, size, aspect_ratio, offset);
	const auto frustum = MakeFrustum(proj, world);
	return {frustum, proj, view};
}

ViewState ComputePerspectiveViewState(const Mat4 &world, float fov, float znear, float zfar, const Vec2 &aspect_ratio, const Vec2 &offset) {
	const bgfx::Caps *caps = bgfx::getCaps();
	const auto view = InverseFast(world);
	const auto proj = ComputePerspectiveProjectionMatrix(znear, zfar, FovToZoomFactor(fov), aspect_ratio, offset);
	const auto frustum = MakeFrustum(proj, world);
	return {frustum, proj, view};
}

//
Mat4 ComputeBillboardMat4(const Vec3 &pos, const ViewState &view_state, const Vec3 &scale) {
	return ComputeBillboardMat4(pos, Transpose(GetRotationMatrix(view_state.view)), scale);
}

//
uint32_t ComputeSortKey(float view_depth) {
	view_depth += 100.f; // sort up to 100 meters behind view origin
	if (view_depth < 0.f)
		view_depth = 0.f;
	return uint32_t(view_depth * 1000.f); // 1mm precision
}

uint32_t ComputeSortKeyFromWorld(const Vec3 &T, const Mat4 &view) { return ComputeSortKey((view * T).z); }
uint32_t ComputeSortKeyFromWorld(const Vec3 &T, const Mat4 &view, const Mat4 &model) { return ComputeSortKey(((view * model) * T).z); }

//
UniformSetValue::UniformSetValue(const UniformSetValue &v) { *this = v; }

UniformSetValue &UniformSetValue::operator=(const UniformSetValue &v) {
	if (bgfx::isValid(uniform))
		bgfx::destroy(uniform);

	if (bgfx::isValid(v.uniform)) {
		bgfx::UniformInfo info;
		bgfx::getUniformInfo(v.uniform, info);

		uniform = bgfx::createUniform(info.name, info.type, info.num);
		value = v.value;
		count = v.count;
	} else {
		uniform = BGFX_INVALID_HANDLE;
		value.clear();
		count = 0;
	}
	return *this;
}

UniformSetValue::~UniformSetValue() {
	if (IsRenderUp() && bgfx::isValid(uniform))
		bgfx::destroy(uniform);
}

//
UniformSetTexture::UniformSetTexture(const UniformSetTexture &v) { *this = v; }

UniformSetTexture &UniformSetTexture::operator=(const UniformSetTexture &v) {
	if (bgfx::isValid(uniform))
		bgfx::destroy(uniform);

	if (bgfx::isValid(v.uniform)) {
		bgfx::UniformInfo info;
		bgfx::getUniformInfo(v.uniform, info);

		uniform = bgfx::createUniform(info.name, info.type, info.num);
		texture = v.texture;
		stage = v.stage;
	} else {
		uniform = BGFX_INVALID_HANDLE;
		texture = {};
		stage = 0;
	}
	return *this;
}

UniformSetTexture::~UniformSetTexture() {
	if (IsRenderUp() && bgfx::isValid(uniform))
		bgfx::destroy(uniform);
}

//
void SetMaterialProgram(Material &mat, PipelineProgramRef prg) { mat.program = prg; }

void SetMaterialValue(Material &mat, const char *name, float val) {
	const auto &i = mat.values.find(name);

	if (i == std::end(mat.values)) {
		auto &v = mat.values[name];
		v.type = bgfx::UniformType::Vec4;
		v.uniform = bgfx::createUniform(name, v.type);
		v.value = {val, 0, 0, 0};
		v.count = 1;
	} else {
		i->second.value = {val, 0, 0, 0};
	}
}

void SetMaterialValue(Material &mat, const char *name, const Vec2 &val) {
	const auto &i = mat.values.find(name);

	if (i == std::end(mat.values)) {
		auto &v = mat.values[name];
		v.type = bgfx::UniformType::Vec4;
		v.uniform = bgfx::createUniform(name, v.type);
		v.value = {val.x, val.y, 0, 0};
		v.count = 1;
	} else {
		i->second.value = {val.x, val.y, 0, 0};
	}
}

void SetMaterialValue(Material &mat, const char *name, const Vec3 &val) {
	const auto &i = mat.values.find(name);

	if (i == std::end(mat.values)) {
		auto &v = mat.values[name];
		v.type = bgfx::UniformType::Vec4;
		v.uniform = bgfx::createUniform(name, v.type);
		v.value = {val.x, val.y, val.z, 0};
		v.count = 1;
	} else {
		i->second.value = {val.x, val.y, val.z, 0};
	}
}

void SetMaterialValue(Material &mat, const char *name, const Vec4 &val) {
	const auto &i = mat.values.find(name);

	if (i == std::end(mat.values)) {
		auto &v = mat.values[name];
		v.type = bgfx::UniformType::Vec4;
		v.uniform = bgfx::createUniform(name, v.type);
		v.value = {val.x, val.y, val.z, val.w};
		v.count = 1;
	} else {
		i->second.value = {val.x, val.y, val.z, val.w};
	}
}

void SetMaterialValue(Material &mat, const char *name, const Mat3 &m) {
	const auto &i = mat.values.find(name);
	const auto m_ = to_bgfx(m);

	if (i == std::end(mat.values)) {
		auto &v = mat.values[name];
		v.type = bgfx::UniformType::Mat3;
		v.uniform = bgfx::createUniform(name, v.type);
		std::copy(std::begin(m_), std::end(m_), std::back_inserter(v.value));
		v.count = 1;
	} else {
		i->second.value.clear();
		std::copy(std::begin(m_), std::end(m_), std::back_inserter(i->second.value));
	}
}

void SetMaterialValue(Material &mat, const char *name, const Mat4 &m) {
	const auto &i = mat.values.find(name);
	const auto m_ = to_bgfx(m);

	if (i == std::end(mat.values)) {
		auto &v = mat.values[name];
		v.type = bgfx::UniformType::Mat4;
		v.uniform = bgfx::createUniform(name, v.type);
		std::copy(std::begin(m_), std::end(m_), std::back_inserter(v.value));
		v.count = 1;
	} else {
		i->second.value.clear();
		std::copy(std::begin(m_), std::end(m_), std::back_inserter(i->second.value));
	}
}

void SetMaterialValue(Material &mat, const char *name, const Mat44 &m) {
	const auto &i = mat.values.find(name);
	const auto m_ = to_bgfx(m);

	if (i == std::end(mat.values)) {
		auto &v = mat.values[name];
		v.type = bgfx::UniformType::Mat4;
		v.uniform = bgfx::createUniform(name, v.type);
		std::copy(std::begin(m_), std::end(m_), std::back_inserter(v.value));
		v.count = 1;
	} else {
		i->second.value.clear();
		std::copy(std::begin(m_), std::end(m_), std::back_inserter(i->second.value));
	}
}

void SetMaterialTexture(Material &mat, const char *name, TextureRef tex, uint8_t stage) {
	const auto &i = mat.textures.find(name);

	if (i == std::end(mat.textures)) {
		auto &v = mat.textures[name];
		v.uniform = bgfx::createUniform(name, bgfx::UniformType::Sampler);
		v.texture = tex;
		v.channel = stage;
	} else {
		auto &v = i->second;
		v.texture = tex;
		v.channel = stage;
	}
}

bool SetMaterialTextureRef(Material &mat, const char *name, TextureRef tex) {
	const auto &i = mat.textures.find(name);

	if (i == std::end(mat.textures))
		return false;

	i->second.texture = tex;
	return true;
}

TextureRef GetMaterialTexture(Material &mat, const char *name) {
	if (mat.textures.find(name) != std::end(mat.textures))
		return mat.textures[name].texture;
	return InvalidTextureRef;
}

std::vector<std::string> GetMaterialTextures(Material &mat) {
	std::vector<std::string> names;
	for (auto const &texture : mat.textures)
		names.push_back(texture.first);
	return names;
}

std::vector<std::string> GetMaterialValues(Material &mat) {
	std::vector<std::string> names;
	for (auto const &value : mat.values)
		names.push_back(value.first);
	return names;
}

//
void GetMaterialWriteRGBA(const Material &m, bool &write_r, bool &write_g, bool &write_b, bool &write_a) {
	write_r = m.state.state & BGFX_STATE_WRITE_R;
	write_g = m.state.state & BGFX_STATE_WRITE_G;
	write_b = m.state.state & BGFX_STATE_WRITE_B;
	write_a = m.state.state & BGFX_STATE_WRITE_A;
}

bool GetMaterialWriteZ(const Material &m) { return m.state.state & BGFX_STATE_WRITE_Z; }

//
void SetMaterialWriteRGBA(Material &m, bool write_r, bool write_g, bool write_b, bool write_a) {
	m.state.state &= ~(BGFX_STATE_WRITE_R | BGFX_STATE_WRITE_G | BGFX_STATE_WRITE_B | BGFX_STATE_WRITE_A);
	m.state.state |=
		(write_r ? BGFX_STATE_WRITE_R : 0) | (write_g ? BGFX_STATE_WRITE_G : 0) | (write_b ? BGFX_STATE_WRITE_B : 0) | (write_a ? BGFX_STATE_WRITE_A : 0);
}

void SetMaterialWriteZ(Material &m, bool enable) {
	if (enable)
		m.state.state |= BGFX_STATE_WRITE_Z;
	else
		m.state.state &= ~BGFX_STATE_WRITE_Z;
}

//
FaceCulling GetMaterialFaceCulling(const Material &mat) {
	const auto cull = mat.state.state & BGFX_STATE_CULL_MASK;

	if (cull == BGFX_STATE_CULL_CW)
		return FC_Clockwise;
	if (cull == BGFX_STATE_CULL_CCW)
		return FC_CounterClockwise;

	return FC_Disabled;
}

void SetMaterialFaceCulling(Material &mat, FaceCulling mode) {
	mat.state.state &= ~BGFX_STATE_CULL_MASK;

	if (mode == FC_Clockwise)
		mat.state.state |= BGFX_STATE_CULL_CW;
	else if (mode == FC_CounterClockwise)
		mat.state.state |= BGFX_STATE_CULL_CCW;
}

DepthTest GetMaterialDepthTest(const Material &mat) {
	const auto depth_test = mat.state.state & BGFX_STATE_DEPTH_TEST_MASK;

	if (depth_test == BGFX_STATE_DEPTH_TEST_LESS)
		return DT_Less;
	if (depth_test == BGFX_STATE_DEPTH_TEST_LEQUAL)
		return DT_LessEqual;
	if (depth_test == BGFX_STATE_DEPTH_TEST_EQUAL)
		return DT_Equal;
	if (depth_test == BGFX_STATE_DEPTH_TEST_GEQUAL)
		return DT_GreaterEqual;
	if (depth_test == BGFX_STATE_DEPTH_TEST_GREATER)
		return DT_Greater;
	if (depth_test == BGFX_STATE_DEPTH_TEST_NOTEQUAL)
		return DT_NotEqual;
	if (depth_test == BGFX_STATE_DEPTH_TEST_NEVER)
		return DT_Never;
	if (depth_test == BGFX_STATE_DEPTH_TEST_ALWAYS)
		return DT_Always;

	return DT_Disabled;
}

void SetMaterialDepthTest(Material &mat, DepthTest test) {
	mat.state.state &= ~BGFX_STATE_DEPTH_TEST_MASK;

	if (test == DT_Less)
		mat.state.state |= BGFX_STATE_DEPTH_TEST_LESS;
	else if (test == DT_LessEqual)
		mat.state.state |= BGFX_STATE_DEPTH_TEST_LEQUAL;
	else if (test == DT_Equal)
		mat.state.state |= BGFX_STATE_DEPTH_TEST_EQUAL;
	else if (test == DT_GreaterEqual)
		mat.state.state |= BGFX_STATE_DEPTH_TEST_GEQUAL;
	else if (test == DT_Greater)
		mat.state.state |= BGFX_STATE_DEPTH_TEST_GREATER;
	else if (test == DT_NotEqual)
		mat.state.state |= BGFX_STATE_DEPTH_TEST_NOTEQUAL;
	else if (test == DT_Never)
		mat.state.state |= BGFX_STATE_DEPTH_TEST_NEVER;
	else if (test == DT_Always)
		mat.state.state |= BGFX_STATE_DEPTH_TEST_ALWAYS;
}

BlendMode GetMaterialBlendMode(const Material &mat) {
	const auto blend_state = mat.state.state & (BGFX_STATE_BLEND_MASK | BGFX_STATE_BLEND_EQUATION_MASK);

	if (blend_state == BGFX_STATE_BLEND_ADD)
		return BM_Additive;
	if (blend_state == BGFX_STATE_BLEND_ALPHA)
		return BM_Alpha;
	if (blend_state == BGFX_STATE_BLEND_DARKEN)
		return BM_Darken;
	if (blend_state == BGFX_STATE_BLEND_LIGHTEN)
		return BM_Lighten;
	if (blend_state == BGFX_STATE_BLEND_MULTIPLY)
		return BM_Multiply;
	if (blend_state == BGFX_STATE_BLEND_NORMAL)
		return BM_Opaque;
	if (blend_state == BGFX_STATE_BLEND_SCREEN)
		return BM_Screen;
	if (blend_state == BGFX_STATE_BLEND_LINEAR_BURN)
		return BM_LinearBurn;

	return BM_Opaque;
}

void SetMaterialBlendMode(Material &mat, BlendMode mode) {
	mat.state.state &= ~(BGFX_STATE_BLEND_MASK | BGFX_STATE_BLEND_EQUATION_MASK);

	if (mode == BM_Additive)
		mat.state.state |= BGFX_STATE_BLEND_ADD;
	else if (mode == BM_Alpha)
		mat.state.state |= BGFX_STATE_BLEND_ALPHA;
	else if (mode == BM_Darken)
		mat.state.state |= BGFX_STATE_BLEND_DARKEN;
	else if (mode == BM_Lighten)
		mat.state.state |= BGFX_STATE_BLEND_LIGHTEN;
	else if (mode == BM_Multiply)
		mat.state.state |= BGFX_STATE_BLEND_MULTIPLY;
	//	else if (mode == BM_Opaque)
	//		mat.state.state |= BGFX_STATE_BLEND_NORMAL;
	else if (mode == BM_Screen)
		mat.state.state |= BGFX_STATE_BLEND_SCREEN;
	else if (mode == BM_LinearBurn)
		mat.state.state |= BGFX_STATE_BLEND_LINEAR_BURN;
}

bool GetMaterialNormalMapInWorldSpace(const Material &m) { return m.flags & MF_NormalMapInWorldSpace; }
void SetMaterialNormalMapInWorldSpace(Material &m, bool enable) {
	if (enable)
		m.flags |= MF_NormalMapInWorldSpace;
	else
		m.flags &= ~MF_NormalMapInWorldSpace;
}

bool GetMaterialDiffuseUsesUV1(const Material &m) { return m.flags & MF_DiffuseUV1; }
void SetMaterialDiffuseUsesUV1(Material &m, bool enable) {
	if (enable)
		m.flags |= MF_DiffuseUV1;
	else
		m.flags &= ~MF_DiffuseUV1;
}

bool GetMaterialSpecularUsesUV1(const Material &m) { return m.flags & MF_SpecularUV1; }
void SetMaterialSpecularUsesUV1(Material &m, bool enable) {
	if (enable)
		m.flags |= MF_SpecularUV1;
	else
		m.flags &= ~MF_SpecularUV1;
}

bool GetMaterialAmbientUsesUV1(const Material &m) { return m.flags & MF_AmbientUV1; }
void SetMaterialAmbientUsesUV1(Material &m, bool enable) {
	if (enable)
		m.flags |= MF_AmbientUV1;
	else
		m.flags &= ~MF_AmbientUV1;
}

bool GetMaterialSkinning(const Material &m) { return m.flags & MF_EnableSkinning; }
void SetMaterialSkinning(Material &m, bool enable) {
	if (enable)
		m.flags |= MF_EnableSkinning;
	else
		m.flags &= ~MF_EnableSkinning;
}

bool GetMaterialAlphaCut(const Material &m) { return m.flags & MF_EnableAlphaCut; }
void SetMaterialAlphaCut(Material &m, bool enable) {
	if (enable)
		m.flags |= MF_EnableAlphaCut;
	else
		m.flags &= ~MF_EnableAlphaCut;
}

//
RenderState ComputeRenderState(BlendMode blend, DepthTest test, FaceCulling culling, bool write_z, bool write_r, bool write_g, bool write_b, bool write_a) {
	RenderState state{BGFX_STATE_MSAA, 0};

	if (write_r)
		state.state |= BGFX_STATE_WRITE_R;
	if (write_g)
		state.state |= BGFX_STATE_WRITE_G;
	if (write_b)
		state.state |= BGFX_STATE_WRITE_B;
	if (write_a)
		state.state |= BGFX_STATE_WRITE_A;

	if (write_z)
		state.state |= BGFX_STATE_WRITE_Z;

	if (culling == FC_Clockwise)
		state.state |= BGFX_STATE_CULL_CW;
	else if (culling == FC_CounterClockwise)
		state.state |= BGFX_STATE_CULL_CCW;

	if (test == DT_Less)
		state.state |= BGFX_STATE_DEPTH_TEST_LESS;
	else if (test == DT_LessEqual)
		state.state |= BGFX_STATE_DEPTH_TEST_LEQUAL;
	else if (test == DT_Equal)
		state.state |= BGFX_STATE_DEPTH_TEST_EQUAL;
	else if (test == DT_GreaterEqual)
		state.state |= BGFX_STATE_DEPTH_TEST_GEQUAL;
	else if (test == DT_Greater)
		state.state |= BGFX_STATE_DEPTH_TEST_GREATER;
	else if (test == DT_NotEqual)
		state.state |= BGFX_STATE_DEPTH_TEST_NOTEQUAL;
	else if (test == DT_Never)
		state.state |= BGFX_STATE_DEPTH_TEST_NEVER;
	else if (test == DT_Always)
		state.state |= BGFX_STATE_DEPTH_TEST_ALWAYS;

	if (blend == BM_Additive)
		state.state |= BGFX_STATE_BLEND_ADD;
	else if (blend == BM_Alpha)
		state.state |= BGFX_STATE_BLEND_ALPHA;
	else if (blend == BM_Darken)
		state.state |= BGFX_STATE_BLEND_DARKEN;
	else if (blend == BM_Lighten)
		state.state |= BGFX_STATE_BLEND_LIGHTEN;
	else if (blend == BM_Multiply)
		state.state |= BGFX_STATE_BLEND_MULTIPLY;
	// else if (mode == BM_Opaque)
	//	state.state |= BGFX_STATE_BLEND_NORMAL;
	else if (blend == BM_Screen)
		state.state |= BGFX_STATE_BLEND_SCREEN;
	else if (blend == BM_LinearBurn)
		state.state |= BGFX_STATE_BLEND_LINEAR_BURN;

	return state;
}

RenderState ComputeRenderState(BlendMode blend, bool write_z, bool write_r, bool write_g, bool write_b, bool write_a) {
	return ComputeRenderState(blend, DT_Less, FC_Clockwise, write_z, write_r, write_g, write_b, write_a);
}

//
static bgfx::ShaderHandle LoadShader(const Reader &ir, const Handle &h, const char *name) {
	const auto data = LoadData(ir, h);
	if (!data.GetSize())
		return BGFX_INVALID_HANDLE;
	const auto mem = bgfx::copy(data.GetData(), uint32_t(data.GetSize()));
	auto hnd = bgfx::createShader(mem);
	if (bgfx::isValid(hnd))
		bgfx::setName(hnd, name);
	return hnd;
}

//
struct ProgramComparator {
	bool operator()(const bgfx::ProgramHandle &a, const bgfx::ProgramHandle &b) const { return a.idx < b.idx; }
};

static std::map<bgfx::ProgramHandle, std::vector<bgfx::ShaderHandle>, ProgramComparator> program_shaders;

std::vector<bgfx::ShaderHandle> GetProgramShaders(bgfx::ProgramHandle prg_h) { return program_shaders[prg_h]; }

//
bgfx::ProgramHandle LoadProgram(const Reader &ir, const ReadProvider &ip, const char *vs_name, const char *fs_name, bool silent) {
	ProfilerPerfSection section("LoadProgram", vs_name);

	ScopedReadHandle vs_h(ip, vs_name, silent), fs_h(ip, fs_name, silent);

	if (!ir.is_valid(vs_h)) {
		if (!silent)
			warn(format("Vertex shader '%1' not found").arg(vs_name));
		return BGFX_INVALID_HANDLE;
	}

	if (!ir.is_valid(fs_h)) {
		if (!silent)
			warn(format("Fragment shader '%1' not found").arg(fs_name));
		return BGFX_INVALID_HANDLE;
	}

	const auto vs = LoadShader(ir, vs_h, vs_name);

	if (!bgfx::isValid(vs)) {
		if (!silent)
			warn(format("Failed to load vertex shader '%1'").arg(vs_name));
		return BGFX_INVALID_HANDLE;
	}

	const auto fs = LoadShader(ir, fs_h, fs_name);

	if (!bgfx::isValid(fs)) {
		if (!silent)
			warn(format("Failed to load fragment shader '%1'").arg(vs_name));
		return BGFX_INVALID_HANDLE;
	}

	const auto prg_h = bgfx::createProgram(vs, fs, true);

	if (!bgfx::isValid(prg_h)) {
		if (!silent)
			warn(format("Failed to create program from shader '%1' and '%2'").arg(vs_name).arg(fs_name));
		return BGFX_INVALID_HANDLE;
	}

	program_shaders[prg_h] = {vs, fs}; // TODO make this optional? (this is only required to retrieved uniforms for editing ATM)

	return prg_h;
}

bgfx::ProgramHandle LoadProgram(const Reader &ir, const ReadProvider &ip, const char *name, bool silent) {
	const std::string _name(name);
	return LoadProgram(ir, ip, (_name + ".vsb").c_str(), (_name + ".fsb").c_str(), silent);
}

bgfx::ProgramHandle LoadComputeProgram(const Reader &ir, const ReadProvider &ip, const char *cs_name, bool silent) {
	ProfilerPerfSection section("LoadComputeProgram", cs_name);

	ScopedReadHandle cs_h(ip, cs_name);

	if (!ir.is_valid(cs_h)) {
		if (!silent)
			warn(format("Compute shader '%1' not found").arg(cs_name));
		return BGFX_INVALID_HANDLE;
	}

	const auto cs = LoadShader(ir, cs_h, nullptr);

	if (!bgfx::isValid(cs)) {
		if (!silent)
			warn(format("Failed to load compute shader '%1'").arg(cs_name));
		return BGFX_INVALID_HANDLE;
	}

	const auto prg_h = bgfx::createProgram(cs, BGFX_INVALID_HANDLE, true);

	if (!bgfx::isValid(prg_h)) {
		if (!silent)
			warn(format("Failed to create program from shader '%1'").arg(cs_name));
		return BGFX_INVALID_HANDLE;
	}

	program_shaders[prg_h] = {cs, BGFX_INVALID_HANDLE}; // TODO make this optional? (this is only required to retrieved uniforms for editing ATM)

	return prg_h;
}

//
bgfx::ProgramHandle LoadProgramFromFile(const char *vs, const char *fs, bool silent) {
	return LoadProgram(g_file_reader, g_file_read_provider, vs, fs, silent);
}
bgfx::ProgramHandle LoadProgramFromAssets(const char *vs, const char *fs, bool silent) {
	return LoadProgram(g_assets_reader, g_assets_read_provider, vs, fs, silent);
}

bgfx::ProgramHandle LoadProgramFromFile(const char *path, bool silent) { return LoadProgram(g_file_reader, g_file_read_provider, path, silent); }
bgfx::ProgramHandle LoadProgramFromAssets(const char *name, bool silent) { return LoadProgram(g_assets_reader, g_assets_read_provider, name, silent); }

bgfx::ProgramHandle LoadComputeProgramFromFile(const char *cs_path, bool silent) {
	return LoadComputeProgram(g_file_reader, g_file_read_provider, cs_path, silent);
}
bgfx::ProgramHandle LoadComputeProgramFromAssets(const char *cs_name, bool silent) {
	return LoadComputeProgram(g_assets_reader, g_assets_read_provider, cs_name, silent);
}

//
std::vector<PipelineProgramFeature> LoadPipelineProgramFeatures(const Reader &ir, const ReadProvider &ip, const char *name, bool &success, bool silent) {
	ProfilerPerfSection section("LoadPipelineProgramFeatures", name);

	const auto js = LoadJson(ir, ScopedReadHandle(ip, name, true), &success);
	if (!success)
		return {};

	std::vector<PipelineProgramFeature> features;

	const auto i = js.find("features");

	if (i != std::end(js)) {
		features.reserve(i->size());

		for (const auto &j : *i) {
			if (!j.is_string())
				continue;

			const auto feat = j.get<std::string>();

			if (feat == "OptionalBaseColorOpacityMap")
				features.push_back(OptionalBaseColorOpacityMap);
			else if (feat == "OptionalOcclusionRoughnessMetalnessMap")
				features.push_back(OptionalOcclusionRoughnessMetalnessMap);
			else if (feat == "OptionalDiffuseMap")
				features.push_back(OptionalDiffuseMap);
			else if (feat == "OptionalSpecularMap")
				features.push_back(OptionalSpecularMap);
			else if (feat == "OptionalLightMap")
				features.push_back(OptionalLightMap);
			else if (feat == "OptionalSelfMap")
				features.push_back(OptionalSelfMap);
			else if (feat == "OptionalOpacityMap")
				features.push_back(OptionalOpacityMap);
			else if (feat == "OptionalAmbientMap")
				features.push_back(OptionalAmbientMap);
			else if (feat == "OptionalReflectionMap")
				features.push_back(OptionalReflectionMap);
			else if (feat == "OptionalNormalMap")
				features.push_back(OptionalNormalMap);
			else if (feat == "NormalMapInWorldSpace")
				features.push_back(NormalMapInWorldSpace);
			else if (feat == "DiffuseUV1")
				features.push_back(DiffuseUV1);
			else if (feat == "SpecularUV1")
				features.push_back(SpecularUV1);
			else if (feat == "AmbientUV1")
				features.push_back(AmbientUV1);
			else if (feat == "OptionalSkinning")
				features.push_back(OptionalSkinning);
			else if (feat == "OptionalAlphaCut")
				features.push_back(OptionalAlphaCut);
			else if (!silent)
				warn(format("Ignoring unknown pipeline shader feature '%1' in '%2'").arg(feat).arg(name));
		}
	}

	success = true;

	return features;
}

std::vector<PipelineProgramFeature> LoadPipelineProgramFeaturesFromFile(const char *path, bool &success, bool silent) {
	try {
		return LoadPipelineProgramFeatures(g_file_reader, g_file_read_provider, path, success, silent);
	} catch (...) { debug(format("Failed to read program features from file '%1'").arg(path)); }
	return {};
}

std::vector<PipelineProgramFeature> LoadPipelineProgramFeaturesFromAssets(const char *name, bool &success, bool silent) {
	try {
		return LoadPipelineProgramFeatures(g_assets_reader, g_assets_read_provider, name, success, silent);
	} catch (...) { debug(format("Failed to read program features from asset '%1'").arg(name)); }
	return {};
}

//
bool LoadPipelineProgramUniforms(const Reader &ir, const ReadProvider &ip, const char *name, std::vector<TextureUniform> &texs, std::vector<Vec4Uniform> &vecs,
	PipelineResources &resources, bool silent) {
	ProfilerPerfSection section("LoadPipelineProgramUniforms", name);

	bool result;
	const auto js = LoadJson(ir, ScopedReadHandle(ip, name, true), &result);
	if (!result)
		return false;

	const auto i = js.find("uniforms");
	if (i == std::end(js))
		return true; // [EJ] if there's no uniforms declared look no further

	for (const auto &j : *i) {
		const auto uname = j["name"].get<std::string>();
		const auto utype = j["type"].get<std::string>();

		if (utype == "Vec4" || utype == "Color") {
			Vec4Uniform u;
			u.handle = bgfx::createUniform(uname.c_str(), bgfx::UniformType::Vec4);
			u.is_color = utype == "Color";

			const auto v = j.find("value");
			if (v != std::end(j))
				if (v->is_array()) {
					int k = 0;
					for (auto f : *v) {
						u.value[k] = f.get<float>();
						if (++k == 4)
							break;
					}
				}

			vecs.push_back(u);
		} else if (utype == "Sampler") {
			TextureUniform u;
			u.handle = bgfx::createUniform(uname.c_str(), bgfx::UniformType::Sampler);

			auto c = j.find("channel");
			if (c != std::end(j)) {
				u.channel = c->get<uint8_t>();

				const auto v = j.find("value");
				if (v != std::end(j))
					if (v->is_string()) {
						const auto path = v->get<std::string>();
						const auto flags = LoadTextureFlags(ir, ip, path);
						u.tex_ref = LoadTexture(ir, ip, path.c_str(), flags, resources, silent);
					}

				texs.push_back(u);
			} else {
				warn(format("Ignoring invalid uniform '%1' sampler definition in '%2': no channel specified").arg(uname).arg(name));
			}
		}
	}

	return true;
}

bool LoadPipelineProgramUniformsFromFile(
	const char *path, std::vector<TextureUniform> &texs, std::vector<Vec4Uniform> &vecs, PipelineResources &resources, bool silent) {
	return LoadPipelineProgramUniforms(g_file_reader, g_file_read_provider, path, texs, vecs, resources, silent);
}

bool LoadPipelineProgramUniformsFromAssets(
	const char *name, std::vector<TextureUniform> &texs, std::vector<Vec4Uniform> &vecs, PipelineResources &resources, bool silent) {
	return LoadPipelineProgramUniforms(g_assets_reader, g_assets_read_provider, name, texs, vecs, resources, silent);
}

//
PipelineProgram LoadPipelineProgram(
	const Reader &ir, const ReadProvider &ip, const char *name, PipelineResources &resources, const PipelineInfo &pipeline, bool silent) {
	PipelineProgram prg;

	bool success;
	prg.features = LoadPipelineProgramFeatures(ir, ip, name, success, silent);

	if (!success)
		warn(format("Failed to load pipeline program features '%1'").arg(name));
	else
		prg.programs.resize(GetPipelineProgramVariantCount(prg.features) * pipeline.configs.size());

	if (!LoadPipelineProgramUniforms(ir, ip, name, prg.texture_uniforms, prg.vec4_uniforms, resources, silent))
		warn(format("Failed to load pipeline program uniforms '%1'").arg(name));

	prg.name = CutFileExtension(name);
	prg.pipeline = pipeline;
	prg.ip = ip;
	prg.ir = ir;

	return prg;
}

PipelineProgram LoadPipelineProgramFromFile(const char *path, PipelineResources &resources, const PipelineInfo &pipeline, bool silent) {
	return LoadPipelineProgram(g_file_reader, g_file_read_provider, path, resources, pipeline, silent);
}

PipelineProgram LoadPipelineProgramFromAssets(const char *name, PipelineResources &resources, const PipelineInfo &pipeline, bool silent) {
	return LoadPipelineProgram(g_assets_reader, g_assets_read_provider, name, resources, pipeline, silent);
}

//
int GetPipelineProgramFeatureStateCount(PipelineProgramFeature feature) { return 2; }

int GetPipelineProgramVariantCount(const std::vector<PipelineProgramFeature> &feats) {
	int count = 1;
	for (const auto feat : feats)
		count *= GetPipelineProgramFeatureStateCount(feat);
	return count;
}

int GetPipelineProgramVariantIndex(const std::vector<PipelineProgramFeature> &feats, const std::vector<int> &states) {
	__ASSERT__(feats.size() == states.size());

	int k = 1, idx = 0;
	for (size_t i = 0; i < feats.size(); ++i) {
		idx += states[i] * k;
		k *= GetPipelineProgramFeatureStateCount(feats[i]);
	}

	return idx;
}

std::string GetPipelineProgramVariantName(const char *name, const std::vector<PipelineProgramFeature> &feats, const std::vector<int> &states) {
	__ASSERT__(feats.size() == states.size());

	std::vector<std::string> variant_tags;
	variant_tags.reserve(feats.size() + 2);

	variant_tags.push_back(name);
	const auto idx = GetPipelineProgramVariantIndex(feats, states);
	variant_tags.push_back(std::string("var-") + std::to_string(idx));

	return join(std::begin(variant_tags), std::end(variant_tags), "_");
}

//
std::vector<int> GetMaterialPipelineProgramFeatureStates(const Material &mat, const std::vector<PipelineProgramFeature> &features) {
	std::set<std::string> tex_names;
	for (const auto &i : mat.textures)
		if (i.second.texture != InvalidTextureRef)
			tex_names.insert(i.first);

	const auto has_texture = [&](const char *name) -> bool { return tex_names.find(name) != std::end(tex_names); };

	std::vector<int> states;
	states.reserve(features.size());

	for (const auto &feat : features) {
		if (feat == OptionalBaseColorOpacityMap)
			states.push_back(has_texture("uBaseOpacityMap") ? 1 : 0);
		else if (feat == OptionalOcclusionRoughnessMetalnessMap)
			states.push_back(has_texture("uOcclusionRoughnessMetalnessMap") ? 1 : 0);
		else if (feat == OptionalDiffuseMap)
			states.push_back(has_texture("uDiffuseMap") ? 1 : 0);
		else if (feat == OptionalSpecularMap)
			states.push_back(has_texture("uSpecularMap") ? 1 : 0);
		else if (feat == OptionalLightMap)
			states.push_back(has_texture("uLightMap") ? 1 : 0);
		else if (feat == OptionalSelfMap)
			states.push_back(has_texture("uSelfMap") ? 1 : 0);
		else if (feat == OptionalOpacityMap)
			states.push_back(has_texture("uOpacityMap") ? 1 : 0);
		else if (feat == OptionalAmbientMap)
			states.push_back(has_texture("uAmbientMap") ? 1 : 0);
		else if (feat == OptionalReflectionMap)
			states.push_back(has_texture("uReflectionMap") ? 1 : 0);
		else if (feat == OptionalNormalMap)
			states.push_back(has_texture("uNormalMap") ? 1 : 0);
		else if (feat == NormalMapInWorldSpace)
			states.push_back(mat.flags & MF_NormalMapInWorldSpace ? 1 : 0);
		else if (feat == DiffuseUV1)
			states.push_back(mat.flags & MF_DiffuseUV1 ? 1 : 0);
		else if (feat == SpecularUV1)
			states.push_back(mat.flags & MF_SpecularUV1 ? 1 : 0);
		else if (feat == AmbientUV1)
			states.push_back(mat.flags & MF_AmbientUV1 ? 1 : 0);
		else if (feat == OptionalSkinning)
			states.push_back(mat.flags & MF_EnableSkinning ? 1 : 0);
		else if (feat == OptionalAlphaCut)
			states.push_back(mat.flags & MF_EnableAlphaCut ? 1 : 0);
	}

	return states;
}

//
static void CreateMissingMaterialSampler(
	Material &mat, const std::string &name, uint8_t stage, const std::string &tex, PipelineResources &resources, const Reader &ir, const ReadProvider &ip) {
	if (mat.values.find(name) == std::end(mat.values)) {
		debug(format("Creating missing material sampler uniform %1").arg(name));

		TextureRef ref{};
		if (!tex.empty()) {
			debug(format("Loading missing material sampler uniform texture %1").arg(tex));
			ref = LoadTexture(ir, ip, tex.c_str(), 0, resources, true);
		}

		SetMaterialTexture(mat, name.c_str(), ref, stage);
	}
}

static void CreateMissingMaterialVec4(Material &mat, const std::string &name, const Vec4 &val) {
	if (mat.values.find(name) == std::end(mat.values)) {
		debug(format("Creating missing material vec4 uniform %1").arg(name));
		SetMaterialValue(mat, name.c_str(), val);
	}
}

void CreateMissingMaterialProgramValues(Material &mat, PipelineResources &resources, const Reader &ir, const ReadProvider &ip) {
	if (resources.programs.IsValidRef(mat.program)) {
		const auto &prg = resources.programs.Get_unsafe_(mat.program.ref.idx);

		std::set<std::string> tex_names;
		for (const auto &i : mat.textures)
			tex_names.insert(i.first);

		const auto has_texture = [&](const char *name) -> bool { return tex_names.find(name) != std::end(tex_names); };

		for (const auto &feat : prg.features) {
			if (feat == OptionalBaseColorOpacityMap) {
				if (!has_texture("uBaseOpacityMap"))
					CreateMissingMaterialVec4(mat, "uBaseOpacityColor", {0.75f, 0.75f, 0.75f, 1.f});
			} else if (feat == OptionalOcclusionRoughnessMetalnessMap) {
				if (!has_texture("uOcclusionRoughnessMetalnessMap"))
					CreateMissingMaterialVec4(mat, "uOcclusionRoughnessMetalnessColor", {1.f, 0.5f, 0.1f, 1.f});
			} else if (feat == OptionalDiffuseMap) {
				if (!has_texture("uDiffuseMap"))
					CreateMissingMaterialVec4(mat, "uDiffuseColor", {0.75f, 0.75f, 0.75f, 0.1f});
			} else if (feat == OptionalSpecularMap) {
				if (!has_texture("uSpecularMap"))
					CreateMissingMaterialVec4(mat, "uSpecularColor", {0.75f, 0.75f, 0.75f, 0.25f});
			} else if (feat == OptionalSelfMap) {
				if (!has_texture("uSelfMap"))
					CreateMissingMaterialVec4(mat, "uSelfColor", {0.f, 0.f, 0.f, 1.f});
			}
		}

		// the following optional features do not fall back to a material value
		//
		// OptionalLightMap
		// OptionalOpacityMap
		// OptionalAmbientMap
		// OptionalReflectionMap
		// OptionalNormalMap
		//
		// NormalMapWorldOrTangent
		// DiffuseUV1
		// SpecularUV1
		// AmbientUV1
	}
}

void CreateMissingMaterialProgramValuesFromFile(Material &mat, PipelineResources &resources) {
	CreateMissingMaterialProgramValues(mat, resources, g_file_reader, g_file_read_provider);
}

void CreateMissingMaterialProgramValuesFromAssets(Material &mat, PipelineResources &resources) {
	CreateMissingMaterialProgramValues(mat, resources, g_assets_reader, g_assets_read_provider);
}

//
void UpdateMaterialPipelineProgramVariant(Material &mat, const PipelineResources &resources) {
	if (resources.programs.IsValidRef(mat.program)) {
		const auto &prg = resources.programs.Get_unsafe_(mat.program.ref.idx);
		const auto states = GetMaterialPipelineProgramFeatureStates(mat, prg.features);
		mat.variant_idx = GetPipelineProgramVariantIndex(prg.features, states);
	}
}

//
static bx::DefaultAllocator g_allocator;

//
json LoadResourceMeta(const Reader &ir, const ReadProvider &ip, const std::string &name) {
	const auto meta_path = name + ".meta";
	return LoadJson(ir, ScopedReadHandle(ip, meta_path.c_str(), true));
}

json LoadResourceMetaFromFile(const std::string &path) { return LoadResourceMeta(g_file_reader, g_file_read_provider, path); }
json LoadResourceMetaFromAssets(const std::string &name) { return LoadResourceMeta(g_assets_reader, g_assets_read_provider, name); }

bool SaveResourceMetaToFile(const std::string &path, const json &meta) { return SaveJsonToFile(meta, (path + ".meta").c_str()); }

//
TextureMeta LoadTextureMeta(const Reader &ir, const ReadProvider &ip, const std::string &name, bool silent) {
	ProfilerPerfSection section("LoadTextureMeta", name);

	const auto js = LoadResourceMeta(ir, ip, name);

	TextureMeta meta;

	{
		std::string filter;

		GetMetaValue(js, "min-filter", filter);
		if (filter == "Nearest")
			meta.flags |= BGFX_SAMPLER_MIN_POINT;
		else if (filter == "Linear")
			;
		else // if (filter == "Anisotropic")
			meta.flags |= BGFX_SAMPLER_MIN_ANISOTROPIC;

		GetMetaValue(js, "mag-filter", filter);
		if (filter == "Nearest")
			meta.flags |= BGFX_SAMPLER_MAG_POINT;
		else if (filter == "Linear")
			;
		else // if (filter == "Anisotropic")
			meta.flags |= BGFX_SAMPLER_MAG_ANISOTROPIC;
	}

	{
		std::string wrap;

		GetMetaValue(js, "wrap-U", wrap);
		if (wrap == "Clamp")
			meta.flags |= BGFX_SAMPLER_U_CLAMP;
		else if (wrap == "Border")
			meta.flags |= BGFX_SAMPLER_U_BORDER;
		else if (wrap == "Mirror")
			meta.flags |= BGFX_SAMPLER_U_MIRROR;

		GetMetaValue(js, "wrap-V", wrap);
		if (wrap == "Clamp")
			meta.flags |= BGFX_SAMPLER_V_CLAMP;
		else if (wrap == "Border")
			meta.flags |= BGFX_SAMPLER_V_BORDER;
		else if (wrap == "Mirror")
			meta.flags |= BGFX_SAMPLER_V_MIRROR;
	}

	{
		bool sRGB = false;

		GetMetaValue(js, "sRGB", sRGB);
		if (sRGB)
			meta.flags |= BGFX_TEXTURE_SRGB;
		else
			meta.flags &= ~BGFX_TEXTURE_SRGB;
	}

	return meta;
}

TextureMeta LoadTextureMetaFromFile(const std::string &path, bool silent) { return LoadTextureMeta(g_file_reader, g_file_read_provider, path, silent); }
TextureMeta LoadTextureMetaFromAssets(const std::string &name, bool silent) { return LoadTextureMeta(g_assets_reader, g_assets_read_provider, name, silent); }

//
uint64_t LoadTextureFlags(const Reader &ir, const ReadProvider &ip, const std::string &name, bool silent) {
	return LoadTextureMeta(ir, ip, name, silent).flags;
}

uint64_t LoadTextureFlagsFromFile(const std::string &path, bool silent) { return LoadTextureFlags(g_file_reader, g_file_read_provider, path, silent); }
uint64_t LoadTextureFlagsFromAssets(const std::string &name, bool silent) { return LoadTextureFlags(g_assets_reader, g_assets_read_provider, name, silent); }

//
Texture CreateTexture(int width, int height, const char *name, uint64_t flags, bgfx::TextureFormat::Enum texture_format) {
	ProfilerPerfSection section("CreateTexture", name);

	log(format("Creating texture '%1' (empty %2x%3)").arg(name).arg(width).arg(height).c_str());

	bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;

	if (bgfx::isTextureValid(0, false, 1, texture_format, flags))
		handle = bgfx::createTexture2D(width, height, false, 1, texture_format, flags);

	if (bgfx::isValid(handle))
		bgfx::setName(handle, name);
	else
		warn(format("Failed to create texture '%1', format:%2 flags:%3").arg(name).arg(texture_format).arg(flags).c_str());

	return MakeTexture(handle, flags);
}

Texture CreateTextureFromPicture(const Picture &pic, const char *name, uint64_t flags, bgfx::TextureFormat::Enum texture_format) {
	ProfilerPerfSection section("CreateTextureFromPicture", name);

	log(format("Creating texture '%1' from picture").arg(name).c_str());

	bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;

	const auto *mem = bgfx::makeRef(pic.GetData(), pic.GetWidth() * pic.GetHeight() * size_of(pic.GetFormat()));

	if (bgfx::isTextureValid(0, false, 1, texture_format, flags))
		handle = bgfx::createTexture2D(pic.GetWidth(), pic.GetHeight(), false, 1, texture_format, flags, mem);

	if (bgfx::isValid(handle))
		bgfx::setName(handle, name);
	else
		warn(format("Failed to create texture '%1', format:%2 flags:%3").arg(name).arg(texture_format).arg(flags).c_str());

	return MakeTexture(handle, flags);
}

void UpdateTextureFromPicture(Texture &tex, const Picture &pic) {
	ProfilerPerfSection section("UpdateTextureFromPicture");
	const auto *mem = bgfx::makeRef(pic.GetData(), pic.GetWidth() * pic.GetHeight() * size_of(pic.GetFormat()));
	bgfx::updateTexture2D(tex.handle, 0, 0, 0, 0, pic.GetWidth(), pic.GetHeight(), mem);
}

//
Texture LoadTexture(
	const Reader &ir, const ReadProvider &ip, const char *name, uint64_t flags, bgfx::TextureInfo *info, bimg::Orientation::Enum *orientation, bool silent) {
	ProfilerPerfSection section("LoadTexture", name);

	if (!silent)
		log(format("Loading texture '%1'").arg(name).c_str());

	bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;

	const auto data = LoadData(ir, ScopedReadHandle(ip, name, silent));

	if (data.GetSize() > 0) {
		if (auto container = bimg::imageParse(&g_allocator, data.GetData(), numeric_cast<uint32_t>(data.GetSize()), bimg::TextureFormat::Count)) {
			const auto *mem = bgfx::makeRef(
				container->m_data, container->m_size, [](void *ptr, void *user) { BX_ALIGNED_FREE(&g_allocator, user, 16); }, container);

			if (container->m_cubeMap) {
				handle = bgfx::createTextureCube(
					uint16_t(container->m_width), 1 < container->m_numMips, container->m_numLayers, bgfx::TextureFormat::Enum(container->m_format), flags, mem);
			} else if (1 < container->m_depth) {
				handle = bgfx::createTexture3D(uint16_t(container->m_width), uint16_t(container->m_height), uint16_t(container->m_depth),
					1 < container->m_numMips, bgfx::TextureFormat::Enum(container->m_format), flags, mem);
			} else if (bgfx::isTextureValid(0, false, container->m_numLayers, bgfx::TextureFormat::Enum(container->m_format), flags)) {
				handle = bgfx::createTexture2D(uint16_t(container->m_width), uint16_t(container->m_height), 1 < container->m_numMips, container->m_numLayers,
					bgfx::TextureFormat::Enum(container->m_format), flags, mem);
			}

			if (info)
				bgfx::calcTextureSize(*info, uint16_t(container->m_width), uint16_t(container->m_height), uint16_t(container->m_depth), container->m_cubeMap,
					1 < container->m_numMips, container->m_numLayers, bgfx::TextureFormat::Enum(container->m_format));
		}

		if (!bgfx::isValid(handle)) {
			if (!silent)
				warn(format("Failed to load texture '%1', unsupported format").arg(name).c_str());

			static const uint32_t dummy = 0xff00ffff;
			handle = bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::RGBA8, BGFX_SAMPLER_NONE, bgfx::copy(&dummy, 4));
		}

		if (bgfx::isValid(handle))
			bgfx::setName(handle, name);
	} else {
		if (!silent)
			warn(format("Failed to load texture '%1', could not load data").arg(name).c_str());
	}

	return MakeTexture(handle, flags);
}

Texture LoadTextureFromFile(const char *name, uint64_t flags, bgfx::TextureInfo *info, bimg::Orientation::Enum *orientation, bool silent) {
	return LoadTexture(g_file_reader, g_file_read_provider, name, flags, info, orientation, silent);
}

Texture LoadTextureFromAssets(const char *name, uint64_t flags, bgfx::TextureInfo *info, bimg::Orientation::Enum *orientation, bool silent) {
	return LoadTexture(g_assets_reader, g_assets_read_provider, name, flags, info, orientation, silent);
}

RenderBufferResourceFactory RenderBufferResourceFactory::Custom(uint16_t width, uint16_t height) {
	RenderBufferResourceFactory rb_factory;

	rb_factory.create_texture2d = [width, height](
									  bgfx::BackbufferRatio::Enum ratio, bool hasMips, uint16_t numLayers, bgfx::TextureFormat::Enum format, uint64_t flags) {
		auto tex_width = width;
		auto tex_height = height;
		bgfx::getTextureSizeFromRatio(ratio, tex_width, tex_height);
		return bgfx::createTexture2D(tex_width, tex_height, hasMips, numLayers, format, flags);
	};

	rb_factory.create_framebuffer = [width, height](bgfx::BackbufferRatio::Enum ratio, bgfx::TextureFormat::Enum format, uint64_t textureFlags) {
		auto tex_width = width;
		auto tex_height = height;
		bgfx::getTextureSizeFromRatio(ratio, tex_width, tex_height);
		textureFlags |= textureFlags & BGFX_TEXTURE_RT_MSAA_MASK ? 0 : BGFX_TEXTURE_RT;
		bgfx::TextureHandle th = bgfx::createTexture2D(tex_width, tex_height, false, 1, format, textureFlags);
		return bgfx::createFrameBuffer(1, &th, true);
	};

	return rb_factory;
}

RenderBufferResourceFactory RenderBufferResourceFactory::Backbuffer() {
	RenderBufferResourceFactory rb_factory;

	// use back buffer
	rb_factory.create_texture2d = [](bgfx::BackbufferRatio::Enum ratio, bool hasMips, uint16_t numLayers, bgfx::TextureFormat::Enum format, uint64_t flags) {
		return bgfx::createTexture2D(ratio, hasMips, numLayers, format, flags);
	};

	rb_factory.create_framebuffer = [](bgfx::BackbufferRatio::Enum ratio, bgfx::TextureFormat::Enum format, uint64_t textureFlags) {
		return bgfx::createFrameBuffer(ratio, format, textureFlags);
	};

	return rb_factory;
}

//
NLOHMANN_JSON_SERIALIZE_ENUM(FaceCulling, {{FC_Disabled, "disabled"}, {FC_Clockwise, "cw"}, {FC_CounterClockwise, "ccw"}});
NLOHMANN_JSON_SERIALIZE_ENUM(DepthTest, {{DT_Less, "less"}, {DT_LessEqual, "leq"}, {DT_Equal, "eq"}, {DT_GreaterEqual, "geq"}, {DT_Greater, "greater"},
											{DT_NotEqual, "neq"}, {DT_Never, "never"}, {DT_Always, "always"}, {DT_Disabled, "disabled"}});
NLOHMANN_JSON_SERIALIZE_ENUM(BlendMode, {{BM_Additive, "add"}, {BM_Alpha, "alpha"}, {BM_Darken, "darken"}, {BM_Lighten, "lighten"}, {BM_Multiply, "multiply"},
											{BM_Opaque, "opaque"}, {BM_Screen, "screen"}, {BM_LinearBurn, "linearburn"}, {BM_Undefined, "undefined"}});

bool SaveMaterial(const Material &mat, json &js, const PipelineResources &resources) {
	js["program"] = resources.programs.GetName(mat.program);

	for (const auto &i : mat.values) {
		auto &values = js["values"];

		values.push_back({
			{"name", i.first},
			{"type", i.second.type},
			{"value", i.second.value},
		});

		if (i.second.count != 1)
			values["count"] = i.second.count;
	}

	for (const auto &i : mat.textures) {
		json tex = {{"name", i.first}, {"stage", i.second.channel}};

		const auto &texture = i.second.texture;
		if (resources.textures.IsValidRef(texture))
			tex["path"] = resources.textures.GetName(texture);

		js["textures"].push_back(tex);
	}

	js["face_culling"] = GetMaterialFaceCulling(mat);
	js["depth_test"] = GetMaterialDepthTest(mat);
	js["blend_mode"] = GetMaterialBlendMode(mat);

	bool write_r, write_g, write_b, write_a;
	GetMaterialWriteRGBA(mat, write_r, write_g, write_b, write_a);
	js["write_r"] = write_r;
	js["write_g"] = write_g;
	js["write_b"] = write_b;
	js["write_a"] = write_a;

	bool write_z = GetMaterialWriteZ(mat);
	js["write_z"] = write_z;

	json flags = json::array();
	if (mat.flags & MF_EnableSkinning)
		flags.push_back("EnableSkinning");
	if (mat.flags & MF_DiffuseUV1)
		flags.push_back("DiffuseUV1");
	if (mat.flags & MF_SpecularUV1)
		flags.push_back("SpecularUV1");
	if (mat.flags & MF_AmbientUV1)
		flags.push_back("AmbientUV1");
	if (mat.flags & MF_NormalMapInWorldSpace)
		flags.push_back("NormalMapInWorldSpace");
	if (mat.flags & MF_EnableAlphaCut)
		flags.push_back("EnableAlphaCut");
	js["flags"] = flags;
	return true;
}

bool SaveMaterial(const Material &mat, const Writer &iw, const Handle &h, const PipelineResources &resources) {
	Write(iw, h, resources.programs.GetName(mat.program));
	Write(iw, h, uint16_t(mat.values.size()));

	for (const auto &i : mat.values) {
		Write(iw, h, i.first);
		Write(iw, h, i.second.type);
		Write(iw, h, i.second.count);
		Write(iw, h, uint16_t(i.second.value.size()));
		iw.write(h, i.second.value.data(), sizeof(float) * i.second.value.size());
	}

	Write(iw, h, uint16_t(mat.textures.size()));

	for (const auto &i : mat.textures) {
		Write(iw, h, i.first);
		Write(iw, h, i.second.channel);

		if (resources.textures.IsValidRef(i.second.texture)) {
			Write(iw, h, uint8_t(1));
			Write(iw, h, resources.textures.GetName(i.second.texture));
			Write(iw, h, uint32_t(/*resources.textures.Get(i.second.texture).flags*/ 0xffffffff)); // TODO remove obsolete flag
		} else {
			Write(iw, h, uint8_t(0));
		}
	}

	Write(iw, h, mat.state.state);
	Write(iw, h, mat.state.rgba);

	Write(iw, h, mat.flags);
	return true;
}

bool SaveMaterialToFile(const char *path, const Material &m, const PipelineResources &resources) {
	return SaveMaterial(m, g_file_writer, ScopedWriteHandle(g_file_write_provider, path), resources);
}

template <typename T> T GetOptional(const json &js, const char *key, T dflt) {
	auto i = js.find(key);
	if (i == std::end(js))
		return dflt;
	return i->get<T>();
}

TextureRef SkipLoadOrQueueTextureLoad(
	const Reader &ir, const ReadProvider &ip, const char *path, PipelineResources &resources, bool queue_load, bool do_not_load, bool silent) {
	if (do_not_load)
		return resources.textures.Add(path, {});

	auto ref = resources.textures.Has(path);

	if (ref == InvalidTextureRef) {
		const auto meta = LoadTextureMeta(ir, ip, path, silent);

		if (queue_load)
			ref = QueueLoadTexture(ir, ip, path, meta.flags, resources);
		else
			ref = LoadTexture(ir, ip, path, meta.flags, resources, silent);
	}

	return ref;
}

Material LoadMaterial(const json &js, const Reader &deps_ir, const ReadProvider &deps_ip, PipelineResources &resources, const PipelineInfo &pipeline,
	bool queue_texture_loads, bool do_not_load_resources, bool silent) {
	Material mat;

	const std::string prg_name = js.at("program");

	if (do_not_load_resources)
		mat.program = resources.programs.Add(prg_name.c_str(), {});
	else
		mat.program = LoadPipelineProgramRef(deps_ir, deps_ip, prg_name.c_str(), resources, pipeline, silent);

	const auto i_values = js.find("values");

	if (i_values != std::end(js))
		for (const auto &val : *i_values) {
			const auto name = val.at("name").get<std::string>();

			auto &v = mat.values[name];
			v.type = val.at("type");
			v.count = val.find("count") != std::end(val) ? val.at("count").get<uint16_t>() : uint16_t(1);

			const auto &vval = val.at("value");
			v.value.clear();
			v.value.reserve(vval.size());
			for (const auto &vv : vval)
				v.value.push_back(vv);

			v.uniform = BGFX_INVALID_HANDLE;
			if (IsRenderUp())
				v.uniform = bgfx::createUniform(name.c_str(), v.type, v.count);
		}

	const auto i_textures = js.find("textures");

	if (i_textures != std::end(js))
		for (const auto &tex : *i_textures) {
			const auto name = tex.at("name").get<std::string>();

			auto &t = mat.textures[name];
			t.channel = tex.at("stage");

			std::string path;

			const auto i_path = tex.find("path");
			if (i_path != std::end(tex)) {
				path = i_path->get<std::string>();
			} else {
				const auto i_texture = tex.find("texture"); // legacy texture {path, flag} support
				if (i_texture != std::end(tex))
					path = i_texture->at("name").get<std::string>();
			}

			if (!path.empty())
				t.texture = SkipLoadOrQueueTextureLoad(deps_ir, deps_ip, path.c_str(), resources, queue_texture_loads, do_not_load_resources, silent);

			t.uniform = BGFX_INVALID_HANDLE;
			if (IsRenderUp())
				t.uniform = bgfx::createUniform(name.c_str(), bgfx::UniformType::Sampler);
		}

	SetMaterialFaceCulling(mat, GetOptional(js, "face_culling", FC_Clockwise));
	SetMaterialDepthTest(mat, GetOptional(js, "depth_test", DT_Less));
	SetMaterialBlendMode(mat, GetOptional(js, "blend_mode", BM_Opaque));

	const bool write_r = GetOptional(js, "write_r", true), write_g = GetOptional(js, "write_g", true), write_b = GetOptional(js, "write_b", true),
			   write_a = GetOptional(js, "write_a", true);
	SetMaterialWriteRGBA(mat, write_r, write_g, write_b, write_a);

	const bool write_z = GetOptional(js, "write_z", true);
	SetMaterialWriteZ(mat, write_z);

	const auto i_flags = js.find("flags");
	if (i_flags != std::end(js))
		for (auto flag : *i_flags) {
			if (flag == "EnableSkinning")
				mat.flags |= MF_EnableSkinning;
			else if (flag == "DiffuseUV1")
				mat.flags |= MF_DiffuseUV1;
			else if (flag == "SpecularUV1")
				mat.flags |= MF_SpecularUV1;
			else if (flag == "AmbientUV1")
				mat.flags |= MF_AmbientUV1;
			else if (flag == "NormalMapInWorldSpace")
				mat.flags |= MF_NormalMapInWorldSpace;
			else if (flag == "EnableAlphaCut")
				mat.flags |= MF_EnableAlphaCut;
		}

	UpdateMaterialPipelineProgramVariant(mat, resources);
	return mat;
}

Material LoadMaterial(const Reader &ir, const Handle &h, const Reader &deps_ir, const ReadProvider &deps_ip, PipelineResources &resources,
	const PipelineInfo &pipeline, bool queue_texture_loads, bool do_not_load_resources, bool silent) {
	ProfilerPerfSection section("LoadMaterial");

	Material mat;

	std::string name;
	Read(ir, h, name);

	if (do_not_load_resources)
		mat.program = resources.programs.Add(name.c_str(), {});
	else
		mat.program = LoadPipelineProgramRef(deps_ir, deps_ip, name.c_str(), resources, pipeline, silent);

	const auto value_count = Read<uint16_t>(ir, h);

	for (size_t i = 0; i < value_count; ++i) {
		Read(ir, h, name);
		auto &v = mat.values[name];

		Read(ir, h, v.type);
		Read(ir, h, v.count);

		uint16_t value_count;
		Read(ir, h, value_count);

		v.uniform = BGFX_INVALID_HANDLE;
		if (IsRenderUp())
			v.uniform = bgfx::createUniform(name.c_str(), v.type);

		v.value.clear();
		v.value.resize(value_count);
		for (auto &vv : v.value)
			ir.read(h, &vv, sizeof(float));
	}

	const auto texture_count = Read<uint16_t>(ir, h);

	for (size_t i = 0; i < texture_count; ++i) {
		Read(ir, h, name);
		auto &t = mat.textures[name];

		Read(ir, h, t.channel);

		if (Read<uint8_t>(ir, h) == 1) {
			std::string tex_name;
			Read(ir, h, tex_name);

			uint32_t dummy_flags; // probably will be removed, serves no purpose since proper meta file support
			Read(ir, h, dummy_flags);

			if (!tex_name.empty())
				t.texture = SkipLoadOrQueueTextureLoad(deps_ir, deps_ip, tex_name.c_str(), resources, queue_texture_loads, do_not_load_resources, silent);
		}

		t.uniform = BGFX_INVALID_HANDLE;
		if (IsRenderUp())
			t.uniform = bgfx::createUniform(name.c_str(), bgfx::UniformType::Sampler);
	}

	Read(ir, h, mat.state.state);
	Read(ir, h, mat.state.rgba);

	Read(ir, h, mat.flags);

	UpdateMaterialPipelineProgramVariant(mat, resources);
	return mat;
}

Material LoadMaterialFromFile(
	const char *path, PipelineResources &resources, const PipelineInfo &pipeline, bool queue_texture_loads, bool do_not_load_resources, bool silent) {
	return LoadMaterial(g_file_reader, ScopedReadHandle(g_file_read_provider, path), g_file_reader, g_file_read_provider, resources, pipeline,
		queue_texture_loads, do_not_load_resources, silent);
}

Material LoadMaterialFromAssets(
	const char *path, PipelineResources &resources, const PipelineInfo &pipeline, bool queue_texture_loads, bool do_not_load_resources, bool silent) {
	return LoadMaterial(g_assets_reader, ScopedReadHandle(g_assets_read_provider, path), g_assets_reader, g_assets_read_provider, resources, pipeline,
		queue_texture_loads, do_not_load_resources, silent);
}

//
PipelineProgramRef LoadPipelineProgramRef(
	const Reader &ir, const ReadProvider &ip, const char *name, PipelineResources &resources, const PipelineInfo &pipeline, bool silent) {
	auto ref = resources.programs.Has(name);
	if (ref == InvalidPipelineProgramRef) {
		auto prg = LoadPipelineProgram(ir, ip, name, resources, pipeline, silent);
		ref = resources.programs.Add(name, std::move(prg));
	}
	return ref;
}

PipelineProgramRef LoadPipelineProgramRefFromFile(const char *path, PipelineResources &resources, const PipelineInfo &pipeline, bool silent) {
	return LoadPipelineProgramRef(g_file_reader, g_file_read_provider, path, resources, pipeline, silent);
}

PipelineProgramRef LoadPipelineProgramRefFromAssets(const char *name, PipelineResources &resources, const PipelineInfo &pipeline, bool silent) {
	return LoadPipelineProgramRef(g_assets_reader, g_assets_read_provider, name, resources, pipeline, silent);
}

//
bgfx::VertexLayout VertexLayoutPosFloatNormFloat() {
	bgfx::VertexLayout vtx_layout;
	vtx_layout.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float).end();
	return vtx_layout;
}

bgfx::VertexLayout VertexLayoutPosFloatNormUInt8() {
	bgfx::VertexLayout vtx_layout;
	vtx_layout.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Uint8, true, true).end();
	return vtx_layout;
}

bgfx::VertexLayout VertexLayoutPosFloatColorFloat() {
	bgfx::VertexLayout vtx_layout;
	vtx_layout.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Float).end();
	return vtx_layout;
}

bgfx::VertexLayout VertexLayoutPosFloatColorUInt8() {
	bgfx::VertexLayout vtx_layout;
	vtx_layout.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true, false).end();
	return vtx_layout;
}

bgfx::VertexLayout VertexLayoutPosFloatTexCoord0UInt8() {
	bgfx::VertexLayout vtx_layout;
	vtx_layout.begin()
		.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
		.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Uint8, true, false)
		.skip(2)
		.end();
	return vtx_layout;
}

bgfx::VertexLayout VertexLayoutPosFloatNormUInt8TexCoord0UInt8() {
	bgfx::VertexLayout vtx_layout;
	vtx_layout.begin()
		.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
		.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true, false)
		.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Uint8, true, false)
		.skip(2)
		.end();
	return vtx_layout;
}

//
size_t ProcessModelLoadQueue(PipelineResources &res, time_ns t_budget, bool silent) {
	ProfilerPerfSection section("ProcessModelLoadQueue");

	size_t processed = 0;

	const auto t_start = time_now();

	while (!res.model_loads.empty()) {
		const auto &m = res.model_loads.front();

		if (res.models.IsValidRef(m.ref)) {
			auto &mdl = res.models.Get(m.ref);
			const auto name = res.models.GetName(m.ref);
			if (!silent)
				debug(format("Queued model load '%1'").arg(name));

			ModelInfo info;
			ScopedReadHandle h(m.ip, name.c_str(), silent);
			mdl = LoadModel(m.ir, h, name.c_str(), &info, silent);
			res.model_infos[m.ref.ref] = info;
		}

		res.model_loads.pop_front();

		++processed;

		const auto elapsed = time_now() - t_start;
		if (elapsed >= t_budget)
			break;
	}

	return processed;
}

ModelRef QueueLoadModel(const Reader &ir, const ReadProvider &ip, const char *name, PipelineResources &resources) {
	auto ref = resources.models.Has(name);
	if (ref != InvalidModelRef)
		return ref;

	ref = resources.models.Add(name, {});
	resources.model_loads.push_back({ir, ip, ref});
	return ref;
}

ModelRef QueueLoadModelFromFile(const char *path, PipelineResources &resources) { return QueueLoadModel(g_file_reader, g_file_read_provider, path, resources); }

ModelRef QueueLoadModelFromAssets(const char *name, PipelineResources &resources) {
	return QueueLoadModel(g_assets_reader, g_assets_read_provider, name, resources);
}

ModelRef SkipLoadOrQueueModelLoad(
	const Reader &ir, const ReadProvider &ip, const char *path, PipelineResources &resources, bool queue_load, bool do_not_load, bool silent) {
	if (do_not_load)
		return resources.models.Add(path, {});

	auto ref = resources.models.Has(path);

	if (ref == InvalidModelRef) {
		if (queue_load)
			ref = QueueLoadModel(ir, ip, path, resources);
		else
			ref = LoadModel(ir, ip, path, resources, silent);
	}

	return ref;
}

//
Model LoadModel(const Reader &ir, const Handle &h, const char *name, ModelInfo *info, bool silent) {
	ProfilerPerfSection section("LoadModel", name);

	const auto t = time_now();

	if (!ir.is_valid(h)) {
		if (!silent)
			warn(format("Cannot load model '%1', invalid file handle").arg(name));
		return {};
	}

	if (Read<uint32_t>(ir, h) != HarfangMagic) {
		if (!silent)
			warn(format("Cannot load model '%1', invalid magic marker").arg(name));
		return {};
	}

	if (Read<uint8_t>(ir, h) != ModelMarker) {
		if (!silent)
			warn(format("Cannot load model '%1', invalid file marker").arg(name));
		return {};
	}

	const auto version = Read<uint8_t>(ir, h);

	if (version > 2) {
		if (!silent)
			warn(format("Cannot load model '%1', unsupported version %2").arg(name).arg(version));
		return {};
	}

	Model model;

	bgfx::VertexLayout vs_decl;
	ir.read(h, &vs_decl, sizeof(bgfx::VertexLayout)); // read vertex declaration

	uint32_t tri_count{};

	while (true) {
		uint8_t idx_type_size = 2; // legacy is 16 bit indices
		if (version > 1) {
			Read(ir, h, idx_type_size); // idx type size in bytes

			if (idx_type_size == 0)
				break; // EOLists

			__ASSERT_MSG__(idx_type_size == 2 || idx_type_size == 4, "BGFX only supports 16 or 32 bit index buffer");
		}

		// index buffer
		auto size = Read<uint32_t>(ir, h);

		if (version < 2)
			if (size == 0)
				break; // EOLists

		const auto idx_mem = bgfx::alloc(size);
		ir.read(h, idx_mem->data, idx_mem->size);
		tri_count += (size / idx_type_size) / 3;

		const auto idx_hnd = bgfx::createIndexBuffer(idx_mem, idx_type_size == 4 ? BGFX_BUFFER_INDEX32 : BGFX_BUFFER_NONE);
		if (!bgfx::isValid(idx_hnd)) {
			warn(format("%1: failed to create index buffer").arg(name));
			break;
		}

		bgfx::setName(idx_hnd, name);
		
		// vertex buffer
		size = Read<uint32_t>(ir, h);
		const auto vtx_mem = bgfx::alloc(size);
		ir.read(h, vtx_mem->data, vtx_mem->size);

		const auto vtx_hnd = bgfx::createVertexBuffer(vtx_mem, vs_decl);
		if (!bgfx::isValid(vtx_hnd)) {
			warn(format("%1: failed to create vertex buffer").arg(name));
			bgfx::destroy(idx_hnd);
			break;
		}
		bgfx::setName(vtx_hnd, name);

		// bones table
		size = Read<uint32_t>(ir, h);
		std::vector<uint16_t> bones_table;
		bones_table.resize(size);
		ir.read(h, bones_table.data(), bones_table.size() * sizeof(bones_table[0]));

		//
		model.lists.push_back({idx_hnd, vtx_hnd, std::move(bones_table)});
		model.bounds.push_back(Read<MinMax>(ir, h));
		model.mats.push_back(Read<uint16_t>(ir, h));
	}

	if (info) {
		info->vs_decl = vs_decl;
		info->tri_count = tri_count;
	}

	if (version > 0) { // version 1: add bind poses
		const auto bone_count = Read<uint32_t>(ir, h);

		model.bind_pose.resize(bone_count);
		for (uint32_t j = 0; j < bone_count; ++j)
			Read(ir, h, model.bind_pose[j]);
	}

	if (!silent)
		log(format("Load model '%1' (%2 triangles, %3 lists), took %4 ms")
				.arg(name)
				.arg(tri_count)
				.arg(model.lists.size())
				.arg(time_to_ms(time_now() - t))
				.c_str());

	return model;
}

Model LoadModelFromFile(const char *path, ModelInfo *info, bool silent) {
	return LoadModel(g_file_reader, ScopedReadHandle(g_file_read_provider, path, silent), path, info, silent);
}

Model LoadModelFromAssets(const char *name, ModelInfo *info, bool silent) {
	return LoadModel(g_assets_reader, ScopedReadHandle(g_assets_read_provider, name, silent), name, info, silent);
}

//
size_t GetModelMaterialCount(const Model &model) {
	size_t count = 0;
	for (auto i : model.mats)
		if (i > count)
			count = i;
	return count + 1;
}

//
UniformSetValue MakeUniformSetValue(const char *name, float val, uint16_t count) {
	UniformSetValue v;
	v.uniform = bgfx::createUniform(name, bgfx::UniformType::Vec4, count);
	v.value.resize(4 * count);
	v.count = count;
	for (size_t i = 0; i < count; ++i)
		reinterpret_cast<Vec4 *>(v.value.data())[i] = {val, 0.f, 0.f, 0.f};
	return v;
}

UniformSetValue MakeUniformSetValue(const char *name, const Vec2 &val, uint16_t count) {
	UniformSetValue v;
	v.uniform = bgfx::createUniform(name, bgfx::UniformType::Vec4, count);
	v.value.resize(4 * count);
	v.count = count;
	for (size_t i = 0; i < count; ++i)
		reinterpret_cast<Vec4 *>(v.value.data())[i] = {val.x, val.y, 0.f, 0.f};
	return v;
}

UniformSetValue MakeUniformSetValue(const char *name, const Vec3 &val, uint16_t count) {
	UniformSetValue v;
	v.uniform = bgfx::createUniform(name, bgfx::UniformType::Vec4, count);
	v.value.resize(4 * count);
	for (size_t i = 0; i < count; ++i)
		reinterpret_cast<Vec4 *>(v.value.data())[i] = {val.x, val.y, val.z, 1.f};
	return v;
}

UniformSetValue MakeUniformSetValue(const char *name, const Vec4 &val, uint16_t count) {
	UniformSetValue v;
	v.uniform = bgfx::createUniform(name, bgfx::UniformType::Vec4, count);
	v.value.resize(4 * count);
	v.count = count;
	for (size_t i = 0; i < count; ++i)
		reinterpret_cast<Vec4 *>(v.value.data())[i] = val;
	return v;
}

UniformSetValue MakeUniformSetValue(const char *name, const Mat3 &mtx, uint16_t count) {
	UniformSetValue v;
	v.uniform = bgfx::createUniform(name, bgfx::UniformType::Mat4, count);
	v.value.resize(sizeof(bgfxMatrix3) / sizeof(float) * count);
	v.count = count;
	const auto m = to_bgfx(mtx);
	for (size_t i = 0; i < count; ++i)
		reinterpret_cast<bgfxMatrix3 *>(v.value.data())[i] = m;
	return v;
}

UniformSetValue MakeUniformSetValue(const char *name, const Mat4 &mtx, uint16_t count) {
	UniformSetValue v;
	v.uniform = bgfx::createUniform(name, bgfx::UniformType::Mat4, count);
	v.value.resize(sizeof(bgfxMatrix4) / sizeof(float) * count);
	v.count = count;
	const auto m = to_bgfx(mtx);
	for (size_t i = 0; i < count; ++i)
		reinterpret_cast<bgfxMatrix4 *>(v.value.data())[i] = m;
	return v;
}

UniformSetValue MakeUniformSetValue(const char *name, const Mat44 &mtx, uint16_t count) {
	UniformSetValue v;
	v.uniform = bgfx::createUniform(name, bgfx::UniformType::Mat4, count);
	v.value.resize(sizeof(bgfxMatrix4) / sizeof(float) * count);
	v.count = count;
	const auto m = to_bgfx(mtx);
	for (size_t i = 0; i < count; ++i)
		reinterpret_cast<bgfxMatrix4 *>(v.value.data())[i] = m;
	return v;
}

UniformSetTexture MakeUniformSetTexture(const char *name, const Texture &texture, uint8_t stage) {
	UniformSetTexture v;
	v.uniform = bgfx::createUniform(name, bgfx::UniformType::Sampler);
	v.texture = texture;
	v.stage = stage;
	return v;
}

// OK
void SetUniforms(const std::vector<UniformSetValue> &values, const std::vector<UniformSetTexture> &textures) {
	for (auto &v : values)
		bgfx::setUniform(v.uniform, v.value.data(), v.count);
	for (auto &t : textures)
		bgfx::setTexture(t.stage, t.uniform, t.texture.handle, uint32_t(t.texture.flags)); // only retain the BGFX_SAMPLER_XXX bits of the texture flag
}

void DrawDisplayList(bgfx::ViewId view_id, bgfx::IndexBufferHandle idx, bgfx::VertexBufferHandle vtx, bgfx::ProgramHandle prg,
	const std::vector<UniformSetValue> &values, const std::vector<UniformSetTexture> &textures, RenderState state, uint32_t depth) {
	SetUniforms(values, textures);

	bgfx::setIndexBuffer(idx);
	bgfx::setVertexBuffer(0, vtx);

	bgfx::setState(state.state, state.rgba);
	bgfx::submit(view_id, prg, depth);
}

static void RenderDisplayLists(bgfx::ViewId view_id, const std::vector<DisplayList> &lists, bgfx::ProgramHandle prg, const std::vector<UniformSetValue> &values,
	const std::vector<UniformSetTexture> &textures, const bgfxMatrix4 *mtxs, size_t mtx_count, RenderState state, uint32_t depth) {
	const auto i_mtx = bgfx::setTransform(mtxs, uint16_t(mtx_count));

	const auto lists_size = lists.size();

	for (size_t i = 0; i < lists_size; ++i) {
		bgfx::setTransform(i_mtx);
		DrawDisplayList(view_id, lists[i].index_buffer, lists[i].vertex_buffer, prg, values, textures, state, depth);
	}
}

//
void DrawModel(bgfx::ViewId view_id, const Model &mdl, bgfx::ProgramHandle prg, const std::vector<UniformSetValue> &values,
	const std::vector<UniformSetTexture> &textures, const Mat4 *mtxs, size_t mtx_count, RenderState state, uint32_t depth) {
	std::vector<bgfxMatrix4> bgfx_mtxs(mtx_count);
	for (size_t i = 0; i < mtx_count; ++i)
		bgfx_mtxs[i] = to_bgfx(mtxs[i]);
	RenderDisplayLists(view_id, mdl.lists, prg, values, textures, bgfx_mtxs.data(), mtx_count, state, depth);
}

//
void Destroy(PipelineProgram &pprg) {
	pprg.features.clear();

	for (auto &u : pprg.texture_uniforms)
		bgfx::destroy(u.handle);
	pprg.texture_uniforms.clear();

	for (auto &u : pprg.vec4_uniforms)
		bgfx::destroy(u.handle);
	pprg.vec4_uniforms.clear();

	for (auto &prg : pprg.programs)
		if (bgfx::isValid(prg.handle))
			bgfx::destroy(prg.handle);
	pprg.programs.clear();
}

void Destroy(Texture &texture) {
	if (bgfx::isValid(texture.handle))
		bgfx::destroy(texture.handle);
	texture.handle = BGFX_INVALID_HANDLE;
}

void Destroy(Model &model) {
	for (auto &l : model.lists) {
		if (bgfx::isValid(l.index_buffer))
			bgfx::destroy(l.index_buffer);
		l.index_buffer = BGFX_INVALID_HANDLE;

		if (bgfx::isValid(l.vertex_buffer))
			bgfx::destroy(l.vertex_buffer);
		l.vertex_buffer = BGFX_INVALID_HANDLE;
	}
	model.lists.clear();
}

void Destroy(Material &material) {
	for (auto &i : material.textures) {
		if (bgfx::isValid(i.second.uniform))
			bgfx::destroy(i.second.uniform);
		i.second.uniform = BGFX_INVALID_HANDLE;
	}

	for (auto &i : material.values) {
		if (bgfx::isValid(i.second.uniform))
			bgfx::destroy(i.second.uniform);
		i.second.uniform = BGFX_INVALID_HANDLE;
	}
}

//
void PipelineResources::DestroyAll() {
	if (bgfx_is_up) {
		programs.DestroyAll();
		textures.DestroyAll();
		materials.DestroyAll();
		models.DestroyAll();
	}

	texture_loads.clear();
	texture_infos.clear();

	model_loads.clear();
}

//
static size_t ComputePipelineProgramVariantConfigProgramIndex(uint32_t variant_idx, size_t pipeline_config_count, uint8_t pipeline_config_idx) {
	return variant_idx * pipeline_config_count + pipeline_config_idx;
}

static bgfx::ProgramHandle RequestPipelineProgramVariantConfigProgram(const PipelineProgram &pprg, uint32_t variant_idx, uint8_t pipeline_config_idx) {
	const auto idx = ComputePipelineProgramVariantConfigProgramIndex(variant_idx, pprg.pipeline.configs.size(), pipeline_config_idx);

	if (idx < pprg.programs.size()) {
		auto &prg = const_cast<ProgramHandle &>(pprg.programs[idx]); // OUCH!

		if (!prg.loaded) {
			const std::string path = format("%1_var-%2_pipe-%3-cfg-%4").arg(pprg.name).arg(variant_idx).arg(pprg.pipeline.name).arg(pipeline_config_idx);

			log(format("On-request loading pipeline shader program '%1'").arg(path));

			prg.handle = LoadProgram(pprg.ir, pprg.ip, path.c_str());
			prg.loaded = true; // we tried at least...
		}

		return prg.handle;
	}

	return BGFX_INVALID_HANDLE;
}

// OK
static void _RenderPipelineStageDisplayList(bgfx::ViewId view_id, const DisplayList &display_list, const Material &mat, uint8_t pipeline_config_idx,
	const PipelineResources &res, const std::vector<UniformSetValue> &values, const std::vector<UniformSetTexture> &textures, uint32_t depth) {
	const auto &prg = res.programs.Get_unsafe_(mat.program.ref.idx);

	const auto prg_h = RequestPipelineProgramVariantConfigProgram(prg, mat.variant_idx, pipeline_config_idx);
	if (!bgfx::isValid(prg_h))
		return;

	// material values/fallback to shader default
	std::vector<uint16_t> uniforms_set;
	uniforms_set.reserve(32);

	for (const auto &i : mat.values) {
		bgfx::setUniform(i.second.uniform, i.second.value.data(), i.second.count);
		uniforms_set.push_back(i.second.uniform.idx);
	}

	for (const auto &u : prg.vec4_uniforms)
		if (std::find(std::begin(uniforms_set), std::end(uniforms_set), u.handle.idx) == std::end(uniforms_set))
			bgfx::setUniform(u.handle, &u.value.x); // not set, set from shader default

	// material texture/fallback to shader default
	uniforms_set.clear();

	for (const auto &i : mat.textures) {
		const auto &tex = res.textures.Get(i.second.texture);
		if (bgfx::isValid(tex.handle))
			bgfx::setTexture(i.second.channel, i.second.uniform, tex.handle, uint32_t(tex.flags)); // only retain the BGFX_SAMPLER_XXX bits of the texture flag
		uniforms_set.push_back(i.second.uniform.idx);
	}

	for (const auto &u : prg.texture_uniforms)
		if (std::find(std::begin(uniforms_set), std::end(uniforms_set), u.handle.idx) == std::end(uniforms_set)) {
			const auto &tex = res.textures.Get(u.tex_ref);
			if (bgfx::isValid(tex.handle))
				bgfx::setTexture(u.channel, u.handle, tex.handle, uint32_t(tex.flags)); // not set, set from shader default
		}

	DrawDisplayList(view_id, display_list.index_buffer, display_list.vertex_buffer, prg_h, values, textures, mat.state, depth);
}

static void _RenderDisplayLists(bgfx::ViewId view_id, const std::vector<DisplayList> &lists, const std::vector<uint32_t> &depths, const Material *mats,
	uint8_t pipeline_config_idx, const PipelineResources &res, const std::vector<UniformSetValue> &values, const std::vector<UniformSetTexture> &textures,
	const bgfxMatrix4 *mtxs, size_t mtx_count) {
	const auto i_mtx = bgfx::setTransform(mtxs, uint16_t(mtx_count));

	const auto lists_size = lists.size();
	__ASSERT__(lists_size == depths.size());

	for (size_t i = 0; i < lists_size; ++i) {
		bgfx::setTransform(i_mtx);
		_RenderPipelineStageDisplayList(view_id, lists[i], mats[i], pipeline_config_idx, res, values, textures, depths[i]);
	}
}

static void _RenderDisplayLists(bgfx::ViewId view_id, const std::vector<DisplayList> &lists, const std::vector<uint32_t> &depths, const Material &mat,
	uint8_t pipeline_config_idx, const PipelineResources &res, const std::vector<UniformSetValue> &values, const std::vector<UniformSetTexture> &textures,
	const bgfxMatrix4 *mtxs, size_t mtx_count) {
	std::vector<Material> mats(lists.size(), mat);
	_RenderDisplayLists(view_id, lists, depths, mats.data(), pipeline_config_idx, res, values, textures, mtxs, mtx_count);
}

//
void CullModelDisplayLists(const Frustum &frustum, std::vector<ModelDisplayList> &display_lists, const std::vector<Mat4> &mtxs, const PipelineResources &res) {
	const auto i = std::remove_if(std::begin(display_lists), std::end(display_lists), [&](const ModelDisplayList &display_list) {
		const auto &model = res.models.Get_unsafe_(display_list.mdl_idx);
		return TestVisibility(frustum, model.bounds[display_list.lst_idx] * mtxs[display_list.mtx_idx]) == V_Outside;
	});
	display_lists.resize(std::distance(std::begin(display_lists), i));
}

//
static void _DrawModelDisplayLists(bgfx::ViewId view_id, const std::vector<ModelDisplayList> &display_lists, const std::vector<uint32_t> *depths,
	uint8_t pipeline_config_idx, const std::vector<UniformSetValue> &values, const std::vector<UniformSetTexture> &textures, const std::vector<Mat4> &mtxs,
	const std::vector<Mat4> *prv_mtxs, const PipelineResources &res) {
	bgfxMatrix4 _mtx;
	uint32_t mtx_idx = 0xffffffff, i_mtx = 0xffffffff; // last set_matrix

	const auto dl_size = display_lists.size();
	__ASSERT__(depths == nullptr || dl_size == depths->size());

	for (size_t i = 0; i < dl_size; ++i) {
		const auto &dl = display_lists[i];

		if (mtx_idx == dl.mtx_idx) {
			bgfx::setTransform(i_mtx, 1); // reuse last cache index
		} else {
			mtx_idx = dl.mtx_idx;
			_mtx = to_bgfx(mtxs[mtx_idx]);
			i_mtx = bgfx::setTransform(&_mtx, 1);
		}

		if (prv_mtxs) {
			_mtx = to_bgfx((*prv_mtxs)[dl.mtx_idx]);
			bgfx::setUniform(u_previous_model, &_mtx, 1);
		}

		const auto &mdl = res.models.Get_unsafe_(dl.mdl_idx);
		__ASSERT__(dl.mat != nullptr);

		_RenderPipelineStageDisplayList(view_id, mdl.lists[dl.lst_idx], *dl.mat, pipeline_config_idx, res, values, textures, depths ? (*depths)[i] : 0);
	}
}

void DrawModelDisplayLists(bgfx::ViewId view_id, const std::vector<ModelDisplayList> &display_lists, uint8_t pipeline_config_idx,
	const std::vector<UniformSetValue> &values, const std::vector<UniformSetTexture> &textures, const std::vector<Mat4> &mtxs, const PipelineResources &res) {
	_DrawModelDisplayLists(view_id, display_lists, nullptr, pipeline_config_idx, values, textures, mtxs, nullptr, res);
}

void DrawModelDisplayLists(bgfx::ViewId view_id, const std::vector<ModelDisplayList> &display_lists, const std::vector<uint32_t> &depths,
	uint8_t pipeline_config_idx, const std::vector<UniformSetValue> &values, const std::vector<UniformSetTexture> &textures, const std::vector<Mat4> &mtxs,
	const PipelineResources &res) {
	_DrawModelDisplayLists(view_id, display_lists, &depths, pipeline_config_idx, values, textures, mtxs, nullptr, res);
}

void DrawModelDisplayLists(bgfx::ViewId view_id, const std::vector<ModelDisplayList> &display_lists, uint8_t pipeline_config_idx,
	const std::vector<UniformSetValue> &values, const std::vector<UniformSetTexture> &textures, const std::vector<Mat4> &mtxs,
	const std::vector<Mat4> &prv_mtxs, const PipelineResources &res) {
	_DrawModelDisplayLists(view_id, display_lists, nullptr, pipeline_config_idx, values, textures, mtxs, &prv_mtxs, res);
}

//
static void _DrawSkinnedModelDisplayLists(bgfx::ViewId view_id, const std::vector<SkinnedModelDisplayList> &display_lists, const std::vector<uint32_t> *depths,
	uint8_t pipeline_config_idx, const std::vector<UniformSetValue> &values, const std::vector<UniformSetTexture> &textures, const std::vector<Mat4> &mtxs,
	const std::vector<Mat4> *prv_mtxs, const PipelineResources &res) {
	bgfxMatrix4 _mtx[max_skinned_model_matrix_count] = {0};

	const auto dl_size = display_lists.size();
	__ASSERT__(depths == nullptr || dl_size == depths->size());

	for (size_t i = 0; i < dl_size; ++i) {
		const auto &dl = display_lists[i];

		__ASSERT__(dl.bone_count <= max_skinned_model_matrix_count);

		const auto &mdl = res.models.Get_unsafe_(dl.mdl_idx);

		for (int j = 0; j < dl.bone_count; ++j)
			_mtx[j] = to_bgfx(mtxs[dl.mtx_idxs[j]] * mdl.bind_pose[dl.bones_idxs[j]]);

		bgfx::setTransform(_mtx, dl.bone_count); // TODO [EJ] implement matrix caching here

		if (prv_mtxs) {
			for (int j = 0; j < dl.bone_count; ++j)
				_mtx[j] = to_bgfx((*prv_mtxs)[dl.mtx_idxs[j]] * mdl.bind_pose[dl.bones_idxs[j]]);

			bgfx::setUniform(u_previous_model, _mtx, dl.bone_count);
		}

		__ASSERT__(dl.mat != nullptr);

		_RenderPipelineStageDisplayList(view_id, mdl.lists[dl.lst_idx], *dl.mat, pipeline_config_idx, res, values, textures, depths ? (*depths)[i] : 0);
	}
}

void DrawSkinnedModelDisplayLists(bgfx::ViewId view_id, const std::vector<SkinnedModelDisplayList> &display_lists, uint8_t pipeline_config_idx,
	const std::vector<UniformSetValue> &values, const std::vector<UniformSetTexture> &textures, const std::vector<Mat4> &mtxs, const PipelineResources &res) {
	_DrawSkinnedModelDisplayLists(view_id, display_lists, nullptr, pipeline_config_idx, values, textures, mtxs, nullptr, res);
}

void DrawSkinnedModelDisplayLists(bgfx::ViewId view_id, const std::vector<SkinnedModelDisplayList> &display_lists, const std::vector<uint32_t> &depths,
	uint8_t pipeline_config_idx, const std::vector<UniformSetValue> &values, const std::vector<UniformSetTexture> &textures, const std::vector<Mat4> &mtxs,
	const PipelineResources &res) {
	_DrawSkinnedModelDisplayLists(view_id, display_lists, &depths, pipeline_config_idx, values, textures, mtxs, nullptr, res);
}

void DrawSkinnedModelDisplayLists(bgfx::ViewId view_id, const std::vector<SkinnedModelDisplayList> &display_lists, uint8_t pipeline_config_idx,
	const std::vector<UniformSetValue> &values, const std::vector<UniformSetTexture> &textures, const std::vector<Mat4> &mtxs,
	const std::vector<Mat4> &prv_mtxs, const PipelineResources &res) {
	_DrawSkinnedModelDisplayLists(view_id, display_lists, nullptr, pipeline_config_idx, values, textures, mtxs, &prv_mtxs, res);
}

//
ModelRef LoadModel(const Reader &ir, const ReadProvider &ip, const char *path, PipelineResources &resources, bool silent) {
	auto ref = resources.models.Has(path);
	if (ref == InvalidModelRef) {
		auto mdl = LoadModel(ir, ScopedReadHandle(ip, path), path, nullptr, silent);
		ref = resources.models.Add(path, std::move(mdl));
	}
	return ref;
}

ModelRef LoadModelFromFile(const char *path, PipelineResources &resources, bool silent) {
	return LoadModel(g_file_reader, g_file_read_provider, path, resources, silent);
}

ModelRef LoadModelFromAssets(const char *path, PipelineResources &resources, bool silent) {
	return LoadModel(g_assets_reader, g_assets_read_provider, path, resources, silent);
}

MaterialRef LoadMaterialRef(const Reader &ir, const Handle &h, const char *path, const Reader &deps_ir, const ReadProvider &deps_ip,
	PipelineResources &resources, const PipelineInfo &pipeline, bool queue_texture_loads, bool do_not_load_resources, bool silent) {
	auto ref = resources.materials.Has(path);
	if (ref == InvalidMaterialRef) {
		auto mat = LoadMaterial(ir, h, deps_ir, deps_ip, resources, pipeline, queue_texture_loads, do_not_load_resources, silent);
		ref = resources.materials.Add(path, std::move(mat));
	}
	return ref;
}

MaterialRef LoadMaterialRefFromFile(
	const char *path, PipelineResources &resources, const PipelineInfo &pipeline, bool queue_texture_loads, bool do_not_load_resources, bool silent) {
	return LoadMaterialRef(g_file_reader, ScopedReadHandle(g_file_read_provider, path), path, g_file_reader, g_file_read_provider, resources, pipeline,
		queue_texture_loads, do_not_load_resources, silent);
}

MaterialRef LoadMaterialRefFromAssets(
	const char *path, PipelineResources &resources, const PipelineInfo &pipeline, bool queue_texture_loads, bool do_not_load_resources, bool silent) {
	return LoadMaterialRef(g_assets_reader, ScopedReadHandle(g_assets_read_provider, path), path, g_assets_reader, g_assets_read_provider, resources, pipeline,
		queue_texture_loads, do_not_load_resources, silent);
}

//
size_t ProcessTextureLoadQueue(PipelineResources &res, time_ns t_budget, bool silent) {
	ProfilerPerfSection section("ProcessTextureLoadQueue");

	size_t processed = 0;

	const auto t_start = time_now();

	while (!res.texture_loads.empty()) {
		const auto &t = res.texture_loads.front();

		if (res.textures.IsValidRef(t.ref)) {
			auto &tex = res.textures.Get(t.ref);
			const auto name = res.textures.GetName(t.ref);
			debug(format("Queued texture load '%1'").arg(name));

			bgfx::TextureInfo info;
			tex = LoadTexture(t.ir, t.ip, name.c_str(), tex.flags, &info, nullptr, silent);
			res.texture_infos[t.ref.ref] = info;
		}

		res.texture_loads.pop_front();

		++processed;

		const auto elapsed = time_now() - t_start;
		if (elapsed >= t_budget)
			break;
	}

	return processed;
}

TextureRef QueueLoadTexture(const Reader &ir, const ReadProvider &ip, const char *name, uint64_t flags, PipelineResources &resources) {
	auto ref = resources.textures.Has(name);
	if (ref != InvalidTextureRef)
		return ref;

	ref = resources.textures.Add(name, {flags, bgfx::kInvalidHandle});
	resources.texture_loads.push_back({ir, ip, ref});
	return ref;
}

TextureRef QueueLoadTextureFromFile(const char *path, uint64_t flags, PipelineResources &resources) {
	return QueueLoadTexture(g_file_reader, g_file_read_provider, path, flags, resources);
}

TextureRef QueueLoadTextureFromAssets(const char *name, uint64_t flags, PipelineResources &resources) {
	return QueueLoadTexture(g_assets_reader, g_assets_read_provider, name, flags, resources);
}

//
TextureRef LoadTexture(const Reader &ir, const ReadProvider &ip, const char *name, uint64_t flags, PipelineResources &resources, bool silent) {
	auto ref = resources.textures.Has(name);
	if (ref == InvalidTextureRef) {
		bgfx::TextureInfo info;
		const auto tex = LoadTexture(ir, ip, name, flags, &info, nullptr, silent);
		ref = resources.textures.Add(name, tex);
		resources.texture_infos[ref.ref] = info;
	}
	return ref;
}

TextureRef LoadTextureFromFile(const char *path, uint64_t flags, PipelineResources &resources, bool silent) {
	return LoadTexture(g_file_reader, g_file_read_provider, path, flags, resources, silent);
}

TextureRef LoadTextureFromAssets(const char *name, uint64_t flags, PipelineResources &resources, bool silent) {
	return LoadTexture(g_assets_reader, g_assets_read_provider, name, flags, resources, silent);
}

//
uint32_t CaptureTexture(const PipelineResources &resources, const TextureRef &t, Picture &pic) {
	const auto ref = resources.textures.Get(t);
	return bgfx::readTexture(ref.handle, pic.GetData());
}

//
size_t GetQueuedResourceCount(const PipelineResources &res) { return res.model_loads.size() + res.texture_loads.size(); }

size_t ProcessLoadQueues(PipelineResources &res, time_ns t_budget, bool silent) {
	ProfilerPerfSection section("ProcessLoadQueues");

	size_t total = 0;

	total += ProcessModelLoadQueue(res, t_budget, silent);
	total += ProcessTextureLoadQueue(res, t_budget, silent);

	return total;
}

//
void SetTransform(const Mat4 &mtx) { bgfx::setTransform(to_bgfx(mtx).data()); }

//
void Vertices::Clear() { data.clear(); }

void Vertices::Reserve(size_t count) { data.reserve(decl.getSize(numeric_cast<uint32_t>(count))); }
void Vertices::Resize(size_t count) { data.resize(decl.getSize(numeric_cast<uint32_t>(count))); }

Vertices::Vertices(const bgfx::VertexLayout &decl_, size_t count) : decl(decl_) { Resize(count); }

Vertices &Vertices::Begin(size_t i) {
	if (i >= GetCount()) {
		if (i >= GetCapacity()) {
			Reserve(i + 1024);
			debug(format("Vertices Begin() called with index %1, resizing buffer to accommodate request").arg(i));
		}
		Resize(i + 1);
	}

	vtx_attr_flag = 0;

	__ASSERT__(idx == -1);
	idx = numeric_cast<int>(i);
	return *this;
}

Vertices &Vertices::SetPos(const Vec3 &pos) {
	__ASSERT__(idx != -1);
	bgfx::vertexPack(&pos.x, false, bgfx::Attrib::Position, decl, data.data(), idx);
	vtx_attr_flag |= 1 << bgfx::Attrib::Position;
	return *this;
}

Vertices &Vertices::SetNormal(const Vec3 &normal) {
	__ASSERT__(idx != -1);
	bgfx::vertexPack(&normal.x, true, bgfx::Attrib::Normal, decl, data.data(), idx);
	vtx_attr_flag |= 1 << bgfx::Attrib::Normal;
	return *this;
}

Vertices &Vertices::SetTangent(const Vec3 &tangent) {
	__ASSERT__(idx != -1);
	bgfx::vertexPack(&tangent.x, true, bgfx::Attrib::Tangent, decl, data.data(), idx);
	vtx_attr_flag |= 1 << bgfx::Attrib::Tangent;
	return *this;
}

Vertices &Vertices::SetBinormal(const Vec3 &binormal) {
	__ASSERT__(idx != -1);
	bgfx::vertexPack(&binormal.x, true, bgfx::Attrib::Bitangent, decl, data.data(), idx);
	vtx_attr_flag |= 1 << bgfx::Attrib::Bitangent;
	return *this;
}

Vertices &Vertices::SetTexCoord0(const Vec2 &uv) {
	__ASSERT__(idx != -1);
	bgfx::vertexPack(&uv.x, true, bgfx::Attrib::TexCoord0, decl, data.data(), idx);
	vtx_attr_flag |= 1 << bgfx::Attrib::TexCoord0;
	return *this;
}

Vertices &Vertices::SetTexCoord1(const Vec2 &uv) {
	__ASSERT__(idx != -1);
	bgfx::vertexPack(&uv.x, true, bgfx::Attrib::TexCoord1, decl, data.data(), idx);
	vtx_attr_flag |= 1 << bgfx::Attrib::TexCoord1;
	return *this;
}

Vertices &Vertices::SetTexCoord2(const Vec2 &uv) {
	__ASSERT__(idx != -1);
	bgfx::vertexPack(&uv.x, true, bgfx::Attrib::TexCoord2, decl, data.data(), idx);
	vtx_attr_flag |= 1 << bgfx::Attrib::TexCoord2;
	return *this;
}

Vertices &Vertices::SetTexCoord3(const Vec2 &uv) {
	__ASSERT__(idx != -1);
	bgfx::vertexPack(&uv.x, true, bgfx::Attrib::TexCoord3, decl, data.data(), idx);
	vtx_attr_flag |= 1 << bgfx::Attrib::TexCoord3;
	return *this;
}

Vertices &Vertices::SetTexCoord4(const Vec2 &uv) {
	__ASSERT__(idx != -1);
	bgfx::vertexPack(&uv.x, true, bgfx::Attrib::TexCoord4, decl, data.data(), idx);
	vtx_attr_flag |= 1 << bgfx::Attrib::TexCoord4;
	return *this;
}

Vertices &Vertices::SetTexCoord5(const Vec2 &uv) {
	__ASSERT__(idx != -1);
	bgfx::vertexPack(&uv.x, true, bgfx::Attrib::TexCoord5, decl, data.data(), idx);
	vtx_attr_flag |= 1 << bgfx::Attrib::TexCoord5;
	return *this;
}

Vertices &Vertices::SetTexCoord6(const Vec2 &uv) {
	__ASSERT__(idx != -1);
	bgfx::vertexPack(&uv.x, true, bgfx::Attrib::TexCoord6, decl, data.data(), idx);
	vtx_attr_flag |= 1 << bgfx::Attrib::TexCoord6;
	return *this;
}

Vertices &Vertices::SetTexCoord7(const Vec2 &uv) {
	__ASSERT__(idx != -1);
	bgfx::vertexPack(&uv.x, true, bgfx::Attrib::TexCoord7, decl, data.data(), idx);
	vtx_attr_flag |= 1 << bgfx::Attrib::TexCoord7;
	return *this;
}

Vertices &Vertices::SetColor0(const Color &color) {
	__ASSERT__(idx != -1);
	bgfx::vertexPack(&color.r, true, bgfx::Attrib::Color0, decl, data.data(), idx);
	vtx_attr_flag |= 1 << bgfx::Attrib::Color0;
	return *this;
}

Vertices &Vertices::SetColor1(const Color &color) {
	__ASSERT__(idx != -1);
	bgfx::vertexPack(&color.r, true, bgfx::Attrib::Color1, decl, data.data(), idx);
	vtx_attr_flag |= 1 << bgfx::Attrib::Color1;
	return *this;
}

Vertices &Vertices::SetColor2(const Color &color) {
	__ASSERT__(idx != -1);
	bgfx::vertexPack(&color.r, true, bgfx::Attrib::Color2, decl, data.data(), idx);
	vtx_attr_flag |= 1 << bgfx::Attrib::Color2;
	return *this;
}

Vertices &Vertices::SetColor3(const Color &color) {
	__ASSERT__(idx != -1);
	bgfx::vertexPack(&color.r, true, bgfx::Attrib::Color3, decl, data.data(), idx);
	vtx_attr_flag |= 1 << bgfx::Attrib::Color3;
	return *this;
}

void Vertices::End(bool validate) {
	__ASSERT__(idx != -1);

	if (validate) {
		static const char *attr_name[bgfx::Attrib::Count] = {"Position", "Normal", "Tangent", "Bitangent", "Color0", "Color1", "Color2", "Color3", "Indices",
			"Weight", "TexCoord0", "TexCoord1", "TexCoord2", "TexCoord3", "TexCoord4", "TexCoord5", "TexCoord6", "TexCoord7"};

		for (int attr = 0; attr < bgfx::Attrib::Count; ++attr)
			if (decl.has(bgfx::Attrib::Enum(attr)) &&
				!(vtx_attr_flag & (1 << attr))) // attribute expected by the vertex layout but missing from vertex declaration
				warn(format("Incomplete vertex #%1 declaration: missing %2 attribute").arg(idx).arg(attr_name[attr]));
	}

	idx = -1;
}

//
static void DrawPrimitives(bgfx::ViewId view_id, const Vertices &vtx, bgfx::ProgramHandle program, const std::vector<UniformSetValue> &values,
	const std::vector<UniformSetTexture> &textures, const std::vector<uint16_t> *idx, uint64_t state, uint32_t rgba, uint32_t depth) {
	const auto vtx_count = vtx.GetCount();

	bgfx::TransientVertexBuffer vb;
	bgfx::allocTransientVertexBuffer(&vb, numeric_cast<uint32_t>(vtx_count), vtx.GetDecl());

	memcpy(vb.data, vtx.GetData(), vtx.GetSize());

	bgfx::TransientIndexBuffer ib;

	if (idx) {
		bgfx::allocTransientIndexBuffer(&ib, uint32_t(idx->size()));
		std::copy(std::begin(*idx), std::end(*idx), reinterpret_cast<uint16_t *>(ib.data));
	} else {
		bgfx::allocTransientIndexBuffer(&ib, numeric_cast<uint32_t>(vtx_count));
		for (size_t i = 0; i < vtx_count; ++i)
			reinterpret_cast<uint16_t *>(ib.data)[i] = uint16_t(i);
	}

	bgfx::setVertexBuffer(0, &vb);
	bgfx::setIndexBuffer(&ib);

	SetUniforms(values, textures);

	bgfx::setState(state, rgba);
	bgfx::submit(view_id, program, depth);
}

//
void DrawLines(bgfx::ViewId view_id, const Vertices &vtx, bgfx::ProgramHandle program, const std::vector<UniformSetValue> &values,
	const std::vector<UniformSetTexture> &textures, RenderState state, uint32_t depth) {
	DrawPrimitives(view_id, vtx, program, values, textures, nullptr, (state.state & ~BGFX_STATE_PT_MASK) | BGFX_STATE_PT_LINES, state.rgba, depth);
}

void DrawTriangles(bgfx::ViewId view_id, const Vertices &vtx, bgfx::ProgramHandle program, const std::vector<UniformSetValue> &values,
	const std::vector<UniformSetTexture> &textures, RenderState state, uint32_t depth) {
	DrawPrimitives(view_id, vtx, program, values, textures, nullptr, state.state & ~BGFX_STATE_PT_MASK, state.rgba, depth);
}

void DrawLines(bgfx::ViewId view_id, const Vertices &vtx, bgfx::ProgramHandle program, RenderState state, uint32_t depth) {
	DrawLines(view_id, vtx, program, {}, {}, state, depth);
}

void DrawTriangles(bgfx::ViewId view_id, const Vertices &vtx, bgfx::ProgramHandle program, RenderState state, uint32_t depth) {
	DrawTriangles(view_id, vtx, program, {}, {}, state, depth);
}

//
void DrawLines(bgfx::ViewId view_id, const Indices &idx, const Vertices &vtx, bgfx::ProgramHandle program, const std::vector<UniformSetValue> &values,
	const std::vector<UniformSetTexture> &textures, RenderState state, uint32_t depth) {
	DrawPrimitives(view_id, vtx, program, values, textures, &idx, (state.state & ~BGFX_STATE_PT_MASK) | BGFX_STATE_PT_LINES, state.rgba, depth);
}

void DrawTriangles(bgfx::ViewId view_id, const Indices &idx, const Vertices &vtx, bgfx::ProgramHandle program, const std::vector<UniformSetValue> &values,
	const std::vector<UniformSetTexture> &textures, RenderState state, uint32_t depth) {
	DrawPrimitives(view_id, vtx, program, values, textures, &idx, state.state & ~BGFX_STATE_PT_MASK, state.rgba, depth);
}

void DrawLines(bgfx::ViewId view_id, const Indices &idx, const Vertices &vtx, bgfx::ProgramHandle program, RenderState state, uint32_t depth) {
	DrawLines(view_id, idx, vtx, program, {}, {}, state, depth);
}

void DrawTriangles(bgfx::ViewId view_id, const Indices &idx, const Vertices &vtx, bgfx::ProgramHandle program, RenderState state, uint32_t depth) {
	DrawTriangles(view_id, vtx, program, {}, {}, state, depth);
}

//
void DrawSprites(bgfx::ViewId view_id, const Mat3 &inv_view_R, bgfx::VertexLayout &decl, const std::vector<Vec3> &pos, const Vec2 &size,
	bgfx::ProgramHandle prg, const std::vector<UniformSetValue> &values, const std::vector<UniformSetTexture> &textures, RenderState state, uint32_t depth) {
	const auto X = GetX(inv_view_R) * size.x, Y = GetY(inv_view_R) * size.y;

	const auto count = pos.size();

	std::vector<uint16_t> idx(count * 6, 0);
	for (size_t i = 0; i < count * 6; i += 6) {
		idx[i + 0] = 0;
		idx[i + 1] = 1;
		idx[i + 2] = 2;
		idx[i + 3] = 0;
		idx[i + 4] = 2;
		idx[i + 5] = 3;
	}

	Vertices vtx(decl, count * 4);
	for (size_t i = 0; i < count; ++i) {
		const auto &P = pos[i];

		const size_t j = i * 4;
		vtx.Begin(j + 0).SetPos(P - X + Y).SetTexCoord0({0, 0}).End();
		vtx.Begin(j + 1).SetPos(P + X + Y).SetTexCoord0({1, 0}).End();
		vtx.Begin(j + 2).SetPos(P + X - Y).SetTexCoord0({1, 1}).End();
		vtx.Begin(j + 3).SetPos(P - X - Y).SetTexCoord0({0, 1}).End();
	}

	DrawTriangles(view_id, idx, vtx, prg, values, textures, state, depth);
}

void DrawSprites(bgfx::ViewId view_id, const Mat3 &inv_view_R, bgfx::VertexLayout &decl, const std::vector<Vec3> &pos, const Vec2 &size,
	bgfx::ProgramHandle prg, RenderState state, uint32_t depth) {
	DrawSprites(view_id, inv_view_R, decl, pos, size, prg, {}, {}, state, depth);
}

//
void DestroyPipeline(Pipeline &pipeline) {
	pipeline.uniform_values.clear();
	pipeline.uniform_textures.clear();

	for (auto &i : pipeline.textures)
		bgfx::destroy(i.second);
	pipeline.textures.clear();

	for (auto &i : pipeline.framebuffers)
		bgfx::destroy(i.second);
	pipeline.framebuffers.clear();
}

//
static void DumpRenderPipelineInfos() {
	log("Render pipeline memory footprint:");
	log("");
	log("    Structure         | sizeof (bytes)");
	log("    ------------------+---------------");
	log(format("	PipelineInfo      | %1").arg(sizeof(PipelineInfo)).c_str());
	log(format("	PipelineProgram   | %1").arg(sizeof(PipelineProgram)).c_str());
	log(format("	Texture           | %1").arg(sizeof(Texture)).c_str());
	log(format("	UniformSetValue   | %1").arg(sizeof(UniformSetValue)).c_str());
	log(format("	UniformSetTexture | %1").arg(sizeof(UniformSetTexture)).c_str());
	log(format("	ProgramHandle     | %1").arg(sizeof(ProgramHandle)).c_str());
	log(format("	Material          | %1").arg(sizeof(Material)).c_str());
	log(format("	DisplayList       | %1").arg(sizeof(DisplayList)).c_str());
	log(format("	ModelDisplayList  | %1").arg(sizeof(ModelDisplayList)).c_str());
	log(format("	Model             | %1").arg(sizeof(Model)).c_str());
	log(format("	ModelInfo         | %1").arg(sizeof(ModelInfo)).c_str());
	log(format("	ViewState         | %1").arg(sizeof(ViewState)).c_str());
	log(format("	Indices           | %1").arg(sizeof(Indices)).c_str());
	log(format("	Vertices          | %1").arg(sizeof(Vertices)).c_str());
	log("");
}

//
bool IsRenderUp() { return bgfx_is_up; }

bool RenderInit(Window *window, bgfx::RendererType::Enum type, bgfx::CallbackI *callback) {
	bgfx::PlatformData pd;
	bx::memSet(&pd, 0, sizeof(pd));
	pd.ndt = GetDisplay();
	pd.nwh = GetWindowHandle(window);
	bgfx::setPlatformData(pd);

	bgfx::Init init;
	// bx::memSet(&init, 0, sizeof(bgfx::Init));

	init.callback = callback;
	if (type != bgfx::RendererType::Noop) {
		init.type = type;
		init.platformData.nwh = GetWindowHandle(window);
	}
#ifndef NDEBUG
	init.debug = true;
#endif

	DumpRenderPipelineInfos();

	int w, h;
	GetWindowClientSize(window, w, h);
	init.resolution.width = w;
	init.resolution.height = h;

	init.resolution.maxFrameLatency = 1; // 0 is default (3) for DX11
	// bgfx::renderFrame();

	//
	if (!bgfx::init(init))
		return false;

	bgfx::reset(w, h, BGFX_RESET_FLIP_AFTER_RENDER | BGFX_RESET_FLUSH_AFTER_RENDER | BGFX_RESET_MAXANISOTROPY);

	u_previous_model = bgfx::createUniform("uPreviousModel", bgfx::UniformType::Mat4, 96); // WARNING keep in sync with BGFX_MAX_BONES_CONFIG in bgfx_shader.sh

	const bgfx::Caps *caps = bgfx::getCaps();
	SetNDCInfos(caps->originBottomLeft, caps->homogeneousDepth);

	bgfx_is_up = true;
	return true;
}

bool RenderInit(Window *window, bgfx::CallbackI *callback) { return RenderInit(window, bgfx::RendererType::Noop, callback); }

Window *RenderInit(const char *window_title, int width, int height, bgfx::RendererType::Enum type, uint32_t reset_flags, bgfx::TextureFormat::Enum format,
	uint32_t debug_flags, bgfx::CallbackI *callback) {
	auto win = window_title ? NewWindow(window_title, width, height) : NewWindow(width, height);

	if (!RenderInit(win, type, callback)) {
		DestroyWindow(win);
		return nullptr;
	}

	reset_flags |= BGFX_RESET_FLIP_AFTER_RENDER | BGFX_RESET_FLUSH_AFTER_RENDER | BGFX_RESET_MAXANISOTROPY;

	if (reset_flags)
		bgfx::reset(width, height, reset_flags, format);

	if (debug_flags)
		bgfx::setDebug(debug_flags);

	return win;
}

Window *RenderInit(int width, int height, bgfx::RendererType::Enum type, uint32_t reset_flags, bgfx::TextureFormat::Enum format, uint32_t debug_flags,
	bgfx::CallbackI *callback) {
	return RenderInit(nullptr, width, height, type, reset_flags, format, debug_flags, callback);
}

Window *RenderInit(int width, int height, uint32_t reset_flags, bgfx::TextureFormat::Enum format, uint32_t debug_flags, bgfx::CallbackI *callback) {
	return RenderInit(width, height, bgfx::RendererType::Noop, reset_flags, format, debug_flags, callback);
}

Window *RenderInit(
	const char *window_title, int width, int height, uint32_t reset_flags, bgfx::TextureFormat::Enum format, uint32_t debug_flags, bgfx::CallbackI *callback) {
	return RenderInit(window_title, width, height, bgfx::RendererType::Noop, reset_flags, format, debug_flags, callback);
}

void RenderShutdown() {
	bgfx::shutdown();
	bgfx_is_up = false;
}

//
bool RenderResetToWindow(Window *win, int &width, int &height, uint32_t reset_flags) {
	ProfilerPerfSection section("RenderResetToWindow");

	int new_width, new_height;
	if (!GetWindowClientSize(win, new_width, new_height))
		return false; // query failed...

	if (new_width == width && new_height == height)
		return false; // nothing to be done

	width = new_width;
	height = new_height;
	bgfx::reset(width, height, reset_flags);
	return true;
}

//
void SetView2D(bgfx::ViewId id, int x, int y, int res_x, int res_y, float znear, float zfar, uint16_t clear_flags, const Color &clear_color, float depth,
	uint8_t stencil, bool y_up) {
	const bgfx::Caps *caps = bgfx::getCaps();

	bgfx::setViewClear(id, clear_flags, ColorToABGR32(clear_color), depth, stencil);
	bgfx::setViewRect(id, x, y, res_x, res_y);

	static const auto _view = to_bgfx(Mat4::Identity);

	const auto proj = Compute2DProjectionMatrix(znear, zfar, float(res_x), float(res_y), y_up);
	const auto _proj = to_bgfx(proj);

	bgfx::setViewTransform(id, _view.data(), _proj.data());
}

void SetViewPerspective(bgfx::ViewId id, int x, int y, int res_x, int res_y, const Mat4 &world, float znear, float zfar, float zoom_factor,
	uint16_t clear_flags, const Color &clear_color, float depth, uint8_t stencil, const Vec2 &aspect_ratio) {
	const bgfx::Caps *caps = bgfx::getCaps();

	bgfx::setViewClear(id, clear_flags, ColorToABGR32(clear_color), depth, stencil);
	bgfx::setViewRect(id, x, y, res_x, res_y);

	const auto _aspect_ratio = aspect_ratio == Vec2::Zero ? ComputeAspectRatioX(float(res_x), float(res_y)) : aspect_ratio;

	const auto view = to_bgfx(InverseFast(world));
	const auto proj = to_bgfx(ComputePerspectiveProjectionMatrix(znear, zfar, zoom_factor, _aspect_ratio));

	bgfx::setViewTransform(id, view.data(), proj.data());
}

void SetViewOrthographic(bgfx::ViewId id, int x, int y, int res_x, int res_y, const Mat4 &world, float znear, float zfar, float size, uint16_t clear_flags,
	const Color &clear_color, float depth, uint8_t stencil, const Vec2 &aspect_ratio) {
	const bgfx::Caps *caps = bgfx::getCaps();

	bgfx::setViewClear(id, clear_flags, ColorToABGR32(clear_color), depth, stencil);
	bgfx::setViewRect(id, x, y, res_x, res_y);

	const auto _aspect_ratio = aspect_ratio == Vec2::Zero ? ComputeAspectRatioX(float(res_x), float(res_y)) : aspect_ratio;

	const auto view = to_bgfx(InverseFast(world));
	const auto proj = to_bgfx(ComputeOrthographicProjectionMatrix(znear, zfar, size, _aspect_ratio));

	bgfx::setViewTransform(id, view.data(), proj.data());
}

//
static inline FrameBuffer CreateFrameBuffer(bgfx::TextureHandle color, bgfx::TextureHandle depth, const char *name, bool own_textures) {
	bgfx::TextureHandle texs[] = {color, depth};

	if (own_textures) {
		bgfx::setName(color, format("FrameBuffer.color (%1)").arg(name).c_str());
		bgfx::setName(depth, format("FrameBuffer.depth (%1)").arg(name).c_str());
	}

	bgfx::FrameBufferHandle handle = bgfx::createFrameBuffer(2, texs, own_textures);
	bgfx::setName(handle, format("FrameBuffer (%1)").arg(name).c_str());

	return {handle};
}

FrameBuffer CreateFrameBuffer(const Texture &color, const Texture &depth, const char *name) {
	return CreateFrameBuffer(color.handle, depth.handle, name, false);
}

static inline uint64_t ComputeTextureRTMSAAFlag(int aa) {
	switch (aa) {
		case 2:
			return BGFX_TEXTURE_RT_MSAA_X2;
		case 4:
			return BGFX_TEXTURE_RT_MSAA_X4;
		case 8:
			return BGFX_TEXTURE_RT_MSAA_X8;
		case 16:
			return BGFX_TEXTURE_RT_MSAA_X16;
		default:
			return BGFX_TEXTURE_RT;
	}
}

FrameBuffer CreateFrameBuffer(bgfx::TextureFormat::Enum color_format, bgfx::TextureFormat::Enum depth_format, int aa, const char *name) {
	uint64_t flags = ComputeTextureRTMSAAFlag(aa);

	const auto color = bgfx::createTexture2D(bgfx::BackbufferRatio::Equal, false, 1, color_format, flags);
	const auto depth = bgfx::createTexture2D(bgfx::BackbufferRatio::Equal, false, 1, depth_format, flags | BGFX_TEXTURE_RT_WRITE_ONLY);

	return CreateFrameBuffer(color, depth, name, true);
}

FrameBuffer CreateFrameBuffer(int width, int height, bgfx::TextureFormat::Enum color_format, bgfx::TextureFormat::Enum depth_format, int aa, const char *name) {
	uint64_t flags = ComputeTextureRTMSAAFlag(aa);

	const auto color = bgfx::createTexture2D(width, height, false, 1, color_format, flags);
	const auto depth = bgfx::createTexture2D(width, height, false, 1, depth_format, flags | BGFX_TEXTURE_RT_WRITE_ONLY);

	return CreateFrameBuffer(color, depth, name, true);
}

Texture GetColorTexture(FrameBuffer &frameBuffer) { return MakeTexture(bgfx::getTexture(frameBuffer.handle, 0)); }
Texture GetDepthTexture(FrameBuffer &frameBuffer) { return MakeTexture(bgfx::getTexture(frameBuffer.handle, 1)); }

void DestroyFrameBuffer(FrameBuffer &frameBuffer) { bgfx::destroy(frameBuffer.handle); }

//
bool CreateFullscreenQuad(bgfx::TransientIndexBuffer &idx, bgfx::TransientVertexBuffer &vtx) {
	bgfx::VertexLayout layout;
	layout.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float).end();

	if (bgfx::getAvailTransientIndexBuffer(6) != 6 || bgfx::getAvailTransientVertexBuffer(4, layout) != 4)
		return false;

	//
	bgfx::allocTransientIndexBuffer(&idx, 6);
	{
		uint16_t *data = reinterpret_cast<uint16_t *>(idx.data);

		*data++ = 0;
		*data++ = 1;
		*data++ = 2;
		*data++ = 0;
		*data++ = 2;
		*data++ = 3;
	}

	//
	const float x_min = 0.f, x_max = 1.f;
	const float y_min = 0.f, y_max = 1.f;
	const float u_min = 0.f, u_max = 1.f;

	float v_min = 0.f, v_max = 1.f;
	if (bgfx::getCaps()->originBottomLeft)
		std::swap(v_min, v_max);

	bgfx::allocTransientVertexBuffer(&vtx, 4, layout);
	{
		float *data = reinterpret_cast<float *>(vtx.data);

		*data++ = x_min; // x
		*data++ = y_min; // y
		*data++ = 0.f; // z
		*data++ = u_min; // u
		*data++ = v_min; // v

		*data++ = x_max;
		*data++ = y_min;
		*data++ = 0.f;
		*data++ = u_max;
		*data++ = v_min;

		*data++ = x_max;
		*data++ = y_max;
		*data++ = 0.f;
		*data++ = u_max;
		*data++ = v_max;

		*data++ = x_min;
		*data++ = y_max;
		*data++ = 0.f;
		*data++ = u_min;
		*data++ = v_max;
	}

	return true;
}

//
Material CreateMaterial(PipelineProgramRef prg) { return {prg}; }

Material CreateMaterial(PipelineProgramRef prg, const char *value_name, const Vec4 &value) {
	Material mat;
	mat.program = prg;
	SetMaterialValue(mat, value_name, value);
	return mat;
}

Material CreateMaterial(PipelineProgramRef prg, const char *value_name_0, const Vec4 &value_0, const char *value_name_1, const Vec4 &value_1) {
	Material mat;
	mat.program = prg;
	SetMaterialValue(mat, value_name_0, value_0);
	SetMaterialValue(mat, value_name_1, value_1);
	return mat;
}

//

bimg::ImageContainer *LoadImage(const Reader &ir, const ReadProvider &ip, const char *name) {
	ProfilerPerfSection section("LoadImage", name);

	const auto data = LoadData(ir, ScopedReadHandle(ip, name));
	if (data.GetSize() == 0)
		return nullptr;

	return bimg::imageParse(&g_allocator, data.GetData(), uint32_t(data.GetSize()), bimg::TextureFormat::Count);
}

bimg::ImageContainer *LoadImageFromFile(const char *name) { return LoadImage(g_file_reader, g_file_read_provider, name); }

bimg::ImageContainer *LoadImageFromAssets(const char *name) { return LoadImage(g_assets_reader, g_assets_read_provider, name); }

void UpdateTextureFromImage(Texture &tex, bimg::ImageContainer *img, bool auto_delete) {
	ProfilerPerfSection section("UpdateTextureFromImage");
	const bgfx::Memory *mem = bgfx::makeRef(
		img->m_data, img->m_size, auto_delete ? [](void *ptr, void *user) { bimg::imageFree((bimg::ImageContainer *)user); } : (bgfx::ReleaseFn)0, img);
	bgfx::updateTexture2D(tex.handle, 0, 0, 0, 0, img->m_width, img->m_height, mem);
}

} // namespace hg
