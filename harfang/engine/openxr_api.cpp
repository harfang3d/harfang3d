// HARFANG(R) Copyright (C) 2019 Emmanuel Julien, Movida Production. Released under GPL/LGPL/Commercial Licence, see licence.txt.

// TODO:
// * add holographic support extension
// * add haptic force
// * Test in dx12
// * Find a way to get the information needed for Vulkan
// * CONS:
//		* the extension eye gaze return only 1 matrix, it's the focus eye 3D point
//
// DOC : https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html

#include <../src/bgfx_p.h>
#include <bgfx/platform.h>

#include "engine/openxr_api.h"
#include "engine/scene.h"

#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/math.h"
#include "foundation/matrix3.h"
#include "foundation/projection.h"
#include "foundation/quaternion.h"

#include "platform/input_system.h"

#include <array>
#include <vector>

#if HG_ENABLE_OPENXR_API
#include <list>
#include <stdexcept>
//
// Graphics Headers
//
#ifdef XR_USE_GRAPHICS_API_D3D11
#include <d3d11_4.h>
#endif

#ifdef XR_USE_GRAPHICS_API_D3D12
#include <d3d12.h>
#endif

#ifdef XR_USE_GRAPHICS_API_OPENGL
#if defined(XR_USE_PLATFORM_XLIB) || defined(XR_USE_PLATFORM_XCB)
#include <GL/glx.h>
#endif
#ifdef XR_USE_PLATFORM_XCB
#include <xcb/glx.h>
#endif
#ifdef XR_USE_PLATFORM_WIN32
#include <GL/gl.h>
#include <wingdi.h> // For HGLRC
#endif
#endif

#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
#include <EGL/egl.h>
#endif

#ifdef XR_USE_GRAPHICS_API_VULKAN
#ifdef XR_USE_PLATFORM_WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#ifdef XR_USE_PLATFORM_ANDROID
#define VK_USE_PLATFORM_ANDROID_KHR
#endif
#include <../src/renderer_vk.h>
#include <vulkan/vulkan.h>
#endif

//
// OpenXR Headers
//
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_reflection.h>
#include <openxr/src/common/xr_linear.h>

#define XR_ENUM_CASE_STR(name, val) case name: return #name;
#define XR_ENUM_STR(enumType)                         \
    constexpr const char* XrEnumStr(enumType e) {     \
        switch (e) {                                  \
            XR_LIST_ENUM_##enumType(XR_ENUM_CASE_STR) \
            default: return "Unknown";                \
        }                                             \
    }
XR_ENUM_STR(XrResult);

inline XrResult _CheckXrResult(XrResult res, const char *originator = nullptr, const char *sourceLocation = nullptr) {
	if (XR_FAILED(res))
		hg::debug(hg::format("XrResult failure [%1], from: %2, at %3").arg(XrEnumStr(res)).arg(originator).arg(sourceLocation));
	return res;
}

#define CHK_STRINGIFY(x) #x
#define TOSTRING(x) CHK_STRINGIFY(x)
#define FILE_AND_LINE __FILE__ ":" TOSTRING(__LINE__)
#define CHECK_XRCMD(cmd) _CheckXrResult(cmd, #cmd, FILE_AND_LINE)

namespace hg {
static std::array<const char*, 7> OpenXRExtensionName = {
	"", //
	XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME, //
	XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME, //
	XR_FB_PASSTHROUGH_EXTENSION_NAME, //
	XR_EXT_HAND_TRACKING_EXTENSION_NAME, //
	XR_VARJO_QUAD_VIEWS_EXTENSION_NAME, //
	XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME //
};
OpenXRExtensionsFlags ExtensionsFlagsEnable;

static XrInstance m_instance{XR_NULL_HANDLE};
static XrSession m_session{XR_NULL_HANDLE};
static XrSpace m_appSpace{XR_NULL_HANDLE}, m_appLocalSpace{XR_NULL_HANDLE}, m_appViewSpace{XR_NULL_HANDLE};
static XrFormFactor m_formFactor{XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY};
static XrViewConfigurationType m_viewConfigType{XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};
static XrEnvironmentBlendMode m_environmentBlendMode{XR_ENVIRONMENT_BLEND_MODE_OPAQUE};
static XrSystemId m_systemId{XR_NULL_SYSTEM_ID};

static XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
static XrCompositionLayerPassthroughFB layerPasstrough{XR_TYPE_COMPOSITION_LAYER_PASSTHROUGH_FB};
static std::vector<XrCompositionLayerProjectionView> projectionLayerViews;
static std::vector<XrCompositionLayerDepthInfoKHR> depthLayerViews;

std::string FormFactor{"Hmd"};
std::string ViewConfiguration{"Stereo"};
std::string EnvironmentBlendMode{"Opaque"};
std::string AppSpace{"Stage"};

struct Swapchain {
	XrSwapchain handle, handleDepth;
	int32_t width, height;
};

std::vector<XrViewConfigurationView> m_configViews;
std::vector<Swapchain> m_swapchains;
std::map<XrSwapchain, std::vector<XrSwapchainImageBaseHeader*>> m_swapchainImages, m_swapchainDepthImages;
std::map<XrSwapchain, std::vector<OpenXREyeFrameBuffer>> m_swapchainEyesFramebuffer;
std::vector<XrView> m_views;
int64_t m_colorSwapchainFormat{-1}, m_depthSwapchainFormat{-1};

std::vector<XrSpace> m_visualizedSpaces;

// Application's current lifecycle state according to the runtime
XrSessionState m_sessionState{XR_SESSION_STATE_UNKNOWN};
bool m_sessionRunning{false};

XrEventDataBuffer m_eventDataBuffer;

struct InputState {
	Mat4 head_pose;
	bool head_pose_available;
	XrActionSet actionSet{XR_NULL_HANDLE};
	XrAction squeezeAction{XR_NULL_HANDLE};
	XrAction triggerValueAction{XR_NULL_HANDLE};
	XrAction triggerClickAction{XR_NULL_HANDLE};
	XrAction surfaceXAction{XR_NULL_HANDLE};
	XrAction surfaceYAction{XR_NULL_HANDLE};
	XrAction poseAction{XR_NULL_HANDLE};
	XrAction gazeAction{XR_NULL_HANDLE};
	XrAction vibrateAction{XR_NULL_HANDLE};
	XrAction quitAction{XR_NULL_HANDLE};
	std::array<XrPath, HandsSide::COUNT> handSubactionPath;
	std::array<XrSpace, HandsSide::COUNT> handSpace;
	std::array<XrPath, 14> trackerSubactionPath;
	std::array<XrAction, 14> trackerPoseAction;
	std::array<XrSpace, 14> trackerSpace;
	XrSpace gazeSpace;
	Mat4 eye_gaze;
	bool eye_gaze_available;
	std::array<float, HandsSide::COUNT> handScale = {{1.0f, 1.0f}};

	// hand joints
	PFN_xrLocateHandJointsEXT pfnLocateHandJointsEXT;
	std::array<XrHandTrackerEXT, HandsSide::COUNT> handJointTracker;
	std::array<XrHandJointLocationEXT, XR_HAND_JOINT_COUNT_EXT> jointLocation[HandsSide::COUNT];
	std::array<XrHandJointLocationsEXT, HandsSide::COUNT> jointLocations;
	std::array<XrHandJointVelocityEXT, XR_HAND_JOINT_COUNT_EXT> jointVelocitie[HandsSide::COUNT];
	std::array<XrHandJointVelocitiesEXT, HandsSide::COUNT> jointVelocities;
};

InputState m_input;

struct OpenXRTrackedDeviceState {
	bool connected;
	Mat4 mtx;
	Vec2 surface;
	float triggerValue;
	bool triggerClick;
	uint16_t haptic_pulse;
};

static std::array<OpenXRTrackedDeviceState, 2> OpenXR_hand_device_states = {};
static std::array<OpenXRTrackedDeviceState, 14> OpenXR_tracker_device_states = {};

static uint32_t rt_width = 0, rt_height = 0;

static Mat44 OXRToMat44(const XrMatrix4x4f &m) {
	static Mat44 VR_to_HG(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1);
	return /*VR_to_HG * */ Mat44(m.m[0], m.m[1], m.m[2], m.m[3], m.m[4], m.m[5], m.m[6], m.m[7], m.m[8], m.m[9], m.m[10], m.m[11], m.m[12], m.m[13], m.m[14],
	                             m.m[15]) * VR_to_HG;
}

static Mat4 OXRToMat4(const XrMatrix4x4f &m) {
	static Mat4 VR_to_HG(1, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0, 0);
	return VR_to_HG * Mat4(m.m[0], m.m[1], m.m[2], m.m[4], m.m[5], m.m[6], m.m[8], m.m[9], m.m[10], m.m[12], m.m[13], m.m[14]) * VR_to_HG;
}

template <int idx>
VRControllerState OpenXRControllerReader() {
	const auto &xr_state = OpenXR_hand_device_states[idx];

	VRControllerState state;

	state.connected = xr_state.connected;
	state.world = xr_state.mtx;

	state.surface[0] = {xr_state.surface.x, xr_state.surface.y};
	state.pressed[VRCB_Axis1] = xr_state.triggerClick;
	state.surface[1] = {xr_state.triggerValue, 0};

	/*
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
			*/
	return state;
}

template <int idx>
void OpenXRControllerSendHapticPulse(time_ns duration) {
	OpenXR_hand_device_states[idx].haptic_pulse = numeric_cast<uint16_t>(Min<int64_t>(time_to_us(duration), 65535));
}

template <int idx>
VRGenericTrackerState OpenXRGenericTrackerReader() {
	const auto &vr_state = OpenXR_tracker_device_states[idx];

	VRGenericTrackerState state;
	state.connected = vr_state.connected;
	state.world = vr_state.mtx;

	return state;
}

inline std::string GetXrVersionString(XrVersion ver) {
	return format("%1.%2.%3").arg(XR_VERSION_MAJOR(ver)).arg(XR_VERSION_MINOR(ver)).arg(XR_VERSION_PATCH(ver));
}

bool LogLayersAndExtensions() {
	// Write out extension properties for a given layer.
	const auto logExtensions = [](const char *layerName, int indent = 0) {
		uint32_t instanceExtensionCount;
		if (CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layerName, 0, &instanceExtensionCount, nullptr)) < 0)
			return false;

		std::vector<XrExtensionProperties> extensionsAvailable;
		extensionsAvailable.resize(instanceExtensionCount);
		for (XrExtensionProperties &extension : extensionsAvailable) {
			extension.type = XR_TYPE_EXTENSION_PROPERTIES;
		}

		if (CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layerName, (uint32_t)extensionsAvailable.size(), & instanceExtensionCount, extensionsAvailable.data())) < 0)
			return false;

		const std::string indentStr(indent, ' ');
		debug(format("%1Available Extensions: (%2)").arg(indentStr.c_str()).arg(instanceExtensionCount));
		for (const XrExtensionProperties &extension : extensionsAvailable) {
			debug(format("%1  Name=%2 SpecVersion=%3").arg(indentStr.c_str()).arg(extension.extensionName).arg(extension.extensionVersion));
		}
		return true;
	};
	// Log non-layer extensions (layerName==nullptr).
	logExtensions(nullptr);

	// Log layers and any of their extensions.
	{
		uint32_t layerCount;
		if (CHECK_XRCMD(xrEnumerateApiLayerProperties(0, &layerCount, nullptr)) < 0)
			return false;

		std::vector<XrApiLayerProperties> layers(layerCount);
		for (XrApiLayerProperties &layer : layers) {
			layer.type = XR_TYPE_API_LAYER_PROPERTIES;
		}

		if (CHECK_XRCMD(xrEnumerateApiLayerProperties((uint32_t)layers.size(), &layerCount, layers.data())) < 0)
			return false;

		debug(format("Available Layers: (%1)").arg(layerCount));
		for (const XrApiLayerProperties &layer : layers) {
			debug(format("  Name=%1 SpecVersion=%2 LayerVersion=%3 Description=%4").arg(layer.layerName).arg(GetXrVersionString(layer.specVersion).c_str()).
			                                                                        arg(layer.layerVersion).arg(layer.description));
			logExtensions(layer.layerName, 4);
		}
	}
	return true;
}

std::string OpenXRGetInstanceInfo() {
	XrInstanceProperties instanceProperties{XR_TYPE_INSTANCE_PROPERTIES};
	if (CHECK_XRCMD(xrGetInstanceProperties(m_instance, &instanceProperties)) < 0)
		return "no openxr instance";

	return instanceProperties.runtimeName;
}

bool LogInstanceInfo() {

	XrInstanceProperties instanceProperties{XR_TYPE_INSTANCE_PROPERTIES};
	if (CHECK_XRCMD(xrGetInstanceProperties(m_instance, &instanceProperties)) < 0)
		return false;

	debug(format("Instance RuntimeName=%1 RuntimeVersion=%2").arg(instanceProperties.runtimeName).arg(GetXrVersionString(instanceProperties.runtimeVersion).c_str()));
	return true;
}

