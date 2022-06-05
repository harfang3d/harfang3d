// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "platform/input_system.h"

#include "foundation/utf8.h"
#include "platform/glfw/window_system.h"
#include "platform/window_system.h"

#include <GLFW/glfw3.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <map>

namespace hg {

static double wheel = 0, hwheel = 0;
static int inhibit_click = 0;

static MouseState ReadMouse() {
	int w, h;

	auto win = GetWindowInFocus();
	if (!GetWindowClientSize(win, w, h)) {
		inhibit_click = 3;
		return {};
	}

	auto glfw_win = GetGLFWWindow(win);
	if (!glfw_win) {
		inhibit_click = 3;
		return {};
	}

	double x, y;
	glfwGetCursorPos(glfw_win, &x, &y);
	y = double(h) - y;

	MouseState state;

	state.x = int(x);
	state.y = int(y);
	state.button.reset();

	for (auto i = 0; i < 8; ++i)
		state.button[i] = glfwGetMouseButton(glfw_win, i) == GLFW_PRESS && inhibit_click == 0;

	if (inhibit_click > 0)
		--inhibit_click;

	state.wheel = int(wheel);
	state.hwheel = int(hwheel);

	wheel = 0;
	hwheel = 0;

	return state;
}

static KeyboardState ReadKeyboard() {
	auto glfw_win = GetGLFWWindow(GetWindowInFocus());
	if (!glfw_win)
		return {};

	KeyboardState state;
	state.key[K_LShift] = glfwGetKey(glfw_win, GLFW_KEY_LEFT_SHIFT);
	state.key[K_RShift] = glfwGetKey(glfw_win, GLFW_KEY_RIGHT_SHIFT);
	state.key[K_LCtrl] = glfwGetKey(glfw_win, GLFW_KEY_LEFT_CONTROL);
	state.key[K_RCtrl] = glfwGetKey(glfw_win, GLFW_KEY_RIGHT_CONTROL);
	state.key[K_LAlt] = glfwGetKey(glfw_win, GLFW_KEY_LEFT_ALT);
	state.key[K_RAlt] = glfwGetKey(glfw_win, GLFW_KEY_RIGHT_ALT);
	state.key[K_LWin] = glfwGetKey(glfw_win, GLFW_KEY_LEFT_SUPER);
	state.key[K_RWin] = glfwGetKey(glfw_win, GLFW_KEY_RIGHT_SUPER);
	state.key[K_Tab] = glfwGetKey(glfw_win, GLFW_KEY_TAB);
	state.key[K_CapsLock] = glfwGetKey(glfw_win, GLFW_KEY_CAPS_LOCK);
	state.key[K_Space] = glfwGetKey(glfw_win, GLFW_KEY_SPACE);
	state.key[K_Backspace] = glfwGetKey(glfw_win, GLFW_KEY_BACKSPACE);
	state.key[K_Insert] = glfwGetKey(glfw_win, GLFW_KEY_INSERT);
	state.key[K_Suppr] = glfwGetKey(glfw_win, GLFW_KEY_DELETE);
	state.key[K_Home] = glfwGetKey(glfw_win, GLFW_KEY_HOME);
	state.key[K_End] = glfwGetKey(glfw_win, GLFW_KEY_END);
	state.key[K_PageUp] = glfwGetKey(glfw_win, GLFW_KEY_PAGE_UP);
	state.key[K_PageDown] = glfwGetKey(glfw_win, GLFW_KEY_PAGE_DOWN);
	state.key[K_Up] = glfwGetKey(glfw_win, GLFW_KEY_UP);
	state.key[K_Down] = glfwGetKey(glfw_win, GLFW_KEY_DOWN);
	state.key[K_Left] = glfwGetKey(glfw_win, GLFW_KEY_LEFT);
	state.key[K_Right] = glfwGetKey(glfw_win, GLFW_KEY_RIGHT);
	state.key[K_Escape] = glfwGetKey(glfw_win, GLFW_KEY_ESCAPE);
	state.key[K_F1] = glfwGetKey(glfw_win, GLFW_KEY_F1);
	state.key[K_F2] = glfwGetKey(glfw_win, GLFW_KEY_F2);
	state.key[K_F3] = glfwGetKey(glfw_win, GLFW_KEY_F3);
	state.key[K_F4] = glfwGetKey(glfw_win, GLFW_KEY_F4);
	state.key[K_F5] = glfwGetKey(glfw_win, GLFW_KEY_F5);
	state.key[K_F6] = glfwGetKey(glfw_win, GLFW_KEY_F6);
	state.key[K_F7] = glfwGetKey(glfw_win, GLFW_KEY_F7);
	state.key[K_F8] = glfwGetKey(glfw_win, GLFW_KEY_F8);
	state.key[K_F9] = glfwGetKey(glfw_win, GLFW_KEY_F9);
	state.key[K_F10] = glfwGetKey(glfw_win, GLFW_KEY_F10);
	state.key[K_F11] = glfwGetKey(glfw_win, GLFW_KEY_F11);
	state.key[K_F12] = glfwGetKey(glfw_win, GLFW_KEY_F12);
	state.key[K_PrintScreen] = glfwGetKey(glfw_win, GLFW_KEY_PRINT_SCREEN);
	state.key[K_ScrollLock] = glfwGetKey(glfw_win, GLFW_KEY_SCROLL_LOCK);
	state.key[K_Pause] = glfwGetKey(glfw_win, GLFW_KEY_PAUSE);
	state.key[K_NumLock] = glfwGetKey(glfw_win, GLFW_KEY_NUM_LOCK);
	state.key[K_Return] = glfwGetKey(glfw_win, GLFW_KEY_ENTER);
	state.key[K_0] = glfwGetKey(glfw_win, GLFW_KEY_0);
	state.key[K_1] = glfwGetKey(glfw_win, GLFW_KEY_1);
	state.key[K_2] = glfwGetKey(glfw_win, GLFW_KEY_2);
	state.key[K_3] = glfwGetKey(glfw_win, GLFW_KEY_3);
	state.key[K_4] = glfwGetKey(glfw_win, GLFW_KEY_4);
	state.key[K_5] = glfwGetKey(glfw_win, GLFW_KEY_5);
	state.key[K_6] = glfwGetKey(glfw_win, GLFW_KEY_6);
	state.key[K_7] = glfwGetKey(glfw_win, GLFW_KEY_7);
	state.key[K_8] = glfwGetKey(glfw_win, GLFW_KEY_8);
	state.key[K_9] = glfwGetKey(glfw_win, GLFW_KEY_9);
	state.key[K_Numpad0] = glfwGetKey(glfw_win, GLFW_KEY_KP_0);
	state.key[K_Numpad1] = glfwGetKey(glfw_win, GLFW_KEY_KP_1);
	state.key[K_Numpad2] = glfwGetKey(glfw_win, GLFW_KEY_KP_2);
	state.key[K_Numpad3] = glfwGetKey(glfw_win, GLFW_KEY_KP_3);
	state.key[K_Numpad4] = glfwGetKey(glfw_win, GLFW_KEY_KP_4);
	state.key[K_Numpad5] = glfwGetKey(glfw_win, GLFW_KEY_KP_5);
	state.key[K_Numpad6] = glfwGetKey(glfw_win, GLFW_KEY_KP_6);
	state.key[K_Numpad7] = glfwGetKey(glfw_win, GLFW_KEY_KP_7);
	state.key[K_Numpad8] = glfwGetKey(glfw_win, GLFW_KEY_KP_8);
	state.key[K_Numpad9] = glfwGetKey(glfw_win, GLFW_KEY_KP_9);
	state.key[K_Add] = glfwGetKey(glfw_win, GLFW_KEY_KP_ADD);
	state.key[K_Sub] = glfwGetKey(glfw_win, GLFW_KEY_KP_SUBTRACT);
	state.key[K_Mul] = glfwGetKey(glfw_win, GLFW_KEY_KP_MULTIPLY);
	state.key[K_Div] = glfwGetKey(glfw_win, GLFW_KEY_KP_DIVIDE);
	state.key[K_Enter] = glfwGetKey(glfw_win, GLFW_KEY_KP_ENTER);
	state.key[K_A] = glfwGetKey(glfw_win, GLFW_KEY_A);
	state.key[K_B] = glfwGetKey(glfw_win, GLFW_KEY_B);
	state.key[K_C] = glfwGetKey(glfw_win, GLFW_KEY_C);
	state.key[K_D] = glfwGetKey(glfw_win, GLFW_KEY_D);
	state.key[K_E] = glfwGetKey(glfw_win, GLFW_KEY_E);
	state.key[K_F] = glfwGetKey(glfw_win, GLFW_KEY_F);
	state.key[K_G] = glfwGetKey(glfw_win, GLFW_KEY_G);
	state.key[K_H] = glfwGetKey(glfw_win, GLFW_KEY_H);
	state.key[K_I] = glfwGetKey(glfw_win, GLFW_KEY_I);
	state.key[K_J] = glfwGetKey(glfw_win, GLFW_KEY_J);
	state.key[K_K] = glfwGetKey(glfw_win, GLFW_KEY_K);
	state.key[K_L] = glfwGetKey(glfw_win, GLFW_KEY_L);
	state.key[K_M] = glfwGetKey(glfw_win, GLFW_KEY_M);
	state.key[K_N] = glfwGetKey(glfw_win, GLFW_KEY_N);
	state.key[K_O] = glfwGetKey(glfw_win, GLFW_KEY_O);
	state.key[K_P] = glfwGetKey(glfw_win, GLFW_KEY_P);
	state.key[K_Q] = glfwGetKey(glfw_win, GLFW_KEY_Q);
	state.key[K_R] = glfwGetKey(glfw_win, GLFW_KEY_R);
	state.key[K_S] = glfwGetKey(glfw_win, GLFW_KEY_S);
	state.key[K_T] = glfwGetKey(glfw_win, GLFW_KEY_T);
	state.key[K_U] = glfwGetKey(glfw_win, GLFW_KEY_U);
	state.key[K_V] = glfwGetKey(glfw_win, GLFW_KEY_V);
	state.key[K_W] = glfwGetKey(glfw_win, GLFW_KEY_W);
	state.key[K_X] = glfwGetKey(glfw_win, GLFW_KEY_X);
	state.key[K_Y] = glfwGetKey(glfw_win, GLFW_KEY_Y);
	state.key[K_Z] = glfwGetKey(glfw_win, GLFW_KEY_Z);
	state.key[K_Plus] = glfwGetKey(glfw_win, GLFW_KEY_EQUAL);
	state.key[K_Comma] = glfwGetKey(glfw_win, GLFW_KEY_COMMA);
	state.key[K_Minus] = glfwGetKey(glfw_win, GLFW_KEY_MINUS);
	state.key[K_Period] = glfwGetKey(glfw_win, GLFW_KEY_PERIOD);
	return state;
}

static const char *GetKeyName(Key key) {
	static std::map<Key, int> key_to_glfw = {
		{K_LShift, GLFW_KEY_LEFT_SHIFT},
		{K_RShift, GLFW_KEY_RIGHT_SHIFT},
		{K_LCtrl, GLFW_KEY_LEFT_CONTROL},
		{K_RCtrl, GLFW_KEY_RIGHT_CONTROL},
		{K_LAlt, GLFW_KEY_LEFT_ALT},
		{K_RAlt, GLFW_KEY_RIGHT_ALT},
		{K_LWin, GLFW_KEY_LEFT_SUPER},
		{K_RWin, GLFW_KEY_RIGHT_SUPER},
		{K_Tab, GLFW_KEY_TAB},
		{K_CapsLock, GLFW_KEY_CAPS_LOCK},
		{K_Space, GLFW_KEY_SPACE},
		{K_Backspace, GLFW_KEY_BACKSPACE},
		{K_Insert, GLFW_KEY_INSERT},
		{K_Suppr, GLFW_KEY_DELETE},
		{K_Home, GLFW_KEY_HOME},
		{K_End, GLFW_KEY_END},
		{K_PageUp, GLFW_KEY_PAGE_UP},
		{K_PageDown, GLFW_KEY_PAGE_DOWN},
		{K_Up, GLFW_KEY_UP},
		{K_Down, GLFW_KEY_DOWN},
		{K_Left, GLFW_KEY_LEFT},
		{K_Right, GLFW_KEY_RIGHT},
		{K_Escape, GLFW_KEY_ESCAPE},
		{K_F1, GLFW_KEY_F1},
		{K_F2, GLFW_KEY_F2},
		{K_F3, GLFW_KEY_F3},
		{K_F4, GLFW_KEY_F4},
		{K_F5, GLFW_KEY_F5},
		{K_F6, GLFW_KEY_F6},
		{K_F7, GLFW_KEY_F7},
		{K_F8, GLFW_KEY_F8},
		{K_F9, GLFW_KEY_F9},
		{K_F10, GLFW_KEY_F10},
		{K_F11, GLFW_KEY_F11},
		{K_F12, GLFW_KEY_F12},
		{K_PrintScreen, GLFW_KEY_PRINT_SCREEN},
		{K_ScrollLock, GLFW_KEY_SCROLL_LOCK},
		{K_Pause, GLFW_KEY_PAUSE},
		{K_NumLock, GLFW_KEY_NUM_LOCK},
		{K_Return, GLFW_KEY_ENTER},
		{K_0, GLFW_KEY_0},
		{K_1, GLFW_KEY_1},
		{K_2, GLFW_KEY_2},
		{K_3, GLFW_KEY_3},
		{K_4, GLFW_KEY_4},
		{K_5, GLFW_KEY_5},
		{K_6, GLFW_KEY_6},
		{K_7, GLFW_KEY_7},
		{K_8, GLFW_KEY_8},
		{K_9, GLFW_KEY_9},
		{K_Numpad0, GLFW_KEY_KP_0},
		{K_Numpad1, GLFW_KEY_KP_1},
		{K_Numpad2, GLFW_KEY_KP_2},
		{K_Numpad3, GLFW_KEY_KP_3},
		{K_Numpad4, GLFW_KEY_KP_4},
		{K_Numpad5, GLFW_KEY_KP_5},
		{K_Numpad6, GLFW_KEY_KP_6},
		{K_Numpad7, GLFW_KEY_KP_7},
		{K_Numpad8, GLFW_KEY_KP_8},
		{K_Numpad9, GLFW_KEY_KP_9},
		{K_Add, GLFW_KEY_KP_ADD},
		{K_Sub, GLFW_KEY_KP_SUBTRACT},
		{K_Mul, GLFW_KEY_KP_MULTIPLY},
		{K_Div, GLFW_KEY_KP_DIVIDE},
		{K_Enter, GLFW_KEY_KP_ENTER},
		{K_A, GLFW_KEY_A},
		{K_B, GLFW_KEY_B},
		{K_C, GLFW_KEY_C},
		{K_D, GLFW_KEY_D},
		{K_E, GLFW_KEY_E},
		{K_F, GLFW_KEY_F},
		{K_G, GLFW_KEY_G},
		{K_H, GLFW_KEY_H},
		{K_I, GLFW_KEY_I},
		{K_J, GLFW_KEY_J},
		{K_K, GLFW_KEY_K},
		{K_L, GLFW_KEY_L},
		{K_M, GLFW_KEY_M},
		{K_N, GLFW_KEY_N},
		{K_O, GLFW_KEY_O},
		{K_P, GLFW_KEY_P},
		{K_Q, GLFW_KEY_Q},
		{K_R, GLFW_KEY_R},
		{K_S, GLFW_KEY_S},
		{K_T, GLFW_KEY_T},
		{K_U, GLFW_KEY_U},
		{K_V, GLFW_KEY_V},
		{K_W, GLFW_KEY_W},
		{K_X, GLFW_KEY_X},
		{K_Y, GLFW_KEY_Y},
		{K_Z, GLFW_KEY_Z},
		{K_Plus, GLFW_KEY_EQUAL},
		{K_Comma, GLFW_KEY_COMMA},
		{K_Minus, GLFW_KEY_MINUS},
		{K_Period, GLFW_KEY_PERIOD},
	};

	const auto i = key_to_glfw.find(key);
	if (i != std::end(key_to_glfw))
		return glfwGetKeyName(i->second, -1);

	return nullptr;
}

static void ScrollCallback(GLFWwindow *w, double xoffset, double yoffset) {
	hwheel += xoffset;
	wheel += yoffset;
}

static void CharCallback(GLFWwindow *w, unsigned int codepoint) {
	utf8_cp utf8[16]{};
	utf32_to_utf8(codepoint, utf8);
	on_text_input.Emit(reinterpret_cast<const char *>(utf8));
}

static void OnNewWindow(const Window *win) {
	auto glfw_window = GetGLFWWindow(win);
	glfwSetScrollCallback(glfw_window, ScrollCallback);
	glfwSetCharCallback(glfw_window, CharCallback);
}

//
#if WIN32

static MouseState ReadRawMouse() {
	MouseState state;

	POINT pos;

	if (GetCursorPos(&pos)) {
		state.x = pos.x;
		state.y = pos.y;
	}

	state.button[MB_0] = GetAsyncKeyState(VK_LBUTTON) & 0x8000 ? true : false;
	state.button[MB_1] = GetAsyncKeyState(VK_RBUTTON) & 0x8000 ? true : false;
	state.button[MB_2] = GetAsyncKeyState(VK_MBUTTON) & 0x8000 ? true : false;

	return state;
}

//
static KeyboardState ReadRawKeyboard() {
	KeyboardState state;

	state.key[K_LShift] = bool(GetAsyncKeyState(VK_LSHIFT) & 0x8000);
	state.key[K_RShift] = bool(GetAsyncKeyState(VK_RSHIFT) & 0x8000);
	state.key[K_LCtrl] = bool(GetAsyncKeyState(VK_LCONTROL) & 0x8000);
	state.key[K_RCtrl] = bool(GetAsyncKeyState(VK_RCONTROL) & 0x8000);
	state.key[K_LAlt] = bool(GetAsyncKeyState(VK_LMENU) & 0x8000);
	state.key[K_RAlt] = bool(GetAsyncKeyState(VK_RMENU) & 0x8000);
	state.key[K_LWin] = bool(GetAsyncKeyState(VK_LWIN) & 0x8000);
	state.key[K_RWin] = bool(GetAsyncKeyState(VK_RWIN) & 0x8000);
	state.key[K_Tab] = bool(GetAsyncKeyState(VK_TAB) & 0x8000);
	state.key[K_CapsLock] = bool(GetAsyncKeyState(VK_CAPITAL) & 0x8000);
	state.key[K_Space] = bool(GetAsyncKeyState(VK_SPACE) & 0x8000);
	state.key[K_Backspace] = bool(GetAsyncKeyState(VK_BACK) & 0x8000);
	state.key[K_Insert] = bool(GetAsyncKeyState(VK_INSERT) & 0x8000);
	state.key[K_Suppr] = bool(GetAsyncKeyState(VK_DELETE) & 0x8000);
	state.key[K_Home] = bool(GetAsyncKeyState(VK_HOME) & 0x8000);
	state.key[K_End] = bool(GetAsyncKeyState(VK_END) & 0x8000);
	state.key[K_PageUp] = bool(GetAsyncKeyState(VK_PRIOR) & 0x8000);
	state.key[K_PageDown] = bool(GetAsyncKeyState(VK_NEXT) & 0x8000);
	state.key[K_Up] = bool(GetAsyncKeyState(VK_UP) & 0x8000);
	state.key[K_Down] = bool(GetAsyncKeyState(VK_DOWN) & 0x8000);
	state.key[K_Left] = bool(GetAsyncKeyState(VK_LEFT) & 0x8000);
	state.key[K_Right] = bool(GetAsyncKeyState(VK_RIGHT) & 0x8000);
	state.key[K_Escape] = bool(GetAsyncKeyState(VK_ESCAPE) & 0x8000);
	state.key[K_F1] = bool(GetAsyncKeyState(VK_F1) & 0x8000);
	state.key[K_F2] = bool(GetAsyncKeyState(VK_F2) & 0x8000);
	state.key[K_F3] = bool(GetAsyncKeyState(VK_F3) & 0x8000);
	state.key[K_F4] = bool(GetAsyncKeyState(VK_F4) & 0x8000);
	state.key[K_F5] = bool(GetAsyncKeyState(VK_F5) & 0x8000);
	state.key[K_F6] = bool(GetAsyncKeyState(VK_F6) & 0x8000);
	state.key[K_F7] = bool(GetAsyncKeyState(VK_F7) & 0x8000);
	state.key[K_F8] = bool(GetAsyncKeyState(VK_F8) & 0x8000);
	state.key[K_F9] = bool(GetAsyncKeyState(VK_F9) & 0x8000);
	state.key[K_F10] = bool(GetAsyncKeyState(VK_F10) & 0x8000);
	state.key[K_F11] = bool(GetAsyncKeyState(VK_F11) & 0x8000);
	state.key[K_F12] = bool(GetAsyncKeyState(VK_F12) & 0x8000);
	state.key[K_PrintScreen] = bool(GetAsyncKeyState(VK_PRINT) & 0x8000);
	state.key[K_ScrollLock] = bool(GetAsyncKeyState(VK_SCROLL) & 0x8000);
	state.key[K_Pause] = bool(GetAsyncKeyState(VK_PAUSE) & 0x8000);
	state.key[K_NumLock] = bool(GetAsyncKeyState(VK_NUMLOCK) & 0x8000);
	state.key[K_Return] = bool(GetAsyncKeyState(VK_RETURN) & 0x8000);
	state.key[K_0] = bool(GetAsyncKeyState(0x30) & 0x8000);
	state.key[K_1] = bool(GetAsyncKeyState(0x31) & 0x8000);
	state.key[K_2] = bool(GetAsyncKeyState(0x32) & 0x8000);
	state.key[K_3] = bool(GetAsyncKeyState(0x33) & 0x8000);
	state.key[K_4] = bool(GetAsyncKeyState(0x34) & 0x8000);
	state.key[K_5] = bool(GetAsyncKeyState(0x35) & 0x8000);
	state.key[K_6] = bool(GetAsyncKeyState(0x36) & 0x8000);
	state.key[K_7] = bool(GetAsyncKeyState(0x37) & 0x8000);
	state.key[K_8] = bool(GetAsyncKeyState(0x38) & 0x8000);
	state.key[K_9] = bool(GetAsyncKeyState(0x39) & 0x8000);
	state.key[K_Numpad0] = bool(GetAsyncKeyState(VK_NUMPAD0) & 0x8000);
	state.key[K_Numpad1] = bool(GetAsyncKeyState(VK_NUMPAD1) & 0x8000);
	state.key[K_Numpad2] = bool(GetAsyncKeyState(VK_NUMPAD2) & 0x8000);
	state.key[K_Numpad3] = bool(GetAsyncKeyState(VK_NUMPAD3) & 0x8000);
	state.key[K_Numpad4] = bool(GetAsyncKeyState(VK_NUMPAD4) & 0x8000);
	state.key[K_Numpad5] = bool(GetAsyncKeyState(VK_NUMPAD5) & 0x8000);
	state.key[K_Numpad6] = bool(GetAsyncKeyState(VK_NUMPAD6) & 0x8000);
	state.key[K_Numpad7] = bool(GetAsyncKeyState(VK_NUMPAD7) & 0x8000);
	state.key[K_Numpad8] = bool(GetAsyncKeyState(VK_NUMPAD8) & 0x8000);
	state.key[K_Numpad9] = bool(GetAsyncKeyState(VK_NUMPAD9) & 0x8000);
	state.key[K_Add] = bool(GetAsyncKeyState(VK_ADD) & 0x8000);
	state.key[K_Sub] = bool(GetAsyncKeyState(VK_SUBTRACT) & 0x8000);
	state.key[K_Mul] = bool(GetAsyncKeyState(VK_MULTIPLY) & 0x8000);
	state.key[K_Div] = bool(GetAsyncKeyState(VK_DIVIDE) & 0x8000);
	state.key[K_Enter] = bool(GetAsyncKeyState(VK_RETURN) & 0x8000);
	state.key[K_A] = bool(GetAsyncKeyState(0x41) & 0x8000);
	state.key[K_B] = bool(GetAsyncKeyState(0x42) & 0x8000);
	state.key[K_C] = bool(GetAsyncKeyState(0x43) & 0x8000);
	state.key[K_D] = bool(GetAsyncKeyState(0x44) & 0x8000);
	state.key[K_E] = bool(GetAsyncKeyState(0x45) & 0x8000);
	state.key[K_F] = bool(GetAsyncKeyState(0x46) & 0x8000);
	state.key[K_G] = bool(GetAsyncKeyState(0x47) & 0x8000);
	state.key[K_H] = bool(GetAsyncKeyState(0x48) & 0x8000);
	state.key[K_I] = bool(GetAsyncKeyState(0x49) & 0x8000);
	state.key[K_J] = bool(GetAsyncKeyState(0x4a) & 0x8000);
	state.key[K_K] = bool(GetAsyncKeyState(0x4b) & 0x8000);
	state.key[K_L] = bool(GetAsyncKeyState(0x4c) & 0x8000);
	state.key[K_M] = bool(GetAsyncKeyState(0x4d) & 0x8000);
	state.key[K_N] = bool(GetAsyncKeyState(0x4e) & 0x8000);
	state.key[K_O] = bool(GetAsyncKeyState(0x4f) & 0x8000);
	state.key[K_P] = bool(GetAsyncKeyState(0x50) & 0x8000);
	state.key[K_Q] = bool(GetAsyncKeyState(0x51) & 0x8000);
	state.key[K_R] = bool(GetAsyncKeyState(0x52) & 0x8000);
	state.key[K_S] = bool(GetAsyncKeyState(0x53) & 0x8000);
	state.key[K_T] = bool(GetAsyncKeyState(0x54) & 0x8000);
	state.key[K_U] = bool(GetAsyncKeyState(0x55) & 0x8000);
	state.key[K_V] = bool(GetAsyncKeyState(0x56) & 0x8000);
	state.key[K_W] = bool(GetAsyncKeyState(0x57) & 0x8000);
	state.key[K_X] = bool(GetAsyncKeyState(0x58) & 0x8000);
	state.key[K_Y] = bool(GetAsyncKeyState(0x59) & 0x8000);
	state.key[K_Z] = bool(GetAsyncKeyState(0x5a) & 0x8000);
	state.key[K_Plus] = bool(GetAsyncKeyState(VK_OEM_PLUS) & 0x8000);
	state.key[K_Comma] = bool(GetAsyncKeyState(VK_OEM_COMMA) & 0x8000);
	state.key[K_Minus] = bool(GetAsyncKeyState(VK_OEM_MINUS) & 0x8000);
	state.key[K_Period] = bool(GetAsyncKeyState(VK_OEM_PERIOD) & 0x8000);
	state.key[K_OEM1] = bool(GetAsyncKeyState(VK_OEM_1) & 0x8000);
	state.key[K_OEM2] = bool(GetAsyncKeyState(VK_OEM_2) & 0x8000);
	state.key[K_OEM3] = bool(GetAsyncKeyState(VK_OEM_3) & 0x8000);
	state.key[K_OEM4] = bool(GetAsyncKeyState(VK_OEM_4) & 0x8000);
	state.key[K_OEM5] = bool(GetAsyncKeyState(VK_OEM_5) & 0x8000);
	state.key[K_OEM6] = bool(GetAsyncKeyState(VK_OEM_6) & 0x8000);
	state.key[K_OEM7] = bool(GetAsyncKeyState(VK_OEM_7) & 0x8000);
	state.key[K_OEM8] = bool(GetAsyncKeyState(VK_OEM_8) & 0x8000);
	state.key[K_BrowserBack] = bool(GetAsyncKeyState(VK_BROWSER_BACK) & 0x8000);
	state.key[K_BrowserForward] = bool(GetAsyncKeyState(VK_BROWSER_FORWARD) & 0x8000);
	state.key[K_BrowserRefresh] = bool(GetAsyncKeyState(VK_BROWSER_REFRESH) & 0x8000);
	state.key[K_BrowserStop] = bool(GetAsyncKeyState(VK_BROWSER_STOP) & 0x8000);
	state.key[K_BrowserSearch] = bool(GetAsyncKeyState(VK_BROWSER_SEARCH) & 0x8000);
	state.key[K_BrowserFavorites] = bool(GetAsyncKeyState(VK_BROWSER_FAVORITES) & 0x8000);
	state.key[K_BrowserHome] = bool(GetAsyncKeyState(VK_BROWSER_HOME) & 0x8000);
	state.key[K_VolumeMute] = bool(GetAsyncKeyState(VK_VOLUME_MUTE) & 0x8000);
	state.key[K_VolumeDown] = bool(GetAsyncKeyState(VK_VOLUME_DOWN) & 0x8000);
	state.key[K_VolumeUp] = bool(GetAsyncKeyState(VK_VOLUME_UP) & 0x8000);
	state.key[K_MediaNextTrack] = bool(GetAsyncKeyState(VK_MEDIA_NEXT_TRACK) & 0x8000);
	state.key[K_MediaPrevTrack] = bool(GetAsyncKeyState(VK_MEDIA_PREV_TRACK) & 0x8000);
	state.key[K_MediaStop] = bool(GetAsyncKeyState(VK_MEDIA_STOP) & 0x8000);
	state.key[K_MediaPlayPause] = bool(GetAsyncKeyState(VK_MEDIA_PLAY_PAUSE) & 0x8000);
	state.key[K_LaunchMail] = bool(GetAsyncKeyState(VK_LAUNCH_MAIL) & 0x8000);
	state.key[K_LaunchMediaSelect] = bool(GetAsyncKeyState(VK_LAUNCH_MEDIA_SELECT) & 0x8000);
	state.key[K_LaunchApp1] = bool(GetAsyncKeyState(VK_LAUNCH_APP1) & 0x8000);
	state.key[K_LaunchApp2] = bool(GetAsyncKeyState(VK_LAUNCH_APP2) & 0x8000);

	return state;
}

#endif // WIN32

//
#if ((GLFW_VERSION_MAJOR >= 3) && (GLFW_VERSION_MINOR >= 3))

static int GamepadSlotToGLFWJoystick(const std::string& name) {
	if (name == "gamepad_slot_0")
		return GLFW_JOYSTICK_1;
	else if (name == "gamepad_slot_1")
		return GLFW_JOYSTICK_2;
	else if (name == "gamepad_slot_2")
		return GLFW_JOYSTICK_3;
	else if (name == "gamepad_slot_3")
		return GLFW_JOYSTICK_4;
	else if (name == "gamepad_slot_4")
		return GLFW_JOYSTICK_5;
	else if (name == "gamepad_slot_5")
		return GLFW_JOYSTICK_6;
	else if (name == "gamepad_slot_6")
		return GLFW_JOYSTICK_7;
	else if (name == "gamepad_slot_7")
		return GLFW_JOYSTICK_8;
	else if (name == "gamepad_slot_8")
		return GLFW_JOYSTICK_9;
	else if (name == "gamepad_slot_9")
		return GLFW_JOYSTICK_10;
	else if (name == "gamepad_slot_10")
		return GLFW_JOYSTICK_11;
	else if (name == "gamepad_slot_11")
		return GLFW_JOYSTICK_12;
	else if (name == "gamepad_slot_12")
		return GLFW_JOYSTICK_13;
	else if (name == "gamepad_slot_13")
		return GLFW_JOYSTICK_14;
	else if (name == "gamepad_slot_14")
		return GLFW_JOYSTICK_15;
	else if (name == "gamepad_slot_15")
		return GLFW_JOYSTICK_16;
	return -1;
}

template <int ID> GamepadState ReadGamepad() {
	GLFWgamepadstate glfw_state;
	glfwGetGamepadState(ID, &glfw_state);

	GamepadState state;
	state.connected = glfwJoystickIsGamepad(ID);

	state.axes[GA_LeftX] = glfw_state.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
	state.axes[GA_LeftY] = glfw_state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
	state.axes[GA_RightX] = glfw_state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
	state.axes[GA_RightY] = glfw_state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];
	state.axes[GA_LeftTrigger] = glfw_state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER];
	state.axes[GA_RightTrigger] = glfw_state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER];

	state.button[GB_ButtonA] = glfw_state.buttons[GLFW_GAMEPAD_BUTTON_A] == GLFW_PRESS;
	state.button[GB_ButtonB] = glfw_state.buttons[GLFW_GAMEPAD_BUTTON_B] == GLFW_PRESS;
	state.button[GB_ButtonX] = glfw_state.buttons[GLFW_GAMEPAD_BUTTON_X] == GLFW_PRESS;
	state.button[GB_ButtonY] = glfw_state.buttons[GLFW_GAMEPAD_BUTTON_Y] == GLFW_PRESS;
	state.button[GB_LeftBumper] = glfw_state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER] == GLFW_PRESS;
	state.button[GB_RightBumper] = glfw_state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER] == GLFW_PRESS;
	state.button[GB_Back] = glfw_state.buttons[GLFW_GAMEPAD_BUTTON_BACK] == GLFW_PRESS;
	state.button[GB_Start] = glfw_state.buttons[GLFW_GAMEPAD_BUTTON_START] == GLFW_PRESS;
	state.button[GB_Guide] = glfw_state.buttons[GLFW_GAMEPAD_BUTTON_GUIDE] == GLFW_PRESS;
	state.button[GB_LeftThumb] = glfw_state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_THUMB] == GLFW_PRESS;
	state.button[GB_RightThumb] = glfw_state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_THUMB] == GLFW_PRESS;
	state.button[GB_DPadUp] = glfw_state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP] == GLFW_PRESS;
	state.button[GB_DPadRight] = glfw_state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT] == GLFW_PRESS;
	state.button[GB_DPadDown] = glfw_state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN] == GLFW_PRESS;
	state.button[GB_DPadLeft] = glfw_state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT] == GLFW_PRESS;
	return state;
}

