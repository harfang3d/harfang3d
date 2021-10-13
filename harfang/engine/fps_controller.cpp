// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/math.h"
#include "foundation/matrix3.h"
#include "foundation/time.h"
#include "foundation/vector3.h"
#include "platform/input_system.h"

namespace hg {

void FpsController(bool key_up, bool key_down, bool key_left, bool key_right, bool btn, float dx, float dy, Vec3 &pos, Vec3 &rot, float speed, time_ns dt_t) {
	const auto world = RotationMat3(rot.x, rot.y, rot.z);
	const auto right = GetX(world), front = GetZ(world);

	Vec3 dt{};

	if (key_up)
		dt += front;
	else if (key_down)
		dt -= front;

	if (key_left)
		dt -= right;
	else if (key_right)
		dt += right;

	pos += dt * speed * time_to_sec_f(dt_t);

	if (btn) {
		rot += {-dy * 0.005f, dx * 0.005f, 0.f};
		rot.x = Wrap(rot.x, -Pi, Pi);
	}
}

void FpsController(const Keyboard &keyboard, const Mouse &mouse, Vec3 &pos, Vec3 &rot, float speed, time_ns dt_t) {
	FpsController(keyboard.Down(K_Up) || keyboard.Down(K_W), keyboard.Down(K_Down) || keyboard.Down(K_S), keyboard.Down(K_Left) || keyboard.Down(K_A),
		keyboard.Down(K_Right) || keyboard.Down(K_D), mouse.Down(0), float(mouse.DtX()), float(mouse.DtY()), pos, rot, speed, dt_t);
}

} // namespace hg