bool CreateInstanceInternal() {
	// Create union of extensions required by platform and graphics plugins.
	std::vector<const char*> extensions;

	// Transform platform and graphics extension std::strings to C strings.
	const std::vector<std::string> platformExtensions = {}; // different for android
	std::transform(platformExtensions.begin(), platformExtensions.end(), std::back_inserter(extensions), [](const std::string &ext) { return ext.c_str(); });

	switch (bgfx::getRendererType()) {
#ifdef XR_USE_GRAPHICS_API_D3D11
		case bgfx::RendererType::Direct3D11: extensions.push_back(XR_KHR_D3D11_ENABLE_EXTENSION_NAME);
			break;
#endif
#ifdef XR_USE_GRAPHICS_API_D3D12
		case bgfx::RendererType::Direct3D12:
			extensions.push_back(XR_KHR_D3D12_ENABLE_EXTENSION_NAME);
			break;
#endif
		case bgfx::RendererType::OpenGL: extensions.push_back(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);
			break;
#ifdef XR_USE_GRAPHICS_API_VULKAN
		case bgfx::RendererType::Vulkan:
			extensions.push_back(XR_KHR_VULKAN_ENABLE_EXTENSION_NAME);
			break;
#endif
	}

	// get available extensions
	uint32_t instanceExtensionCount;
	CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(nullptr, 0, &instanceExtensionCount, nullptr));

	std::vector<XrExtensionProperties> extensionsAvailable;
	extensionsAvailable.resize(instanceExtensionCount);
	for (XrExtensionProperties &extension : extensionsAvailable)
		extension.type = XR_TYPE_EXTENSION_PROPERTIES;

	CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(nullptr, (uint32_t)extensionsAvailable.size(), &instanceExtensionCount, extensionsAvailable.data()));

	uint16_t ExtensionsFlagsEnableAvailable = 0;
	for (int i = 1; i < OpenXRExtensionName.size(); ++i) {
		if (ExtensionsFlagsEnable & 1 << i) {
			// to handle the htc vive tracker device
			if (std::find_if(extensionsAvailable.begin(), extensionsAvailable.end(), [=](const XrExtensionProperties &item) {
				return strcmp(item.extensionName, OpenXRExtensionName[i]) == 0;
			}) != extensionsAvailable.end()) {
				extensions.push_back(OpenXRExtensionName[i]);
				ExtensionsFlagsEnableAvailable |= 1 << i;
			}
		}
	}
	// save current available and active extensions
	ExtensionsFlagsEnable = ExtensionsFlagsEnableAvailable;
	// special quad views
	if (ExtensionsFlagsEnable & OXRExtensions_VARJO_QUADVIEWS)
		ViewConfiguration = XR_VARJO_QUAD_VIEWS_EXTENSION_NAME;

	debug("extension found:");
	for (const auto &extension : extensions)
		debug(format("    Name=%1").arg(extension));

	XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
	createInfo.next = {}; // different for android
	createInfo.enabledExtensionCount = (uint32_t)extensions.size();
	createInfo.enabledExtensionNames = extensions.data();

	strcpy(createInfo.applicationInfo.applicationName, "Harfang");
	createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

	if (CHECK_XRCMD(xrCreateInstance(&createInfo, &m_instance)) < 0)
		return false;
	return true;
}

inline bool EqualsIgnoreCase(const std::string &s1, const std::string &s2, const std::locale &loc = std::locale()) {
	const std::ctype<char> &ctype = std::use_facet<std::ctype<char>>(loc);
	const auto compareCharLower = [&](char c1, char c2) { return ctype.tolower(c1) == ctype.tolower(c2); };
	return s1.size() == s2.size() && std::equal(s1.begin(), s1.end(), s2.begin(), compareCharLower);
}

inline XrFormFactor GetXrFormFactor(const std::string &formFactorStr) {
	if (EqualsIgnoreCase(formFactorStr, "Hmd")) {
		return XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	}
	if (EqualsIgnoreCase(formFactorStr, "Handheld")) {
		return XR_FORM_FACTOR_HANDHELD_DISPLAY;
	}
	throw std::invalid_argument(format("Unknown form factor '%1'").arg(formFactorStr.c_str()).str());
}

inline XrViewConfigurationType GetXrViewConfigurationType(const std::string &viewConfigurationStr) {
	if (EqualsIgnoreCase(viewConfigurationStr, "Mono")) {
		return XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO;
	}
	if (EqualsIgnoreCase(viewConfigurationStr, "Stereo")) {
		return XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
	}
	if (EqualsIgnoreCase(viewConfigurationStr, XR_VARJO_QUAD_VIEWS_EXTENSION_NAME)) {
		return XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO;
	}
	throw std::invalid_argument(format("Unknown view configuration '%1'").arg(viewConfigurationStr.c_str()).str());
}

inline XrEnvironmentBlendMode GetXrEnvironmentBlendMode(const std::string &environmentBlendModeStr) {
	if (EqualsIgnoreCase(environmentBlendModeStr, "Opaque")) {
		return XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
	}
	if (EqualsIgnoreCase(environmentBlendModeStr, "Additive")) {
		return XR_ENVIRONMENT_BLEND_MODE_ADDITIVE;
	}
	if (EqualsIgnoreCase(environmentBlendModeStr, "AlphaBlend")) {
		return XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND;
	}
	throw std::invalid_argument(format("Unknown environment blend mode '%1'").arg(environmentBlendModeStr.c_str()).str());
}

bool LogEnvironmentBlendMode(XrViewConfigurationType type) {
	uint32_t count;
	if (CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(m_instance, m_systemId, type, 0, &count, nullptr)) < 0)
		return false;

	debug(format("Available Environment Blend Mode count : (%1)").arg(count));

	std::vector<XrEnvironmentBlendMode> blendModes(count);
	if (CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(m_instance, m_systemId, type, count, &count, blendModes.data())) < 0)
		return false;

	bool blendModeFound = false;
	for (XrEnvironmentBlendMode mode : blendModes) {
		const bool blendModeMatch = (mode == m_environmentBlendMode);
		debug(format("Environment Blend Mode (%1) : %2").arg(to_string(mode)).arg(blendModeMatch ? "(Selected)" : ""));
		blendModeFound |= blendModeMatch;
	}
	if (!blendModeFound)
		m_environmentBlendMode = blendModes[0];

	return true;
}

bool LogViewConfigurations() {
	uint32_t viewConfigTypeCount;
	if (CHECK_XRCMD(xrEnumerateViewConfigurations(m_instance, m_systemId, 0, &viewConfigTypeCount, nullptr)) < 0)
		return false;

	std::vector<XrViewConfigurationType> viewConfigTypes(viewConfigTypeCount);
	if (CHECK_XRCMD(xrEnumerateViewConfigurations(m_instance, m_systemId, viewConfigTypeCount, &viewConfigTypeCount, viewConfigTypes.data())) < 0)
		return false;

	debug(format("Available View Configuration Types: (%1)").arg(viewConfigTypeCount));
	for (XrViewConfigurationType viewConfigType : viewConfigTypes) {
		debug(format("  View Configuration Type: %1 %2").arg(to_string(viewConfigType)).arg(viewConfigType == m_viewConfigType ? "(Selected)" : ""));

		XrViewConfigurationProperties viewConfigProperties{XR_TYPE_VIEW_CONFIGURATION_PROPERTIES};
		if (CHECK_XRCMD(xrGetViewConfigurationProperties(m_instance, m_systemId, viewConfigType, &viewConfigProperties)) < 0)
			return false;

		debug(format("  View configuration FovMutable=%1").arg(viewConfigProperties.fovMutable == XR_TRUE ? "True" : "False"));

		uint32_t viewCount;
		if (CHECK_XRCMD(xrEnumerateViewConfigurationViews(m_instance, m_systemId, viewConfigType, 0, &viewCount, nullptr)) < 0)
			return false;

		if (viewCount > 0) {
			std::vector<XrViewConfigurationView> views(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
			if (CHECK_XRCMD(xrEnumerateViewConfigurationViews(m_instance, m_systemId, viewConfigType, viewCount, &viewCount, views.data())) < 0)
				return false;

			for (uint32_t i = 0; i < views.size(); i++) {
				const XrViewConfigurationView &view = views[i];

				debug(format("    View [%1]: Recommended Width=%2 Height=%3 SampleCount=%4").arg(i).arg(view.recommendedImageRectWidth).arg(view.recommendedImageRectHeight).
				                                                                             arg(view.recommendedSwapchainSampleCount));
				debug(format("    View [%1]:     Maximum Width=%2 Height=%3 SampleCount=%4").arg(i).arg(view.maxImageRectWidth).arg(view.maxImageRectHeight).arg(
					view.maxSwapchainSampleCount));
			}
		} else {
			debug("Empty view configuration type");
		}

		if (!LogEnvironmentBlendMode(viewConfigType))
			return false;
	}
	return true;
}

bool InitializeSystem() {
	m_formFactor = GetXrFormFactor(FormFactor);
	m_viewConfigType = GetXrViewConfigurationType(ViewConfiguration);
	m_environmentBlendMode = GetXrEnvironmentBlendMode(EnvironmentBlendMode);

	XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
	systemInfo.formFactor = m_formFactor;
	if (CHECK_XRCMD(xrGetSystem(m_instance, &systemInfo, &m_systemId)) < 0)
		return false;

	debug(format("Using system %1 for form factor %2").arg(m_systemId).arg(to_string(m_formFactor)));

	if (!LogViewConfigurations())
		return false;

	return true;
}

namespace Math {
namespace Pose {
XrPosef Identity() {
	XrPosef t{};
	t.orientation.w = 1;
	return t;
}

XrPosef Translation(const XrVector3f &translation) {
	XrPosef t = Identity();
	t.position = translation;
	return t;
}

XrPosef RotateCCWAboutYAxis(float radians, XrVector3f translation) {
	XrPosef t = Identity();
	t.orientation.x = 0.f;
	t.orientation.y = std::sin(radians * 0.5f);
	t.orientation.z = 0.f;
	t.orientation.w = std::cos(radians * 0.5f);
	t.position = translation;
	return t;
}
} // namespace Pose
} // namespace Math

inline XrReferenceSpaceCreateInfo GetXrReferenceSpaceCreateInfo(const std::string &referenceSpaceTypeStr) {
	XrReferenceSpaceCreateInfo referenceSpaceCreateInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
	referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::Identity();
	if (EqualsIgnoreCase(referenceSpaceTypeStr, "View")) {
		referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
	} else if (EqualsIgnoreCase(referenceSpaceTypeStr, "ViewFront")) {
		// Render head-locked 2m in front of device.
		referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::Translation({0.f, 0.f, -2.f});
		referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
	} else if (EqualsIgnoreCase(referenceSpaceTypeStr, "Local")) {
		referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
	} else if (EqualsIgnoreCase(referenceSpaceTypeStr, "Stage")) {
		referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
	} else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageLeft")) {
		referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(0.f, {-2.f, 0.f, -2.f});
		referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
	} else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageRight")) {
		referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(0.f, {2.f, 0.f, -2.f});
		referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
	} else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageLeftRotated")) {
		referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(3.14f / 3.f, {-2.f, 0.5f, -2.f});
		referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
	} else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageRightRotated")) {
		referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(-3.14f / 3.f, {2.f, 0.5f, -2.f});
		referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
	} else {
		throw std::invalid_argument(format("Unknown reference space type '%1'").arg(referenceSpaceTypeStr.c_str()).str());
	}
	return referenceSpaceCreateInfo;
}

bool LogReferenceSpaces() {
	uint32_t spaceCount;
	if (CHECK_XRCMD(xrEnumerateReferenceSpaces(m_session, 0, &spaceCount, nullptr)) < 0)
		return false;
	std::vector<XrReferenceSpaceType> spaces(spaceCount);
	if (CHECK_XRCMD(xrEnumerateReferenceSpaces(m_session, spaceCount, &spaceCount, spaces.data())) < 0)
		return false;

	debug(format("Available reference spaces: %1").arg(spaceCount));
	for (XrReferenceSpaceType space : spaces) {
		debug(format("  Name: %1").arg(to_string(space)));
	}
	return true;
}

bool InitHandJoint() {
	// Inspect hand tracking system properties
	XrSystemHandTrackingPropertiesEXT handTrackingSystemProperties{XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT};
	XrSystemProperties systemProperties{XR_TYPE_SYSTEM_PROPERTIES, &handTrackingSystemProperties};
	if (CHECK_XRCMD(xrGetSystemProperties(m_instance, m_systemId, &systemProperties)) < 0)
		return false;

	if (!handTrackingSystemProperties.supportsHandTracking) {
		// The system does not support hand tracking
		return false;
	}

	// Get function pointer for xrCreateHandTrackerEXT
	PFN_xrCreateHandTrackerEXT pfnCreateHandTrackerEXT;
	if (CHECK_XRCMD(xrGetInstanceProcAddr(m_instance, "xrCreateHandTrackerEXT", reinterpret_cast<PFN_xrVoidFunction *>(& pfnCreateHandTrackerEXT))) < 0)
		return false;

	// Create a hand tracker for the hands that tracks default set of hand joints.
	for (auto hand : {HandsSide::LEFT, HandsSide::RIGHT}) {
		XrHandTrackerCreateInfoEXT createInfo{XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT};
		if (hand == HandsSide::LEFT)
			createInfo.hand = XR_HAND_LEFT_EXT;
		else
			createInfo.hand = XR_HAND_RIGHT_EXT;

		createInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
		if (pfnCreateHandTrackerEXT(m_session, &createInfo, &m_input.handJointTracker[hand]) < 0)
			return false;

		// Allocate buffers to receive joint location and velocity data before frame
		// loop starts;
		m_input.jointVelocities[hand].type = XR_TYPE_HAND_JOINT_VELOCITIES_EXT;
		m_input.jointVelocities[hand].jointCount = XR_HAND_JOINT_COUNT_EXT;
		m_input.jointVelocities[hand].jointVelocities = m_input.jointVelocitie[hand].data();

		m_input.jointLocations[hand].type = XR_TYPE_HAND_JOINT_LOCATIONS_EXT;
		m_input.jointLocations[hand].next = &m_input.jointVelocities;
		m_input.jointLocations[hand].jointCount = XR_HAND_JOINT_COUNT_EXT;
		m_input.jointLocations[hand].jointLocations = m_input.jointLocation[hand].data();
	}

	// Get function pointer for xrLocateHandJointsEXT
	if (CHECK_XRCMD(xrGetInstanceProcAddr(m_instance, "xrLocateHandJointsEXT", reinterpret_cast<PFN_xrVoidFunction *>(&m_input. pfnLocateHandJointsEXT))) < 0)
		return false;
	return true;
}

