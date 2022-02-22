// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "platform/window_system.h"
#include "foundation/log.h"
#include <SDL.h>
#include <iostream>

namespace hg {

//-- Monitor
struct Monitor::Impl {
	std::string id;
	iRect rect;
	bool primary;
};

Monitor::Monitor() : impl_(new Monitor::Impl()) {}
Monitor::~Monitor() = default;
Monitor::Monitor(const Monitor &m) : impl_(new Monitor::Impl(*m.impl_)) {}
Monitor::Monitor(Monitor &&) noexcept = default;
Monitor &Monitor::operator=(const Monitor &m) {
	if (this != &m) {
		impl_.reset(new Monitor::Impl(*m.impl_));
	}
	return *this;
}
Monitor &Monitor::operator=(Monitor &&) noexcept = default;

std::vector<Monitor> GetMonitors() {
	std::vector<Monitor> monitors;
	return monitors;
}

iRect GetMonitorRect(const Monitor &m) { return m.impl_->rect; }

bool IsPrimaryMonitor(const Monitor &m) { return m.impl_->primary; }
// TODO Return true if the monitor is connected.
bool IsMonitorConnected(const Monitor &monitor) { return true; }
// TODO Return monitor name.
std::string GetMonitorName(const Monitor &monitor) { return "TODO IN SDL"; }
// TODO Return monitor size in millimeters.
iVec2 GetMonitorSizeMM(const Monitor &monitor) { return iVec2(0, 0); }
// TODO Get the list of screen modes for a given monitor.
bool GetMonitorModes(const Monitor &monitor, std::vector<MonitorMode> &modes) { return true; }

//-- Window
//
struct SDLWindow {
	SDL_Window *w{0};
	bool is_foreign{false};
};

void WindowSystemInit() { SDL_Init(SDL_INIT_VIDEO); }

Window NewWindow(int width, int height, int bpp, Window::Visibility visibility) {

	Window w;
	static_assert(sizeof(SDLWindow) <= sizeof(Window), "Window OS object size exceeds generic object size");
	auto data = reinterpret_cast<SDLWindow *>(w.data.data());

	uint window_flag = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

	switch (visibility) {
		case Window::Fullscreen:
			window_flag |= SDL_WINDOW_FULLSCREEN;
			break;

		case Window::Undecorated:
			window_flag |= SDL_WINDOW_BORDERLESS;
			break;

		default:
		case Window::Windowed:
			break;
	}

	data->w = SDL_CreateWindow("Harfang", 0, 0, width, height, window_flag);

	SetWindowTitle(w, "Harfang");
	UpdateWindow(w);

	debug(format("NewWindow: %1").arg((void *)data->w));
	return w;
}

Window NewWindowFrom(void *handle) {
	Window w;
	auto data = reinterpret_cast<SDLWindow *>(w.data.data());

	data->is_foreign = true;
	data->w = reinterpret_cast<SDL_Window *>(handle);

	SDL_CreateWindowFrom(data->w);
	return w;
}

// TODO Create a new fullscreen window on a specified monitor.
Window NewFullscreenWindow(const Monitor &monitor, int mode_index, MonitorRotation rotation) { return NewWindow(512, 512, 32, Window::Windowed); }

void *GetDisplay() { return nullptr; }

void *GetWindowHandle(const Window &w) { return reinterpret_cast<void *>(reinterpret_cast<const SDLWindow *>(w.data.data())->w); }

Window GetWindowInFocus() { return g_window_system.get().window_in_focus; }

bool DestroyWindow(Window &w) {
	auto data = reinterpret_cast<SDLWindow *>(w.data.data());

	debug(format("DestroyWindow: %1").arg((void *)data->w));
	g_window_system.get().window_focus_signal.Emit(w, false);
	SDL_DestroyWindow(data->w);
	data->w = 0;

	return true;
}

//
bool UpdateWindow(const Window &w) {

	auto data = reinterpret_cast<const SDLWindow *>(w.data.data());

	if (SDL_GetWindowFlags(data->w) & SDL_WINDOW_INPUT_FOCUS) {
		// in focus
		if (!(g_window_system.get().window_in_focus == w)) {

			g_window_system.get().window_in_focus = w;
			g_window_system.get().window_focus_signal.Emit(w, true);
		}
	} else if (g_window_system.get().window_in_focus == w) {
		// not in focus
		g_window_system.get().window_in_focus = Window(); // blank window
		g_window_system.get().window_focus_signal.Emit(w, false);
	}
	return true;
}

//
bool GetWindowClientSize(const Window &w, int &width, int &height) {
	auto data = reinterpret_cast<const SDLWindow *>(w.data.data());
	SDL_GetWindowSize(data->w, &width, &height);
	return true;
}

bool SetWindowClientSize(const Window &w, int width, int height) {
	auto data = reinterpret_cast<const SDLWindow *>(w.data.data());
	SDL_SetWindowSize(data->w, width, height);
	return true;
}

bool GetWindowTitle(const Window &w, std::string &title) {
	auto data = reinterpret_cast<const SDLWindow *>(w.data.data());
	return true;
}

bool SetWindowTitle(const Window &w, const std::string &title) {
	auto data = reinterpret_cast<const SDLWindow *>(w.data.data());
	return true;
}

bool WindowHasFocus(const Window &w) {
	auto data = reinterpret_cast<const SDLWindow *>(w.data.data());
	return true;
}

bool SetWindowPos(const Window &w, const iVec2 &v) {
	auto data = reinterpret_cast<const SDLWindow *>(w.data.data());
	SDL_SetWindowPosition(data->w, v.x, v.y);
	UpdateWindow(w); // process messages on the spot
	return true;
}

iVec2 GetWindowPos(const Window &w) {
	auto data = reinterpret_cast<const SDLWindow *>(w.data.data());

	UpdateWindow(w);

	iVec2 pos;
	SDL_GetWindowPosition(data->w, &pos.x, &pos.y);
	return pos;
}

void ShowCursor() { SDL_ShowCursor(SDL_ENABLE); }
void HideCursor() { SDL_ShowCursor(SDL_DISABLE); }

} // namespace hg
