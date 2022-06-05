// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/matrix4.h"
#include "foundation/time.h"
#include "foundation/vector2.h"
#include "foundation/signal.h"

#include <array>
#include <bitset>
#include <functional>
#include <string>
#include <vector>

namespace hg {

enum MouseButton { MB_0, MB_1, MB_2, MB_3, MB_4, MB_5, MB_6, MB_7, MB_Count };

struct MouseState {
	int x, y;
	std::bitset<MB_Count> button;
	int wheel, hwheel;
};

using MouseReader = MouseState (*)();

void AddMouseReader(const char *name, MouseReader reader);
void RemoveMouseReader(const char *name);

MouseState ReadMouse(const char *name = "default");

//
struct Mouse {
	explicit Mouse(const char *name = "default");

	int X() const { return state.x; }
	int Y() const { return state.y; }
	int DtX() const { return state.x - old_state.x; }
	int DtY() const { return state.y - old_state.y; }

	bool Down(int btn) const { return state.button[btn]; }
	bool Pressed(int btn) const { return state.button[btn] && !old_state.button[btn]; }
	bool Released(int btn) const { return !state.button[btn] && old_state.button[btn]; }

	int Wheel() const { return state.wheel; }
	int HWheel() const { return state.hwheel; }

	void Update();

	const MouseState &GetState() const { return state; }
	const MouseState &GetOldState() const { return old_state; }

private:
	std::string name;
	MouseState state{}, old_state{};
};

std::vector<std::string> GetMouseNames();

//
enum Key {
	K_LShift,
	K_RShift,
	K_LCtrl,
	K_RCtrl,
	K_LAlt,
	K_RAlt,
	K_LWin,
	K_RWin,
	K_Tab,
	K_CapsLock,
	K_Space,
	K_Backspace,
	K_Insert,
	K_Suppr,
	K_Home,
	K_End,
	K_PageUp,
	K_PageDown,
	K_Up,
	K_Down,
	K_Left,
	K_Right,
	K_Escape,
	K_F1,
	K_F2,
	K_F3,
	K_F4,
	K_F5,
	K_F6,
	K_F7,
	K_F8,
	K_F9,
	K_F10,
	K_F11,
	K_F12,
	K_PrintScreen,
	K_ScrollLock,
	K_Pause,
	K_NumLock,
	K_Return,
	K_0,
	K_1,
	K_2,
	K_3,
	K_4,
	K_5,
	K_6,
	K_7,
	K_8,
	K_9,
	K_Numpad0,
	K_Numpad1,
	K_Numpad2,
	K_Numpad3,
	K_Numpad4,
	K_Numpad5,
	K_Numpad6,
	K_Numpad7,
	K_Numpad8,
	K_Numpad9,
	K_Add,
	K_Sub,
	K_Mul,
	K_Div,
	K_Enter,
	K_A,
	K_B,
	K_C,
	K_D,
	K_E,
	K_F,
	K_G,
	K_H,
	K_I,
	K_J,
	K_K,
	K_L,
	K_M,
	K_N,
	K_O,
	K_P,
	K_Q,
	K_R,
	K_S,
	K_T,
	K_U,
	K_V,
	K_W,
	K_X,
	K_Y,
	K_Z,
	K_Plus,
	K_Comma,
	K_Minus,
	K_Period,
	K_OEM1,
	K_OEM2,
	K_OEM3,
	K_OEM4,
	K_OEM5,
	K_OEM6,
	K_OEM7,
	K_OEM8,
	K_BrowserBack,
	K_BrowserForward,
	K_BrowserRefresh,
	K_BrowserStop,
	K_BrowserSearch,
	K_BrowserFavorites,
	K_BrowserHome,
	K_VolumeMute,
	K_VolumeDown,
	K_VolumeUp,
	K_MediaNextTrack,
	K_MediaPrevTrack,
	K_MediaStop,
	K_MediaPlayPause,
	K_LaunchMail,
	K_LaunchMediaSelect,
	K_LaunchApp1,
	K_LaunchApp2,
	K_Last
};

struct KeyboardState {
	std::bitset<K_Last> key;
};

using KeyboardReader = KeyboardState (*)();
using KeyboardGetKeyName = const char *(*)(Key key);

void AddKeyboardReader(const char *name, KeyboardReader reader, KeyboardGetKeyName get_key_name);
void RemoveKeyboardReader(const char *name);

KeyboardState ReadKeyboard(const char *name = "default");
const char *GetKeyName(Key key, const char *name = "default");

std::vector<std::string> GetKeyboardNames();

//
struct Keyboard {
	explicit Keyboard(const char *name = "default");