bool InitializeActions() {
	// Create an action set.
	{
		XrActionSetCreateInfo actionSetInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
		strcpy(actionSetInfo.actionSetName, "gameplay");
		strcpy(actionSetInfo.localizedActionSetName, "Gameplay");
		actionSetInfo.priority = 0;
		if (CHECK_XRCMD(xrCreateActionSet(m_instance, &actionSetInfo, &m_input.actionSet)) < 0)
			return false;
	}

	// Get the XrPath for the left and right hands - we will use them as subaction paths.
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left", &m_input.handSubactionPath[HandsSide::LEFT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right", &m_input.handSubactionPath[HandsSide::RIGHT])) < 0)
		return false;

	// Create actions.
	{
		XrActionCreateInfo actionInfo{XR_TYPE_ACTION_CREATE_INFO};

		// Create an input action for squeezing with the left and right hands.
		actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
		strcpy(actionInfo.actionName, "squeeze");
		strcpy(actionInfo.localizedActionName, "Squeeze");
		actionInfo.countSubactionPaths = uint32_t(m_input.handSubactionPath.size());
		actionInfo.subactionPaths = m_input.handSubactionPath.data();
		if (CHECK_XRCMD(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.squeezeAction)) < 0)
			return false;

		// Create an input action for trigger objects with the left and right hands.
		actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
		strcpy(actionInfo.actionName, "trigger_value");
		strcpy(actionInfo.localizedActionName, "Trigger Value");
		actionInfo.countSubactionPaths = uint32_t(m_input.handSubactionPath.size());
		actionInfo.subactionPaths = m_input.handSubactionPath.data();
		if (CHECK_XRCMD(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.triggerValueAction)) < 0)
			return false;

		// Create an input action for trigger objects with the left and right hands.
		actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
		strcpy(actionInfo.actionName, "trigger_click");
		strcpy(actionInfo.localizedActionName, "Trigger Click");
		actionInfo.countSubactionPaths = uint32_t(m_input.handSubactionPath.size());
		actionInfo.subactionPaths = m_input.handSubactionPath.data();
		if (CHECK_XRCMD(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.triggerClickAction)) < 0)
			return false;

		// Create an input action for surface X objects with the left and right hands.
		actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
		strcpy(actionInfo.actionName, "surface_x");
		strcpy(actionInfo.localizedActionName, "Surface X");
		actionInfo.countSubactionPaths = uint32_t(m_input.handSubactionPath.size());
		actionInfo.subactionPaths = m_input.handSubactionPath.data();
		if (CHECK_XRCMD(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.surfaceXAction)) < 0)
			return false;

		// Create an input action for surface Y objects with the left and right hands.
		actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
		strcpy(actionInfo.actionName, "surface_y");
		strcpy(actionInfo.localizedActionName, "Surface Y");
		actionInfo.countSubactionPaths = uint32_t(m_input.handSubactionPath.size());
		actionInfo.subactionPaths = m_input.handSubactionPath.data();
		if (CHECK_XRCMD(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.surfaceYAction)) < 0)
			return false;

		// Create an input action getting the left and right hand poses.
		actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
		strcpy(actionInfo.actionName, "hand_pose");
		strcpy(actionInfo.localizedActionName, "Hand Pose");
		actionInfo.countSubactionPaths = uint32_t(m_input.handSubactionPath.size());
		actionInfo.subactionPaths = m_input.handSubactionPath.data();
		if (CHECK_XRCMD(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.poseAction)) < 0)
			return false;

		// Create output actions for vibrating the left and right controller.
		actionInfo.actionType = XR_ACTION_TYPE_VIBRATION_OUTPUT;
		strcpy(actionInfo.actionName, "vibrate_hand");
		strcpy(actionInfo.localizedActionName, "Vibrate Hand");
		actionInfo.countSubactionPaths = uint32_t(m_input.handSubactionPath.size());
		actionInfo.subactionPaths = m_input.handSubactionPath.data();
		if (CHECK_XRCMD(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.vibrateAction)) < 0)
			return false;

		// Create input actions for quitting the session using the left and right controller.
		// Since it doesn't matter which hand did this, we do not specify subaction paths for it.
		// We will just suggest bindings for both hands, where possible.
		actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
		strcpy(actionInfo.actionName, "quit_session");
		strcpy(actionInfo.localizedActionName, "Quit Session");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		if (CHECK_XRCMD(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.quitAction)) < 0)
			return false;
	}

	std::array<XrPath, HandsSide::COUNT> selectPath;
	std::array<XrPath, HandsSide::COUNT> squeezeValuePath;
	std::array<XrPath, HandsSide::COUNT> squeezeForcePath;
	std::array<XrPath, HandsSide::COUNT> squeezeClickPath;
	std::array<XrPath, HandsSide::COUNT> posePath;
	std::array<XrPath, HandsSide::COUNT> hapticPath;
	std::array<XrPath, HandsSide::COUNT> menuClickPath;
	std::array<XrPath, HandsSide::COUNT> bClickPath;
	std::array<XrPath, HandsSide::COUNT> aClickPath;
	std::array<XrPath, HandsSide::COUNT> triggerValuePath;
	std::array<XrPath, HandsSide::COUNT> triggerClickPath;
	std::array<XrPath, HandsSide::COUNT> trackpadXPath;
	std::array<XrPath, HandsSide::COUNT> trackpadYPath;
	std::array<XrPath, HandsSide::COUNT> trackpadClickPath;
	std::array<XrPath, HandsSide::COUNT> trackpadTouchPath;
	std::array<XrPath, HandsSide::COUNT> thumbstickXPath;
	std::array<XrPath, HandsSide::COUNT> thumbstickYPath;
	std::array<XrPath, HandsSide::COUNT> thumbstickClickPath;
	std::array<XrPath, HandsSide::COUNT> thumbstickTouchPath;

	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/select/click", &selectPath[HandsSide::LEFT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/select/click", &selectPath[HandsSide::RIGHT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/squeeze/value", &squeezeValuePath[HandsSide::LEFT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/squeeze/value", &squeezeValuePath[HandsSide::RIGHT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/squeeze/force", &squeezeForcePath[HandsSide::LEFT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/squeeze/force", &squeezeForcePath[HandsSide::RIGHT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/squeeze/click", &squeezeClickPath[HandsSide::LEFT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/squeeze/click", &squeezeClickPath[HandsSide::RIGHT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/aim/pose", &posePath[HandsSide::LEFT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/aim/pose", &posePath[HandsSide::RIGHT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/output/haptic", &hapticPath[HandsSide::LEFT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/output/haptic", &hapticPath[HandsSide::RIGHT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/menu/click", &menuClickPath[HandsSide::LEFT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/menu/click", &menuClickPath[HandsSide::RIGHT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/a/click", &aClickPath[HandsSide::LEFT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/a/click", &aClickPath[HandsSide::RIGHT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/b/click", &bClickPath[HandsSide::LEFT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/b/click", &bClickPath[HandsSide::RIGHT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/trigger/value", &triggerValuePath[HandsSide::LEFT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/trigger/value", &triggerValuePath[HandsSide::RIGHT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/trigger/click", &triggerClickPath[HandsSide::LEFT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/trigger/click", &triggerClickPath[HandsSide::RIGHT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/trackpad/x", &trackpadXPath[HandsSide::LEFT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/trackpad/x", &trackpadXPath[HandsSide::RIGHT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/trackpad/y", &trackpadYPath[HandsSide::LEFT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/trackpad/y", &trackpadYPath[HandsSide::RIGHT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/trackpad/click", &trackpadClickPath[HandsSide::LEFT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/trackpad/click", &trackpadClickPath[HandsSide::RIGHT] )) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/trackpad/touch", &trackpadTouchPath[HandsSide::LEFT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/trackpad/touch", &trackpadTouchPath[HandsSide::RIGHT] )) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/thumbstick/x", &thumbstickXPath[HandsSide::LEFT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/thumbstick/x", &thumbstickXPath[HandsSide::RIGHT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/thumbstick/y", &thumbstickYPath[HandsSide::LEFT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/thumbstick/y", &thumbstickYPath[HandsSide::RIGHT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/thumbstick/click", &thumbstickClickPath[HandsSide:: LEFT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/thumbstick/click", &thumbstickClickPath[HandsSide::RIGHT] )) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/thumbstick/touch", &thumbstickTouchPath[HandsSide:: LEFT])) < 0)
		return false;
	if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/thumbstick/touch", &thumbstickTouchPath[HandsSide::RIGHT] )) < 0)
		return false;

	// Suggest bindings for KHR Simple.
	{
		XrPath khrSimpleInteractionProfilePath;
		if (CHECK_XRCMD(xrStringToPath(m_instance, "/interaction_profiles/khr/simple_controller", & khrSimpleInteractionProfilePath)) < 0)
			return false;
		std::vector<XrActionSuggestedBinding> bindings{
			{
				// Fall back to a click input for the grab action.
				{m_input.triggerClickAction, selectPath[HandsSide::LEFT]}, {m_input.triggerClickAction, selectPath[HandsSide::RIGHT]},
				{m_input.poseAction, posePath[HandsSide::LEFT]}, {m_input.poseAction, posePath[HandsSide::RIGHT]}, {m_input.quitAction, menuClickPath[HandsSide::LEFT]},
				{m_input.quitAction, menuClickPath[HandsSide::RIGHT]}, {m_input.vibrateAction, hapticPath[HandsSide::LEFT]},
				{m_input.vibrateAction, hapticPath[HandsSide::RIGHT]}
			}
		};
		XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
		suggestedBindings.interactionProfile = khrSimpleInteractionProfilePath;
		suggestedBindings.suggestedBindings = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
		if (CHECK_XRCMD(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings)) < 0)
			return false;
	}
	// Suggest bindings for the Oculus Touch.
	{
		XrPath oculusTouchInteractionProfilePath;
		if (CHECK_XRCMD(xrStringToPath(m_instance, "/interaction_profiles/oculus/touch_controller", & oculusTouchInteractionProfilePath)) < 0)
			return false;
		std::vector<XrActionSuggestedBinding> bindings{
			{
				{m_input.squeezeAction, squeezeValuePath[HandsSide::LEFT]}, {m_input.squeezeAction, squeezeValuePath[HandsSide::RIGHT]},
				{m_input.triggerValueAction, triggerValuePath[HandsSide::LEFT]}, {m_input.triggerValueAction, triggerValuePath[HandsSide::RIGHT]},
				{m_input.surfaceXAction, thumbstickXPath[HandsSide::LEFT]}, {m_input.surfaceXAction, thumbstickXPath[HandsSide::RIGHT]},
				{m_input.surfaceYAction, thumbstickYPath[HandsSide::LEFT]}, {m_input.surfaceYAction, thumbstickYPath[HandsSide::RIGHT]},
				{m_input.poseAction, posePath[HandsSide::LEFT]}, {m_input.poseAction, posePath[HandsSide::RIGHT]}, {m_input.quitAction, menuClickPath[HandsSide::LEFT]},
				{m_input.vibrateAction, hapticPath[HandsSide::LEFT]}, {m_input.vibrateAction, hapticPath[HandsSide::RIGHT]}
			}
		};
		XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
		suggestedBindings.interactionProfile = oculusTouchInteractionProfilePath;
		suggestedBindings.suggestedBindings = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
		if (CHECK_XRCMD(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings)) < 0)
			return false;
	}
	// Suggest bindings for the Vive Controller.
	{
		XrPath viveControllerInteractionProfilePath;
		if (CHECK_XRCMD(xrStringToPath(m_instance, "/interaction_profiles/htc/vive_controller", & viveControllerInteractionProfilePath)) < 0)
			return false;
		std::vector<XrActionSuggestedBinding> bindings{
			{
				{m_input.squeezeAction, triggerValuePath[HandsSide::LEFT]}, {m_input.squeezeAction, triggerValuePath[HandsSide::RIGHT]},
				{m_input.triggerValueAction, triggerValuePath[HandsSide::LEFT]}, {m_input.triggerValueAction, triggerValuePath[HandsSide::RIGHT]},
				{m_input.triggerClickAction, triggerClickPath[HandsSide::LEFT]}, {m_input.triggerClickAction, triggerClickPath[HandsSide::RIGHT]},
				{m_input.surfaceXAction, trackpadXPath[HandsSide::LEFT]}, {m_input.surfaceXAction, trackpadXPath[HandsSide::RIGHT]},
				{m_input.surfaceYAction, trackpadYPath[HandsSide::LEFT]}, {m_input.surfaceYAction, trackpadYPath[HandsSide::RIGHT]},
				{m_input.poseAction, posePath[HandsSide::LEFT]}, {m_input.poseAction, posePath[HandsSide::RIGHT]}, {m_input.quitAction, menuClickPath[HandsSide::LEFT]},
				{m_input.quitAction, menuClickPath[HandsSide::RIGHT]}, {m_input.vibrateAction, hapticPath[HandsSide::LEFT]},
				{m_input.vibrateAction, hapticPath[HandsSide::RIGHT]}
			}
		};
		XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
		suggestedBindings.interactionProfile = viveControllerInteractionProfilePath;
		suggestedBindings.suggestedBindings = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
		if (CHECK_XRCMD(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings)) < 0)
			return false;
	}

	// Suggest bindings for the Valve Index Controller.
	{
		XrPath indexControllerInteractionProfilePath;
		if (CHECK_XRCMD(xrStringToPath(m_instance, "/interaction_profiles/valve/index_controller", & indexControllerInteractionProfilePath)) < 0)
			return false;
		std::vector<XrActionSuggestedBinding> bindings{
			{
				{m_input.squeezeAction, squeezeForcePath[HandsSide::LEFT]}, {m_input.squeezeAction, squeezeForcePath[HandsSide::RIGHT]},
				{m_input.triggerValueAction, triggerValuePath[HandsSide::LEFT]}, {m_input.triggerValueAction, triggerValuePath[HandsSide::RIGHT]},
				{m_input.triggerClickAction, triggerClickPath[HandsSide::LEFT]}, {m_input.triggerClickAction, triggerClickPath[HandsSide::RIGHT]},
				{m_input.surfaceXAction, thumbstickXPath[HandsSide::LEFT]}, {m_input.surfaceXAction, thumbstickXPath[HandsSide::RIGHT]},
				{m_input.surfaceYAction, thumbstickYPath[HandsSide::LEFT]}, {m_input.surfaceYAction, thumbstickYPath[HandsSide::RIGHT]},
				{m_input.poseAction, posePath[HandsSide::LEFT]}, {m_input.poseAction, posePath[HandsSide::RIGHT]}, {m_input.quitAction, bClickPath[HandsSide::LEFT]},
				{m_input.quitAction, bClickPath[HandsSide::RIGHT]}, {m_input.vibrateAction, hapticPath[HandsSide::LEFT]}, {m_input.vibrateAction, hapticPath[HandsSide::RIGHT]}
			}
		};
		XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
		suggestedBindings.interactionProfile = indexControllerInteractionProfilePath;
		suggestedBindings.suggestedBindings = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
		if (CHECK_XRCMD(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings)) < 0)
			return false;
	}

	// Suggest bindings for the Microsoft Mixed Reality Motion Controller.
	{
		XrPath microsoftMixedRealityInteractionProfilePath;
		if (CHECK_XRCMD(xrStringToPath(m_instance, "/interaction_profiles/microsoft/motion_controller", & microsoftMixedRealityInteractionProfilePath)) < 0)
			return false;
		std::vector<XrActionSuggestedBinding> bindings{
			{
				{m_input.squeezeAction, squeezeClickPath[HandsSide::LEFT]}, {m_input.squeezeAction, squeezeClickPath[HandsSide::RIGHT]},
				{m_input.triggerValueAction, triggerValuePath[HandsSide::LEFT]}, {m_input.triggerValueAction, triggerValuePath[HandsSide::RIGHT]},
				{m_input.surfaceXAction, thumbstickXPath[HandsSide::LEFT]}, {m_input.surfaceXAction, thumbstickXPath[HandsSide::RIGHT]},
				{m_input.surfaceYAction, thumbstickYPath[HandsSide::LEFT]}, {m_input.surfaceYAction, thumbstickYPath[HandsSide::RIGHT]},
				{m_input.poseAction, posePath[HandsSide::LEFT]}, {m_input.poseAction, posePath[HandsSide::RIGHT]}, {m_input.quitAction, menuClickPath[HandsSide::LEFT]},
				{m_input.quitAction, menuClickPath[HandsSide::RIGHT]}, {m_input.vibrateAction, hapticPath[HandsSide::LEFT]},
				{m_input.vibrateAction, hapticPath[HandsSide::RIGHT]}
			}
		};
		XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
		suggestedBindings.interactionProfile = microsoftMixedRealityInteractionProfilePath;
		suggestedBindings.suggestedBindings = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
		if (CHECK_XRCMD(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings)) < 0)
			return false;
	}
	XrActionSpaceCreateInfo actionSpaceInfo{XR_TYPE_ACTION_SPACE_CREATE_INFO};
	actionSpaceInfo.action = m_input.poseAction;
	actionSpaceInfo.poseInActionSpace.orientation.w = 1.f;
	actionSpaceInfo.subactionPath = m_input.handSubactionPath[HandsSide::LEFT];
	if (CHECK_XRCMD(xrCreateActionSpace(m_session, &actionSpaceInfo, &m_input.handSpace[HandsSide::LEFT])) < 0)
		return false;
	actionSpaceInfo.subactionPath = m_input.handSubactionPath[HandsSide::RIGHT];
	if (CHECK_XRCMD(xrCreateActionSpace(m_session, &actionSpaceInfo, &m_input.handSpace[HandsSide::RIGHT])) < 0)
		return false;

	// Suggest bindings eye gazing
	if (ExtensionsFlagsEnable & OXRExtensions_EyeGaze) {
		XrPath EyeGazeInteractionProfilePath;
		if (CHECK_XRCMD(xrStringToPath(m_instance, "/interaction_profiles/ext/eye_gaze_interaction", & EyeGazeInteractionProfilePath)) < 0)
			return false;
		XrPath gazePosePath;
		if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/eyes_ext/input/gaze_ext/pose", &gazePosePath)) < 0)
			return false;

		// Create an input action getting the left and right eye action.
		XrActionCreateInfo actionInfo{XR_TYPE_ACTION_CREATE_INFO};
		actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
		strcpy(actionInfo.actionName, "gaze_action");
		strcpy(actionInfo.localizedActionName, "Gaze Action");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		if (CHECK_XRCMD(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.gazeAction)) < 0)
			return false;

		std::vector<XrActionSuggestedBinding> bindings{{{m_input.gazeAction, gazePosePath}}};
		XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
		suggestedBindings.interactionProfile = EyeGazeInteractionProfilePath;
		suggestedBindings.suggestedBindings = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
		if (CHECK_XRCMD(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings)) < 0)
			debug("can't bind eye gazing");
		// return false;

		// gaze info
		XrActionSpaceCreateInfo createInfo{XR_TYPE_ACTION_SPACE_CREATE_INFO};
		createInfo.action = m_input.gazeAction;
		createInfo.poseInActionSpace.orientation.w = 1.f;
		if (CHECK_XRCMD(xrCreateActionSpace(m_session, &createInfo, &m_input.gazeSpace)) < 0)
			return false;
	}

	// htc vive tracker
	if (ExtensionsFlagsEnable & OXRExtensions_Tracker && xrStringToPath(m_instance, "/user/vive_tracker_htcx/role/chest", &m_input.trackerSubactionPath[0]) >= 0) {
		XrActionCreateInfo actionInfo{XR_TYPE_ACTION_CREATE_INFO};
		actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
		strcpy(actionInfo.actionName, "chest_tracker");
		strcpy(actionInfo.localizedActionName, "Chest Tracker");
		actionInfo.countSubactionPaths = 1;
		actionInfo.subactionPaths = &m_input.trackerSubactionPath[0];
		if (CHECK_XRCMD(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.trackerPoseAction[0])) < 0)
			return false;

		// Describe a suggested binding for that action and subaction path.
		XrPath suggestedBindingPath;
		if (CHECK_XRCMD(xrStringToPath(m_instance, "/user/vive_tracker_htcx/role/chest/input/grip/pose", & suggestedBindingPath)) < 0)
			return false;

		std::vector<XrActionSuggestedBinding> actionSuggBindings;
		XrActionSuggestedBinding actionSuggBinding;
		actionSuggBinding.action = m_input.trackerPoseAction[0];
		actionSuggBinding.binding = suggestedBindingPath;
		actionSuggBindings.push_back(actionSuggBinding);

		// Suggest that binding for the VIVE tracker interaction profile
		XrPath viveTrackerInteractionProfilePath;
		if (CHECK_XRCMD(xrStringToPath(m_instance, "/interaction_profiles/htc/vive_tracker_htcx", & viveTrackerInteractionProfilePath)) < 0)
			return false;

		XrInteractionProfileSuggestedBinding profileSuggBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
		profileSuggBindings.interactionProfile = viveTrackerInteractionProfilePath;
		profileSuggBindings.suggestedBindings = actionSuggBindings.data();
		profileSuggBindings.countSuggestedBindings = (uint32_t)actionSuggBindings.size();

		if (CHECK_XRCMD(xrSuggestInteractionProfileBindings(m_instance, &profileSuggBindings)) < 0)
			return false;

		// Create action space for locating tracker
		XrActionSpaceCreateInfo actionSpaceInfo{XR_TYPE_ACTION_SPACE_CREATE_INFO};
		actionSpaceInfo.action = m_input.trackerPoseAction[0];
		actionSpaceInfo.subactionPath = m_input.trackerSubactionPath[0];
		actionSpaceInfo.poseInActionSpace.orientation.w = 1.f;
		if (CHECK_XRCMD(xrCreateActionSpace(m_session, &actionSpaceInfo, &m_input.trackerSpace[0])) < 0)
			return false;
	}

	// reference space
	{
		XrReferenceSpaceCreateInfo createReferenceSpaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
		createReferenceSpaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
		createReferenceSpaceInfo.poseInReferenceSpace = {{0, 0, 0, 1}, {0, 0, 0}};
		if (CHECK_XRCMD(xrCreateReferenceSpace(m_session, &createReferenceSpaceInfo, &m_appLocalSpace)) < 0) {
		}
	}

	// view space
	{
		XrReferenceSpaceCreateInfo createReferenceSpaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
		createReferenceSpaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
		createReferenceSpaceInfo.poseInReferenceSpace = {{0, 0, 0, 1}, {0, 0, 0}};
		if (CHECK_XRCMD(xrCreateReferenceSpace(m_session, &createReferenceSpaceInfo, &m_appViewSpace)) < 0) {
		}
	}

	// attach info
	XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
	attachInfo.countActionSets = 1;
	attachInfo.actionSets = &m_input.actionSet;
	if (CHECK_XRCMD(xrAttachSessionActionSets(m_session, &attachInfo)) < 0)
		return false;

	// hand join
	if (ExtensionsFlagsEnable & OXRExtensions_HandTracking)
		InitHandJoint();
	return true;
}

