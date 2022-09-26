// HARFANG(R) Copyright (C) 2019 Emmanuel Julien, Movida Production. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include "platform/sdl/input_system_sdl.h"
#include "platform/input_system.h"
#include "foundation/cext.h"
#include "foundation/log.h"
#include "foundation/format.h"
#include "foundation/utf8.h"
#include "platform/window_system.h"
#include <SDL.h>
#include <array>
#include <memory>
#include <map>

namespace hg {

static double wheel = 0, hwheel = 0;
static int inhibit_click = 0;

MouseState previous_state;

static MouseState ReadMouse() {
	MouseState state = previous_state;

	state.wheel = 0;
	int w, h;

	auto win = GetWindowInFocus();
	if (!GetWindowClientSize(win, w, h)) {
		inhibit_click = 3;
		return {};
	}

	// ...and update it
	SDL_Event event;
	while (SDL_PollEvent(&event))
		switch (event.type) {
			case SDL_FINGERMOTION: {
				// use only if there is one finger
				if (SDL_GetTouchFinger(event.tfinger.touchId, 1))
					break;
				state.x = float(event.tfinger.x) * w;
				state.y = float(1.f - event.tfinger.y) * h;
			} break;

			case SDL_FINGERDOWN: {
				// use only if there is one finger
				if (SDL_GetTouchFinger(event.tfinger.touchId, 1))
					break;
				state.button[0] = true;
				state.x = float(event.tfinger.x) * w;
				state.y = float(1.f - event.tfinger.y) * h;
			} break;

			case SDL_MULTIGESTURE: // use this for pinch
			{
				if (fabs(event.mgesture.dDist) > 0.002f) {
					//Pinch open
					if (event.mgesture.dDist > 0) {
						state.wheel = 1;
					}
					//Pinch close
					else {
						state.wheel = -1;
					}
				}
			} break;

			case SDL_FINGERUP:
				// use only if there is one finger
				if (SDL_GetTouchFinger(event.tfinger.touchId, 1))
					break;
				state.button[0] = false;
				break;
				
			case SDL_MOUSEMOTION: {
				state.x = float(event.motion.x);
				state.y = float(h - event.motion.y);
			} break;

			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
				switch (event.button.button) {
					case SDL_BUTTON_LEFT:
						state.button[0] = (event.button.state == SDL_PRESSED);
						break;
					case SDL_BUTTON_MIDDLE:
						state.button[2] = (event.button.state == SDL_PRESSED);
						break;
					case SDL_BUTTON_RIGHT:
						state.button[1] = (event.button.state == SDL_PRESSED);
						break;
				}
				break;

			case SDL_MOUSEWHEEL:
				if (event.wheel.y >= 1) // scroll up
					state.wheel = 1;
				else if (event.wheel.y <= -1) // scroll down
					state.wheel = -1;
				break;
		}

	previous_state = state;
	return state;
}

// Keyboard

static KeyboardState ReadKeyboard() {
	auto handle = GetWindowHandle(GetWindowInFocus());
	if (!handle)
		return {};

	KeyboardState state;

	// ...and update it.
	const Uint8 *keys = SDL_GetKeyboardState(nullptr);

	state.key[K_Up] = keys[SDL_SCANCODE_UP];
	state.key[K_Down] = keys[SDL_SCANCODE_DOWN];
	state.key[K_Left] = keys[SDL_SCANCODE_LEFT];
	state.key[K_Right] = keys[SDL_SCANCODE_RIGHT];

	state.key[K_Escape] = keys[SDL_SCANCODE_ESCAPE];

	state.key[K_Add] = keys[SDL_SCANCODE_KP_PLUS];
	state.key[K_Sub] = keys[SDL_SCANCODE_KP_MINUS];
	state.key[K_Mul] = keys[SDL_SCANCODE_KP_MULTIPLY];
	state.key[K_Div] = keys[SDL_SCANCODE_KP_DIVIDE];
	state.key[K_Enter] = keys[SDL_SCANCODE_KP_ENTER];

	state.key[K_PrintScreen] = keys[SDL_SCANCODE_PRINTSCREEN];
	state.key[K_ScrollLock] = keys[SDL_SCANCODE_SCROLLLOCK];
	state.key[K_Pause] = keys[SDL_SCANCODE_PAUSE];
	state.key[K_NumLock] = keys[SDL_SCANCODE_NUMLOCKCLEAR];
	state.key[K_Return] = keys[SDL_SCANCODE_RETURN];

	state.key[K_LShift] = keys[SDL_SCANCODE_LSHIFT];
	state.key[K_RShift] = keys[SDL_SCANCODE_RSHIFT];
	state.key[K_LCtrl] = keys[SDL_SCANCODE_LCTRL];
	state.key[K_RCtrl] = keys[SDL_SCANCODE_RCTRL];
	state.key[K_LAlt] = keys[SDL_SCANCODE_LALT];
	state.key[K_RAlt] = keys[SDL_SCANCODE_RALT];
	state.key[K_LWin] = keys[SDL_SCANCODE_LGUI];
	state.key[K_RWin] = keys[SDL_SCANCODE_RGUI];

	state.key[K_Tab] = keys[SDL_SCANCODE_TAB];
	state.key[K_CapsLock] = keys[SDL_SCANCODE_CAPSLOCK];
	state.key[K_Space] = keys[SDL_SCANCODE_SPACE];
	state.key[K_Backspace] = keys[SDL_SCANCODE_BACKSPACE];
	state.key[K_Insert] = keys[SDL_SCANCODE_INSERT];
	state.key[K_Suppr] = keys[SDL_SCANCODE_DELETE];
	state.key[K_Home] = keys[SDL_SCANCODE_HOME];
	state.key[K_End] = keys[SDL_SCANCODE_END];
	state.key[K_PageUp] = keys[SDL_SCANCODE_PAGEUP];
	state.key[K_PageDown] = keys[SDL_SCANCODE_PAGEDOWN];

	state.key[K_F1] = keys[SDL_SCANCODE_F1];
	state.key[K_F2] = keys[SDL_SCANCODE_F2];
	state.key[K_F3] = keys[SDL_SCANCODE_F3];
	state.key[K_F4] = keys[SDL_SCANCODE_F4];
	state.key[K_F5] = keys[SDL_SCANCODE_F5];
	state.key[K_F6] = keys[SDL_SCANCODE_F6];
	state.key[K_F7] = keys[SDL_SCANCODE_F7];
	state.key[K_F8] = keys[SDL_SCANCODE_F8];
	state.key[K_F9] = keys[SDL_SCANCODE_F9];
	state.key[K_F10] = keys[SDL_SCANCODE_F10];
	state.key[K_F11] = keys[SDL_SCANCODE_F11];
	state.key[K_F12] = keys[SDL_SCANCODE_F12];

	state.key[K_Numpad0] = keys[SDL_SCANCODE_KP_0];
	state.key[K_Numpad1] = keys[SDL_SCANCODE_KP_1];
	state.key[K_Numpad2] = keys[SDL_SCANCODE_KP_2];
	state.key[K_Numpad3] = keys[SDL_SCANCODE_KP_3];
	state.key[K_Numpad4] = keys[SDL_SCANCODE_KP_4];
	state.key[K_Numpad5] = keys[SDL_SCANCODE_KP_5];
	state.key[K_Numpad6] = keys[SDL_SCANCODE_KP_6];
	state.key[K_Numpad7] = keys[SDL_SCANCODE_KP_7];
	state.key[K_Numpad8] = keys[SDL_SCANCODE_KP_8];
	state.key[K_Numpad9] = keys[SDL_SCANCODE_KP_9];

	state.key[K_A] = keys[SDL_SCANCODE_A];
	state.key[K_B] = keys[SDL_SCANCODE_B];
	state.key[K_C] = keys[SDL_SCANCODE_C];
	state.key[K_D] = keys[SDL_SCANCODE_D];
	state.key[K_E] = keys[SDL_SCANCODE_E];
	state.key[K_F] = keys[SDL_SCANCODE_F];
	state.key[K_G] = keys[SDL_SCANCODE_G];
	state.key[K_H] = keys[SDL_SCANCODE_H];
	state.key[K_I] = keys[SDL_SCANCODE_I];
	state.key[K_J] = keys[SDL_SCANCODE_J];
	state.key[K_K] = keys[SDL_SCANCODE_K];
	state.key[K_L] = keys[SDL_SCANCODE_L];
	state.key[K_M] = keys[SDL_SCANCODE_M];
	state.key[K_N] = keys[SDL_SCANCODE_N];
	state.key[K_O] = keys[SDL_SCANCODE_O];
	state.key[K_P] = keys[SDL_SCANCODE_P];
	state.key[K_Q] = keys[SDL_SCANCODE_Q];
	state.key[K_R] = keys[SDL_SCANCODE_R];
	state.key[K_S] = keys[SDL_SCANCODE_S];
	state.key[K_T] = keys[SDL_SCANCODE_T];
	state.key[K_U] = keys[SDL_SCANCODE_U];
	state.key[K_V] = keys[SDL_SCANCODE_V];
	state.key[K_W] = keys[SDL_SCANCODE_W];
	state.key[K_X] = keys[SDL_SCANCODE_X];
	state.key[K_Y] = keys[SDL_SCANCODE_Y];
	state.key[K_Z] = keys[SDL_SCANCODE_Z];
	return state;
}


static const char *GetKeyName(Key key) {
	static std::map<Key, int> key_to_sdl = {
		{K_LShift, SDL_SCANCODE_LSHIFT},
		{K_RShift, SDL_SCANCODE_RSHIFT},
		{K_LCtrl, SDL_SCANCODE_LCTRL},
		{K_RCtrl, SDL_SCANCODE_RCTRL},
		{K_LAlt, SDL_SCANCODE_LALT},
		{K_RAlt, SDL_SCANCODE_RALT},
		{K_LWin, SDL_SCANCODE_LGUI},
		{K_RWin, SDL_SCANCODE_RGUI},
		{K_Tab, SDL_SCANCODE_TAB},
		{K_CapsLock, SDL_SCANCODE_CAPSLOCK},
		{K_Space, SDL_SCANCODE_SPACE},
		{K_Backspace, SDL_SCANCODE_BACKSPACE},
		{K_Insert, SDL_SCANCODE_INSERT},
		{K_Suppr, SDL_SCANCODE_DELETE},
		{K_Home, SDL_SCANCODE_HOME},
		{K_End, SDL_SCANCODE_END},
		{K_PageUp, SDL_SCANCODE_PAGEUP},
		{K_PageDown, SDL_SCANCODE_PAGEDOWN},
		{K_Up, SDL_SCANCODE_UP},
		{K_Down, SDL_SCANCODE_DOWN},
		{K_Left, SDL_SCANCODE_LEFT},
		{K_Right, SDL_SCANCODE_RIGHT},
		{K_Escape, SDL_SCANCODE_ESCAPE},
		{K_F1, SDL_SCANCODE_F1},
		{K_F2, SDL_SCANCODE_F2},
		{K_F3, SDL_SCANCODE_F3},
		{K_F4, SDL_SCANCODE_F4},
		{K_F5, SDL_SCANCODE_F5},
		{K_F6, SDL_SCANCODE_F6},
		{K_F7, SDL_SCANCODE_F7},
		{K_F8, SDL_SCANCODE_F8},
		{K_F9, SDL_SCANCODE_F9},
		{K_F10, SDL_SCANCODE_F10},
		{K_F11, SDL_SCANCODE_F11},
		{K_F12, SDL_SCANCODE_F12},
		{K_PrintScreen, SDL_SCANCODE_PRINTSCREEN},
		{K_ScrollLock, SDL_SCANCODE_SCROLLLOCK},
		{K_Pause, SDL_SCANCODE_PAUSE},
		{K_NumLock, SDL_SCANCODE_NUMLOCKCLEAR},
		{K_Return, SDL_SCANCODE_RETURN},
		{K_0, SDL_SCANCODE_0},
		{K_1, SDL_SCANCODE_1},
		{K_2, SDL_SCANCODE_2},
		{K_3, SDL_SCANCODE_3},
		{K_4, SDL_SCANCODE_4},
		{K_5, SDL_SCANCODE_5},
		{K_6, SDL_SCANCODE_6},
		{K_7, SDL_SCANCODE_7},
		{K_8, SDL_SCANCODE_8},
		{K_9, SDL_SCANCODE_9},
		{K_Numpad0, SDL_SCANCODE_KP_0},
		{K_Numpad1, SDL_SCANCODE_KP_1},
		{K_Numpad2, SDL_SCANCODE_KP_2},
		{K_Numpad3, SDL_SCANCODE_KP_3},
		{K_Numpad4, SDL_SCANCODE_KP_4},
		{K_Numpad5, SDL_SCANCODE_KP_5},
		{K_Numpad6, SDL_SCANCODE_KP_6},
		{K_Numpad7, SDL_SCANCODE_KP_7},
		{K_Numpad8, SDL_SCANCODE_KP_8},
		{K_Numpad9, SDL_SCANCODE_KP_9},
		{K_Add, SDL_SCANCODE_KP_PLUS},
		{K_Sub, SDL_SCANCODE_KP_MINUS},
		{K_Mul, SDL_SCANCODE_KP_MULTIPLY},
		{K_Div, SDL_SCANCODE_KP_DIVIDE},
		{K_Enter, SDL_SCANCODE_KP_ENTER},
		{K_A, SDL_SCANCODE_A},
		{K_B, SDL_SCANCODE_B},
		{K_C, SDL_SCANCODE_C},
		{K_D, SDL_SCANCODE_D},
		{K_E, SDL_SCANCODE_E},
		{K_F, SDL_SCANCODE_F},
		{K_G, SDL_SCANCODE_G},
		{K_H, SDL_SCANCODE_H},
		{K_I, SDL_SCANCODE_I},
		{K_J, SDL_SCANCODE_J},
		{K_K, SDL_SCANCODE_K},
		{K_L, SDL_SCANCODE_L},
		{K_M, SDL_SCANCODE_M},
		{K_N, SDL_SCANCODE_N},
		{K_O, SDL_SCANCODE_O},
		{K_P, SDL_SCANCODE_P},
		{K_Q, SDL_SCANCODE_Q},
		{K_R, SDL_SCANCODE_R},
		{K_S, SDL_SCANCODE_S},
		{K_T, SDL_SCANCODE_T},
		{K_U, SDL_SCANCODE_U},
		{K_V, SDL_SCANCODE_V},
		{K_W, SDL_SCANCODE_W},
		{K_X, SDL_SCANCODE_X},
		{K_Y, SDL_SCANCODE_Y},
		{K_Z, SDL_SCANCODE_Z},
		{K_Plus, SDL_SCANCODE_EQUALS},
		{K_Comma, SDL_SCANCODE_COMMA},
		{K_Minus, SDL_SCANCODE_MINUS},
		{K_Period, SDL_SCANCODE_PERIOD},
	};

	const auto i = key_to_sdl.find(key);
	if (i != std::end(key_to_sdl))
		return SDL_GetKeyName(SDL_GetKeyFromScancode((SDL_Scancode)i->second));

	return nullptr;
}


static Signal<void(const Window *)>::Connection on_new_window_connection;

void InputInit() {
	AddMouseReader("default", ReadMouse);
	AddKeyboardReader("default", ReadKeyboard, GetKeyName);
}

void InputShutdown() { new_window_signal.Disconnect(on_new_window_connection); }

} // namespace hg
