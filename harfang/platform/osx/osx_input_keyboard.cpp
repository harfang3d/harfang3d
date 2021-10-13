// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include <Carbon/Carbon.h>
#include "osx_input_system/osx_input_keyboard.h"
#include "osx_input_system/osx_input_system.h"
#include "cocoa_window_system/cocoa_window_system.h"
#include "window_system/window_system.h"
#include "cstl/log.h"

namespace gs {
namespace input {

enum {
	NSAlphaShiftKeyMask = 1 << 16,
	NSShiftKeyMask      = 1 << 17,
	NSControlKeyMask    = 1 << 18,
	NSAlternateKeyMask  = 1 << 19,
	NSCommandKeyMask    = 1 << 20,
	NSNumericPadKeyMask = 1 << 21,
	NSHelpKeyMask       = 1 << 22,
	NSFunctionKeyMask   = 1 << 23,
	NSDeviceIndependentModifierFlagsMask = 0xffff0000U
};

void KeyboardOSX::Update(Window::Handle w)
{
	// store current state...
	memory::Copy(was_down, is_down, sizeof(bool) * KeyLast);

	// ...and update it
	is_down[KeyA] = cocoa_keyboard[kVK_ANSI_A];
	is_down[KeyB] = cocoa_keyboard[kVK_ANSI_B];
	is_down[KeyC] = cocoa_keyboard[kVK_ANSI_C];
	is_down[KeyD] = cocoa_keyboard[kVK_ANSI_D];
	is_down[KeyE] = cocoa_keyboard[kVK_ANSI_E];
	is_down[KeyF] = cocoa_keyboard[kVK_ANSI_F];
	is_down[KeyG] = cocoa_keyboard[kVK_ANSI_G];
	is_down[KeyH] = cocoa_keyboard[kVK_ANSI_H];
	is_down[KeyI] = cocoa_keyboard[kVK_ANSI_I];
	is_down[KeyJ] = cocoa_keyboard[kVK_ANSI_J];
	is_down[KeyK] = cocoa_keyboard[kVK_ANSI_K];
	is_down[KeyL] = cocoa_keyboard[kVK_ANSI_L];
	is_down[KeyM] = cocoa_keyboard[kVK_ANSI_M];
	is_down[KeyN] = cocoa_keyboard[kVK_ANSI_N];
	is_down[KeyO] = cocoa_keyboard[kVK_ANSI_O];
	is_down[KeyP] = cocoa_keyboard[kVK_ANSI_P];
	is_down[KeyQ] = cocoa_keyboard[kVK_ANSI_Q];
	is_down[KeyR] = cocoa_keyboard[kVK_ANSI_R];
	is_down[KeyS] = cocoa_keyboard[kVK_ANSI_S];
	is_down[KeyT] = cocoa_keyboard[kVK_ANSI_T];
	is_down[KeyU] = cocoa_keyboard[kVK_ANSI_U];
	is_down[KeyV] = cocoa_keyboard[kVK_ANSI_V];
	is_down[KeyW] = cocoa_keyboard[kVK_ANSI_W];
	is_down[KeyX] = cocoa_keyboard[kVK_ANSI_X];
	is_down[KeyY] = cocoa_keyboard[kVK_ANSI_Y];
	is_down[KeyZ] = cocoa_keyboard[kVK_ANSI_Z];

//	kVK_ANSI_Equal

//	kVK_ANSI_Minus

//	kVK_ANSI_RightBracket
//	kVK_ANSI_LeftBracket

//	kVK_ANSI_Quote

//	kVK_ANSI_Semicolon
//	kVK_ANSI_Backslash
//	kVK_ANSI_Comma
//	kVK_ANSI_Slash

//	kVK_ANSI_Period
//	kVK_ANSI_Grave
//	kVK_ANSI_KeypadDecimal

	is_down[KeyMul] = cocoa_keyboard[kVK_ANSI_KeypadMultiply];
	is_down[KeyAdd] = cocoa_keyboard[kVK_ANSI_KeypadPlus];
//	kVK_ANSI_KeypadClear
	is_down[KeyDiv] = cocoa_keyboard[kVK_ANSI_KeypadDivide];
	is_down[KeyEnter] = cocoa_keyboard[kVK_ANSI_KeypadEnter];
	is_down[KeySub] = cocoa_keyboard[kVK_ANSI_KeypadMinus];
//	kVK_ANSI_KeypadEquals

	is_down[KeyNumpad0] = cocoa_keyboard[kVK_ANSI_Keypad0];
	is_down[KeyNumpad1] = cocoa_keyboard[kVK_ANSI_Keypad1];
	is_down[KeyNumpad2] = cocoa_keyboard[kVK_ANSI_Keypad2];
	is_down[KeyNumpad3] = cocoa_keyboard[kVK_ANSI_Keypad3];
	is_down[KeyNumpad4] = cocoa_keyboard[kVK_ANSI_Keypad4];
	is_down[KeyNumpad5] = cocoa_keyboard[kVK_ANSI_Keypad5];
	is_down[KeyNumpad6] = cocoa_keyboard[kVK_ANSI_Keypad6];
	is_down[KeyNumpad7] = cocoa_keyboard[kVK_ANSI_Keypad7];
	is_down[KeyNumpad8] = cocoa_keyboard[kVK_ANSI_Keypad8];
	is_down[KeyNumpad9] = cocoa_keyboard[kVK_ANSI_Keypad9];

	is_down[KeyReturn] = cocoa_keyboard[kVK_Return];
	is_down[KeyTab] = cocoa_keyboard[kVK_Tab];
	is_down[KeySpace] = cocoa_keyboard[kVK_Space];
	is_down[KeyBackspace] = cocoa_keyboard[kVK_Delete];
	is_down[KeyEscape] = cocoa_keyboard[kVK_Escape];
	is_down[KeyLAlt] = cocoa_keyboard_flags & NSAlternateKeyMask; // cocoa_keyboard[kVK_Command];
	is_down[KeyLShift] = cocoa_keyboard_flags & NSShiftKeyMask; // cocoa_keyboard[kVK_Shift];
	is_down[KeyCapsLock] = cocoa_keyboard_flags & NSAlphaShiftKeyMask; // cocoa_keyboard[kVK_CapsLock];
	is_down[KeyLWin] = cocoa_keyboard_flags & NSCommandKeyMask; // cocoa_keyboard[kVK_Option];
	is_down[KeyLCtrl] = cocoa_keyboard_flags & NSControlKeyMask; // cocoa_keyboard[kVK_Control];
	is_down[KeyRShift] = cocoa_keyboard[kVK_RightShift];
	is_down[KeyRWin] = cocoa_keyboard[kVK_RightOption];
	is_down[KeyRCtrl] = cocoa_keyboard[kVK_RightControl];

//	kVK_Function
//	kVK_F17
//	kVK_VolumeUp
//	kVK_VolumeDown
//	kVK_Mute
//	kVK_F18
//	kVK_F19
//	kVK_F20

	is_down[KeyF1] = cocoa_keyboard[kVK_F1];
	is_down[KeyF2] = cocoa_keyboard[kVK_F2];
	is_down[KeyF3] = cocoa_keyboard[kVK_F3];
	is_down[KeyF4] = cocoa_keyboard[kVK_F4];
	is_down[KeyF5] = cocoa_keyboard[kVK_F5];
	is_down[KeyF6] = cocoa_keyboard[kVK_F6];
	is_down[KeyF7] = cocoa_keyboard[kVK_F7];
	is_down[KeyF8] = cocoa_keyboard[kVK_F8];
	is_down[KeyF9] = cocoa_keyboard[kVK_F9];
	is_down[KeyF10] = cocoa_keyboard[kVK_F10];
	is_down[KeyF11] = cocoa_keyboard[kVK_F11];
	is_down[KeyF12] = cocoa_keyboard[kVK_F12];

//	kVK_F13
//	kVK_F16
//	kVK_F14
//	kVK_F15

//	kVK_Help

	is_down[KeyHome] = cocoa_keyboard[kVK_Home];
	is_down[KeyPageUp] = cocoa_keyboard[kVK_PageUp];
	is_down[KeySuppr] = cocoa_keyboard[kVK_ForwardDelete];
	is_down[KeyEnd] = cocoa_keyboard[kVK_End];
	is_down[KeyPageDown] = cocoa_keyboard[kVK_PageDown];

	is_down[KeyUp] = cocoa_keyboard[kVK_UpArrow];
	is_down[KeyDown] = cocoa_keyboard[kVK_DownArrow];
	is_down[KeyLeft] = cocoa_keyboard[kVK_LeftArrow];
	is_down[KeyRight] = cocoa_keyboard[kVK_RightArrow]; // A, Start
}

} // input
} // gs