void CreateVisualizedSpaces() {
	std::string visualizedSpaces[] = {"ViewFront", "Local", "Stage", "StageLeft", "StageRight", "StageLeftRotated", "StageRightRotated"};

	for (const auto &visualizedSpace : visualizedSpaces) {
		XrReferenceSpaceCreateInfo referenceSpaceCreateInfo = GetXrReferenceSpaceCreateInfo(visualizedSpace);
		XrSpace space;
		XrResult res = xrCreateReferenceSpace(m_session, &referenceSpaceCreateInfo, &space);
		if (XR_SUCCEEDED(res)) {
			m_visualizedSpaces.push_back(space);
		} else {
			warn(format("Failed to create reference space %1 with error %2").arg(visualizedSpace.c_str()).arg(res));
		}
	}
}

bool InitializeSession() {
	switch (bgfx::getRendererType()) {
#ifdef XR_USE_GRAPHICS_API_D3D11
		case bgfx::RendererType::Direct3D11: {
			PFN_xrGetD3D11GraphicsRequirementsKHR pfnGetD3D11GraphicsRequirementsKHR = nullptr;
			if (CHECK_XRCMD(
				xrGetInstanceProcAddr( m_instance, "xrGetD3D11GraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction *>(& pfnGetD3D11GraphicsRequirementsKHR))) < 0)
				return false;

			// Create the D3D11 device for the adapter associated with the system.
			XrGraphicsRequirementsD3D11KHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
			if (pfnGetD3D11GraphicsRequirementsKHR(m_instance, m_systemId, &graphicsRequirements) < 0)
				return false;

			debug("Creating session...");
			XrGraphicsBindingD3D11KHR graphicsBinding{XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};
			graphicsBinding.device = (ID3D11Device*)bgfx::getInternalData()->context;

			XrSessionCreateInfo createInfo{XR_TYPE_SESSION_CREATE_INFO};
			createInfo.next = &graphicsBinding;
			createInfo.systemId = m_systemId;
			if (CHECK_XRCMD(xrCreateSession(m_instance, &createInfo, &m_session)) < 0)
				return false;
		}
		break;
#endif
#ifdef XR_USE_GRAPHICS_API_D3D12
		case bgfx::RendererType::Direct3D12: {
			PFN_xrGetD3D12GraphicsRequirementsKHR pfnGetD3D12GraphicsRequirementsKHR = nullptr;
			if (CHECK_XRCMD(xrGetInstanceProcAddr(
					m_instance, "xrGetD3D12GraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction *>(&pfnGetD3D12GraphicsRequirementsKHR))) < 0)
				return false;

			// Create the D3D12 device for the adapter associated with the system.
			XrGraphicsRequirementsD3D12KHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR};
			if (pfnGetD3D12GraphicsRequirementsKHR(m_instance, m_systemId, &graphicsRequirements) < 0)
				return false;

			debug("Creating session...");
			XrGraphicsBindingD3D12KHR graphicsBinding{XR_TYPE_GRAPHICS_BINDING_D3D12_KHR};
			graphicsBinding.device = (ID3D12Device *)bgfx::getInternalData()->context;
			static const GUID IID_ID3D12CommandQueue = {0x0ec870a6, 0x5d7e, 0x4c22, {0x8c, 0xfc, 0x5b, 0xaa, 0xe0, 0x76, 0x16, 0xed}};
			ID3D12CommandQueue *commandQueue;
			UINT size = sizeof(commandQueue);
			graphicsBinding.device->GetPrivateData(IID_ID3D12CommandQueue, &size, &commandQueue);
			graphicsBinding.queue = commandQueue;

			XrSessionCreateInfo createInfo{XR_TYPE_SESSION_CREATE_INFO};
			createInfo.next = &graphicsBinding;
			createInfo.systemId = m_systemId;
			if (CHECK_XRCMD(xrCreateSession(m_instance, &createInfo, &m_session)) < 0)
				return false;
		} break;
#endif
		case bgfx::RendererType::OpenGL: {
			PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;
			if (CHECK_XRCMD(
				xrGetInstanceProcAddr( m_instance, "xrGetOpenGLGraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction *>(& pfnGetOpenGLGraphicsRequirementsKHR))) < 0)
				return false;

			// Create the OpenGL device for the adapter associated with the system.
			XrGraphicsRequirementsOpenGLKHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};
			if (pfnGetOpenGLGraphicsRequirementsKHR(m_instance, m_systemId, &graphicsRequirements) < 0)
				return false;

			debug("Creating session...");

#ifdef XR_USE_PLATFORM_WIN32
			XrGraphicsBindingOpenGLWin32KHR graphicsBinding{XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR};
			graphicsBinding.hDC = GetDC((HWND)bgfx::g_platformData.nwh);
			graphicsBinding.hGLRC = (HGLRC)bgfx::getInternalData()->context;
#endif // XR_USE_PLATFORM_WIN32
#ifdef XR_USE_PLATFORM_XLIB
			XrGraphicsBindingOpenGLXlibKHR graphicsBinding{XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR};
			graphicsBinding.xDisplay = GetDisplay();
			graphicsBinding.glxContext = (GLXContext)bgfx::getInternalData()->context;
#endif // XR_USE_PLATFORM_XLIB
#ifdef XR_USE_PLATFORM_XCB
			XrGraphicsBindingOpenGLXcbKHR graphicsBinding{XR_TYPE_GRAPHICS_BINDING_OPENGL_XCB_KHR};
#endif // XR_USE_PLATFORM_XCB
#ifdef XR_USE_PLATFORM_WAYLAND
			XrGraphicsBindingOpenGLWaylandKHR graphicsBinding{XR_TYPE_GRAPHICS_BINDING_OPENGL_WAYLAND_KHR};
#endif // XR_USE_PLATFORM_WAYLAND

			XrSessionCreateInfo createInfo{XR_TYPE_SESSION_CREATE_INFO};
			createInfo.next = &graphicsBinding;
			createInfo.systemId = m_systemId;
			if (CHECK_XRCMD(xrCreateSession(m_instance, &createInfo, &m_session)) < 0)
				return false;
		}
		break;
#ifdef XR_USE_GRAPHICS_API_VULKAN
		case bgfx::RendererType::Vulkan: {
			PFN_xrGetVulkanGraphicsRequirementsKHR pfnGetVulkanGraphicsRequirementsKHR = nullptr;
			if (CHECK_XRCMD(xrGetInstanceProcAddr(
					m_instance, "xrGetVulkanGraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction *>(&pfnGetVulkanGraphicsRequirementsKHR))) < 0)
				return false;

			// Create the Vulkan device for the adapter associated with the system.
			XrGraphicsRequirementsVulkanKHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR};
			if (pfnGetVulkanGraphicsRequirementsKHR(m_instance, m_systemId, &graphicsRequirements) < 0)
				return false;

			debug("Creating session...");
			XrGraphicsBindingVulkanKHR graphicsBinding{XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR};

			// TODO find a way to get these value from bgfx
			// VkInstance m_instance found in src/renderer_vk.cpp l.4353

			// bgfx::vk::VkContext *vkcontext = (bgfx::vk::VkContext *)(bgfx::getInternalData()->context);

			// graphicsBinding.instance = vkcontext->m_instance;
			// graphicsBinding.physicalDevice = vkcontext->m_physicalDevice;
			// graphicsBinding.device = vkcontext->m_device;
			// graphicsBinding.queueFamilyIndex = vkcontext->m_globalQueueFamily;
			// graphicsBinding.queueIndex = 0;

			XrSessionCreateInfo createInfo{XR_TYPE_SESSION_CREATE_INFO};
			createInfo.next = &graphicsBinding;
			createInfo.systemId = m_systemId;
			if (CHECK_XRCMD(xrCreateSession(m_instance, &createInfo, &m_session)) < 0)
				return false;
		} break;
#endif
	}

	LogReferenceSpaces();
	InitializeActions();
	CreateVisualizedSpaces();

	{
		XrReferenceSpaceCreateInfo referenceSpaceCreateInfo = GetXrReferenceSpaceCreateInfo(AppSpace);
		if (CHECK_XRCMD(xrCreateReferenceSpace(m_session, &referenceSpaceCreateInfo, &m_appSpace)) < 0)
			return false;
	}
	return true;
}

int64_t SelectColorSwapchainFormat(const std::vector<int64_t> &runtimeFormats) {
	constexpr int SupportedColorSwapchainFormats[] = {
#if defined(XR_USE_GRAPHICS_API_D3D11) || defined(XR_USE_GRAPHICS_API_D3D12)
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
#endif
		// 0x8C43, // GL_SRGB8_ALPHA8, // need srgb shader, so take the one below (but in float)
		0x8C43, // GL_SRGB8_ALPHA8
		0x8C41, // GL_SRGB8
		0x881A, // GL_RGBA16F
		//	0x8058, // GL_RGBA8
	};
	auto swapchainFormatIt = std::find_first_of(runtimeFormats.begin(), runtimeFormats.end(), std::begin(SupportedColorSwapchainFormats),
	                                            std::end(SupportedColorSwapchainFormats));
	if (swapchainFormatIt == runtimeFormats.end()) {
		error("No runtime swapchain format supported for color swapchain");
	}

	return *swapchainFormatIt;
}

int64_t SelectDepthSwapchainFormat(const std::vector<int64_t> &runtimeFormats) {
	constexpr int SupportedDepthSwapchainFormats[] = {
		// 0x8CAD, // GL_DEPTH32F_STENCIL8
		//	0x8CAC, //GL_DEPTH_COMPONENT32F
		0x88F0, // GL_DEPTH24_STENCIL8
		45, // DXGI_FORMAT_D24_UNORM_S8_UINT
	};
	auto swapchainFormatIt = std::find_first_of(runtimeFormats.begin(), runtimeFormats.end(), std::begin(SupportedDepthSwapchainFormats),
	                                            std::end(SupportedDepthSwapchainFormats));
	if (swapchainFormatIt == runtimeFormats.end()) {
		error("No runtime swapchain format supported for depth swapchain");
	}

	return *swapchainFormatIt;
}

#ifdef XR_USE_GRAPHICS_API_D3D11
std::list<std::vector<XrSwapchainImageD3D11KHR>> m_swapchainImageBuffersD11;
#endif
#ifdef XR_USE_GRAPHICS_API_D3D12
std::list<std::vector<XrSwapchainImageD3D12KHR>> m_swapchainImageBuffersD12;
#endif
std::list<std::vector<XrSwapchainImageOpenGLKHR>> m_swapchainImageBuffersOpenGL;
#ifdef XR_USE_GRAPHICS_API_VULKAN
std::list<std::vector<XrSwapchainImageVulkanKHR>> m_swapchainImageBuffersVulkan;
#endif
std::vector<XrSwapchainImageBaseHeader*> AllocateSwapchainImageStructs(uint32_t capacity, const XrSwapchainCreateInfo & /*swapchainCreateInfo*/) {
	// Allocate and initialize the buffer of image structs (must be sequential in memory for xrEnumerateSwapchainImages).
	// Return back an array of pointers to each swapchain image struct so the consumer doesn't need to know the type/size.
	std::vector<XrSwapchainImageBaseHeader*> swapchainImageBase;
	switch (bgfx::getRendererType()) {
#ifdef XR_USE_GRAPHICS_API_D3D11
		case bgfx::RendererType::Direct3D11: {
			std::vector<XrSwapchainImageD3D11KHR> swapchainImageBuffer(capacity);
			for (XrSwapchainImageD3D11KHR &image : swapchainImageBuffer) {
				image.type = XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR;
				swapchainImageBase.push_back(reinterpret_cast<XrSwapchainImageBaseHeader*>(&image));
			}

			// Keep the buffer alive by moving it into the list of buffers.
			m_swapchainImageBuffersD11.push_back(std::move(swapchainImageBuffer));
		}
		break;
#endif
#ifdef XR_USE_GRAPHICS_API_D3D12
		case bgfx::RendererType::Direct3D12: {
			std::vector<XrSwapchainImageD3D12KHR> swapchainImageBuffer(capacity);
			for (XrSwapchainImageD3D12KHR &image : swapchainImageBuffer) {
				image.type = XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR;
				swapchainImageBase.push_back(reinterpret_cast<XrSwapchainImageBaseHeader *>(&image));
			}

			// Keep the buffer alive by moving it into the list of buffers.
			m_swapchainImageBuffersD12.push_back(std::move(swapchainImageBuffer));
		} break;
#endif
		case bgfx::RendererType::OpenGL: {
			std::vector<XrSwapchainImageOpenGLKHR> swapchainImageBuffer(capacity);
			for (XrSwapchainImageOpenGLKHR &image : swapchainImageBuffer) {
				image.type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
				swapchainImageBase.push_back(reinterpret_cast<XrSwapchainImageBaseHeader*>(&image));
			}

			// Keep the buffer alive by moving it into the list of buffers.
			m_swapchainImageBuffersOpenGL.push_back(std::move(swapchainImageBuffer));
		}
		break;
#ifdef XR_USE_GRAPHICS_API_VULKAN
		case bgfx::RendererType::Vulkan:
			std::vector<XrSwapchainImageVulkanKHR> swapchainImageBuffer(capacity);
			for (XrSwapchainImageVulkanKHR &image : swapchainImageBuffer) {
				image.type = XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR;
				swapchainImageBase.push_back(reinterpret_cast<XrSwapchainImageBaseHeader *>(&image));
			}

			// Keep the buffer alive by moving it into the list of buffers.
			m_swapchainImageBuffersVulkan.push_back(std::move(swapchainImageBuffer));
			break;
#endif
	}
	return swapchainImageBase;
}

bool CreateSwapchains() {
	// Read graphics properties for preferred swapchain length and logging.
	XrSystemProperties systemProperties{XR_TYPE_SYSTEM_PROPERTIES};
	if (CHECK_XRCMD(xrGetSystemProperties(m_instance, m_systemId, &systemProperties)) < 0)
		return false;

	// Log system properties.
	debug(format("System Properties: Name=%1 VendorId=%2").arg(systemProperties.systemName).arg(systemProperties.vendorId));
	debug(format("System Graphics Properties: MaxWidth=%1 MaxHeight=%2 MaxLayers=%3").arg(systemProperties.graphicsProperties.maxSwapchainImageWidth).arg(
		systemProperties.graphicsProperties.maxSwapchainImageHeight).arg(systemProperties.graphicsProperties.maxLayerCount));
	debug(format("System Tracking Properties: OrientationTracking=%1 PositionTracking=%2").arg(
		systemProperties.trackingProperties.orientationTracking == XR_TRUE ? "True" : "False").arg(
		systemProperties.trackingProperties.positionTracking == XR_TRUE ? "True" : "False"));

	// Note: No other view configurations exist at the time this code was written. If this
	// condition is not met, the project will need to be audited to see how support should be
	// added.
	// if (CHECK_XRCMD(xrGetSystem(m_instance, &systemInfo, &m_systemId)) < 0)
	//	return false;

	//	CHECK_MSG(m_viewConfigType == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, "Unsupported view configuration type");

	// Query and cache view configuration views.
	uint32_t viewCount;
	if (CHECK_XRCMD(xrEnumerateViewConfigurationViews(m_instance, m_systemId, m_viewConfigType, 0, &viewCount, nullptr)) < 0)
		return false;

	m_configViews.resize(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
	if (CHECK_XRCMD(xrEnumerateViewConfigurationViews(m_instance, m_systemId, m_viewConfigType, viewCount, &viewCount, m_configViews.data())) < 0)
		return false;

	// Create and cache view buffer for xrLocateViews later.
	m_views.resize(viewCount, {XR_TYPE_VIEW});

	// Create the swapchain and get the images.
	if (viewCount > 0) {
		// Select a swapchain format.
		uint32_t swapchainFormatCount;
		if (CHECK_XRCMD(xrEnumerateSwapchainFormats(m_session, 0, &swapchainFormatCount, nullptr)) < 0)
			return false;

		std::vector<int64_t> swapchainFormats(swapchainFormatCount);
		if (CHECK_XRCMD(xrEnumerateSwapchainFormats(m_session, (uint32_t)swapchainFormats.size(), &swapchainFormatCount, swapchainFormats.data())) < 0)
			return false;

		//	CHECK(swapchainFormatCount == swapchainFormats.size());
		m_colorSwapchainFormat = SelectColorSwapchainFormat(swapchainFormats);
		m_depthSwapchainFormat = SelectDepthSwapchainFormat(swapchainFormats);

		// Print swapchain formats and the selected one.
		{
			std::string swapchainFormatsString;
			for (int64_t format : swapchainFormats) {
				const bool selected = format == m_colorSwapchainFormat;
				swapchainFormatsString += " ";
				if (selected) {
					swapchainFormatsString += "[";
				}
				swapchainFormatsString += std::to_string(format);
				if (selected) {
					swapchainFormatsString += "]";
				}
			}
			debug(format("Swapchain Formats: %1").arg(swapchainFormatsString.c_str()));
		}

		// Create a swapchain for each view.
		for (uint32_t i = 0; i < viewCount; i++) {
			const XrViewConfigurationView &vp = m_configViews[i];
			debug(format("Creating swapchain for view %1 with dimensions Width=%2 Height=%3 SampleCount=%4").arg(i).arg(vp.recommendedImageRectWidth).
			                                                                                                 arg(vp.recommendedImageRectHeight).arg(
				                                                                                                 vp.recommendedSwapchainSampleCount));

			// Create the swapchain.
			XrSwapchainCreateInfo swapchainCreateInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
			swapchainCreateInfo.arraySize = 1;
			swapchainCreateInfo.format = m_colorSwapchainFormat;
			swapchainCreateInfo.width = vp.recommendedImageRectWidth;
			swapchainCreateInfo.height = vp.recommendedImageRectHeight;
			swapchainCreateInfo.mipCount = 1;
			swapchainCreateInfo.faceCount = 1;
			swapchainCreateInfo.sampleCount = vp.recommendedSwapchainSampleCount;
			swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
			Swapchain swapchain;
			swapchain.width = swapchainCreateInfo.width;
			swapchain.height = swapchainCreateInfo.height;
			if (CHECK_XRCMD(xrCreateSwapchain(m_session, &swapchainCreateInfo, &swapchain.handle)) < 0)
				return false;

			if (ExtensionsFlagsEnable & OXRExtensions_COMPOSITION_LAYER_DEPTH) {
				XrSwapchainCreateInfo swapchainDepthCreateInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
				swapchainDepthCreateInfo.arraySize = 1;
				swapchainDepthCreateInfo.format = m_depthSwapchainFormat;
				swapchainDepthCreateInfo.width = vp.recommendedImageRectWidth;
				swapchainDepthCreateInfo.height = vp.recommendedImageRectHeight;
				swapchainDepthCreateInfo.mipCount = 1;
				swapchainDepthCreateInfo.faceCount = 1;
				swapchainDepthCreateInfo.sampleCount = vp.recommendedSwapchainSampleCount;
				swapchainDepthCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

				if (CHECK_XRCMD(xrCreateSwapchain(m_session, &swapchainDepthCreateInfo, &swapchain.handleDepth)) < 0)
					return false;
			}

			m_swapchains.push_back(swapchain);

			uint32_t imageCount;
			{
				if (CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain.handle, 0, &imageCount, nullptr)) < 0)
					return false;

				// XXX This should really just return XrSwapchainImageBaseHeader*
				std::vector<XrSwapchainImageBaseHeader*> swapchainImages = AllocateSwapchainImageStructs(imageCount, swapchainCreateInfo);
				if (CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain.handle, imageCount, &imageCount, swapchainImages[0])) < 0)
					return false;

				m_swapchainImages.insert(std::make_pair(swapchain.handle, std::move(swapchainImages)));
			}

			// for depth
			if (ExtensionsFlagsEnable & OXRExtensions_COMPOSITION_LAYER_DEPTH) {
				if (CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain.handleDepth, 0, &imageCount, nullptr)) < 0)
					return false;

				// XXX This should really just return XrSwapchainImageBaseHeader*
				std::vector<XrSwapchainImageBaseHeader*> swapchainImages = AllocateSwapchainImageStructs(imageCount, swapchainCreateInfo);
				if (CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain.handleDepth, imageCount, &imageCount, swapchainImages[0 ])) < 0)
					return false;

				m_swapchainDepthImages.insert(std::make_pair(swapchain.handleDepth, std::move(swapchainImages)));
			}
		}
	}
	return true;
}

bool CreatePasstrough() {
	// Get function pointer
	PFN_xrCreatePassthroughFB pfnxrCreatePassthroughFB;
	if (CHECK_XRCMD(xrGetInstanceProcAddr(m_instance, "xrCreatePassthroughFB", reinterpret_cast<PFN_xrVoidFunction *>(& pfnxrCreatePassthroughFB))) < 0)
		return false;

	PFN_xrCreatePassthroughLayerFB pfnxrCreatePassthroughLayerFB;
	if (CHECK_XRCMD(xrGetInstanceProcAddr(m_instance, "xrCreatePassthroughLayerFB", reinterpret_cast<PFN_xrVoidFunction *>(& pfnxrCreatePassthroughLayerFB))) < 0)
		return false;

	PFN_xrPassthroughStartFB pfnxrPassthroughStartFB;
	if (CHECK_XRCMD(xrGetInstanceProcAddr(m_instance, "xrPassthroughStartFB", reinterpret_cast<PFN_xrVoidFunction *>(& pfnxrPassthroughStartFB))) < 0)
		return false;

	// passthrough
	XrPassthroughFB activePassthrough;
	XrPassthroughCreateInfoFB createinfo{XR_TYPE_PASSTHROUGH_CREATE_INFO_FB};
	createinfo.flags = XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB;
	if (pfnxrCreatePassthroughFB(m_session, &createinfo, &activePassthrough) < 0)
		return false;

	XrPassthroughLayerFB activeLayer;
	XrPassthroughLayerCreateInfoFB layercreateinfo{XR_TYPE_PASSTHROUGH_LAYER_CREATE_INFO_FB};
	layercreateinfo.passthrough = activePassthrough;
	layercreateinfo.flags = XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB;
	layercreateinfo.purpose = XR_PASSTHROUGH_LAYER_PURPOSE_RECONSTRUCTION_FB;

	if (pfnxrCreatePassthroughLayerFB(m_session, &layercreateinfo, &activeLayer) < 0)
		return false;

	layerPasstrough.flags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
	layerPasstrough.layerHandle = activeLayer;

	pfnxrPassthroughStartFB(activePassthrough);
	return true;
}

//
bool OpenXRInit(OpenXRExtensionsFlags ExtensionsFlagsEnable_) {
	ExtensionsFlagsEnable = ExtensionsFlagsEnable_;
	if (ExtensionsFlagsEnable & OXRExtensions_PassThrough)
		EnvironmentBlendMode = "AlphaBlend";

	if (m_instance)
		return true; // setup already

	if (!LogLayersAndExtensions())
		return false;

	if (!CreateInstanceInternal())
		return false;

	if (!LogInstanceInfo())
		return false;

	if (!InitializeSystem())
		return false;

	if (!InitializeSession())
		return false;

	if (!CreateSwapchains())
		return false;

	AddVRControllerReader("OpenXR_controller_0", OpenXRControllerReader<0>, OpenXRControllerSendHapticPulse<0>);
	AddVRControllerReader("OpenXR_controller_1", OpenXRControllerReader<1>, OpenXRControllerSendHapticPulse<1>);

	AddVRGenericTrackerReader("OpenXR_generic_tracker_0", OpenXRGenericTrackerReader<0>);

	// passthrough TODO TEST on device with this one working like anything else than vive or mixed reality
	if (ExtensionsFlagsEnable & OXRExtensions_PassThrough)
		CreatePasstrough();

	return true;
}

void OpenXRShutdown() {
	debug("OpenXR shutting down");

	RemoveVRControllerReader("OpenXR_controller_0");
	RemoveVRControllerReader("OpenXR_controller_1");

	RemoveVRGenericTrackerReader("OpenXR_generic_tracker_0");
}

bool HandleSessionStateChangedEvent(const XrEventDataSessionStateChanged &stateChangedEvent, bool *exitRenderLoop, bool *requestRestart) {
	const XrSessionState oldState = m_sessionState;
	m_sessionState = stateChangedEvent.state;

	if ((stateChangedEvent.session != XR_NULL_HANDLE) && (stateChangedEvent.session != m_session)) {
		error("XrEventDataSessionStateChanged for unknown session");
		return false;
	}

	switch (m_sessionState) {
		case XR_SESSION_STATE_READY: {
			XrSessionBeginInfo sessionBeginInfo{XR_TYPE_SESSION_BEGIN_INFO};
			sessionBeginInfo.primaryViewConfigurationType = m_viewConfigType;
			if (CHECK_XRCMD(xrBeginSession(m_session, &sessionBeginInfo)) < 0)
				return false;
			m_sessionRunning = true;
			break;
		}
		case XR_SESSION_STATE_STOPPING: {
			m_sessionRunning = false;
			if (CHECK_XRCMD(xrEndSession(m_session)) < 0)
				return false;
			break;
		}
		case XR_SESSION_STATE_EXITING: {
			*exitRenderLoop = true;
			// Do not attempt to restart because user closed this session.
			*requestRestart = false;
			break;
		}
		case XR_SESSION_STATE_LOSS_PENDING: {
			*exitRenderLoop = true;
			// Poll for a new instance.
			*requestRestart = true;
			break;
		}
		default: break;
	}
	return true;
}

bool LogActionSourceName(XrAction action, const std::string &actionName) {
	XrBoundSourcesForActionEnumerateInfo getInfo = {XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO};
	getInfo.action = action;
	uint32_t pathCount = 0;
	if (CHECK_XRCMD(xrEnumerateBoundSourcesForAction(m_session, &getInfo, 0, &pathCount, nullptr)) < 0)
		return false;
	std::vector<XrPath> paths(pathCount);
	if (CHECK_XRCMD(xrEnumerateBoundSourcesForAction(m_session, &getInfo, uint32_t(paths.size()), &pathCount, paths.data())) < 0)
		return false;

	std::string sourceName;
	for (uint32_t i = 0; i < pathCount; ++i) {
		constexpr XrInputSourceLocalizedNameFlags all = XR_INPUT_SOURCE_LOCALIZED_NAME_USER_PATH_BIT | XR_INPUT_SOURCE_LOCALIZED_NAME_INTERACTION_PROFILE_BIT |
			XR_INPUT_SOURCE_LOCALIZED_NAME_COMPONENT_BIT;

		XrInputSourceLocalizedNameGetInfo nameInfo = {XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO};
		nameInfo.sourcePath = paths[i];
		nameInfo.whichComponents = all;

		uint32_t size = 0;
		if (CHECK_XRCMD(xrGetInputSourceLocalizedName(m_session, &nameInfo, 0, &size, nullptr)) < 0)
			return false;

		if (size < 1) {
			continue;
		}
		std::vector<char> grabSource(size);
		if (CHECK_XRCMD(xrGetInputSourceLocalizedName(m_session, &nameInfo, uint32_t(grabSource.size()), &size, grabSource.data( ))) < 0)
			return false;

		if (!sourceName.empty()) {
			sourceName += " and ";
		}
		sourceName += "'";
		sourceName += std::string(grabSource.data(), size - 1);
		sourceName += "'";
	}

	debug(format("%1 action is bound to %2").arg(actionName.c_str()).arg(((!sourceName.empty()) ? sourceName.c_str() : "nothing")));
	return true;
}

// Return event if one is available, otherwise return null.
const XrEventDataBaseHeader* TryReadNextEvent() {
	// It is sufficient to clear the just the XrEventDataBuffer header to
	// XR_TYPE_EVENT_DATA_BUFFER
	XrEventDataBaseHeader *baseHeader = reinterpret_cast<XrEventDataBaseHeader*>(&m_eventDataBuffer);
	*baseHeader = {XR_TYPE_EVENT_DATA_BUFFER};
	const XrResult xr = xrPollEvent(m_instance, &m_eventDataBuffer);
	if (xr == XR_SUCCESS) {
		if (baseHeader->type == XR_TYPE_EVENT_DATA_EVENTS_LOST) {
			const XrEventDataEventsLost *const eventsLost = reinterpret_cast<const XrEventDataEventsLost*>(baseHeader);
			//			warn(format("%1 events lost").arg(eventsLost));
		}

		return baseHeader;
	}
	if (xr == XR_EVENT_UNAVAILABLE) {
		return nullptr;
	}
	return nullptr;
}

void PollEvents(bool *exitRenderLoop, bool *requestRestart) {
	*exitRenderLoop = *requestRestart = false;

	// Process all pending messages.
	while (const XrEventDataBaseHeader *event = TryReadNextEvent()) {
		switch (event->type) {
			case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
				const auto &instanceLossPending = *reinterpret_cast<const XrEventDataInstanceLossPending*>(event);
				warn(format("XrEventDataInstanceLossPending by %1").arg(instanceLossPending.lossTime));
				*exitRenderLoop = true;
				*requestRestart = true;
				return;
			}
			case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
				auto sessionStateChangedEvent = *reinterpret_cast<const XrEventDataSessionStateChanged*>(event);
				HandleSessionStateChangedEvent(sessionStateChangedEvent, exitRenderLoop, requestRestart);
				break;
			}
			case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED: LogActionSourceName(m_input.squeezeAction, "Grab");
				LogActionSourceName(m_input.quitAction, "Quit");
				LogActionSourceName(m_input.poseAction, "Pose");
				LogActionSourceName(m_input.vibrateAction, "Vibrate");
				break;
			case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING: default: {
				debug(format("Ignoring event type %1").arg(event->type));
				break;
			}
		}
	}
}