static int JoystickSlotToGLFWJoystick(const std::string& name) {
	if (name == "joystick_slot_0")
		return GLFW_JOYSTICK_1;
	else if (name == "joystick_slot_1")
		return GLFW_JOYSTICK_2;
	else if (name == "joystick_slot_2")
		return GLFW_JOYSTICK_3;
	else if (name == "joystick_slot_3")
		return GLFW_JOYSTICK_4;
	else if (name == "joystick_slot_4")
		return GLFW_JOYSTICK_5;
	else if (name == "joystick_slot_5")
		return GLFW_JOYSTICK_6;
	else if (name == "joystick_slot_6")
		return GLFW_JOYSTICK_7;
	else if (name == "joystick_slot_7")
		return GLFW_JOYSTICK_8;
	else if (name == "joystick_slot_8")
		return GLFW_JOYSTICK_9;
	else if (name == "joystick_slot_9")
		return GLFW_JOYSTICK_10;
	else if (name == "joystick_slot_10")
		return GLFW_JOYSTICK_11;
	else if (name == "joystick_slot_11")
		return GLFW_JOYSTICK_12;
	else if (name == "joystick_slot_12")
		return GLFW_JOYSTICK_13;
	else if (name == "joystick_slot_13")
		return GLFW_JOYSTICK_14;
	else if (name == "joystick_slot_14")
		return GLFW_JOYSTICK_15;
	else if (name == "joystick_slot_15")
		return GLFW_JOYSTICK_16;
	return -1;
}

