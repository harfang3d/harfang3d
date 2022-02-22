// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/temporal_accumulation.h"

#include "foundation/file_rw_interface.h"
#include "foundation/format.h"
#include "foundation/projection.h"

#include "engine/assets_rw_interface.h"

namespace hg {

bool IsValid(const TemporalAccumulation &temporal_acc) {
	return bgfx::isValid(temporal_acc.compute) && bgfx::isValid(temporal_acc.u_previous) && bgfx::isValid(temporal_acc.u_current) &&
		   bgfx::isValid(temporal_acc.u_attr1);
}

static TemporalAccumulation _CreateTemporalAccumulation(const Reader &ir, const ReadProvider &ip, const char *path) {
	TemporalAccumulation temporal_acc;
	temporal_acc.compute = LoadProgram(ir, ip, format("%1/shader/temporal_accumulation").arg(path));
	temporal_acc.u_current = bgfx::createUniform("u_current", bgfx::UniformType::Sampler);
	temporal_acc.u_previous = bgfx::createUniform("u_previous", bgfx::UniformType::Sampler);
	temporal_acc.u_attr1 = bgfx::createUniform("u_attr1", bgfx::UniformType::Sampler);
	if (!IsValid(temporal_acc)) {
		DestroyTemporalAccumulation(temporal_acc);
	}
	return temporal_acc;
}

TemporalAccumulation CreateTemporalAccumulationFromFile(const char *path) { return _CreateTemporalAccumulation(g_file_reader, g_file_read_provider, path); }
TemporalAccumulation CreateTemporalAccumulationFromAssets(const char *path) {
	return _CreateTemporalAccumulation(g_assets_reader, g_assets_read_provider, path);
}

void DestroyTemporalAccumulation(TemporalAccumulation &temporal_acc) {
	bgfx_Destroy(temporal_acc.compute);
	bgfx_Destroy(temporal_acc.u_current);
	bgfx_Destroy(temporal_acc.u_previous);
	bgfx_Destroy(temporal_acc.u_attr1);
}

void ComputeTemporalAccumulation(bgfx::ViewId &view_id, const iRect &rect, const Texture &current, const Texture &previous, const Texture &attr1,
	bgfx::FrameBufferHandle output, const TemporalAccumulation &temporal_acc) {
	__ASSERT__(IsValid(temporal_acc));

	const bgfx::Caps *caps = bgfx::getCaps();

	bgfx::TransientIndexBuffer idx;
	bgfx::TransientVertexBuffer vtx;
	CreateFullscreenQuad(idx, vtx);

	float ortho[16];
	memcpy(ortho, to_bgfx(Compute2DProjectionMatrix(0.f, 100.f, 1.f, 1.f, false)).data(), sizeof(float[16]));

	bgfx::setViewName(view_id, "Temporal accumulation");
	bgfx::setViewRect(view_id, rect.sx, rect.sy, GetWidth(rect), GetHeight(rect));
	bgfx::setViewFrameBuffer(view_id, output);
	bgfx::setViewTransform(view_id, NULL, ortho);
	bgfx::setViewClear(view_id, BGFX_CLEAR_NONE, 0, 1.f, UINT8_MAX);
	bgfx::setTexture(0, temporal_acc.u_current, current.handle, uint32_t(current.flags));
	bgfx::setTexture(1, temporal_acc.u_previous, previous.handle, uint32_t(previous.flags));
	bgfx::setTexture(2, temporal_acc.u_attr1, attr1.handle, uint32_t(attr1.flags));

	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_ALWAYS);
	bgfx::setIndexBuffer(&idx);
	bgfx::setVertexBuffer(0, &vtx);
	bgfx::submit(view_id, temporal_acc.compute);
	view_id++;
}

} // namespace hg
