// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include "engine/openvr_api.h"
#include "engine/scene.h"

#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/math.h"

#include "platform/input_system.h"

#include <array>
#include <vector>

#if HG_ENABLE_OPENVR_API

#include <openvr.h>

namespace hg {

static vr::IVRSystem *vr_system = nullptr;

struct OpenVRTrackedDeviceState {
	bool connected;
	Mat4 mtx;
	vr::VRControllerState_t state;
	vr::ETrackedDeviceClass device_class;
	uint16_t haptic_pulse;
};

static std::array<OpenVRTrackedDeviceState, vr::k_unMaxTrackedDeviceCount> openvr_tracked_device_states = {};

static uint32_t rt_width = 0, rt_height = 0;

static Mat44 OVRToMat44(const vr::HmdMatrix44_t &m) {
	static const Mat44 VR_to_gs(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1);
	return /*VR_to_gs * */ Mat44(m.m[0][0], m.m[1][0], m.m[2][0], m.m[3][0], m.m[0][1], m.m[1][1], m.m[2][1], m.m[3][1], m.m[0][2], m.m[1][2], m.m[2][2],
			   m.m[3][2], m.m[0][3], m.m[1][3], m.m[2][3], m.m[3][3]) *
		   VR_to_gs;
}

static Mat4 OVRToMat4(const vr::HmdMatrix34_t &m) {
	static const Mat4 VR_to_gs(1, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0, 0);
	return VR_to_gs * Mat4(m.m[0][0], m.m[1][0], m.m[2][0], m.m[0][1], m.m[1][1], m.m[2][1], m.m[0][2], m.m[1][2], m.m[2][2], m.m[0][3], m.m[1][3], m.m[2][3]) *
		   VR_to_gs;
}

static std::string GetStringTrackedDeviceProperty_(
	vr::IVRSystem *system, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = nullptr) {
	const auto len = system->GetStringTrackedDeviceProperty(unDevice, prop, nullptr, 0, peError);
	if (!len)
		return {};

	std::string buffer(len, 'x');
	system->GetStringTrackedDeviceProperty(unDevice, prop, &buffer[0], len, peError);
	return buffer;
}

//
template <int idx> VRControllerState OpenVRControllerReader() {
	const auto &vr_state = openvr_tracked_device_states[idx];

	VRControllerState state;

	state.connected = vr_state.device_class == vr::TrackedDeviceClass::TrackedDeviceClass_Controller ? vr_state.connected : false;
	state.world = vr_state.mtx;

	state.pressed[VRCB_DPad_Up] = vr_state.state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_DPad_Up);
	state.pressed[VRCB_DPad_Down] = vr_state.state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_DPad_Down);
	state.pressed[VRCB_DPad_Left] = vr_state.state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_DPad_Left);
	state.pressed[VRCB_DPad_Right] = vr_state.state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_DPad_Right);
	state.pressed[VRCB_System] = vr_state.state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_System);
	state.pressed[VRCB_AppMenu] = vr_state.state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_ApplicationMenu);
	state.pressed[VRCB_Grip] = vr_state.state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_Grip);
	state.pressed[VRCB_A] = vr_state.state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_A);
	state.pressed[VRCB_ProximitySensor] = vr_state.state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_ProximitySensor);
	state.pressed[VRCB_Axis0] = vr_state.state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_Axis0);
	state.pressed[VRCB_Axis1] = vr_state.state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_Axis1);
	state.pressed[VRCB_Axis2] = vr_state.state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_Axis2);
	state.pressed[VRCB_Axis3] = vr_state.state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_Axis3);
	state.pressed[VRCB_Axis4] = vr_state.state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_Axis4);

	state.touched[VRCB_DPad_Up] = vr_state.state.ulButtonTouched & vr::ButtonMaskFromId(vr::k_EButton_DPad_Up);
	state.touched[VRCB_DPad_Down] = vr_state.state.ulButtonTouched & vr::ButtonMaskFromId(vr::k_EButton_DPad_Down);
	state.touched[VRCB_DPad_Left] = vr_state.state.ulButtonTouched & vr::ButtonMaskFromId(vr::k_EButton_DPad_Left);
	state.touched[VRCB_DPad_Right] = vr_state.state.ulButtonTouched & vr::ButtonMaskFromId(vr::k_EButton_DPad_Right);
	state.touched[VRCB_System] = vr_state.state.ulButtonTouched & vr::ButtonMaskFromId(vr::k_EButton_System);
	state.touched[VRCB_AppMenu] = vr_state.state.ulButtonTouched & vr::ButtonMaskFromId(vr::k_EButton_ApplicationMenu);
	state.touched[VRCB_Grip] = vr_state.state.ulButtonTouched & vr::ButtonMaskFromId(vr::k_EButton_Grip);
	state.touched[VRCB_A] = vr_state.state.ulButtonTouched & vr::ButtonMaskFromId(vr::k_EButton_A);
	state.touched[VRCB_ProximitySensor] = vr_state.state.ulButtonTouched & vr::ButtonMaskFromId(vr::k_EButton_ProximitySensor);
	state.touched[VRCB_Axis0] = vr_state.state.ulButtonTouched & vr::ButtonMaskFromId(vr::k_EButton_Axis0);
	state.touched[VRCB_Axis1] = vr_state.state.ulButtonTouched & vr::ButtonMaskFromId(vr::k_EButton_Axis1);
	state.touched[VRCB_Axis2] = vr_state.state.ulButtonTouched & vr::ButtonMaskFromId(vr::k_EButton_Axis2);
	state.touched[VRCB_Axis3] = vr_state.state.ulButtonTouched & vr::ButtonMaskFromId(vr::k_EButton_Axis3);
	state.touched[VRCB_Axis4] = vr_state.state.ulButtonTouched & vr::ButtonMaskFromId(vr::k_EButton_Axis4);

	state.surface[0] = {vr_state.state.rAxis[0].x, vr_state.state.rAxis[0].y};
	state.surface[1] = {vr_state.state.rAxis[1].x, vr_state.state.rAxis[1].y};
	state.surface[2] = {vr_state.state.rAxis[2].x, vr_state.state.rAxis[2].y};
	state.surface[3] = {vr_state.state.rAxis[3].x, vr_state.state.rAxis[3].y};
	state.surface[4] = {vr_state.state.rAxis[4].x, vr_state.state.rAxis[4].y};
	return state;
}

