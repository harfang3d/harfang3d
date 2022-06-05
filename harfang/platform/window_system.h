// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/rect.h"
#include "foundation/signal.h"
#include "foundation/string.h"

namespace hg {

/// @defgroup WindowSystemGroup Window System
/// @{

/// @defgroup MonitorGroup Monitor
/// @{

/// Monitor
struct Monitor;

/// Screen rotation (in degree) bitmasks
enum MonitorRotation : uint8_t {
	MR_0 = 1, ///< 0°  (landscape)
	MR_90 = 2, ///< 90° (portrait)
	MR_180 = 4, ///< 180° (landscape upside down)
	MR_270 = 8 ///< 270° (portrait upside down)
};

/// Monitor mode
struct MonitorMode {
	std::string name; ///< Formatted name "widthxheight frequencyHz"
	iRect rect; ///< Screen resolution in pixels.
	int frequency; ///< Frequency in Hz.
	MonitorRotation rotation; ///< Screen orientation.
	uint8_t supported_rotations; ///< List of supported screen rotations as a bit mask.
};

/// Return the monitor rect.
iRect GetMonitorRect(const Monitor *monitor);
/// Return true if the monitor is set as primary.
bool IsPrimaryMonitor(const Monitor *monitor);
/// Return true if the monitor is connected.
bool IsMonitorConnected(const Monitor *monitor);
/// Return monitor name.
std::string GetMonitorName(const Monitor *monitor);
/// Return monitor size in millimeters.
iVec2 GetMonitorSizeMM(const Monitor *monitor);
/// Get the list of screen modes for a given monitor.
bool GetMonitorModes(const Monitor *monitor, std::vector<MonitorMode> &modes);
/// Get a list of monitors connected to the system.
std::vector<Monitor *> GetMonitors();

/// @}

/// @defgroup WindowGroup Window
/// @{

/// Window visibility
enum WindowVisibility { WV_Windowed, WV_Undecorated, WV_Fullscreen, WV_Hidden, WV_FullscreenMonitor1, WV_FullscreenMonitor2, WV_FullscreenMonitor3 };

/// Window
struct Window;

/// Must be called from the main thread.
void WindowSystemInit();
/// Shutdown the window system.
void WindowSystemShutdown();

/// Create a new window.
Window *NewWindow(int width, int height, int bpp = 32, WindowVisibility visibility = WV_Windowed);
Window *NewWindow(const char *title, int width, int height, int bpp = 32, WindowVisibility visibility = WV_Windowed);
/// Create a new fullscreen window on a specified monitor.
Window *NewFullscreenWindow(const Monitor *monitor, int mode_index, MonitorRotation rotation = MR_0);
Window *NewFullscreenWindow(const char *title, const Monitor *monitor, int mode_index, MonitorRotation rotation = MR_0);
/// Wrap a foreign window native handle in a new window.
Window *NewWindowFrom(void *handle);

void *GetDisplay();
/// Return the window native handle.
void *GetWindowHandle(const Window *window);

/// Pool window events.
bool UpdateWindow(const Window *window);

/// Destroy a window.
bool DestroyWindow(Window *window);

/// Get the window client size.
bool GetWindowClientSize(const Window *window, int &width, int &height);
/// Set the window client size.
bool SetWindowClientSize(Window *window, int width, int height);

/// Get the window content scale (1.0 for 96 dpi display).
Vec2 GetWindowContentScale(const Window *window);

/// Get the window title as an UTF-8 string.
bool GetWindowTitle(const Window *window, std::string &title);
/// Set the window title from an UTF-8 string.
bool SetWindowTitle(Window *window, const std::string &title);

// Show window
void ShowWindow(Window* w);
// Hide window
void HideWindow(Window* w);

/// Return true if the provided window has input focus.
bool WindowHasFocus(const Window *window);

/// Return the system handle to the window currently in focus.
Window *GetWindowInFocus();

/// Get the window position
iVec2 GetWindowPos(const Window *window);
/// Set the window position
bool SetWindowPos(Window *window, const iVec2 &position);
/// Center a window on the primary monitor.
void CenterWindow(Window *window);

/// Return true if the window is open.
bool IsWindowOpen(const Window *window);

extern Signal<void(const Window *)> new_window_signal;
extern Signal<void(const Window *, bool)> window_focus_signal;
extern Signal<bool(const Window *)> close_window_signal;
extern Signal<void(const Window *)> destroy_window_signal;
/// @}

void ConnectWindowSystemSignals();
void DisconnectWindowSystemSignals();

/// Show mouse cursor
void ShowCursor();
/// Hide mouse cursor
void HideCursor();

/// Enable drop support on a specific window
void SetWindowDropCallback(const Window *window, void (*cb)(const Window *window, int count, const char **paths));
/// Called when a window needs to be refreshed.
void SetWindowRefreshCallback(const Window *window, void (*cb)(const Window *window));

/// @}

struct Icon {
	int width, height;
	unsigned char *pixels;
};

void SetWindowIcon(const Window *w, int count, const Icon *icons);

} // namespace hg
