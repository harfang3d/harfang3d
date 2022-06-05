// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/time.h"

namespace hg {

struct Vec3;

/*!
	Implement a first-person-shooter like controller.
	The input position and rotation parameters are returned modified according to the state of the control keys.
	This function is usually used by passing the current camera position and rotation then updating the camera transformation with the returned values.
*/
void FpsController(
	bool key_up, bool key_down, bool key_left, bool key_right, bool btn, float dx, float dy, Vec3 &pos, Vec3 &rot, float speed, time_ns dt_t);

struct Keyboard;
struct Mouse;

void FpsController(const Keyboard &keyboard, const Mouse &mouse, Vec3 &pos, Vec3 &rot, float speed, time_ns dt_t);

} // namespace hg
