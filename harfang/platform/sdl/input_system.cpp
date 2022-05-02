// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "platform/input_system.h"
#include "engine/plus.h"
#include "engine/renderer_async.h"
#include "foundation/cext.h"
#include "foundation/log.h"
#include "foundation/utf8.h"
#include "platform/input_device.h"
#include "platform/input_keyboard.h"
#include "platform/input_mouse.h"
#include "platform/window_system.h"
#include <SDL.h>
#include <array>
#include <memory>

namespace hg {

struct Mouse : MouseInputDevice {
	void OnMouseWheel(float v) { state.wheel += v; }
	void OnMouseHWheel(float v) { state.hwheel += v; }

	void Update() override {
		// backup current state...
		last_state = state;
		state.wheel = 0;

		// ...and update it
		SDL_Event event;
		while (SDL_PollEvent(&event))
			switch (event.type) {
				case SDL_FINGERMOTION: {
					// use only if there is one finger
					if (SDL_GetTouchFinger(event.tfinger.touchId, 1))
						break;
					auto &plus = g_plus.get();
					auto size = plus.GetRendererAsync()->GetOutputSurfaceSize().get();

					state.x = float(event.tfinger.x) * size.x;
					state.y = float(1.f - event.tfinger.y) * size.y;
				} break;

				case SDL_FINGERDOWN: {
					// use only if there is one finger
					if (SDL_GetTouchFinger(event.tfinger.touchId, 1))
						break;
					auto &plus = g_plus.get();
					auto size = plus.GetRendererAsync()->GetOutputSurfaceSize().get();

					state.left_button = true;
					state.x = float(event.tfinger.x) * size.x;
					state.y = float(1.f - event.tfinger.y) * size.y;
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
					state.left_button = false;
					break;

				case SDL_MOUSEMOTION: {
					auto &plus = g_plus.get();
					auto size = plus.GetRendererAsync()->GetOutputSurfaceSize().get();
					int h = size.y;

					state.x = float(event.motion.x);
					state.y = float(h - event.motion.y);
				} break;

				case SDL_MOUSEBUTTONUP:
				case SDL_MOUSEBUTTONDOWN:
					switch (event.button.button) {
						case SDL_BUTTON_LEFT:
							state.left_button = (event.button.state == SDL_PRESSED);
							break;
						case SDL_BUTTON_MIDDLE:
							state.middle_button = (event.button.state == SDL_PRESSED);
							break;
						case SDL_BUTTON_RIGHT:
							state.right_button = (event.button.state == SDL_PRESSED);
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
	}

	bool SetValue(AnalogInput inp, float value) override {
		//	SDL_WarpMouseGlobal(msx, msy);
		return false;
	}
};

struct Keyboard : KeyboardInputDevice {
	void Update() {
		auto handle = GetWindowHandle(GetWindowInFocus());
		if (!handle)
			return;

		// store current state...
		was_down = is_down;

		// ...and update it.
		const Uint8 *keys = SDL_GetKeyboardState(nullptr);

		is_down[KeyUp] = keys[SDL_SCANCODE_UP];
		is_down[KeyDown] = keys[SDL_SCANCODE_DOWN];
		is_down[KeyLeft] = keys[SDL_SCANCODE_LEFT];
		is_down[KeyRight] = keys[SDL_SCANCODE_RIGHT];

		is_down[KeyEscape] = keys[SDL_SCANCODE_ESCAPE];

		is_down[KeyAdd] = keys[SDL_SCANCODE_KP_PLUS];
		is_down[KeySub] = keys[SDL_SCANCODE_KP_MINUS];
		is_down[KeyMul] = keys[SDL_SCANCODE_KP_MULTIPLY];
		is_down[KeyDiv] = keys[SDL_SCANCODE_KP_DIVIDE];
		is_down[KeyEnter] = keys[SDL_SCANCODE_KP_ENTER];

		is_down[KeyPrintScreen] = keys[SDL_SCANCODE_PRINTSCREEN];
		is_down[KeyScrollLock] = keys[SDL_SCANCODE_SCROLLLOCK];
		is_down[KeyPause] = keys[SDL_SCANCODE_PAUSE];
		is_down[KeyNumLock] = keys[SDL_SCANCODE_NUMLOCKCLEAR];
		is_down[KeyReturn] = keys[SDL_SCANCODE_RETURN];

		is_down[KeyLShift] = keys[SDL_SCANCODE_LSHIFT];
		is_down[KeyRShift] = keys[SDL_SCANCODE_RSHIFT];
		is_down[KeyLCtrl] = keys[SDL_SCANCODE_LCTRL];
		is_down[KeyRCtrl] = keys[SDL_SCANCODE_RCTRL];
		is_down[KeyLAlt] = keys[SDL_SCANCODE_LALT];
		is_down[KeyRAlt] = keys[SDL_SCANCODE_RALT];
		is_down[KeyLWin] = keys[SDL_SCANCODE_LGUI];
		is_down[KeyRWin] = keys[SDL_SCANCODE_RGUI];

		is_down[KeyTab] = keys[SDL_SCANCODE_TAB];
		is_down[KeyCapsLock] = keys[SDL_SCANCODE_CAPSLOCK];
		is_down[KeySpace] = keys[SDL_SCANCODE_SPACE];
		is_down[KeyBackspace] = keys[SDL_SCANCODE_BACKSPACE];
		is_down[KeyInsert] = keys[SDL_SCANCODE_INSERT];
		is_down[KeySuppr] = keys[SDL_SCANCODE_DELETE];
		is_down[KeyHome] = keys[SDL_SCANCODE_HOME];
		is_down[KeyEnd] = keys[SDL_SCANCODE_END];
		is_down[KeyPageUp] = keys[SDL_SCANCODE_PAGEUP];
		is_down[KeyPageDown] = keys[SDL_SCANCODE_PAGEDOWN];

		is_down[KeyF1] = keys[SDL_SCANCODE_F1];
		is_down[KeyF2] = keys[SDL_SCANCODE_F2];
		is_down[KeyF3] = keys[SDL_SCANCODE_F3];
		is_down[KeyF4] = keys[SDL_SCANCODE_F4];
		is_down[KeyF5] = keys[SDL_SCANCODE_F5];
		is_down[KeyF6] = keys[SDL_SCANCODE_F6];
		is_down[KeyF7] = keys[SDL_SCANCODE_F7];
		is_down[KeyF8] = keys[SDL_SCANCODE_F8];
		is_down[KeyF9] = keys[SDL_SCANCODE_F9];
		is_down[KeyF10] = keys[SDL_SCANCODE_F10];
		is_down[KeyF11] = keys[SDL_SCANCODE_F11];
		is_down[KeyF12] = keys[SDL_SCANCODE_F12];

		is_down[KeyNumpad0] = keys[SDL_SCANCODE_KP_0];
		is_down[KeyNumpad1] = keys[SDL_SCANCODE_KP_1];
		is_down[KeyNumpad2] = keys[SDL_SCANCODE_KP_2];
		is_down[KeyNumpad3] = keys[SDL_SCANCODE_KP_3];
		is_down[KeyNumpad4] = keys[SDL_SCANCODE_KP_4];
		is_down[KeyNumpad5] = keys[SDL_SCANCODE_KP_5];
		is_down[KeyNumpad6] = keys[SDL_SCANCODE_KP_6];
		is_down[KeyNumpad7] = keys[SDL_SCANCODE_KP_7];
		is_down[KeyNumpad8] = keys[SDL_SCANCODE_KP_8];
		is_down[KeyNumpad9] = keys[SDL_SCANCODE_KP_9];

		is_down[KeyA] = keys[SDL_SCANCODE_A];
		is_down[KeyB] = keys[SDL_SCANCODE_B];
		is_down[KeyC] = keys[SDL_SCANCODE_C];
		is_down[KeyD] = keys[SDL_SCANCODE_D];
		is_down[KeyE] = keys[SDL_SCANCODE_E];
		is_down[KeyF] = keys[SDL_SCANCODE_F];
		is_down[KeyG] = keys[SDL_SCANCODE_G];
		is_down[KeyH] = keys[SDL_SCANCODE_H];
		is_down[KeyI] = keys[SDL_SCANCODE_I];
		is_down[KeyJ] = keys[SDL_SCANCODE_J];
		is_down[KeyK] = keys[SDL_SCANCODE_K];
		is_down[KeyL] = keys[SDL_SCANCODE_L];
		is_down[KeyM] = keys[SDL_SCANCODE_M];
		is_down[KeyN] = keys[SDL_SCANCODE_N];
		is_down[KeyO] = keys[SDL_SCANCODE_O];
		is_down[KeyP] = keys[SDL_SCANCODE_P];
		is_down[KeyQ] = keys[SDL_SCANCODE_Q];
		is_down[KeyR] = keys[SDL_SCANCODE_R];
		is_down[KeyS] = keys[SDL_SCANCODE_S];
		is_down[KeyT] = keys[SDL_SCANCODE_T];
		is_down[KeyU] = keys[SDL_SCANCODE_U];
		is_down[KeyV] = keys[SDL_SCANCODE_V];
		is_down[KeyW] = keys[SDL_SCANCODE_W];
		is_down[KeyX] = keys[SDL_SCANCODE_X];
		is_down[KeyY] = keys[SDL_SCANCODE_Y];
		is_down[KeyZ] = keys[SDL_SCANCODE_Z];
	}
};

void RegisterInputDevices(InputSystem &system) {
	//	g_window_system.get().new_window_signal.Connect(&OnNewWindow);
	system.RegisterDirectDevice("mouse", std::make_shared<Mouse>());
	system.RegisterDirectDevice("keyboard", std::make_shared<Keyboard>());
}

void InputInit() {}
void InputShutdown() {}

} // namespace hg