template <int idx> void OpenVRControllerSendHapticPulse(time_ns duration) {
	openvr_tracked_device_states[idx].haptic_pulse = numeric_cast<uint16_t>(Min<int64_t>(time_to_us(duration), 65535));
}

template <int idx> VRGenericTrackerState OpenVRGenericTrackerReader() {
	const auto &vr_state = openvr_tracked_device_states[idx];

	VRGenericTrackerState state;

	state.connected = vr_state.device_class == vr::TrackedDeviceClass::TrackedDeviceClass_GenericTracker ? vr_state.connected : false;
	state.world = vr_state.mtx;

	return state;
}

//
bool OpenVRInit() {
	if (vr_system)
		return true; // setup already

	vr::EVRInitError eError = vr::VRInitError_None;
	vr_system = vr::VR_Init(&eError, vr::VRApplication_Scene);

	if (!vr_system) {
		warn(format("OpenVR initialization failed: %1").arg(vr::VR_GetVRInitErrorAsEnglishDescription(eError)));
		return false; // initialization failure
	}

	const auto driver = GetStringTrackedDeviceProperty_(vr_system, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
	const auto display = GetStringTrackedDeviceProperty_(vr_system, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);

	log(format("OpenVR driver %2 initialized on display %1").arg(display).arg(driver));

	vr_system->GetRecommendedRenderTargetSize(&rt_width, &rt_height);
	log(format("OpenVR recommended render target size %1x%2").arg(rt_width).arg(rt_height));

	AddVRControllerReader("openvr_controller_0", OpenVRControllerReader<0>, OpenVRControllerSendHapticPulse<0>);
	AddVRControllerReader("openvr_controller_1", OpenVRControllerReader<1>, OpenVRControllerSendHapticPulse<1>);
	AddVRControllerReader("openvr_controller_2", OpenVRControllerReader<2>, OpenVRControllerSendHapticPulse<2>);
	AddVRControllerReader("openvr_controller_3", OpenVRControllerReader<3>, OpenVRControllerSendHapticPulse<3>);
	AddVRControllerReader("openvr_controller_4", OpenVRControllerReader<4>, OpenVRControllerSendHapticPulse<4>);
	AddVRControllerReader("openvr_controller_5", OpenVRControllerReader<5>, OpenVRControllerSendHapticPulse<5>);
	AddVRControllerReader("openvr_controller_6", OpenVRControllerReader<6>, OpenVRControllerSendHapticPulse<6>);
	AddVRControllerReader("openvr_controller_7", OpenVRControllerReader<7>, OpenVRControllerSendHapticPulse<7>);
	AddVRControllerReader("openvr_controller_8", OpenVRControllerReader<8>, OpenVRControllerSendHapticPulse<8>);
	AddVRControllerReader("openvr_controller_9", OpenVRControllerReader<9>, OpenVRControllerSendHapticPulse<9>);
	AddVRControllerReader("openvr_controller_10", OpenVRControllerReader<10>, OpenVRControllerSendHapticPulse<10>);
	AddVRControllerReader("openvr_controller_11", OpenVRControllerReader<11>, OpenVRControllerSendHapticPulse<11>);
	AddVRControllerReader("openvr_controller_12", OpenVRControllerReader<12>, OpenVRControllerSendHapticPulse<12>);
	AddVRControllerReader("openvr_controller_13", OpenVRControllerReader<13>, OpenVRControllerSendHapticPulse<13>);
	AddVRControllerReader("openvr_controller_14", OpenVRControllerReader<14>, OpenVRControllerSendHapticPulse<14>);
	AddVRControllerReader("openvr_controller_15", OpenVRControllerReader<15>, OpenVRControllerSendHapticPulse<15>);

	AddVRGenericTrackerReader("openvr_generic_tracker_0", OpenVRGenericTrackerReader<0>);
	AddVRGenericTrackerReader("openvr_generic_tracker_1", OpenVRGenericTrackerReader<1>);
	AddVRGenericTrackerReader("openvr_generic_tracker_2", OpenVRGenericTrackerReader<2>);
	AddVRGenericTrackerReader("openvr_generic_tracker_3", OpenVRGenericTrackerReader<3>);
	AddVRGenericTrackerReader("openvr_generic_tracker_4", OpenVRGenericTrackerReader<4>);
	AddVRGenericTrackerReader("openvr_generic_tracker_5", OpenVRGenericTrackerReader<5>);
	AddVRGenericTrackerReader("openvr_generic_tracker_6", OpenVRGenericTrackerReader<6>);
	AddVRGenericTrackerReader("openvr_generic_tracker_7", OpenVRGenericTrackerReader<7>);
	AddVRGenericTrackerReader("openvr_generic_tracker_8", OpenVRGenericTrackerReader<8>);
	AddVRGenericTrackerReader("openvr_generic_tracker_9", OpenVRGenericTrackerReader<9>);
	AddVRGenericTrackerReader("openvr_generic_tracker_10", OpenVRGenericTrackerReader<10>);
	AddVRGenericTrackerReader("openvr_generic_tracker_11", OpenVRGenericTrackerReader<11>);
	AddVRGenericTrackerReader("openvr_generic_tracker_12", OpenVRGenericTrackerReader<12>);
	AddVRGenericTrackerReader("openvr_generic_tracker_13", OpenVRGenericTrackerReader<13>);
	AddVRGenericTrackerReader("openvr_generic_tracker_14", OpenVRGenericTrackerReader<14>);
	AddVRGenericTrackerReader("openvr_generic_tracker_15", OpenVRGenericTrackerReader<15>);
	return true;
}

void OpenVRShutdown() {
	log("OpenVR shutting down");

	RemoveVRControllerReader("openvr_controller_0");
	RemoveVRControllerReader("openvr_controller_1");
	RemoveVRControllerReader("openvr_controller_2");
	RemoveVRControllerReader("openvr_controller_3");
	RemoveVRControllerReader("openvr_controller_4");
	RemoveVRControllerReader("openvr_controller_5");
	RemoveVRControllerReader("openvr_controller_6");
	RemoveVRControllerReader("openvr_controller_7");
	RemoveVRControllerReader("openvr_controller_8");
	RemoveVRControllerReader("openvr_controller_9");
	RemoveVRControllerReader("openvr_controller_10");
	RemoveVRControllerReader("openvr_controller_11");
	RemoveVRControllerReader("openvr_controller_12");
	RemoveVRControllerReader("openvr_controller_13");
	RemoveVRControllerReader("openvr_controller_14");
	RemoveVRControllerReader("openvr_controller_15");

	RemoveVRGenericTrackerReader("openvr_generic_tracker_0");
	RemoveVRGenericTrackerReader("openvr_generic_tracker_1");
	RemoveVRGenericTrackerReader("openvr_generic_tracker_2");
	RemoveVRGenericTrackerReader("openvr_generic_tracker_3");
	RemoveVRGenericTrackerReader("openvr_generic_tracker_4");
	RemoveVRGenericTrackerReader("openvr_generic_tracker_5");
	RemoveVRGenericTrackerReader("openvr_generic_tracker_6");
	RemoveVRGenericTrackerReader("openvr_generic_tracker_7");
	RemoveVRGenericTrackerReader("openvr_generic_tracker_8");
	RemoveVRGenericTrackerReader("openvr_generic_tracker_9");
	RemoveVRGenericTrackerReader("openvr_generic_tracker_10");
	RemoveVRGenericTrackerReader("openvr_generic_tracker_11");
	RemoveVRGenericTrackerReader("openvr_generic_tracker_12");
	RemoveVRGenericTrackerReader("openvr_generic_tracker_13");
	RemoveVRGenericTrackerReader("openvr_generic_tracker_14");
	RemoveVRGenericTrackerReader("openvr_generic_tracker_15");

	vr::VR_Shutdown();
	vr_system = nullptr;
}

//
iVec2 OpenVRGetFrameBufferSize() { 
	return iVec2(rt_width, rt_height);
}

//
OpenVRState OpenVRGetState(const Mat4 &body, float znear, float zfar) {
	vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
	vr::VRCompositor()->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);

	OpenVRState state;
	state.body = body;

	const auto &hmd_pose = m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd];

	if (hmd_pose.bPoseIsValid) {
		state.head = body * OVRToMat4(hmd_pose.mDeviceToAbsoluteTracking);
		state.inv_head = InverseFast(state.head);
	}

	state.width = rt_width;
	state.height = rt_height;

	state.left.offset = OVRToMat4(vr_system->GetEyeToHeadTransform(vr::Eye_Left));
	state.right.offset = OVRToMat4(vr_system->GetEyeToHeadTransform(vr::Eye_Right));
	state.left.projection = OVRToMat44(vr_system->GetProjectionMatrix(vr::Eye_Left, znear, zfar));
	state.right.projection = OVRToMat44(vr_system->GetProjectionMatrix(vr::Eye_Right, znear, zfar));

	for (vr::TrackedDeviceIndex_t unTrackedDevice = vr::k_unTrackedDeviceIndex_Hmd + 1; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; ++unTrackedDevice) {
		auto &vr_state = openvr_tracked_device_states[unTrackedDevice];

		vr_state.connected = vr_system->IsTrackedDeviceConnected(unTrackedDevice);
		vr_state.device_class = vr_system->GetTrackedDeviceClass(unTrackedDevice);
		vr_system->GetControllerState(unTrackedDevice, &vr_state.state, sizeof(vr::VRControllerState_t));

		if (m_rTrackedDevicePose[unTrackedDevice].bPoseIsValid)
			vr_state.mtx = body * OVRToMat4(m_rTrackedDevicePose[unTrackedDevice].mDeviceToAbsoluteTracking);

		if (vr_state.haptic_pulse > 0)
			vr_system->TriggerHapticPulse(unTrackedDevice, 0, vr_state.haptic_pulse);
		vr_state.haptic_pulse = 0;
	}

	return state;
}

