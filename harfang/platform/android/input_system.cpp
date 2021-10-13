// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "platform/android/input_system.h"
#include "platform/input_keyboard.h"
#include "platform/input_mouse.h"
#include "platform/input_system.h"

#include "android/input.h"

namespace hg {

struct AndroidMouse : MouseInputDevice {
	void Update() override;

	State current_state; // current_state as observed through the message pump
};

struct AndroidKeyboard : KeyboardInputDevice {
	void Update() override;

	std::array<bool, KeyLast> current_down;
};

KeyboardInputDevice::KeyCode AndroidKeyCodeToInputSystemKeyCode(int code);

KeyboardInputDevice::KeyCode AndroidKeyCodeToInputSystemKeyCode(int code) {
	switch (code) {
		case AKEYCODE_DPAD_UP: return KeyboardInputDevice::KeyUp;
		case AKEYCODE_DPAD_DOWN: return KeyboardInputDevice::KeyDown;
		case AKEYCODE_DPAD_LEFT: return KeyboardInputDevice::KeyLeft;
		case AKEYCODE_DPAD_RIGHT: return KeyboardInputDevice::KeyRight;

		case AKEYCODE_ESCAPE: return KeyboardInputDevice::KeyEscape;

		case AKEYCODE_NUMPAD_ADD: return KeyboardInputDevice::KeyAdd;
		case AKEYCODE_NUMPAD_SUBTRACT: return KeyboardInputDevice::KeySub;
		case AKEYCODE_NUMPAD_MULTIPLY: return KeyboardInputDevice::KeyMul;
		case AKEYCODE_NUMPAD_DIVIDE: return KeyboardInputDevice::KeyDiv;
		case AKEYCODE_NUMPAD_ENTER: return KeyboardInputDevice::KeyEnter;

		// printscreen
		case AKEYCODE_SCROLL_LOCK: return KeyboardInputDevice::KeyScrollLock;
		// pause
		case AKEYCODE_NUM_LOCK: return KeyboardInputDevice::KeyNumLock;
		case AKEYCODE_ENTER: return KeyboardInputDevice::KeyReturn;

		case AKEYCODE_SHIFT_LEFT: return KeyboardInputDevice::KeyLShift;
		case AKEYCODE_SHIFT_RIGHT: return KeyboardInputDevice::KeyRShift;
		case AKEYCODE_CTRL_LEFT: return KeyboardInputDevice::KeyLCtrl;
		case AKEYCODE_CTRL_RIGHT: return KeyboardInputDevice::KeyRCtrl;
		case AKEYCODE_ALT_LEFT: return KeyboardInputDevice::KeyLAlt;
		case AKEYCODE_ALT_RIGHT: return KeyboardInputDevice::KeyRAlt;
		case AKEYCODE_SOFT_LEFT: return KeyboardInputDevice::KeyLWin;
		case AKEYCODE_SOFT_RIGHT: return KeyboardInputDevice::KeyRWin;

		case AKEYCODE_TAB: return KeyboardInputDevice::KeyTab;
		case AKEYCODE_CAPS_LOCK: return KeyboardInputDevice::KeyCapsLock;
		case AKEYCODE_SPACE: return KeyboardInputDevice::KeySpace;
		case AKEYCODE_BACK: return KeyboardInputDevice::KeyBackspace;
		case AKEYCODE_INSERT: return KeyboardInputDevice::KeyInsert;
		case AKEYCODE_DEL: return KeyboardInputDevice::KeySuppr;
		case AKEYCODE_HOME: return KeyboardInputDevice::KeyHome;
		case AKEYCODE_MOVE_END: return KeyboardInputDevice::KeyEnd;
		case AKEYCODE_PAGE_UP: return KeyboardInputDevice::KeyPageUp;
		case AKEYCODE_PAGE_DOWN: return KeyboardInputDevice::KeyPageDown;

		case AKEYCODE_F1: return KeyboardInputDevice::KeyF1;
		case AKEYCODE_F2: return KeyboardInputDevice::KeyF2;
		case AKEYCODE_F3: return KeyboardInputDevice::KeyF3;
		case AKEYCODE_F4: return KeyboardInputDevice::KeyF4;
		case AKEYCODE_F5: return KeyboardInputDevice::KeyF5;
		case AKEYCODE_F6: return KeyboardInputDevice::KeyF6;
		case AKEYCODE_F7: return KeyboardInputDevice::KeyF7;
		case AKEYCODE_F8: return KeyboardInputDevice::KeyF8;
		case AKEYCODE_F9: return KeyboardInputDevice::KeyF9;
		case AKEYCODE_F10: return KeyboardInputDevice::KeyF10;
		case AKEYCODE_F11: return KeyboardInputDevice::KeyF11;
		case AKEYCODE_F12: return KeyboardInputDevice::KeyF12;

		case AKEYCODE_NUMPAD_0: return KeyboardInputDevice::KeyNumpad0;
		case AKEYCODE_NUMPAD_1: return KeyboardInputDevice::KeyNumpad1;
		case AKEYCODE_NUMPAD_2: return KeyboardInputDevice::KeyNumpad2;
		case AKEYCODE_NUMPAD_3: return KeyboardInputDevice::KeyNumpad3;
		case AKEYCODE_NUMPAD_4: return KeyboardInputDevice::KeyNumpad4;
		case AKEYCODE_NUMPAD_5: return KeyboardInputDevice::KeyNumpad5;
		case AKEYCODE_NUMPAD_6: return KeyboardInputDevice::KeyNumpad6;
		case AKEYCODE_NUMPAD_7: return KeyboardInputDevice::KeyNumpad7;
		case AKEYCODE_NUMPAD_8: return KeyboardInputDevice::KeyNumpad8;
		case AKEYCODE_NUMPAD_9: return KeyboardInputDevice::KeyNumpad9;

		case AKEYCODE_A: return KeyboardInputDevice::KeyA;
		case AKEYCODE_B: return KeyboardInputDevice::KeyB;
		case AKEYCODE_C: return KeyboardInputDevice::KeyC;
		case AKEYCODE_D: return KeyboardInputDevice::KeyD;
		case AKEYCODE_E: return KeyboardInputDevice::KeyE;
		case AKEYCODE_F: return KeyboardInputDevice::KeyF;
		case AKEYCODE_G: return KeyboardInputDevice::KeyG;
		case AKEYCODE_H: return KeyboardInputDevice::KeyH;
		case AKEYCODE_I: return KeyboardInputDevice::KeyI;
		case AKEYCODE_J: return KeyboardInputDevice::KeyJ;
		case AKEYCODE_K: return KeyboardInputDevice::KeyK;
		case AKEYCODE_L: return KeyboardInputDevice::KeyL;
		case AKEYCODE_M: return KeyboardInputDevice::KeyM;
		case AKEYCODE_N: return KeyboardInputDevice::KeyN;
		case AKEYCODE_O: return KeyboardInputDevice::KeyO;
		case AKEYCODE_P: return KeyboardInputDevice::KeyP;
		case AKEYCODE_Q: return KeyboardInputDevice::KeyQ;
		case AKEYCODE_R: return KeyboardInputDevice::KeyR;
		case AKEYCODE_S: return KeyboardInputDevice::KeyS;
		case AKEYCODE_T: return KeyboardInputDevice::KeyT;
		case AKEYCODE_U: return KeyboardInputDevice::KeyU;
		case AKEYCODE_V: return KeyboardInputDevice::KeyV;
		case AKEYCODE_W: return KeyboardInputDevice::KeyW;
		case AKEYCODE_X: return KeyboardInputDevice::KeyX;
		case AKEYCODE_Y: return KeyboardInputDevice::KeyY;
		case AKEYCODE_Z: return KeyboardInputDevice::KeyZ;
	}
	return KeyboardInputDevice::KeyLast;
}

void AndroidMouse::Update() { state = current_state; }
void AndroidKeyboard::Update() { is_down = current_down; }

void RegisterInputDevices(InputSystem &system) {
	system.RegisterDirectDevice("mouse", std::make_shared<AndroidMouse>());
	system.RegisterDirectDevice("keyboard", std::make_shared<AndroidKeyboard>());
}

} // hg