bool PollActions() {
	OpenXR_hand_device_states[0].connected = false;
	OpenXR_hand_device_states[1].connected = false;
	OpenXR_tracker_device_states[0].connected = false;

	// Sync actions
	const XrActiveActionSet activeActionSet{m_input.actionSet, XR_NULL_PATH};
	XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
	syncInfo.countActiveActionSets = 1;
	syncInfo.activeActionSets = &activeActionSet;
	if (CHECK_XRCMD(xrSyncActions(m_session, &syncInfo)) < 0)
		return false;

	// Get trigger action state
	for (auto hand : {HandsSide::LEFT, HandsSide::RIGHT}) {
		XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};

		// get trigger value
		getInfo.action = m_input.triggerValueAction;
		getInfo.subactionPath = m_input.handSubactionPath[hand];
		XrActionStateFloat triggerValue{XR_TYPE_ACTION_STATE_FLOAT};
		if (CHECK_XRCMD(xrGetActionStateFloat(m_session, &getInfo, &triggerValue)) < 0)
			return false;

		if (triggerValue.isActive == XR_TRUE)
			OpenXR_hand_device_states[hand].triggerValue = triggerValue.currentState;

		// get trigger click
		getInfo.action = m_input.triggerClickAction;
		getInfo.subactionPath = m_input.handSubactionPath[hand];
		XrActionStateBoolean triggerClick{XR_TYPE_ACTION_STATE_BOOLEAN};
		if (xrGetActionStateBoolean(m_session, &getInfo, &triggerClick) < 0)
			OpenXR_hand_device_states[hand].triggerClick = OpenXR_hand_device_states[hand].triggerValue > 0.9f;
		else if (triggerClick.isActive == XR_TRUE)
			OpenXR_hand_device_states[hand].triggerClick = triggerClick.currentState;

		/*	if (triggerValue.currentState > 0.9f) {
				XrHapticVibration vibration{XR_TYPE_HAPTIC_VIBRATION};
				vibration.amplitude = 0.5;
				vibration.duration = XR_MIN_HAPTIC_DURATION;
				vibration.frequency = XR_FREQUENCY_UNSPECIFIED;

				XrHapticActionInfo hapticActionInfo{XR_TYPE_HAPTIC_ACTION_INFO};
				hapticActionInfo.action = m_input.vibrateAction;
				hapticActionInfo.subactionPath = m_input.handSubactionPath[hand];
				if (CHECK_XRCMD(xrApplyHapticFeedback(m_session, &hapticActionInfo, (XrHapticBaseHeader *)&vibration)) < 0)
					return false;
			}
			*/

		// get surface X value
		getInfo.action = m_input.surfaceXAction;
		getInfo.subactionPath = m_input.handSubactionPath[hand];
		XrActionStateFloat surfaceXValue{XR_TYPE_ACTION_STATE_FLOAT};
		if (CHECK_XRCMD(xrGetActionStateFloat(m_session, &getInfo, &surfaceXValue)) < 0)
			return false;

		if (surfaceXValue.isActive == XR_TRUE)
			OpenXR_hand_device_states[hand].surface.x = surfaceXValue.currentState;

		// get surface Y value
		getInfo.action = m_input.surfaceYAction;
		getInfo.subactionPath = m_input.handSubactionPath[hand];
		XrActionStateFloat surfaceYValue{XR_TYPE_ACTION_STATE_FLOAT};
		if (CHECK_XRCMD(xrGetActionStateFloat(m_session, &getInfo, &surfaceYValue)) < 0)
			return false;

		if (surfaceYValue.isActive == XR_TRUE)
			OpenXR_hand_device_states[hand].surface.y = surfaceYValue.currentState;

		// get is controller is active
		getInfo.action = m_input.poseAction;
		getInfo.subactionPath = m_input.handSubactionPath[hand];
		XrActionStatePose poseState{XR_TYPE_ACTION_STATE_POSE};
		if (CHECK_XRCMD(xrGetActionStatePose(m_session, &getInfo, &poseState)) < 0)
			return false;
		OpenXR_hand_device_states[hand].connected = poseState.isActive;
	}

	// There were no subaction paths specified for the quit action, because we don't care which hand did it.
	XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO, nullptr, m_input.quitAction, XR_NULL_PATH};
	XrActionStateBoolean quitValue{XR_TYPE_ACTION_STATE_BOOLEAN};
	if (CHECK_XRCMD(xrGetActionStateBoolean(m_session, &getInfo, &quitValue)) < 0)
		return false;
	if ((quitValue.isActive == XR_TRUE) && (quitValue.changedSinceLastSync == XR_TRUE) && (quitValue.currentState == XR_TRUE)) {
		if (CHECK_XRCMD(xrRequestExitSession(m_session)) < 0)
			return false;
	}
	return true;
}

