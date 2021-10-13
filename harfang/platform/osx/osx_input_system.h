// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "input_system/input_system.h"
#include "input_system/input_mouse.h"
#include "input_system/input_keyboard.h"

namespace gs {
namespace input {

/// Input system OSX
class SystemOSX : public System
{
public:
	SystemOSX();

	/// Set window handle.
	void SetWindowHandle(Window::Handle handle) override;

	void Update() override;

	/// Return a list of devices available on this system.
	void GetDevices(vector<DeviceDesc> &devices) const override;
	/// Get device.
	Device *GetDevice(const char *name) override;

private:
	Window::Handle window;

	shared_ptr<Device> mouse, keyboard;
};

} // input
} // gs
