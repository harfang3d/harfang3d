// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/assert.h"
#include "foundation/cext.h"
#include "foundation/log.h"
#include "platform/window_system.h"
#include "platform/android/window_system.h"
#include <android/native_activity.h>

namespace gs {

std::vector<Monitor> GetMonitors() { return std::vector<Monitor>(); }

iRect GetMonitorRect(const Monitor &) { return iRect(0, 0, 0, 0); }
bool IsPrimaryMonitor(const Monitor &) { return true; }

void WindowSystemInit() {}

Window NewWindow(int width, int height, int bpp, Window::Visibility visibility) { return Window(); }
Window NewWindowFrom(void *handle) { return Window(); }

void *GetWindowHandle(const Window &w) { return g_window_system.get().main_window_handle; }

bool UpdateWindow(const Window &) { return true; }
bool DestroyWindow(Window &) { return true; }

//
bool GetWindowClientSize(const Window &, int &width, int &height) {
	width = ANativeWindow_getWidth((ANativeWindow *)g_window_system.get().main_window_handle);
	height = ANativeWindow_getHeight((ANativeWindow *)g_window_system.get().main_window_handle);
	return true;
}

//
bool SetWindowClientSize(const Window &, int width, int height) {
	warn("SetWindowClientSize not supported on Android");
	return false;
}

bool GetWindowTitle(const Window &, std::string &) { return false; }
bool SetWindowTitle(const Window &, const std::string &) { return false; }

bool WindowHasFocus(const Window &) { return true; }

iVector2 GetWindowPos(const Window &) { return iVector2(0, 0); }

bool SetWindowPos(const Window &, const iVector2) {
	warn("SetWindowPos not supported on Android");
	return false;
}

} // gs
