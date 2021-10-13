// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/assert.h"
#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/path_tools.h"
#include "foundation/string.h"
#include "platform/input_system.h"
#include "platform/win32/assert.h"
#include "platform/win32/platform.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shobjidl.h>

namespace hg {

bool InitPlatform() {
	trigger_assert = &win32_trigger_assert;
	return true;
}

std::string GetPlatformLocale() {
	char16_t locale_name[LOCALE_NAME_MAX_LENGTH];
	if (!GetSystemDefaultLocaleName((LPWSTR)locale_name, LOCALE_NAME_MAX_LENGTH)) {
		warn("Failed to retrieve system locale");
		locale_name[0] = 0;
	}
	return utf16_to_utf8(std::u16string(locale_name));
}

bool OpenFolderDialog(const std::string &title, std::string &OUTPUT, const std::string &initial_dir) {
	IFileOpenDialog *pFileOpen;

	// create the FileOpenDialog object
	CoInitialize(NULL);
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void **>(&pFileOpen));

	if (SUCCEEDED(hr)) {
		// set title
		if (!title.empty()) {
			const auto wtitle = utf8_to_utf16(title);
			pFileOpen->SetTitle((LPCWSTR)wtitle.c_str());
		}

		// set base folder
		if (!initial_dir.empty()) {
			auto win_path = initial_dir;
			replace_all(win_path, "/", "\\");
			const auto winitial_dir = utf8_to_utf16(win_path);

			IShellItem *psiFolder;
			const auto hr = SHCreateItemFromParsingName((LPCWSTR)winitial_dir.c_str(), NULL, IID_PPV_ARGS(&psiFolder));

			if (SUCCEEDED(hr)) {
				pFileOpen->SetFolder(psiFolder);
				psiFolder->Release();
			}
		}

		// only show folder
		DWORD dwOptions;
		hr = pFileOpen->GetOptions(&dwOptions);
		if (SUCCEEDED(hr)) {
			pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS);

			// show the Open dialog box.
			hr = pFileOpen->Show(NULL);

			// get the file name from the dialog box
			if (SUCCEEDED(hr)) {
				IShellItem *pItem;
				hr = pFileOpen->GetResult(&pItem);
				if (SUCCEEDED(hr)) {
					PWSTR pszFilePath;
					hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

					// display the file name to the user
					if (SUCCEEDED(hr))
						OUTPUT = CleanPath(utf16_to_utf8(std::u16string((char16_t *)pszFilePath)));
					pItem->Release();
				}
			}
			pFileOpen->Release();
		}
	}
	return SUCCEEDED(hr);
}

bool OpenFileDialog(const std::string &title, const std::string &filter, std::string &OUTPUT, const std::string &initial_dir) {
	IFileOpenDialog *pFileOpen;

	// create the FileOpenDialog object
	CoInitialize(NULL);
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void **>(&pFileOpen));

	if (SUCCEEDED(hr)) {
		// set title
		if (!title.empty()) {
			const auto wtitle = utf8_to_utf16(title);
			pFileOpen->SetTitle((LPCWSTR)wtitle.c_str());
		}

		// set base folder
		if (!initial_dir.empty()) {
			auto win_path = initial_dir;
			replace_all(win_path, "/", "\\");
			const auto winitial_dir = utf8_to_utf16(win_path);

			IShellItem *psiFolder;
			hr = SHCreateItemFromParsingName((LPCWSTR)winitial_dir.c_str(), NULL, IID_PPV_ARGS(&psiFolder));

			if (SUCCEEDED(hr)) {
				hr = pFileOpen->SetDefaultFolder(psiFolder);
				psiFolder->Release();
			} else {
				char msg[512];
				FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)msg, 511, NULL);
				warn(format("Could not set base folder: %1").arg(msg).c_str());
			}
		}

		// add filters
		auto wfilter = utf8_to_utf16(filter);
		COMDLG_FILTERSPEC filters[] = {{L"", (LPCWSTR)wfilter.c_str()}};
		pFileOpen->SetFileTypes(1, filters);

		// show the open dialog box
		hr = pFileOpen->Show(NULL);

		// get the file name from the dialog box
		if (SUCCEEDED(hr)) {
			IShellItem *pItem;
			hr = pFileOpen->GetResult(&pItem);
			if (SUCCEEDED(hr)) {
				PWSTR pszFilePath;
				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

				// display the file name to the user
				if (SUCCEEDED(hr))
					OUTPUT = CleanPath(utf16_to_utf8(std::u16string((char16_t *)pszFilePath)));
				pItem->Release();
			}
		}
		pFileOpen->Release();
	}
	return SUCCEEDED(hr);
}

bool SaveFileDialog(const std::string &title, const std::string &filter, std::string &OUTPUT, const std::string &initial_dir) {
	IFileSaveDialog *pFileSave;

	// create the FilSaveDialog object
	CoInitialize(NULL);
	HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void **>(&pFileSave));

	if (SUCCEEDED(hr)) {

		// set title
		if (!title.empty()) {
			const auto wtitle = utf8_to_utf16(title);
			pFileSave->SetTitle((LPCWSTR)wtitle.c_str());
		}

		// set file name
		if (!OUTPUT.empty()) {
			const auto wfile = utf8_to_utf16(OUTPUT);
			pFileSave->SetFileName((LPCWSTR)wfile.c_str());
		}

		// set base folder
		if (!initial_dir.empty()) {
			auto win_path = initial_dir;
			replace_all(win_path, "/", "\\");
			const auto winitial_dir = utf8_to_utf16(win_path);

			IShellItem *psiFolder;
			hr = SHCreateItemFromParsingName((LPCWSTR)winitial_dir.c_str(), NULL, IID_PPV_ARGS(&psiFolder));

			if (SUCCEEDED(hr)) {
				hr = pFileSave->SetFolder(psiFolder);
				psiFolder->Release();
			} else {
				debug(format("SHCreateItemFromParsingName failed in SaveFileDialog: %1").arg(GetLastError_Win32()));
			}
		}

		// add filters
		auto wfilter = utf8_to_utf16(filter);
		COMDLG_FILTERSPEC filters[] = {{L"", (LPCWSTR)wfilter.c_str()}};
		pFileSave->SetFileTypes(1, filters);
		pFileSave->SetDefaultExtension((LPCWSTR)wfilter.c_str());

		// show the Save dialog box
		hr = pFileSave->Show(NULL);

		// get the file name from the dialog box
		if (SUCCEEDED(hr)) {
			IShellItem *pItem;
			hr = pFileSave->GetResult(&pItem);
			if (SUCCEEDED(hr)) {
				PWSTR pszFilePath;
				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

				// display the file name to the user
				if (SUCCEEDED(hr))
					OUTPUT = CleanPath(utf16_to_utf8(std::u16string((char16_t *)pszFilePath)));
				pItem->Release();
			}
		}
		pFileSave->Release();
	}
	return SUCCEEDED(hr);
}

void DebugBreak() { ::DebugBreak(); }

//
std::string GetLastError_Win32() {
	std::string err;
	LPWSTR err_win32 = NULL;

	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&err_win32, 0, NULL);

	if (err_win32) {
		err = utf16_to_utf8(std::u16string(reinterpret_cast<const char16_t *>(err_win32)));
		LocalFree(err_win32);
	}

	return err;
}

} // namespace hg
