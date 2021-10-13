// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

extern "C" {

struct NSWindow;
struct NSView;

/// Initialize Cocoa.
void cocoaInit();

/// Create a new cocoa window.
NSWindow *cocoaNewWindow(int width, int height, int bpp, int visibility);
/// Destroy a cocoa window.
int cocoaDestroyWindow(NSWindow *);

/// Update a cocoa Window.
void cocoaUpdateWindow(NSWindow *);

/// Set a window title string.
void cocoaSetWindowTitle(NSWindow *window, const char *title);

/// Return the NSView from a NSWindow.
NSView *cocoaGetWindowView(NSWindow *);

/// Return the size of a NSView.
void cocoaGetViewSize(NSView *view, int *width, int *height);
/// Set the size of a NSView.
void cocoaSetViewSize(NSWindow *window, NSView *view, int width, int height);

/// Return the monitors rect.
int *cocoaGetMonitors(int *count);
void cocoaDeleteArrayMonitors(int *array);

/// Get and Set the position of a NSView.
void cocoaGetWindowPos(NSWindow *window, int *x, int *y);
void cocoaSetWindowPos(NSWindow *window, int x, int y);

/// Return whether the given window has keyboard focus.
bool CocoaWindowHasFocus(NSWindow *);

extern char cocoa_keyboard[256];
extern int cocoa_keyboard_flags;

};
