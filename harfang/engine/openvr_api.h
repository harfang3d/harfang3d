// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/vector2.h"
#include "foundation/matrix4.h"
#include "foundation/matrix44.h"

#include <bgfx/bgfx.h>

namespace hg {

struct ViewState;
struct Texture;

bool OpenVRInit();
void OpenVRShutdown();

struct OpenVREye {
	Mat4 offset;
	Mat44 projection;
};

enum OpenVRAA { OVRAA_None, OVRAA_MSAA2x, OVRAA_MSAA4x, OVRAA_MSAA8x, OVRAA_MSAA16x };

struct OpenVREyeFrameBuffer {
	bgfx::FrameBufferHandle fb{bgfx::kInvalidHandle};
	bgfx::TextureHandle color{bgfx::kInvalidHandle}, resolve{bgfx::kInvalidHandle}, depth{bgfx::kInvalidHandle};
	OpenVRAA aa{ OVRAA_None };
	uintptr_t native{0};
};

iVec2 OpenVRGetFrameBufferSize();
OpenVREyeFrameBuffer OpenVRCreateEyeFrameBuffer(OpenVRAA aa = OVRAA_None);
void OpenVRDestroyEyeFrameBuffer(OpenVREyeFrameBuffer &eye_fb);

struct OpenVRState {
	Mat4 body;
	Mat4 head, inv_head;
	OpenVREye left, right;
	uint32_t width, height;
};

OpenVRState OpenVRGetState(const Mat4 &body, float znear, float zfar);
void OpenVRStateToViewState(const OpenVRState &state, ViewState &left, ViewState &right);

void OpenVRSubmitFrame(void *left_eye_texture, void *right_eye_texture);
void OpenVRSubmitFrame(const OpenVREyeFrameBuffer &left, const OpenVREyeFrameBuffer &right);

void OpenVRPostPresentHandoff();

Texture OpenVRGetColorTexture(const OpenVREyeFrameBuffer &eye);
Texture OpenVRGetDepthTexture(const OpenVREyeFrameBuffer &eye);

} // namespace hg
