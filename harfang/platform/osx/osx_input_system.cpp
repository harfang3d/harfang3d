// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "osx_input_system/osx_input_system.h"
#include "osx_input_system/osx_input_keyboard.h"
#include "osx_input_system/osx_input_mouse.h"
#include "window_system/window_system.h"
#include "cstl/log.h"

namespace gs {
namespace input {

void SystemOSX::SetWindowHandle(Window::Handle handle) { window = handle; }

void SystemOSX::Update()
{
	lock_guard guard(lock);
	if (window == nullptr)
		return;

	mouse->Update(window);
	keyboard->Update(window);
}

void SystemOSX::GetDevices(vector<DeviceDesc> &out) const
{
	out.reserve(16);

	{
		auto &d = *out.emplace(out.end());
		d.id = "mouse";
		d.name = "Mouse";
	}

	{
		auto &d = *out.emplace(out.end());
		d.id = "keyboard";
		d.name = "Keyboard";
	}
}

Device *SystemOSX::GetDevice(const char *name)
{
	string _name(name);

	if (_name == "mouse")
		return mouse;
	if (_name == "keyboard")
		return keyboard;

	return nullptr;
}

SystemOSX::SystemOSX() : window(0), mouse(new MouseOSX), keyboard(new KeyboardOSX) {}

} // input
} // gs
