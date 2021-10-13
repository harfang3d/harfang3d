// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/log.h"
#include "foundation/thread.h"
#include "platform/window_system.h"
#include "gtest/gtest.h"

using namespace hg;

TEST(Platform, NewWindow) {
	auto win = NewWindow(320, 200);

	for (int i = 0; i < 100; ++i) {
		UpdateWindow(win);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	DestroyWindow(win);
}

TEST(Platform, SetWindowPos) {
	auto win = NewWindow(320, 200);

	SetWindowPos(win, iVec2(200, 200));

	for (int i = 0; i < 5; ++i) {
		UpdateWindow(win);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	auto pos = GetWindowPos(win);

#ifndef __linux__ // window decorations are handled differently on each desktop on Linux, it's a mess, give up
	EXPECT_EQ(pos.x, 200);
	EXPECT_EQ(pos.y, 200);
#endif

	DestroyWindow(win);
}
