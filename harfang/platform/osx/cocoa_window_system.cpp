// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "window_system/window_system.h"
#include "cocoa_window_system/cocoa_window_system.h"
#include "cstl/main_thread.h"

namespace gs {

struct MonitorCocoa : Monitor {
	MonitorCocoa(string _id) : Monitor() { id = _id; }
	iRect GetRect() override { return rect; }
	iRect rect;
};

static const char *window_type = "Cocoa";

struct WindowCocoa : Window
{
	WindowCocoa() : window(nullptr), view(nullptr) { type = window_type; }

	NSWindow *window;
	NSView *view;
};

#define ASSERT_WINDOW_TYPE(W, R) \
	if ((W) == nullptr || (W)->type != window_type) \
		return (R);

#define CAST_WINDOW_SAFE(W, R) \
	ASSERT_WINDOW_TYPE(W, R)   \
	auto w_cocoa = reinterpret_cast<WindowCocoa *>(W);

#define CAST_CONST_WINDOW_SAFE(W, R) \
	ASSERT_WINDOW_TYPE(W, R)         \
	auto w_cocoa = reinterpret_cast<const WindowCocoa *>(W);

//
void WindowSystemInit() { cocoaInit(); }

Window *NewWindow(int width, int height, int bpp, Window::Visibility visibility)
{
	scoped_ptr<WindowCocoa> w(new WindowCocoa);

	switch (visibility) {
		case Window::Fullscreen: {
		} break;

		case Window::Undecorated:
			break;

		default:
		case Window::Windowed:
			w->window = main_thread::queue(bind(&cocoaNewWindow, width, height, bpp, 2)).get();
			break;
	}

	w->view = cocoaGetWindowView(w->window);

	SetWindowTitle(w, "Harfang");

	new_window_signal.Emit(w);
	return w.detach();
}

Window *NewWindowFrom(void *handle)
{
	auto w = new WindowCocoa;

	w->is_foreign = true;
	w->view = reinterpret_cast<NSView *>(handle);

	return w;
}

Window::Handle GetWindowHandle(const Window *w)
{
	CAST_CONST_WINDOW_SAFE(w, nullptr);
	return w_cocoa->view;
}

bool DestroyWindow(Window *w)
{
	CAST_WINDOW_SAFE(w, false);
	if (w_cocoa->window != nullptr)
		main_thread::queue(bind(&cocoaDestroyWindow, w_cocoa->window)).wait();
	return true;
}

void DeleteWindow(Window *w) { delete w; }

bool UpdateWindow(Window *w)
{
	CAST_CONST_WINDOW_SAFE(w, false);
	cocoaUpdateWindow(w_cocoa->window);
	return true;
}

//
bool GetWindowClientSize(const Window *w, int &width, int &height)
{
	CAST_CONST_WINDOW_SAFE(w, false);
	cocoaGetViewSize(w_cocoa->view, &width, &height);
	return true;
}

//
bool SetWindowClientSize(const Window *w, int width, int height)
{
	CAST_CONST_WINDOW_SAFE(w, false);
	cocoaSetViewSize(w_cocoa->window, w_cocoa->view, width, height);
	return true;
}

bool GetWindowTitle(const Window *w, string &title)
{
	CAST_CONST_WINDOW_SAFE(w, false);
	return false;
}

bool SetWindowTitle(Window *w, const string &title)
{
	CAST_CONST_WINDOW_SAFE(w, false);
	cocoaSetWindowTitle(w_cocoa->window, title.to_utf8());
	return true;
}

void GetMonitors(shared_vector<Monitor *> &monitor_array)
{
	int count = 0;
	int *rects = cocoaGetMonitors(&count);
	for (int i = 0; i < count; i += 4)
	{
		shared_ptr<MonitorCocoa> m(new MonitorCocoa(""));
		m->rect.Set(rects[i], rects[i + 1], rects[i + 2], rects[i + 3]);
		monitor_array.push_back(m);
	}
	cocoaDeleteArrayMonitors(rects);
}

iVector2 GetWindowPos(Window *w)
{
	CAST_CONST_WINDOW_SAFE(w, iVector2());
	iVector2 pos;
	cocoaGetWindowPos(w_cocoa->window, &pos.x, &pos.y);
	return pos;
}

bool SetWindowPos(Window *w, const iVector2 v)
{
	CAST_WINDOW_SAFE(w, false);
	cocoaSetWindowPos(w_cocoa->window, v.x, v.y);
	return true;
}

bool WindowHasFocus(const Window *w)
{
	CAST_CONST_WINDOW_SAFE(w, false);
	return CocoaWindowHasFocus(w_cocoa->window);
}

// TODO
void ShowCursor() {}
void HideCursor() {}

} // hg
