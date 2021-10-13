// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <bgfx/bgfx.h>

#include <vector>

namespace hg {

struct PickingResults {
	int width{}, height{};
	uint32_t result_frame{};
	std::vector<uint8_t> color_data, depth_data;
};

class PickingHelper {
public:
	~PickingHelper();

	void Create(int width, int height);
	void Destroy();

	bgfx::FrameBufferHandle GetFramebuffer() const { return fb; }

	void Readback(bgfx::ViewId &view_id, PickingResults &res) const;

private:
	int width{}, height{};

	bgfx::TextureHandle color = BGFX_INVALID_HANDLE, depth = BGFX_INVALID_HANDLE;
	bgfx::FrameBufferHandle fb = BGFX_INVALID_HANDLE;

	bgfx::TextureHandle color_read = BGFX_INVALID_HANDLE, depth_read = BGFX_INVALID_HANDLE;
};

} // namespace hg