void OpenVRSubmitFrame(void *left_eye_texture, void *right_eye_texture) {
	if (!vr::VRCompositor())
		return;

	vr::Texture_t leftEyeTexture = {left_eye_texture, vr::TextureType_DirectX, vr::ColorSpace_Gamma};
	vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
	vr::Texture_t rightEyeTexture = {right_eye_texture, vr::TextureType_DirectX, vr::ColorSpace_Gamma};
	vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
}

static const uint64_t ovraa_flags[] = {0, BGFX_TEXTURE_RT_MSAA_X2, BGFX_TEXTURE_RT_MSAA_X4, BGFX_TEXTURE_RT_MSAA_X8, BGFX_TEXTURE_RT_MSAA_X16};

//
OpenVREyeFrameBuffer OpenVRCreateEyeFrameBuffer(OpenVRAA aa) {
	OpenVREyeFrameBuffer eye;
	eye.aa = aa;

	eye.color = bgfx::createTexture2D(rt_width, rt_height, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_RT | ovraa_flags[eye.aa]);

	bgfx::frame(); // so that the texture gets created

	eye.native = bgfx::overrideInternal(eye.color, rt_width, rt_height, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_RT | ovraa_flags[eye.aa]);
	eye.depth = bgfx::createTexture2D(rt_width, rt_height, false, 1, bgfx::TextureFormat::D24, BGFX_TEXTURE_RT_WRITE_ONLY | ovraa_flags[eye.aa]);

	bgfx::TextureHandle h[2] = {eye.color, eye.depth};
	eye.fb = bgfx::createFrameBuffer(2, h, false);

	return eye;
}