bool OpenXRGetEyeGaze(Mat4 &eye_gaze) {
	eye_gaze = m_input.eye_gaze;
	return m_input.eye_gaze_available;
}

bool OpenXRGetHeadPose(Mat4 &head_pose) {
	head_pose = m_input.head_pose;
	return m_input.head_pose_available;
}

bool RenderLayer(OpenXRFrameInfo &frameInfo,
                 std::vector<XrCompositionLayerProjectionView> &projectionLayerViews,
                 std::vector<XrCompositionLayerDepthInfoKHR> &depthLayerViews,
                 XrCompositionLayerProjection &layer,
                 const Mat4 &cam_offset,
                 std::function<void(Mat4 *head)> update_controllers,
                 std::function<uint16_t(Rect<int> *rect, ViewState *view_state, uint16_t *view_id, bgfx::FrameBufferHandle *fb)> draw_scene,
                 uint16_t &view_id,
                 float z_near,
                 float z_far) {
	m_input.head_pose_available = false;
	XrResult res;

	XrViewState viewState{XR_TYPE_VIEW_STATE};
	uint32_t viewCapacityInput = (uint32_t)m_views.size();
	uint32_t viewCountOutput;

	XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
	viewLocateInfo.viewConfigurationType = m_viewConfigType;
	viewLocateInfo.displayTime = frameInfo.predictedDisplayTime;
	viewLocateInfo.space = m_appSpace;

	res = xrLocateViews(m_session, &viewLocateInfo, &viewState, viewCapacityInput, &viewCountOutput, m_views.data());
	//	CHECK_XRRESULT(res, "xrLocateViews");
	if ((viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0 || (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0) {
		return false; // There is no valid tracking poses for the views.
	}

	m_input.head_pose_available = true;

	// update hand mat		// immediately update the input matrice for the render sync
	for (auto hand : {HandsSide::LEFT, HandsSide::RIGHT}) {
		XrSpaceLocation spaceLocation{XR_TYPE_SPACE_LOCATION};
		res = xrLocateSpace(m_input.handSpace[hand], m_appSpace, frameInfo.predictedDisplayTime, &spaceLocation);
		if (XR_UNQUALIFIED_SUCCESS(res)) {
			if ((spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 && (spaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
				XrMatrix4x4f handPose;
				XrVector3f scale3{1.f, 1.f, 1.f};
				XrMatrix4x4f_CreateTranslationRotationScale(&handPose, &spaceLocation.pose.position, &spaceLocation.pose.orientation, &scale3);

				OpenXR_hand_device_states[hand].mtx = cam_offset * OXRToMat4(handPose);
			}
		} else {
			// Tracking loss is expected when the hand is not active so only log a message
			// if the hand is active.
			if (OpenXR_hand_device_states[hand].connected == XR_TRUE) {
				const char *handName[] = {"left", "right"};
				debug(format("Unable to locate %1 hand action space in app space: %2").arg(handName[hand]).arg(res));
			}
		}

		// update hand joints
		if (ExtensionsFlagsEnable & OXRExtensions_HandTracking && m_input.jointLocations[hand].type != XR_TYPE_UNKNOWN) {
			XrHandJointsLocateInfoEXT locateInfo{XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT};
			locateInfo.baseSpace = m_appSpace;
			locateInfo.time = frameInfo.predictedDisplayTime;

			if (m_input.pfnLocateHandJointsEXT(m_input.handJointTracker[hand], &locateInfo, &m_input.jointLocations[hand]) < 0)
				debug("no handjoint located");
		}
	}

	// tracker pos
	if (ExtensionsFlagsEnable & OXRExtensions_Tracker && m_input.trackerSpace[0]) {
		OpenXR_tracker_device_states[0].connected = false;
		XrSpaceLocation spaceLocation{XR_TYPE_SPACE_LOCATION};
		res = xrLocateSpace(m_input.trackerSpace[0], m_appSpace, frameInfo.predictedDisplayTime, &spaceLocation);
		if (XR_UNQUALIFIED_SUCCESS(res)) {
			if ((spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 && (spaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
				XrMatrix4x4f triggerPose;
				XrVector3f scale3{1.f, 1.f, 1.f};
				XrMatrix4x4f_CreateTranslationRotationScale(&triggerPose, &spaceLocation.pose.position, &spaceLocation.pose.orientation, &scale3);

				OpenXR_tracker_device_states[0].mtx = cam_offset * OXRToMat4(triggerPose);
				OpenXR_tracker_device_states[0].connected = true;
			}
		} else {
			// Tracking loss is expected when the hand is not active so only log a message
			// if the tracker is active.
			//			if (OpenXR_tracker_device_states[0].connected == XR_TRUE)
			//				debug(format("Unable to locate %1 tracker action space in app space: %2").arg(0).arg(res));
		}
	}

	XrMatrix4x4f headPose;
	XrSpaceLocation view_in_stage = {XR_TYPE_SPACE_LOCATION, NULL, 0, {{0, 0, 0, 1}, {0, 0, 0}}};
	xrLocateSpace(m_appViewSpace, m_appSpace, frameInfo.predictedDisplayTime, &view_in_stage);
	XrVector3f scale3{1.f, 1.f, 1.f};
	XrMatrix4x4f_CreateTranslationRotationScale(&headPose, &view_in_stage.pose.position, &view_in_stage.pose.orientation, &scale3);
	m_input.head_pose = cam_offset * OXRToMat4(headPose);

	// eye gazing
	// m_input.eye_gaze_available = false;
	// XrActionStatePose actionStatePose{XR_TYPE_ACTION_STATE_POSE};
	// XrActionStateGetInfo getActionStateInfo{XR_TYPE_ACTION_STATE_GET_INFO};
	// getActionStateInfo.action = m_input.gazeAction;
	// if (CHECK_XRCMD(xrGetActionStatePose(m_session, &getActionStateInfo, &actionStatePose)) < 0) {
	//	// no gaze
	//}
	// if (actionStatePose.isActive)
	if (ExtensionsFlagsEnable & OXRExtensions_EyeGaze) {
		XrEyeGazeSampleTimeEXT eyeGazeSampleTime{XR_TYPE_EYE_GAZE_SAMPLE_TIME_EXT};
		XrSpaceLocation gazeLocation{XR_TYPE_SPACE_LOCATION, &eyeGazeSampleTime};
		if (CHECK_XRCMD(xrLocateSpace(m_input.gazeSpace, m_appSpace, frameInfo.predictedDisplayTime, &gazeLocation)) >= 0) {
			if ((gazeLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 && (gazeLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
				XrMatrix4x4f eyePose;
				XrVector3f scale3{1.f, 1.f, -1.f};
				XrMatrix4x4f_CreateTranslationRotationScale(&eyePose, &gazeLocation.pose.position, &gazeLocation.pose.orientation, &scale3);
				// inverse x axis rotation for the eye gaze
				m_input.eye_gaze = OXRToMat4(eyePose);
				Vec3 rot = GetR(m_input.eye_gaze);
				rot.x *= -1;
				m_input.eye_gaze = m_input.head_pose * TransformationMat4(GetT(m_input.eye_gaze), RotationMat3(rot), GetS(m_input.eye_gaze));
				m_input.eye_gaze_available = true;
			}
		}
	}

	// call user code to update controllers matrices to be sync
	update_controllers(&m_input.head_pose);

	projectionLayerViews.resize(viewCountOutput);
	depthLayerViews.resize(viewCountOutput);
	frameInfo.id_fbs.resize(viewCountOutput);

	// Render view to the appropriate part of the swapchain image.
	for (uint32_t i = 0; i < viewCountOutput; i++) {
		// Each view has a separate swapchain which is acquired, rendered to, and released.
		const Swapchain viewSwapchain = m_swapchains[i];

		XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};

		uint32_t swapchainImageIndex;
		if (CHECK_XRCMD(xrAcquireSwapchainImage(viewSwapchain.handle, &acquireInfo, &swapchainImageIndex)) < 0)
			return false;

		XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
		waitInfo.timeout = XR_INFINITE_DURATION;
		if (CHECK_XRCMD(xrWaitSwapchainImage(viewSwapchain.handle, &waitInfo)) < 0)
			return false;

		projectionLayerViews[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
		projectionLayerViews[i].pose = m_views[i].pose;
		projectionLayerViews[i].fov = m_views[i].fov;
		projectionLayerViews[i].subImage.swapchain = viewSwapchain.handle;
		projectionLayerViews[i].subImage.imageRect.offset = {0, 0};
		projectionLayerViews[i].subImage.imageRect.extent = {viewSwapchain.width, viewSwapchain.height};

		if (ExtensionsFlagsEnable & OXRExtensions_COMPOSITION_LAYER_DEPTH) {
			depthLayerViews[i] = {XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR};
			depthLayerViews[i].subImage = projectionLayerViews[i].subImage;
			depthLayerViews[i].subImage.swapchain = viewSwapchain.handleDepth;
			depthLayerViews[i].minDepth = 0;
			depthLayerViews[i].maxDepth = 1;
			depthLayerViews[i].nearZ = z_near;
			depthLayerViews[i].farZ = z_far;
			projectionLayerViews[i].next = &depthLayerViews[i];

			uint32_t depthSwapchainImageIndex = 0;
			xrAcquireSwapchainImage(viewSwapchain.handleDepth, &acquireInfo, &depthSwapchainImageIndex);
			xrWaitSwapchainImage(viewSwapchain.handleDepth, &waitInfo);
		}

		const hg::Vec2 aspect_ratio{viewSwapchain.height ? (float)viewSwapchain.width / (float)viewSwapchain.height : 1.f, 1.f};

		// create proj/view matrices
		XrMatrix4x4f xproj;
		XrMatrix4x4f_CreateProjectionFov(&xproj, GRAPHICS_D3D, m_views[i].fov, z_near, z_far);
		XrMatrix4x4f toView;
		XrVector3f scale{1.f, 1.f, 1.f};
		XrMatrix4x4f_CreateTranslationRotationScale(&toView, &m_views[i].pose.position, &m_views[i].pose.orientation, &scale);

		auto cam = cam_offset * OXRToMat4(toView);
		const auto proj = OXRToMat44(xproj);

		hg::ViewState view_state{TransformFrustum(MakeFrustum(proj), cam), proj, hg::InverseFast(cam)};
		auto rect = hg::MakeRectFromWidthHeight(0, 0, int(viewSwapchain.width), int(viewSwapchain.height));

		view_id = draw_scene(&rect, &view_state, &view_id, &m_swapchainEyesFramebuffer[viewSwapchain.handle][swapchainImageIndex].fb);
		frameInfo.id_fbs[i] = swapchainImageIndex + i * m_swapchainEyesFramebuffer[viewSwapchain.handle].size();

		XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
		if (CHECK_XRCMD(xrReleaseSwapchainImage(viewSwapchain.handle, &releaseInfo)) < 0)
			return false;
		if (ExtensionsFlagsEnable & OXRExtensions_COMPOSITION_LAYER_DEPTH)
			xrReleaseSwapchainImage(viewSwapchain.handleDepth, &releaseInfo);
	}

	layer.space = m_appSpace;
	layer.viewCount = (uint32_t)projectionLayerViews.size();
	layer.views = projectionLayerViews.data();
	layer.layerFlags = (m_environmentBlendMode == XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND) ? XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT : 0;
	return true;
}

OpenXRFrameInfo OpenXRSubmitSceneToForwardPipeline(const Mat4 &cam_offset,
                                                   std::function<void(Mat4 *head)> update_controllers,
                                                   std::function<uint16_t(Rect<int> *rect, ViewState *view_state, uint16_t *view_id, bgfx::FrameBufferHandle *fb)> draw_scene,
                                                   uint16_t &view_id,
                                                   float z_near,
                                                   float z_far) {

	OpenXRFrameInfo frameInfo;
	frameInfo.shouldEndFrame = false;

	bool exitRenderLoop = false, requestRestart = false;
	PollEvents(&exitRenderLoop, &requestRestart);

	PollActions();

	XrFrameWaitInfo frameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
	XrFrameState frameState{XR_TYPE_FRAME_STATE};
	if (CHECK_XRCMD(xrWaitFrame(m_session, &frameWaitInfo, &frameState)) < 0)
		return frameInfo;

	XrFrameBeginInfo frameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};
	if (CHECK_XRCMD(xrBeginFrame(m_session, &frameBeginInfo)) < 0)
		return frameInfo;

	frameInfo.shouldEndFrame = true;
	frameInfo.predictedDisplayTime = frameState.predictedDisplayTime;
	if (frameState.shouldRender == XR_TRUE) {
		if (RenderLayer(frameInfo, projectionLayerViews, depthLayerViews, layer, cam_offset, update_controllers, draw_scene, view_id, z_near, z_far)) {
			// for now the layer list is just one layer, but it will be use for AR app
			frameInfo.layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&layer));
			if (layerPasstrough.layerHandle)
				frameInfo.layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&layerPasstrough));
		}
	}

	return frameInfo;
}

void OpenXRFinishSubmitFrameBuffer(const OpenXRFrameInfo &frameInfo) {
	if (frameInfo.shouldEndFrame) {
		XrFrameEndInfo frameEndInfo{XR_TYPE_FRAME_END_INFO};
		frameEndInfo.displayTime = frameInfo.predictedDisplayTime;
		frameEndInfo.environmentBlendMode = m_environmentBlendMode;
		frameEndInfo.layerCount = (uint32_t)frameInfo.layers.size();
		frameEndInfo.layers = frameInfo.layers.data();
		if (CHECK_XRCMD(xrEndFrame(m_session, &frameEndInfo)) < 0)
			return;
	}
}

static const uint64_t OXRaa_flags[] = {0, BGFX_TEXTURE_RT_MSAA_X2, BGFX_TEXTURE_RT_MSAA_X4, BGFX_TEXTURE_RT_MSAA_X8, BGFX_TEXTURE_RT_MSAA_X16};

//
std::vector<OpenXREyeFrameBuffer> OpenXRCreateEyeFrameBuffer(OpenXRAA aa) {
	std::vector<OpenXREyeFrameBuffer> AllEyesFramebuffer;

	// Render view to the appropriate part of the swapchain image.
	for (uint32_t i = 0; i < m_swapchainImages.size(); i++) {
		// Each view has a separate swapchain which is acquired, rendered to, and released.
		const Swapchain viewSwapchain = m_swapchains[i];
		std::vector<OpenXREyeFrameBuffer> swapchainEyesFramebuffer;
		for (uint32_t j = 0; j < m_swapchainImages[viewSwapchain.handle].size(); j++) {
			const auto &swapchainImage = m_swapchainImages[viewSwapchain.handle][j];
			XrSwapchainImageBaseHeader *swapchainDepthImage;
			if (ExtensionsFlagsEnable & OXRExtensions_COMPOSITION_LAYER_DEPTH)
				swapchainDepthImage = m_swapchainDepthImages[viewSwapchain.handleDepth][j];

			uintptr_t colorTexture = 0, depthTexture = 0;
			switch (bgfx::getRendererType()) {
#ifdef XR_USE_GRAPHICS_API_D3D11
				case bgfx::RendererType::Direct3D11: colorTexture = (uintptr_t)(reinterpret_cast<const XrSwapchainImageD3D11KHR*>(swapchainImage)->texture);
					if (ExtensionsFlagsEnable & OXRExtensions_COMPOSITION_LAYER_DEPTH)
						depthTexture = (uintptr_t)(reinterpret_cast<const XrSwapchainImageD3D11KHR*>(
							swapchainDepthImage)->texture);
					break;
#endif
#ifdef XR_USE_GRAPHICS_API_D3D12
				case bgfx::RendererType::Direct3D12:
					colorTexture = (uintptr_t)(reinterpret_cast<const XrSwapchainImageD3D12KHR *>(swapchainImage)->texture);
					if (ExtensionsFlagsEnable & OXRExtensions_COMPOSITION_LAYER_DEPTH)
						depthTexture = (uintptr_t)(reinterpret_cast<const XrSwapchainImageD3D12KHR *>(swapchainDepthImage)->texture);
					break;
#endif
				case bgfx::RendererType::OpenGL: colorTexture = (uintptr_t)(reinterpret_cast<const XrSwapchainImageOpenGLKHR*>(swapchainImage)->image);
					if (ExtensionsFlagsEnable & OXRExtensions_COMPOSITION_LAYER_DEPTH)
						depthTexture = (uintptr_t)(reinterpret_cast<const XrSwapchainImageOpenGLKHR*>(
							swapchainDepthImage)->image);
					break;
#ifdef XR_USE_GRAPHICS_API_VULKAN
				case bgfx::RendererType::Vulkan:
					colorTexture = (uintptr_t)(reinterpret_cast<const XrSwapchainImageVulkanKHR *>(swapchainImage)->image);
					if (ExtensionsFlagsEnable & OXRExtensions_COMPOSITION_LAYER_DEPTH)
						depthTexture = (uintptr_t)(reinterpret_cast<const XrSwapchainImageVulkanKHR *>(swapchainDepthImage)->image);
					break;
#endif
			}

			////////////////////////////////

			// BGFX_TEXTURE_SRGB
			OpenXREyeFrameBuffer eye;
			eye.aa = OXRAA_None;
			eye.width = viewSwapchain.width;
			eye.height = viewSwapchain.height;
			eye.color = bgfx::createTexture2D(viewSwapchain.width, viewSwapchain.height, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_RT | OXRaa_flags[eye.aa]);

			eye.depth = bgfx::createTexture2D(viewSwapchain.width, viewSwapchain.height, false, 1, bgfx::TextureFormat::D24S8,
			                                  BGFX_TEXTURE_RT_WRITE_ONLY | OXRaa_flags[eye.aa]);

			bgfx::frame(); // so that the texture gets created
			bgfx::frame(); // so that the texture gets created

			eye.native = bgfx::overrideInternal(eye.color, colorTexture);
			if (ExtensionsFlagsEnable & OXRExtensions_COMPOSITION_LAYER_DEPTH)
				eye.nativeDepth = bgfx::overrideInternal(eye.depth, depthTexture);

			bgfx::TextureHandle h[2] = {eye.color, eye.depth};
			eye.fb = bgfx::createFrameBuffer(2, h, false);
			swapchainEyesFramebuffer.push_back(eye);
			AllEyesFramebuffer.push_back(eye);
		}
		m_swapchainEyesFramebuffer.insert(std::make_pair(viewSwapchain.handle, std::move(swapchainEyesFramebuffer)));
	}
	return AllEyesFramebuffer;
}

void OpenXRDestroyEyeFrameBuffer(OpenXREyeFrameBuffer &eye_fb) {
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

Texture OpenXRGetColorTexture(const OpenXREyeFrameBuffer &eye) {
	return {BGFX_TEXTURE_RT | OXRaa_flags[eye.aa], eye.color};
}

Texture OpenXRGetDepthTexture(const OpenXREyeFrameBuffer &eye) {
	return {BGFX_TEXTURE_RT_WRITE_ONLY | OXRaa_flags[eye.aa], eye.depth};
}

Texture OpenXRGetColorTextureFromId(const std::vector<OpenXREyeFrameBuffer> &eyes, const OpenXRFrameInfo &frame_info, const int &index) {
	return {BGFX_TEXTURE_RT | OXRaa_flags[eyes[frame_info.id_fbs[index]].aa], eyes[frame_info.id_fbs[index]].color};
}

Texture OpenXRGetDepthTextureFromId(const std::vector<OpenXREyeFrameBuffer> &eyes, const OpenXRFrameInfo &frame_info, const int &index) {
	return {BGFX_TEXTURE_RT_WRITE_ONLY | OXRaa_flags[eyes[frame_info.id_fbs[index]].aa], eyes[frame_info.id_fbs[index]].depth};
}

bool IsHandJointActive(HandsSide hand) { return m_input.jointLocations[hand].isActive; };

Mat4 GetHandJointPose(HandsSide hand, XrHandJoint handJoint) {
	if (handJoint < 0 || handJoint > XR_HAND_JOINT_LITTLE_TIP_EXT)
		return Mat4::Identity;
	XrMatrix4x4f m;
	XrVector3f s;
	XrVector3f_Set(&s, 1);
	XrMatrix4x4f_CreateTranslationRotationScale(&m, &m_input.jointLocation[hand][handJoint].pose.position, &m_input.jointLocation[hand][handJoint].pose.orientation, &s);
	return OXRToMat4(m);
};

float GetHandJointRadius(HandsSide hand, XrHandJoint handJoint) {
	if (handJoint < 0 || handJoint > XR_HAND_JOINT_LITTLE_TIP_EXT)
		return 0.f;
	return m_input.jointLocation[hand][handJoint].radius;
};

Vec3 GetHandJointLinearVelocity(HandsSide hand, XrHandJoint handJoint) {
	if (handJoint < 0 || handJoint > XR_HAND_JOINT_LITTLE_TIP_EXT)
		return Vec3::Zero;
	XrVector3f v(m_input.jointVelocitie[hand][handJoint].linearVelocity);
	return Vec3(v.x, v.y, v.z);
};

Vec3 GetHandJointAngularVelocity(HandsSide hand, XrHandJoint handJoint) {
	if (handJoint < 0 || handJoint > XR_HAND_JOINT_LITTLE_TIP_EXT)
		return Vec3::Zero;
	XrVector3f v(m_input.jointVelocitie[hand][handJoint].angularVelocity);
	return Vec3(v.x, v.y, v.z);
};
} // namespace hg

#else

namespace hg {

bool OpenXRInit(OpenXRExtensionsFlags ExtensionsFlagsEnable) {
	error("OpenXR support DISABLED when building Harfang");
	return false;
}

void OpenXRShutdown() {}

std::vector<OpenXREyeFrameBuffer> OpenXRCreateEyeFrameBuffer(OpenXRAA aa) { return {}; }
void OpenXRDestroyEyeFrameBuffer(OpenXREyeFrameBuffer &) {}

std::string OpenXRGetInstanceInfo() { return ""; };
bool OpenXRGetEyeGaze(Mat4 &eye_gaze) { return false; };
bool OpenXRGetHeadPose(Mat4 &head_pose) { return false; };


OpenXRFrameInfo OpenXRSubmitSceneToForwardPipeline(const Mat4 &cam_offset,
                                                   std::function<void(Mat4 *head)> update_controllers,
                                                   std::function<uint16_t(Rect<int> *rect, ViewState *view_state, uint16_t *view_id, bgfx::FrameBufferHandle *fb)> draw_scene,
                                                   uint16_t &view_id,
                                                   float z_near,
                                                   float z_far) {
	return OpenXRFrameInfo();
};

void OpenXRFinishSubmitFrameBuffer(const OpenXRFrameInfo &frameInfo){};

Texture OpenXRGetColorTexture(const OpenXREyeFrameBuffer &eye) { return {}; }
Texture OpenXRGetDepthTexture(const OpenXREyeFrameBuffer &eye) { return {}; }
Texture OpenXRGetColorTextureFromId(const std::vector<OpenXREyeFrameBuffer> &eyes, const OpenXRFrameInfo &frame_info, const int &index) { return {}; }
Texture OpenXRGetDepthTextureFromId(const std::vector<OpenXREyeFrameBuffer> &eyes, const OpenXRFrameInfo &frame_info, const int &index) { return {}; }

bool IsHandJointActive(HandsSide hand) { return false; };
Mat4 GetHandJointPose(HandsSide hand, XrHandJoint handJoint) { return Mat4::Identity; };
float GetHandJointRadius(HandsSide hand, XrHandJoint handJoint) { return 0.f; };
Vec3 GetHandJointLinearVelocity(HandsSide hand, XrHandJoint handJoint) { return Vec3::Zero; };
Vec3 GetHandJointAngularVelocity(HandsSide hand, XrHandJoint handJoint) { return Vec3::Zero; };

} // namespace hg

#endif