template <int ID> JoystickState ReadJoystick() {

	int axis_count = 0;
	auto axis = glfwGetJoystickAxes(ID, &axis_count);
	int buttons_count = 0;
	auto buttons = glfwGetJoystickButtons(ID, &buttons_count);

	JoystickState state{static_cast<short>(axis_count), static_cast<short>(buttons_count)};
	state.connected = axis_count && buttons_count;

	for (int i = 0; i < axis_count; ++i)
		state.axes[i] = axis[i];

	for (int i = 0; i < buttons_count; ++i)
		state.buttons[i] = buttons[i] == GLFW_PRESS;
	return state;
}

template <int ID> std::string DeviceNameJoystick() {
	auto name = glfwGetJoystickName(ID);
	return name == nullptr ? "" : name;
}

#endif // GLFW >= 3.3

//
static Signal<void(const Window *)>::Connection on_new_window_connection;

void InputInit() {
	on_new_window_connection = new_window_signal.Connect(&OnNewWindow);

	AddMouseReader("default", ReadMouse);
	AddKeyboardReader("default", ReadKeyboard, GetKeyName);

#if WIN32
	AddMouseReader("raw", ReadRawMouse);
	AddKeyboardReader("raw", ReadRawKeyboard, GetKeyName);
#endif

#if ((GLFW_VERSION_MAJOR >= 3) && (GLFW_VERSION_MINOR >= 3))
	AddGamepadReader("default", ReadGamepad<GLFW_JOYSTICK_1>);

	AddGamepadReader("gamepad_slot_0", ReadGamepad<GLFW_JOYSTICK_1>);
	AddGamepadReader("gamepad_slot_1", ReadGamepad<GLFW_JOYSTICK_2>);
	AddGamepadReader("gamepad_slot_2", ReadGamepad<GLFW_JOYSTICK_3>);
	AddGamepadReader("gamepad_slot_3", ReadGamepad<GLFW_JOYSTICK_4>);
	AddGamepadReader("gamepad_slot_4", ReadGamepad<GLFW_JOYSTICK_5>);
	AddGamepadReader("gamepad_slot_5", ReadGamepad<GLFW_JOYSTICK_6>);
	AddGamepadReader("gamepad_slot_6", ReadGamepad<GLFW_JOYSTICK_7>);
	AddGamepadReader("gamepad_slot_7", ReadGamepad<GLFW_JOYSTICK_8>);
	AddGamepadReader("gamepad_slot_8", ReadGamepad<GLFW_JOYSTICK_9>);
	AddGamepadReader("gamepad_slot_9", ReadGamepad<GLFW_JOYSTICK_10>);
	AddGamepadReader("gamepad_slot_10", ReadGamepad<GLFW_JOYSTICK_11>);
	AddGamepadReader("gamepad_slot_11", ReadGamepad<GLFW_JOYSTICK_12>);
	AddGamepadReader("gamepad_slot_12", ReadGamepad<GLFW_JOYSTICK_13>);
	AddGamepadReader("gamepad_slot_13", ReadGamepad<GLFW_JOYSTICK_14>);
	AddGamepadReader("gamepad_slot_14", ReadGamepad<GLFW_JOYSTICK_15>);
	AddGamepadReader("gamepad_slot_15", ReadGamepad<GLFW_JOYSTICK_16>);

	AddJoystickReader("default", ReadJoystick<GLFW_JOYSTICK_1>, DeviceNameJoystick<GLFW_JOYSTICK_1>);

	AddJoystickReader("joystick_slot_0", ReadJoystick<GLFW_JOYSTICK_1>, DeviceNameJoystick<GLFW_JOYSTICK_1>);
	AddJoystickReader("joystick_slot_1", ReadJoystick<GLFW_JOYSTICK_2>, DeviceNameJoystick<GLFW_JOYSTICK_2>);
	AddJoystickReader("joystick_slot_2", ReadJoystick<GLFW_JOYSTICK_3>, DeviceNameJoystick<GLFW_JOYSTICK_3>);
	AddJoystickReader("joystick_slot_3", ReadJoystick<GLFW_JOYSTICK_4>, DeviceNameJoystick<GLFW_JOYSTICK_4>);
	AddJoystickReader("joystick_slot_4", ReadJoystick<GLFW_JOYSTICK_5>, DeviceNameJoystick<GLFW_JOYSTICK_5>);
	AddJoystickReader("joystick_slot_5", ReadJoystick<GLFW_JOYSTICK_6>, DeviceNameJoystick<GLFW_JOYSTICK_6>);
	AddJoystickReader("joystick_slot_6", ReadJoystick<GLFW_JOYSTICK_7>, DeviceNameJoystick<GLFW_JOYSTICK_7>);
	AddJoystickReader("joystick_slot_7", ReadJoystick<GLFW_JOYSTICK_8>, DeviceNameJoystick<GLFW_JOYSTICK_8>);
	AddJoystickReader("joystick_slot_8", ReadJoystick<GLFW_JOYSTICK_9>, DeviceNameJoystick<GLFW_JOYSTICK_9>);
	AddJoystickReader("joystick_slot_9", ReadJoystick<GLFW_JOYSTICK_10>, DeviceNameJoystick<GLFW_JOYSTICK_10>);
	AddJoystickReader("joystick_slot_10", ReadJoystick<GLFW_JOYSTICK_11>, DeviceNameJoystick<GLFW_JOYSTICK_11>);
	AddJoystickReader("joystick_slot_11", ReadJoystick<GLFW_JOYSTICK_12>, DeviceNameJoystick<GLFW_JOYSTICK_12>);
	AddJoystickReader("joystick_slot_12", ReadJoystick<GLFW_JOYSTICK_13>, DeviceNameJoystick<GLFW_JOYSTICK_13>);
	AddJoystickReader("joystick_slot_13", ReadJoystick<GLFW_JOYSTICK_14>, DeviceNameJoystick<GLFW_JOYSTICK_14>);
	AddJoystickReader("joystick_slot_14", ReadJoystick<GLFW_JOYSTICK_15>, DeviceNameJoystick<GLFW_JOYSTICK_15>);
	AddJoystickReader("joystick_slot_15", ReadJoystick<GLFW_JOYSTICK_16>, DeviceNameJoystick<GLFW_JOYSTICK_16>);
#endif
}

void InputShutdown() { new_window_signal.Disconnect(on_new_window_connection); }

} // namespace hg
