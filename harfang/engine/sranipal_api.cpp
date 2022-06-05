// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include "engine/scene.h"
#include "engine/sranipal_api.h"

#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/math.h"

#include "platform/input_system.h"

#include <thread>

#if HG_ENABLE_SRANIPAL_API

#include <SRanipal.h>
#include <SRanipal_Eye.h>

#pragma comment(lib, "SRanipal.lib")
using namespace ViveSR;

namespace hg {

static bool eye_tracking_enabled = false;

//
bool SRanipalInit() {
	if (!ViveSR::anipal::Eye::IsViveProEye()) {
		error("This device does not support SRanipal eye-tracking");
		return false;
	}

	int err = ViveSR::anipal::Initial(ViveSR::anipal::Eye::ANIPAL_TYPE_EYE, NULL);

	if (err == ViveSR::Error::WORK) {
		eye_tracking_enabled = true;
		log("Eye tracking API successfully initialized");
		return true;
	} else if (err == ViveSR::Error::RUNTIME_NOT_FOUND) {
		error("SRanipal runtime not found, please follow the SDK guide on how to install it");
	} else {
		error(format("Failed to initialize eye-tracking runtime, error code: %1").arg(err));
	}

	return false;
}

void SRanipalShutdown() {
	debug("SRanipal shutting down");

	ViveSR::anipal::Release(ViveSR::anipal::Eye::ANIPAL_TYPE_EYE);
}

void SRanipalLaunchEyeCalibration() { ViveSR::anipal::Eye::LaunchEyeCalibration(nullptr); }
bool SRanipalIsViveProEye() { return ViveSR::anipal::Eye::IsViveProEye(); }

//
SRanipalState SRanipalGetState() {
	if (!eye_tracking_enabled)
		return {};

	ViveSR::anipal::Eye::EyeData eye_data;
	if (ViveSR::anipal::Eye::GetEyeData(&eye_data) != ViveSR::Error::WORK)
		return {};

	SRanipalState state;

	state.right_eye.pupil_diameter_valid =
		eye_data.verbose_data.right.eye_data_validata_bit_mask & ViveSR::anipal::Eye::SINGLE_EYE_DATA_PUPIL_DIAMETER_VALIDITY;
	state.right_eye.gaze_origin_mm.x = -eye_data.verbose_data.right.gaze_origin_mm.x;
	state.right_eye.gaze_origin_mm.y = eye_data.verbose_data.right.gaze_origin_mm.y;
	state.right_eye.gaze_origin_mm.z = eye_data.verbose_data.right.gaze_origin_mm.z;
	state.right_eye.gaze_direction_normalized.x = -eye_data.verbose_data.right.gaze_direction_normalized.x;
	state.right_eye.gaze_direction_normalized.y = eye_data.verbose_data.right.gaze_direction_normalized.y;
	state.right_eye.gaze_direction_normalized.z = eye_data.verbose_data.right.gaze_direction_normalized.z;
	state.right_eye.pupil_diameter_mm = eye_data.verbose_data.right.pupil_diameter_mm;
	state.right_eye.eye_openness = eye_data.verbose_data.right.eye_openness;

	state.left_eye.pupil_diameter_valid = eye_data.verbose_data.right.eye_data_validata_bit_mask & ViveSR::anipal::Eye::SINGLE_EYE_DATA_PUPIL_DIAMETER_VALIDITY;
	state.left_eye.gaze_origin_mm.x = -eye_data.verbose_data.left.gaze_origin_mm.x;
	state.left_eye.gaze_origin_mm.y = eye_data.verbose_data.left.gaze_origin_mm.y;
	state.left_eye.gaze_origin_mm.z = eye_data.verbose_data.left.gaze_origin_mm.z;
	state.left_eye.gaze_direction_normalized.x = -eye_data.verbose_data.left.gaze_direction_normalized.x;
	state.left_eye.gaze_direction_normalized.y = eye_data.verbose_data.left.gaze_direction_normalized.y;
	state.left_eye.gaze_direction_normalized.z = eye_data.verbose_data.left.gaze_direction_normalized.z;
	state.left_eye.pupil_diameter_mm = eye_data.verbose_data.left.pupil_diameter_mm;
	state.left_eye.eye_openness = eye_data.verbose_data.left.eye_openness;

	return state;
}

} // namespace hg

#else

namespace hg {

bool SRanipalInit() {
	warn("SRanipal eye-tracking support DISABLED when building Harfang");
	return false;
}

void SRanipalShutdown() {}
void SRanipalLaunchEyeCalibration() {}
bool SRanipalIsViveProEye() { return false; }

SRanipalState SRanipalGetState() { return {}; }

} // namespace hg

#endif
