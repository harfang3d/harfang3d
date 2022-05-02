// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "../platform.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#if HG_USE_GTK3
#include <gtk/gtk.h>
#endif

#include "foundation/string.h"

namespace hg {

bool InitPlatform() {
	return true;
}

enum class Action : int {
	FileOpen = 0,
	FileSave,
	SelectFolder,
	Count
};

static bool FileChooserImpl(const std::string &title, const std::vector<FileFilter> &filters, std::string &output, const std::string &initial_dir, Action file_chooser_action) {
#if HG_USE_GTK3
	GtkFileChooserAction action_id;
	const char *action_str;
	
	switch(file_chooser_action) {
		case Action::FileSave:
			action_id = GTK_FILE_CHOOSER_ACTION_SAVE;
			action_str = "_Save";
			break;
		case Action::SelectFolder:
			action_id = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
			action_str = "_Open";
			break;
		//case Action::FileOpen:
		default:
			action_id = GTK_FILE_CHOOSER_ACTION_OPEN;
			action_str = "_Open";
			break;
	}

	if (!gtk_init_check(NULL, NULL))
		return false;

	GtkWidget *dialog = gtk_file_chooser_dialog_new(title.c_str(),
		NULL,
		action_id,
		"_Cancel", GTK_RESPONSE_CANCEL,
		action_str, GTK_RESPONSE_ACCEPT,
		NULL);

	if (!initial_dir.empty())
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), initial_dir.c_str());

	for(auto &it: filters) {
		GtkFileFilter *filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, it.name.c_str());
		for(auto pat : split(it.pattern, ";")) {
			gtk_file_filter_add_pattern(filter, pat.c_str());
		}
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	}

	bool res = false;

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		output = filename;
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
	return true;
#endif
}

bool OpenFileDialog(const std::string &title, const std::vector<FileFilter> &filters, std::string &output, const std::string &initial_dir) {
	return FileChooserImpl(title, filters, output, initial_dir, Action::FileOpen);
}

bool SaveFileDialog(const std::string &title, const std::vector<FileFilter> &filters, std::string &output, const std::string &initial_dir) {
	return FileChooserImpl(title, filters, output, initial_dir, Action::FileSave);
}

bool OpenFolderDialog(const std::string &title, std::string &output, const std::string &initial_dir) {
	return FileChooserImpl(title, {}, output, initial_dir, Action::SelectFolder);
}

void DebugBreak() { /* STUB */ }

} // namespace hg

