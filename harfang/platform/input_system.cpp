// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/format.h"
#include "foundation/log.h"

#include "platform/input_system.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <iterator>

namespace hg {

Signal<void(const char *)> on_text_input;

// Mouse

static std::map<std::string, MouseReader> mouses;

void AddMouseReader(const char *name, MouseReader reader) { mouses[name] = reader; }

void RemoveMouseReader(const char *name) {
	auto i = mouses.find(name);
	if (i != std::end(mouses))
		mouses.erase(i);
}

MouseState ReadMouse(const char *name) {
	auto i = mouses.find(name);
	if (i == std::end(mouses)) {
		if (name && !strcmp(name, "default"))
			warn("Failed to read default mouse, was InputInit() called?");
		return {};
	}
	return i->second();
}

std::vector<std::string> GetMouseNames() {
	std::vector<std::string> names;
	std::transform(std::begin(mouses), std::end(mouses), std::back_inserter(names), [](const auto &i) { return i.first; });
	return names;
}

Mouse::Mouse(const char *n) : name(n) {}

void Mouse::Update() {
	std::copy(&state, &state + 1, &old_state);
	state = ReadMouse(name.c_str());
}

// Keyboard

struct KeyboardInterface {
	KeyboardReader read;
	const char *(*get_key_name)(Key);
};

std::map<std::string, KeyboardInterface> keyboards;

void AddKeyboardReader(const char *name, KeyboardReader reader, KeyboardGetKeyName get_key_name) { keyboards[name] = {reader, get_key_name}; }

void RemoveKeyboardReader(const char *name) {
	auto i = keyboards.find(name);
	if (i != std::end(keyboards))
		keyboards.erase(i);
}

KeyboardState ReadKeyboard(const char *name) {
	auto i = keyboards.find(name);
	if (i == std::end(keyboards)) {
		if (name && !strcmp(name, "default"))
			warn("Failed to read default keyboard, was InputInit() called?");
		return {};
	}
	return i->second.read();
}

const char *GetKeyName(Key key, const char *name) {
	auto i = keyboards.find(name);
	return i != std::end(keyboards) ? i->second.get_key_name(key) : nullptr;
}

std::vector<std::string> GetKeyboardNames() {
	std::vector<std::string> names;
	std::transform(std::begin(keyboards), std::end(keyboards), std::back_inserter(names), [](const auto &i) { return i.first; });
	return names;
}

Keyboard::Keyboard(const char *n) : name(n) {}

void Keyboard::Update() {
	std::copy(&state, &state + 1, &old_state);
	state = ReadKeyboard(name.c_str());
}

// Gamepad

static std::map<std::string, GamepadReader> joypads;

void AddGamepadReader(const char *name, GamepadReader reader) { joypads[name] = reader; }

void RemoveGamepadReader(const char *name) {
	auto i = joypads.find(name);
	if (i != std::end(joypads))
		joypads.erase(i);
}

GamepadState ReadGamepad(const char *name) {
	auto i = joypads.find(name);
	return i != std::end(joypads) ? i->second() : GamepadState{};
}

std::vector<std::string> GetGamepadNames() {
	std::vector<std::string> names;
	std::transform(std::begin(joypads), std::end(joypads), std::back_inserter(names), [](const auto &i) { return i.first; });
	return names;
}

Gamepad::Gamepad(const char *n) : name(n) {}

void Gamepad::Update() {
	std::copy(&state, &state + 1, &old_state);
	state = ReadGamepad(name.c_str());
}

// Joystick

struct JoystickCallback {
	JoystickReader Reader;
	JoystickDeviceName DeviceName;
};
static std::map<std::string, JoystickCallback> joysticks;

void AddJoystickReader(const char *name, JoystickReader reader, JoystickDeviceName devicename) {
	joysticks[name] = {reader, devicename};
}

void RemoveJoystickReader(const char *name) {
	auto i = joysticks.find(name);
	if (i != std::end(joysticks))
		joysticks.erase(i);
}

JoystickState ReadJoystick(const char *name) {
	auto i = joysticks.find(name);
	return i != std::end(joysticks) ? i->second.Reader() : JoystickState{};
}

std::string DeviceNameJoystick(const char *name) {
	auto i = joysticks.find(name);
	return i != std::end(joysticks) ? i->second.DeviceName() : "";
}

std::vector<std::string> GetJoystickNames() {
	std::vector<std::string> names;
	std::transform(std::begin(joysticks), std::end(joysticks), std::back_inserter(names), [](const auto &i) { return i.first; });
	return names;
}

std::vector<std::string> GetJoystickDeviceNames() {
	std::vector<std::string> names;
	std::transform(std::begin(joysticks), std::end(joysticks), std::back_inserter(names), [](const auto &i) { return i.second.DeviceName(); });
	return names;
}

Joystick::Joystick(const char *n) : name(n) {}

void Joystick::Update() {
	std::copy(&state, &state + 1, &old_state);
	state = ReadJoystick(name.c_str());
}

std::string Joystick::GetDeviceName() const {
	return DeviceNameJoystick(name.c_str());
}

// VR controller

struct VRControllerInterface {
	VRControllerReader read;
	VRControllerSendHapticPulse send_haptic_pulse;
};

static std::map<std::string, VRControllerInterface> vrcontrollers;

void AddVRControllerReader(const char *name, VRControllerReader reader, VRControllerSendHapticPulse send_haptic_pulse) {
	vrcontrollers[name] = {reader, send_haptic_pulse};
}

void RemoveVRControllerReader(const char *name) {
	auto i = vrcontrollers.find(name);
	if (i != std::end(vrcontrollers))
		vrcontrollers.erase(i);
}

VRControllerState ReadVRController(const char *name) {
	auto i = vrcontrollers.find(name);
	return i != std::end(vrcontrollers) ? i->second.read() : VRControllerState{};
}

void SendVRControllerHapticPulse(time_ns duration, const char *name) {
	auto i = vrcontrollers.find(name);
	if (i != std::end(vrcontrollers))
		i->second.send_haptic_pulse(duration);
}

std::vector<std::string> GetVRControllerNames() {
	std::vector<std::string> names;
	std::transform(std::begin(vrcontrollers), std::end(vrcontrollers), std::back_inserter(names), [](const auto &i) { return i.first; });
	return names;
}

VRController::VRController(const char *n) : name(n) {}

void VRController::Update() {
	std::copy(&state, &state + 1, &old_state);
	state = ReadVRController(name.c_str());
}

void VRController::SendHapticPulse(time_ns duration) { SendVRControllerHapticPulse(duration, name.c_str()); }

// VR generic tracker

static std::map<std::string, VRGenericTrackerReader> vrgenerictrackers;

void AddVRGenericTrackerReader(const char *name, VRGenericTrackerReader reader) { vrgenerictrackers[name] = {reader}; }

void RemoveVRGenericTrackerReader(const char *name) {
	auto i = vrgenerictrackers.find(name);
	if (i != std::end(vrgenerictrackers))
		vrgenerictrackers.erase(i);
}

VRGenericTrackerState ReadVRGenericTracker(const char *name) {
	auto i = vrgenerictrackers.find(name);
	return i != std::end(vrgenerictrackers) ? i->second() : VRGenericTrackerState{};
}

std::vector<std::string> GetVRGenericTrackerNames() {
	std::vector<std::string> names;
	std::transform(std::begin(vrgenerictrackers), std::end(vrgenerictrackers), std::back_inserter(names), [](const auto &i) { return i.first; });
	return names;
}

VRGenericTracker::VRGenericTracker(const char *n) : name(n) {}

void VRGenericTracker::Update() { state = ReadVRGenericTracker(name.c_str()); }

} // namespace hg
