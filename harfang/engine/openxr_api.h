// HARFANG(R) Copyright (C) 2019 Emmanuel Julien, Movida Production. Released under GPL/LGPL/Commercial Licence, see licence.txt.

#pragma once

#include "engine/scene_forward_pipeline.h"
#include "foundation/frustum.h"
#include "foundation/matrix4.h"
#include "foundation/matrix44.h"
#include "foundation/rect.h"

#include <bgfx/bgfx.h>
#include <string>

struct XrCompositionLayerBaseHeader;

namespace hg {

struct ViewState;
struct Texture;

enum OpenXRExtensions {
	OXRExtensions_None = 0,
	OXRExtensions_EyeGaze = 1 << 1,
	OXRExtensions_Tracker = 1 << 2,
	OXRExtensions_PassThrough = 1 << 3,
	OXRExtensions_HandTracking = 1 << 4,
	OXRExtensions_VARJO_QUADVIEWS = 1 << 5,
	OXRExtensions_COMPOSITION_LAYER_DEPTH = 1 << 6
};

#define OpenXRExtensionsFlags uint16_t

bool OpenXRInit(OpenXRExtensionsFlags ExtensionsFlagsEnable = OXRExtensions_None);
void OpenXRShutdown();

enum OpenXRAA { OXRAA_None, OXRAA_MSAA2x, OXRAA_MSAA4x, OXRAA_MSAA8x, OXRAA_MSAA16x };

struct OpenXREyeFrameBuffer {
	bgfx::FrameBufferHandle fb{bgfx::kInvalidHandle};
	bgfx::TextureHandle color{bgfx::kInvalidHandle}, resolve{bgfx::kInvalidHandle}, depth{bgfx::kInvalidHandle};
	OpenXRAA aa{OXRAA_None};
	uintptr_t native{0}, nativeDepth{0};
	float width, height;
};

std::vector<OpenXREyeFrameBuffer> OpenXRCreateEyeFrameBuffer(OpenXRAA aa = OXRAA_None);
void OpenXRDestroyEyeFrameBuffer(OpenXREyeFrameBuffer &eye_fb);

std::string OpenXRGetInstanceInfo();
bool OpenXRGetEyeGaze(Mat4 &eye_gaze);

bool OpenXRGetHeadPose(Mat4 &head_pose);

struct OpenXRFrameInfo {
	bool shouldEndFrame;
	std::vector<XrCompositionLayerBaseHeader*> layers;
	std::vector<int> id_fbs;
	int64_t predictedDisplayTime;
};

OpenXRFrameInfo OpenXRSubmitSceneToForwardPipeline(const Mat4 &cam_offset,
                                                   std::function<void(Mat4 *head)> update_controllers,
                                                   std::function<uint16_t(Rect<int> *rect, ViewState *view_state, uint16_t *view_id, bgfx::FrameBufferHandle *fb)> draw_scene,
                                                   uint16_t &view_id,
                                                   float z_near,
                                                   float z_far);

void OpenXRFinishSubmitFrameBuffer(const OpenXRFrameInfo &frameInfo);

Texture OpenXRGetColorTexture(const OpenXREyeFrameBuffer &eye);
Texture OpenXRGetDepthTexture(const OpenXREyeFrameBuffer &eye);
Texture OpenXRGetColorTextureFromId(const std::vector<OpenXREyeFrameBuffer> &eyes, const OpenXRFrameInfo &frame_info, const int &index);
Texture OpenXRGetDepthTextureFromId(const std::vector<OpenXREyeFrameBuffer> &eyes, const OpenXRFrameInfo &frame_info, const int &index);

// hand joints
// Hands
enum HandsSide { LEFT, RIGHT, COUNT };

enum XrHandJoint {
	HAND_JOINT_PALM = 0,
	HAND_JOINT_WRIST = 1,
	HAND_JOINT_THUMB_METACARPAL = 2,
	HAND_JOINT_THUMB_PROXIMAL = 3,
	HAND_JOINT_THUMB_DISTAL = 4,
	HAND_JOINT_THUMB_TIP = 5,
	HAND_JOINT_INDEX_METACARPAL = 6,
	HAND_JOINT_INDEX_PROXIMAL = 7,
	HAND_JOINT_INDEX_INTERMEDIATE = 8,
	HAND_JOINT_INDEX_DISTAL = 9,
	HAND_JOINT_INDEX_TIP = 10,
	HAND_JOINT_MIDDLE_METACARPAL = 11,
	HAND_JOINT_MIDDLE_PROXIMAL = 12,
	HAND_JOINT_MIDDLE_INTERMEDIATE = 13,
	HAND_JOINT_MIDDLE_DISTAL = 14,
	HAND_JOINT_MIDDLE_TIP = 15,
	HAND_JOINT_RING_METACARPAL = 16,
	HAND_JOINT_RING_PROXIMAL = 17,
	HAND_JOINT_RING_INTERMEDIATE = 18,
	HAND_JOINT_RING_DISTAL = 19,
	HAND_JOINT_RING_TIP = 20,
	HAND_JOINT_LITTLE_METACARPAL = 21,
	HAND_JOINT_LITTLE_PROXIMAL = 22,
	HAND_JOINT_LITTLE_INTERMEDIATE = 23,
	HAND_JOINT_LITTLE_DISTAL = 24,
	HAND_JOINT_LITTLE_TIP = 25,
	HAND_JOINT_MAX_ENUM = 0x7FFFFFFF
};

bool IsHandJointActive(HandsSide hand);
Mat4 GetHandJointPose(HandsSide hand, XrHandJoint handJoint);
float GetHandJointRadius(HandsSide hand, XrHandJoint handJoint);
Vec3 GetHandJointLinearVelocity(HandsSide hand, XrHandJoint handJoint);
Vec3 GetHandJointAngularVelocity(HandsSide hand, XrHandJoint handJoint);

} // namespace hg
