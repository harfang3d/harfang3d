// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#if !(defined(EMSCRIPTEN) || defined(HG_EMBEDDED))
#include <gtk/gtk.h>
#endif
#ifdef HG_USE_GLFW
#include "platform/glfw/input_system_glfw.h"
#endif

namespace hg {

bool InitPlatform() {
#ifdef HG_USE_GLFW
	RegisterGLFW3InputSystem();
#endif
	return true;
}

bool OpenFileDialog(const std::string &title, const std::string &filter, std::string &OUTPUT, const std::string &initial_dir) {
#if !(defined(EMSCRIPTEN) || defined(HG_EMBEDDED))
	if (!gtk_init_check(NULL, NULL))
		return false;

	GtkWidget *dialog = gtk_file_chooser_dialog_new(title.c_str(),
		NULL,
		GTK_FILE_CHOOSER_ACTION_OPEN,
		"_Cancel", GTK_RESPONSE_CANCEL,
		"_Open", GTK_RESPONSE_ACCEPT,
		NULL);

	if (!initial_dir.empty())
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), initial_dir.c_str());

	bool res = false;

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		OUTPUT = filename;
		g_free(filename);
		res = true;
	}

	while (gtk_events_pending())
		gtk_main_iteration();

    gtk_widget_destroy(dialog);

	while (gtk_events_pending())
		gtk_main_iteration();

	return res;
#else
	return false;
#endif
}

bool SaveFileDialog(const std::string &title, const std::string &filter, std::string &OUTPUT, const std::string &initial_dir) {
	return false;
}

bool OpenFolderDialog(const std::string &title, std::string &OUTPUT, const std::string &initial_dir) {
	return false;
}

void DebugBreak() { /* STUB */ }

} // namespace hg