	bool Down(Key key) const { return state.key[key]; }
	bool Pressed(Key key) const { return state.key[key] && !old_state.key[key]; }
	bool Released(Key key) const { return !state.key[key] && old_state.key[key]; }

	void Update();

	const KeyboardState &GetState() const { return state; }
	const KeyboardState &GetOldState() const { return old_state; }

private:
	std::string name;
	KeyboardState state{}, old_state{};
};

//
enum GamepadAxes { GA_LeftX, GA_LeftY, GA_RightX, GA_RightY, GA_LeftTrigger, GA_RightTrigger, GA_Count };

enum GamepadButton {
	GB_ButtonA,
	GB_ButtonB,
	GB_ButtonX,
	GB_ButtonY,
	GB_LeftBumper,
	GB_RightBumper,
	GB_Back,
	GB_Start,
	GB_Guide,
	GB_LeftThumb,
	GB_RightThumb,
	GB_DPadUp,
	GB_DPadRight,
	GB_DPadDown,
	GB_DPadLeft,
	GB_Count
};

struct GamepadState {
	bool connected = false;
	std::bitset<GB_Count> button;
	float axes[GA_Count];
};

using GamepadReader = GamepadState (*)();

void AddGamepadReader(const char *name, GamepadReader reader);
void RemoveGamepadReader(const char *name);

GamepadState ReadGamepad(const char *name = "default");

std::vector<std::string> GetGamepadNames();

//
struct Gamepad {
	explicit Gamepad(const char *name = "default");

	bool IsConnected() const { return state.connected; }
	bool Connected() const { return state.connected && !old_state.connected; }
	bool Disconnected() const { return !state.connected && old_state.connected; }

	float Axes(GamepadAxes axis) const { return state.axes[axis]; }
	float DtAxes(GamepadAxes axis) const { return state.axes[axis] - old_state.axes[axis]; }

	bool Down(GamepadButton btn) const { return state.button[btn]; }
	bool Pressed(GamepadButton btn) const { return state.button[btn] && !old_state.button[btn]; }
	bool Released(GamepadButton btn) const { return !state.button[btn] && old_state.button[btn]; }

	void Update();

	const GamepadState &GetState() const { return state; }
	const GamepadState &GetOldState() const { return old_state; }

private:
	GamepadState state{}, old_state{};
	std::string name;
};

//
struct JoystickState {
	short nbAxes, nbButtons;
	bool connected = false;
	float axes[32];
	std::bitset<512> buttons;
};

using JoystickReader = JoystickState (*)();
using JoystickDeviceName = std::string (*)();

void AddJoystickReader(const char *name, JoystickReader reader, JoystickDeviceName devicename);
void RemoveJoystickReader(const char *name);

JoystickState ReadJoystick(const char *name = "default");
std::string DeviceNameJoystick(const char *name = "default");

std::vector<std::string> GetJoystickNames();
std::vector<std::string> GetJoystickDeviceNames();

//
struct Joystick {
	explicit Joystick(const char *name = "default");

	std::string GetDeviceName() const;

	bool IsConnected() const { return state.connected; }
	bool Connected() const { return state.connected && !old_state.connected; }
	bool Disconnected() const { return !state.connected && old_state.connected; }

