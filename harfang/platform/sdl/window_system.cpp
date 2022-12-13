// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "platform/window_system.h"
#include "foundation/format.h"
#include "foundation/log.h"
#include <SDL.h>
#include <iostream>

namespace hg {

//-- Monitor

struct Monitor {
	std::string id;
	iRect rect;
	bool primary;
};

std::vector<Monitor *> GetMonitors() {
	std::vector<Monitor *> monitors;
	return monitors;
}

iRect GetMonitorRect(const Monitor *m) { return m->rect; }

bool IsPrimaryMonitor(const Monitor *m) { return m->primary; }
// TODO Return true if the monitor is connected.
bool IsMonitorConnected(const Monitor *monitor) { return true; }
// TODO Return monitor name.
std::string GetMonitorName(const Monitor *monitor) { return "TODO IN SDL"; }
// TODO Return monitor size in millimeters.
hg::iVec2 GetMonitorSizeMM(const Monitor *monitor) { return hg::iVec2(0, 0); }
// TODO Get the list of screen modes for a given monitor.
bool GetMonitorModes(const Monitor *monitor, std::vector<MonitorMode> &modes) { return true; }

//-- Window
//
struct Window {
	SDL_Window *w{0};
	bool is_foreign{false};
};

static Window *window_in_focus;

Signal<void(const Window *)> new_window_signal;
Signal<void(const Window *, bool)> window_focus_signal;
Signal<bool(const Window *)> close_window_signal;
Signal<void(const Window *)> destroy_window_signal;

void WindowSystemInit() {
	SDL_Init(SDL_INIT_VIDEO);

	// DISABLE ALL TEXT KEYBOARD
	SDL_EventState(SDL_TEXTINPUT, SDL_DISABLE);
	SDL_EventState(SDL_KEYDOWN, SDL_DISABLE);
	SDL_EventState(SDL_KEYUP, SDL_DISABLE);
}

Window *NewWindow(int width, int height, int bpp, WindowVisibility visibility) {

	Window *w = new Window();

	int window_flag = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN;

	switch (visibility) {
		case WV_Fullscreen:
			window_flag |= SDL_WINDOW_FULLSCREEN;
			break;

		case WV_Undecorated:
			window_flag |= SDL_WINDOW_BORDERLESS;
			break;

		default:
		case WV_Windowed:
			break;
	}

	w->w = SDL_CreateWindow(nullptr, 0, 0, width, height, window_flag);
	UpdateWindow(w);

	debug(format("NewWindow: %1").arg((void *)w->w));
	return w;
}

Window *NewWindow(const char *title, int width, int height, int bpp, WindowVisibility visibility) {
	auto win = NewWindow(width, height, bpp, visibility);
	SetWindowTitle(win, title);
	return win;
}

Window *NewWindowFrom(void *handle) {
	Window *w = new Window();

	w->is_foreign = true;
	w->w = reinterpret_cast<SDL_Window *>(handle);

	SDL_CreateWindowFrom(w->w);
	return w;
}

// TODO Create a new fullscreen window on a specified monitor.
Window *NewFullscreenWindow(const Monitor *monitor, int mode_index, MonitorRotation rotation) {
    return NewWindow(512, 512, 32, WV_Windowed);
}
Window * NewFullscreenWindow(const char *title, const Monitor *monitor, int mode_index, MonitorRotation rotation) {
    return NewWindow(512, 512, 32, WV_Windowed);
}


void *GetDisplay() { return nullptr; }

static const char *canvas_name = "#canvas";
void *GetWindowHandle(const Window *w) {
	return (void *)canvas_name;
	// return reinterpret_cast<void *>(w->w);
}

Window *GetWindowInFocus() { return window_in_focus; }

bool DestroyWindow(Window *w) {
	debug(format("DestroyWindow: %1").arg((void *)w->w));
	window_focus_signal.Emit(w, false);
	SDL_DestroyWindow(w->w);
	w->w = 0;
	delete w;

	return true;
}

//
bool UpdateWindow(const Window *w) {

	if (SDL_GetWindowFlags(w->w) & SDL_WINDOW_INPUT_FOCUS) {
		// in focus
		if (!(window_in_focus == w)) {

			window_in_focus = (Window *)w;
			window_focus_signal.Emit(w, true);
		}
	} else if (window_in_focus == w) {
		// not in focus
		window_in_focus = new Window(); // blank window
		window_focus_signal.Emit(w, false);
	}
	return true;
}

//
bool GetWindowClientSize(const Window *w, int &width, int &height) {
	if (!w)
		return false;
	SDL_GetWindowSize(w->w, &width, &height);
	return true;
}

bool SetWindowClientSize(Window *w, int width, int height) {
	if (!w)
		return false;
	SDL_SetWindowSize(w->w, width, height);
	return true;
}

bool GetWindowTitle(const Window *w, std::string &title) { return true; }

bool SetWindowTitle(Window *w, const std::string &title) { return true; }

bool WindowHasFocus(const Window *w) { return true; }

bool SetWindowPos(Window *w, const hg::iVec2 &v) {
	SDL_SetWindowPosition(w->w, v.x, v.y);
	UpdateWindow(w); // process messages on the spot
	return true;
}

hg::iVec2 GetWindowPos(const Window *w) {

	UpdateWindow(w);

	hg::iVec2 pos;
	SDL_GetWindowPosition(w->w, &pos.x, &pos.y);
	return pos;
}

hg::Vec2 GetWindowContentScale(const Window *window) {
    puts(__FILE__ "GetWindowContentScale");
    return { 1, 1 };
}

void ShowCursor() { SDL_ShowCursor(SDL_ENABLE); }
void HideCursor() { SDL_ShowCursor(SDL_DISABLE); }
void DisableCursor() { SDL_SetRelativeMouseMode(SDL_TRUE); }
void WindowSystemShutdown() { }

} // namespace hg
