// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/cext.h"
#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/string.h"

#include "platform/glfw/window_system.h"
#include "platform/window_system.h"

#include <cstdint>

#include <map>

#if defined(GLFW_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_GET_NATIVE_WINDOW_HANDLE glfwGetWin32Window
#elif defined(GLFW_COCOA)
#define GLFW_EXPOSE_NATIVE_COCOA
#define GLFW_GET_NATIVE_WINDOW_HANDLE glfwGetCocoaWindow
#elif defined(GLFW_WAYLAND)
#define GLFW_EXPOSE_NATIVE_WAYLAND
#define GLFW_GET_NATIVE_WINDOW_HANDLE glfwGetWaylandWindow
#elif defined(GLFW_X11)
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_GET_NATIVE_WINDOW_HANDLE glfwGetX11Window
#else
#error Unsupported platform!
#endif

#include <GLFW/glfw3native.h>

#if defined(GLFW_WAYLAND)
#include <wayland-egl.h>
#endif // GLFW_WAYLAND

namespace hg {

static const char *g_default_window_title = "Harfang";

static Window *window_in_focus;

Signal<void(const Window *)> new_window_signal;
Signal<void(const Window *, bool)> window_focus_signal;
Signal<bool(const Window *)> close_window_signal;
Signal<void(const Window *)> destroy_window_signal;

#if ANDROID
void *display = nullptr;
void *main_window_handle = nullptr;
#endif

struct GLFWWindowData {
	char *title;
#if GLFW_WAYLAND
	struct wl_egl_window *egl_win;
#endif // GLFW_WAYLAND
};

static void ErrorCallback(int error_code, const char *description) { error(description); }

static bool glfw_initialized = false;

void WindowSystemInit() {
	glfwSetErrorCallback(ErrorCallback);
	int ret = glfwInit();
	__RASSERT_MSG__(ret == GLFW_TRUE, "Failed to initialize window system");
	ConnectWindowSystemSignals();
	glfw_initialized = true;
}

void WindowSystemShutdown() {
	glfwTerminate();
	DisconnectWindowSystemSignals();
	glfw_initialized = false;
}

//-- Monitor
struct Monitor {
	uintptr_t unused;
};

std::vector<Monitor *> GetMonitors() {
	int count;
	std::vector<Monitor *> out;
	GLFWmonitor **monitor_list = glfwGetMonitors(&count);
	if (count && monitor_list) {
		out.resize(count);
		for (int i = 0; i < count; i++) {
			out[i] = reinterpret_cast<Monitor *>(monitor_list[i]);
		}
	}
	return out;
}

iRect GetMonitorRect(const Monitor *monitor) {
	iRect rect{};

	if (monitor) {
		int x, y;
		glfwGetMonitorPos((GLFWmonitor *)monitor, &x, &y);
		const GLFWvidmode *vidmode = glfwGetVideoMode((GLFWmonitor *)monitor);
		if (vidmode) {
			rect = MakeRectFromWidthHeight(x, y, vidmode->width, vidmode->height);
		}
	}
	return rect;
}

bool IsPrimaryMonitor(const Monitor *monitor) { return ((GLFWmonitor *)monitor == glfwGetPrimaryMonitor()); }

bool IsMonitorConnected(const Monitor *monitor) {
	int count;
	GLFWmonitor **monitors = glfwGetMonitors(&count);
	if (count && monitors) {
		for (int i = 0; i < count; i++) {
			if (monitors[i] == (GLFWmonitor *)monitor) {
				return true;
			}
		}
	}
	return false;
}

std::string GetMonitorName(const Monitor *monitor) { return std::string(glfwGetMonitorName((GLFWmonitor *)monitor)); }

iVec2 GetMonitorSizeMM(const Monitor *monitor) {
	iVec2 size;
	glfwGetMonitorPhysicalSize((GLFWmonitor *)monitor, &size.x, &size.y);
	return size;
}

bool GetMonitorModes(const Monitor *monitor, std::vector<MonitorMode> &out) {
	int count;
	const GLFWvidmode *vidmodes = glfwGetVideoModes((GLFWmonitor *)monitor, &count);
	if (!count || !vidmodes) {
		return false;
	}
	out.resize(count);
	for (int i = 0; i < count; i++) {
		out[i].name = format("%1x%2 %3Hz").arg(vidmodes[i].width).arg(vidmodes[i].height).arg(vidmodes[i].refreshRate).str();
		out[i].rect = MakeRectFromWidthHeight(0, 0, vidmodes[i].width, vidmodes[i].height);
		out[i].frequency = vidmodes[i].refreshRate;
		out[i].rotation = MR_0;
		out[i].supported_rotations = MR_0;
	}
	return true;
}

//-- Window
static std::map<const Window *, void (*)(const Window *w, int count, const char **paths)> window_drop_cb;
static std::map<const Window *, void (*)(const Window *w)> window_refresh_cb;

struct Window {
	uintptr_t unused;
};

void *GetDisplay() {
#if GLFW_WAYLAND
	return glfwGetWaylandDisplay();
#elif GLFW_LINUX
	return glfwGetX11Display();
#else
	return nullptr;
#endif
}

void *GetWindowHandle(const Window *w) {
	if (!w) {
		return nullptr;
	}
#if GLFW_WAYLAND
	GLFWwindow *glfw_win = (GLFWwindow *)w;
	GLFWWindowData *data = reinterpret_cast<GLFWWindowData *>(glfwGetWindowUserPointer(glfw_win));
	if (!data) {
		return nullptr;
	}
	if (!data->egl_win) {
		int width, height;
		glfwGetWindowSize(glfw_win, &width, &height);
		struct wl_surface *surface = (struct wl_surface *)glfwGetWaylandWindow(glfw_win);
		if (!surface) {
			return nullptr;
		}
		data->egl_win = wl_egl_window_create(surface, width, height);
	}
	return data->egl_win;
#else
	return (void *)GLFW_GET_NATIVE_WINDOW_HANDLE((GLFWwindow *)w);
#endif // GLFW_WAYLAND
}

static void WindowFocusCallback(GLFWwindow *window, int focused) {
	if (focused == GLFW_TRUE) {
		window_in_focus = (Window *)window;
		window_focus_signal.Emit((Window *)window, true);
	} else {
		if (window_in_focus == (Window *)window)
			window_in_focus = nullptr;
		window_focus_signal.Emit((Window *)window, false);
	}
}

static void WindowCloseCallback(GLFWwindow *window) { destroy_window_signal.Emit((Window *)window); }

static Window *NewGLFWWindow(int width, int height, int bpp, const GLFWmonitor *monitor) {
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
	//	glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
#if (GLFW_VERSION_MAJOR >= 3) && (GLFW_VERSION_MINOR >= 3)
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
#endif
	GLFWwindow *window = glfwCreateWindow(width, height, g_default_window_title, const_cast<GLFWmonitor *>(monitor), NULL);

	if (window) {
		GLFWWindowData *data = new GLFWWindowData;
		data->title = strdup(g_default_window_title);
#if GLFW_WAYLAND
		struct wl_surface *surface = (struct wl_surface *)glfwGetWaylandWindow(window);
		if (surface) {
			data->egl_win = wl_egl_window_create(surface, width, height);
		}
#endif // GLFW_WAYLAND
		glfwSetWindowUserPointer(window, data);
		glfwSetWindowFocusCallback(window, WindowFocusCallback);
		glfwSetWindowCloseCallback(window, WindowCloseCallback);
		new_window_signal.Emit((Window *)window);
		// force focus callback if the window is visible
		if (glfwGetWindowAttrib(window, GLFW_VISIBLE)) {
			WindowFocusCallback(window, glfwGetWindowAttrib(window, GLFW_FOCUSED));
		}
	}

	new_window_signal.Emit(reinterpret_cast<Window *>(window));
	return (Window *)window;
}

static void CheckWindowSystemInitWasCalled() {
	__ASSERT_MSG__(glfw_initialized, "WindowSystemInit() was not called!");

	if (!glfw_initialized)
		error("WindowSystemInit() was not called!");
}

//
Window *NewWindow(int width, int height, int bpp, WindowVisibility visibility) {
	CheckWindowSystemInitWasCalled();
	glfwDefaultWindowHints();

	GLFWmonitor *monitor = NULL;
	if (visibility == WV_Fullscreen) {
		monitor = glfwGetPrimaryMonitor();
	} else if (visibility >= WV_FullscreenMonitor1) {
		int monitor_index = static_cast<int>(visibility - WV_FullscreenMonitor1);
		int count;
		GLFWmonitor **glfw_monitors = glfwGetMonitors(&count);
		if ((monitor_index >= count) || !glfw_monitors) {
			return nullptr;
		}
		monitor = glfw_monitors[monitor_index];
	} else if (visibility == WV_Undecorated) {
		glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
	} else if (visibility == WV_Hidden) {
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	}

	return NewGLFWWindow(width, height, bpp, monitor);
}

Window *NewWindow(const char *title, int width, int height, int bpp, WindowVisibility visibility) {
	auto win = NewWindow(width, height, bpp, visibility);
	SetWindowTitle(win, title);
	return win;
}

Window *NewFullscreenWindow(const Monitor *monitor, int mode_index, MonitorRotation rotation) {
	CheckWindowSystemInitWasCalled();
	glfwDefaultWindowHints();

	if (!monitor) {
		return nullptr;
	}

	int count;
	const GLFWvidmode *vidmode = glfwGetVideoModes((GLFWmonitor *)monitor, &count);
	if (mode_index >= count) {
		return nullptr;
	}

	return NewGLFWWindow(vidmode[mode_index].width, vidmode[mode_index].height, 32, (GLFWmonitor *)monitor);
}

bool UpdateWindow(const Window *w) {
	glfwPollEvents();
	return true;
}

Window *NewFullscreenWindow(const char *title, const Monitor *monitor, int mode_index, MonitorRotation rotation) {
	auto win = NewFullscreenWindow(monitor, mode_index, rotation);
	SetWindowTitle(win, title);
	return win;
}

Window *NewWindowFrom(void *handle) {
	CheckWindowSystemInitWasCalled();
	__RASSERT_MSG__(false, "Unimplemented");
	return nullptr;
}

bool DestroyWindow(Window *w) {
	if (w) {
		destroy_window_signal.Emit(w);

		window_drop_cb.erase(w);

		GLFWWindowData *data = reinterpret_cast<GLFWWindowData *>(glfwGetWindowUserPointer((GLFWwindow *)w));
		if (data) {
			if (data->title) {
				free(data->title);
			}
#if GLFW_WAYLAND
			if (data->egl_win) {
				wl_egl_window_destroy(data->egl_win);
			}
#endif // GLFW_WAYLAND
			free(data);
		}

		glfwDestroyWindow((GLFWwindow *)w);
	}
	return true;
}

//
bool GetWindowClientSize(const Window *w, int &width, int &height) {
	if (!w) {
		return false;
	}
	glfwGetWindowSize((GLFWwindow *)w, &width, &height);
	return true;
}

bool SetWindowClientSize(Window *w, int width, int height) {
	if (!w)
		return false;

	glfwSetWindowSize((GLFWwindow *)w, width, height);
#if GLFW_WAYLAND
	GLFWWindowData *data = reinterpret_cast<GLFWWindowData *>(glfwGetWindowUserPointer((GLFWwindow *)w));
	if (data) {
		struct wl_surface *surface = (struct wl_surface *)glfwGetWaylandWindow((GLFWwindow *)w);
		if (!surface) {
			return false;
		}
		if (data->egl_win) {
			wl_egl_window_destroy(data->egl_win);
		}
		data->egl_win = wl_egl_window_create(surface, width, height);
	}
#endif // GLFW_WAYLAND
	return true;
}

Vec2 GetWindowContentScale(const Window *w) {
#if (GLFW_VERSION_MAJOR >= 3) && (GLFW_VERSION_MINOR >= 3)
	Vec2 v;
	glfwGetWindowContentScale((GLFWwindow *)w, &v.x, &v.y);
	return v;
#else
	return Vec2(1.f, 1.f);
#endif
}

bool GetWindowTitle(const Window *w, std::string &title) {
	// we have to store the window tile as user pointer as long as glfw doesn't provide a way to retrieve it...
	if (!w)
		return false;

	GLFWWindowData *data = reinterpret_cast<GLFWWindowData *>(glfwGetWindowUserPointer((GLFWwindow *)w));
	if (data) {
		title = data->title;
	}
	return true;
}

bool SetWindowTitle(Window *w, const std::string &title) {
	if (!w)
		return false;
	glfwSetWindowTitle((GLFWwindow *)w, title.c_str());

	// see GetWindowTitle...
	GLFWWindowData *data = reinterpret_cast<GLFWWindowData *>(glfwGetWindowUserPointer((GLFWwindow *)w));
	if (data && data->title) {
		free(data->title);
		data->title = strdup(title.c_str());
	}
	return true;
}

void ShowWindow(Window *w) { glfwShowWindow((GLFWwindow *)w); }
void HideWindow(Window *w) { glfwHideWindow((GLFWwindow *)w); }

bool WindowHasFocus(const Window *w) { return glfwGetWindowAttrib((GLFWwindow *)w, GLFW_FOCUSED) == GLFW_TRUE; }

Window *GetWindowInFocus() { return window_in_focus; }

iVec2 GetWindowPos(const Window *w) {
	iVec2 v;
	glfwGetWindowPos((GLFWwindow *)w, &v.x, &v.y);
	return v;
}

bool SetWindowPos(Window *w, const iVec2 &v) {
	glfwSetWindowPos((GLFWwindow *)w, v.x, v.y);
	return true;
}

void ShowCursor() {
	if (window_in_focus)
		glfwSetInputMode((GLFWwindow *)window_in_focus, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void HideCursor() {
	if (window_in_focus)
		glfwSetInputMode((GLFWwindow *)window_in_focus, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

GLFWwindow *GetGLFWWindow(const Window *w) { return (GLFWwindow *)w; }

static void GLFW_window_drop_cb_proxy(GLFWwindow *w, int count, const char **paths) {
	const auto i = window_drop_cb.find((Window *)w);
	if (i != std::end(window_drop_cb))
		i->second((Window *)w, count, paths);
}

void SetWindowDropCallback(const Window *w, void (*cb)(const Window *w, int count, const char **paths)) {
	window_drop_cb[w] = cb;
	if (auto glfw_w = GetGLFWWindow(w))
		glfwSetDropCallback(glfw_w, GLFW_window_drop_cb_proxy);
}

static void GLFW_window_refresh_cb_proxy(GLFWwindow *w) {
	const auto i = window_refresh_cb.find((Window *)w);
	if (i != std::end(window_refresh_cb))
		i->second((Window *)w);
}

void SetWindowRefreshCallback(const Window *w, void (*cb)(const Window *window)) {
	window_refresh_cb[w] = cb;
	if (auto glfw_w = GetGLFWWindow(w))
		glfwSetWindowRefreshCallback(glfw_w, GLFW_window_refresh_cb_proxy);
}

//
void SetWindowIcon(const Window *w, int count, const Icon *icons) {
	if (auto glfw_w = GetGLFWWindow(w)) {
		std::vector<GLFWimage> images(count);

		for (int i = 0; i < count; ++i)
			images[i] = {icons[i].width, icons[i].height, icons[i].pixels};

		glfwSetWindowIcon(glfw_w, count, images.data());
	}
}

void CenterWindow(Window *w) {
	if (auto glfw_w = GetGLFWWindow(w))
		if (const auto monitor = glfwGetPrimaryMonitor()) {
			int xpos, ypos;
			glfwGetMonitorPos(monitor, &xpos, &ypos);

			int w, h;
			glfwGetWindowSize(glfw_w, &w, &h);

			if (const auto mode = glfwGetVideoMode(monitor))
				glfwSetWindowPos(glfw_w, (mode->width - w) / 2 + xpos, (mode->height - h) / 2 + ypos);
		}
}

} // namespace hg