	int AxesCount() const { return state.nbAxes; }
	float Axes(int axis) const { return state.axes[axis]; }
	float DtAxes(int axis) const { return state.axes[axis] - old_state.axes[axis]; }

	int ButtonsCount() const { return state.nbButtons; }
	bool Down(int btn) const { return state.buttons[btn]; }
	bool Pressed(int btn) const { return state.buttons[btn] && !old_state.buttons[btn]; }
	bool Released(int btn) const { return !state.buttons[btn] && old_state.buttons[btn]; }

	void Update();

	const JoystickState &GetState() const { return state; }
	const JoystickState &GetOldState() const { return old_state; }

private:
	std::string name;
	JoystickState state{}, old_state{};
};

// called whenever text is received from the system
extern Signal<void(const char *)> on_text_input;

//
enum VRControllerButton {
	VRCB_DPad_Up,
	VRCB_DPad_Down,
	VRCB_DPad_Left,
	VRCB_DPad_Right,
	VRCB_System,
	VRCB_AppMenu,
	VRCB_Grip,
	VRCB_A,
	VRCB_ProximitySensor,
	VRCB_Axis0,
	VRCB_Axis1,
	VRCB_Axis2,
	VRCB_Axis3,
	VRCB_Axis4,
	VRCB_Count
};

struct VRControllerState {
	bool connected = false;
	Mat4 world;
	std::bitset<VRCB_Count> pressed, touched;
	std::array<Vec2, 5> surface;
};

using VRControllerReader = VRControllerState (*)();
using VRControllerSendHapticPulse = void (*)(time_ns duration);

void AddVRControllerReader(const char *name, VRControllerReader reader, VRControllerSendHapticPulse send_haptic_pulse);
void RemoveVRControllerReader(const char *name);

VRControllerState ReadVRController(const char *name = "default");
void SendVRControllerHapticPulse(time_ns duration, const char *name = "default");

std::vector<std::string> GetVRControllerNames();

struct VRController {
	explicit VRController(const char *name = "default");

	bool IsConnected() const { return state.connected; }
	bool Connected() const { return state.connected && !old_state.connected; }
	bool Disconnected() const { return !state.connected && old_state.connected; }

	Mat4 World() const { return state.world; }

	bool Down(int btn) const { return state.pressed[btn]; }
	bool Pressed(int btn) const { return state.pressed[btn] && !old_state.pressed[btn]; }
	bool Released(int btn) const { return !state.pressed[btn] && old_state.pressed[btn]; }

	bool Touch(int btn) const { return state.touched[btn]; }
	bool TouchStart(int btn) const { return state.touched[btn] && !old_state.touched[btn]; }
	bool TouchEnd(int btn) const { return !state.touched[btn] && old_state.touched[btn]; }

	Vec2 Surface(int idx) const { return state.surface[idx]; }
	Vec2 DtSurface(int idx) const { return state.surface[idx] - old_state.surface[idx]; }

	void SendHapticPulse(time_ns duration);

	void Update();

	const VRControllerState &GetState() const { return state; }
	const VRControllerState &GetOldState() const { return old_state; }

private:
	std::string name;
	VRControllerState state{}, old_state{};
};

//
struct VRGenericTrackerState {
	bool connected;
	Mat4 world;
};

using VRGenericTrackerReader = VRGenericTrackerState (*)();

void AddVRGenericTrackerReader(const char *name, VRGenericTrackerReader reader);
void RemoveVRGenericTrackerReader(const char *name);

VRGenericTrackerState ReadVRGenericTracker(const char *name = "default");

std::vector<std::string> GetVRGenericTrackerNames();

struct VRGenericTracker {
	explicit VRGenericTracker(const char *name = "default");

	bool IsConnected() const { return state.connected; }
	Mat4 World() const { return state.world; }

	void Update();

private:
	std::string name;
	VRGenericTrackerState state{};
};

//
void InputInit();
void InputShutdown();

} // namespace hg