void OpenVRDestroyEyeFrameBuffer(OpenVREyeFrameBuffer &eye_fb) {
	if (eye_fb.fb.idx != bgfx::kInvalidHandle)
		bgfx::destroy(eye_fb.fb);
	eye_fb.fb = {bgfx::kInvalidHandle};

	if (eye_fb.color.idx != bgfx::kInvalidHandle)
		bgfx::destroy(eye_fb.color);
	eye_fb.color = {bgfx::kInvalidHandle};

	if (eye_fb.depth.idx != bgfx::kInvalidHandle)
		bgfx::destroy(eye_fb.depth);
	eye_fb.depth = {bgfx::kInvalidHandle};

	if (eye_fb.resolve.idx != bgfx::kInvalidHandle)
		bgfx::destroy(eye_fb.resolve);
	eye_fb.resolve = {bgfx::kInvalidHandle};

	eye_fb.native = 0;
}

void OpenVRSubmitFrame(const OpenVREyeFrameBuffer &left, const OpenVREyeFrameBuffer &right) { OpenVRSubmitFrame((void *)left.native, (void *)right.native); }

//
void OpenVRStateToViewState(const OpenVRState &state, ViewState &left, ViewState &right) {
	left.view = InverseFast(state.head * state.left.offset);
	left.proj = state.left.projection;
	left.frustum = MakeFrustum(state.left.projection * left.view);

	right.view = InverseFast(state.head * state.right.offset);
	right.proj = state.right.projection;
	right.frustum = MakeFrustum(state.right.projection * right.view);
}

