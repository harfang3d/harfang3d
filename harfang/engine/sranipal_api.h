// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/matrix4.h"
#include "foundation/matrix44.h"

namespace hg {

struct ViewState;

bool SRanipalInit();
void SRanipalShutdown();

struct SRanipalEyeState {
	bool pupil_diameter_valid; // if the pupil diameter value is valid
	Vec3 gaze_origin_mm; // the point in the eye from which the gaze ray originates in meter miles.(right-handed coordinate system)
	Vec3 gaze_direction_normalized; // the normalized gaze direction of the eye in [0,1].(right-handed coordinate system)
	float pupil_diameter_mm; // the diameter of the pupil in meter miles
	float eye_openness; // a value representing how open the eye is
};

struct SRanipalState {
	SRanipalEyeState right_eye, left_eye;
};

void SRanipalLaunchEyeCalibration();
bool SRanipalIsViveProEye();

SRanipalState SRanipalGetState();

} // namespace hg
