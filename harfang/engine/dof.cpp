// HARFANG(R) Copyright (C) 2023 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "dof.h"
#include "assets_rw_interface.h"

#include <foundation/file_rw_interface.h>
#include <foundation/format.h>
#include <foundation/log.h>
#include <foundation/projection.h>
#include <foundation/rw_interface.h>

namespace hg {

static bool LoadShaders(Dof &dof, const Reader &ir, const ReadProvider &ip, const char *path) {
	// uniforms
	dof.u_color = bgfx::createUniform("u_color", bgfx::UniformType::Sampler);
	dof.u_attr0 = bgfx::createUniform("u_attr0", bgfx::UniformType::Sampler);
	dof.u_params = bgfx::createUniform("u_params", bgfx::UniformType::Vec4);

	if (!(bgfx::isValid(dof.u_color) && bgfx::isValid(dof.u_attr0) && bgfx::isValid(dof.u_params))) {
		warn("failed to create depth of field uniforms.");
		return false;
	}

	// load programs
	dof.prg_dof_coc = LoadProgram(ir, ip, format("%1/shader/dof_coc").arg(path));

	if (!(bgfx::isValid(dof.prg_dof_coc))) {
		warn("failed to load depth of field programs.");
		return false;
	}

	return true;
}

static const uint64_t g_dof_tex_flags = 0 | BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;

bool IsValid(const Dof &dof) {
	return bgfx::isValid(dof.u_color) && bgfx::isValid(dof.u_attr0) && bgfx::isValid(dof.u_params) && bgfx::isValid(dof.prg_dof_coc);
}

static Dof CreateDof(const Reader &ir, const ReadProvider &ip, const char *path) {
	Dof dof;
	if (!LoadShaders(dof, ir, ip, path)) {
		DestroyDof(dof);
		return dof;
	}

	return dof;
}

Dof CreateDofFromFile(const char *path) { return CreateDof(g_file_reader, g_file_read_provider, path); }
Dof CreateDofFromAssets(const char *path) { return CreateDof(g_assets_reader, g_assets_read_provider, path); }

void DestroyDof(Dof &dof) {
	bgfx_Destroy(dof.u_color);
	bgfx_Destroy(dof.u_attr0);
	bgfx_Destroy(dof.u_params);

	bgfx_Destroy(dof.prg_dof_coc);
}

void ApplyDof(bgfx::ViewId &view_id, const iRect &rect, bgfx::BackbufferRatio::Enum ratio, const Texture &color, const Texture &attr0,
	bgfx::FrameBufferHandle output, const Dof &dof, float focus_point, float focus_length) {
	bgfx::TransientIndexBuffer idx;
	bgfx::TransientVertexBuffer vtx;
	CreateFullscreenQuad(idx, vtx);

	float ortho[16];
	memcpy(ortho, to_bgfx(Compute2DProjectionMatrix(0.f, 100.f, 1.f, 1.f, false)).data(), sizeof(float[16]));

	bgfx::setViewName(view_id, "DoF");
	bgfx::setViewRect(view_id, rect.sx, rect.sy, GetWidth(rect), GetHeight(rect));
	bgfx::setViewFrameBuffer(view_id, output);
	bgfx::setViewTransform(view_id, NULL, ortho);
	bgfx::setViewClear(view_id, BGFX_CLEAR_NONE, 0, 1.f, UINT8_MAX);
	bgfx::setTexture(0, dof.u_color, color.handle, uint32_t(color.flags));
	bgfx::setTexture(1, dof.u_attr0, attr0.handle, uint32_t(attr0.flags));

	float params[4];
	params[0] = focus_point;
	params[1] = focus_length;
	bgfx::setUniform(dof.u_params, params);

	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_ALWAYS);
	bgfx::setIndexBuffer(&idx);
	bgfx::setVertexBuffer(0, &vtx);
	bgfx::submit(view_id, dof.prg_dof_coc);
	view_id++;
}

} // namespace hg