void OpenVRPostPresentHandoff() { vr::VRCompositor()->PostPresentHandoff(); }

Texture OpenVRGetColorTexture(const OpenVREyeFrameBuffer &eye) { return {BGFX_TEXTURE_RT | ovraa_flags[eye.aa], eye.color}; }
Texture OpenVRGetDepthTexture(const OpenVREyeFrameBuffer &eye) { return {BGFX_TEXTURE_RT_WRITE_ONLY | ovraa_flags[eye.aa], eye.depth}; }

} // namespace hg

#else

namespace hg {

bool OpenVRInit() {
	warn("OpenVR support DISABLED when building Harfang");
	return false;
}

void OpenVRShutdown() {}

OpenVREyeFrameBuffer OpenVRCreateEyeFrameBuffer(OpenVRAA aa) { return {}; }
void OpenVRDestroyEyeFrameBuffer(OpenVREyeFrameBuffer &) {}

iVec2 OpenVRGetFrameBufferSize() { return iVec2::Zero; }

OpenVRState OpenVRGetState(const Mat4 &, float, float) { return {}; }
void OpenVRStateToViewState(const OpenVRState &, ViewState &, ViewState &) {}

void OpenVRResolveMSAA(bgfx::ViewId, const OpenVREyeFrameBuffer &, const OpenVREyeFrameBuffer &) {}

void OpenVRSubmitFrame(void *, void *) {}
void OpenVRSubmitFrame(const OpenVREyeFrameBuffer &, const OpenVREyeFrameBuffer &) {}

void OpenVRPostPresentHandoff() {}

Texture OpenVRGetColorTexture(const OpenVREyeFrameBuffer &eye) { return {}; }
Texture OpenVRGetDepthTexture(const OpenVREyeFrameBuffer &eye) { return {}; }

} // namespace hg

#endif
