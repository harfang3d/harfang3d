// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/cext.h"
#include "platform/window_system.h"

namespace hg {

static std::vector<const Window*> open_window_list;
static Signal<void(const Window *)>::Connection new_window_connection, destroy_window_connection;

void ConnectWindowSystemSignals() {
	new_window_connection = new_window_signal.Connect([](const Window *w) { push_back_unique(open_window_list, w); });
	destroy_window_connection = destroy_window_signal.Connect([](const Window *w) { find_erase(open_window_list, w); });
}

void DisconnectWindowSystemSignals() {
	new_window_signal.Disconnect(new_window_connection);
	destroy_window_signal.Disconnect(destroy_window_connection);
}

bool IsWindowOpen(const Window *window) {
	for (auto &w : open_window_list)
		if (w == window)
			return true;
	return false;
}

} // namespace hg
