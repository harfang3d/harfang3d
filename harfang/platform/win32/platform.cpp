// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "platform/platform.h"
#include "platform/win32/platform.h"
#include "foundation/assert.h"
#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/path_tools.h"
#include "foundation/string.h"
#include "platform/input_system.h"
#include "platform/win32/assert.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shobjidl.h>

namespace hg {

bool InitPlatform() {
	trigger_assert = &win32_trigger_assert;
	return true;
}

std::string GetPlatformLocale() {
	wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];
	if (!GetSystemDefaultLocaleName((LPWSTR)locale_name, LOCALE_NAME_MAX_LENGTH)) {
		warn("Failed to retrieve system locale");
		locale_name[0] = 0;
	}
	return wchar_to_utf8(locale_name);
}

bool OpenFolderDialog(const std::string &title, std::string &output, const std::string &initial_dir) {
	IFileOpenDialog *pFileOpen;

	// create the FileOpenDialog object
	CoInitialize(NULL);
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void **>(&pFileOpen));

	if (SUCCEEDED(hr)) {
		// set title
		if (!title.empty())
			pFileOpen->SetTitle(utf8_to_wchar(title).c_str());

		// set base folder
		if (!initial_dir.empty()) {
			auto win_path = initial_dir;
			replace_all(win_path, "/", "\\");

			IShellItem *psiFolder;
			const auto hr = SHCreateItemFromParsingName(utf8_to_wchar(win_path).c_str(), NULL, IID_PPV_ARGS(&psiFolder));

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
						output = CleanPath(wchar_to_utf8(pszFilePath));
					pItem->Release();
				}
			}
			pFileOpen->Release();
		}
	}
	return SUCCEEDED(hr);
}

bool OpenFileDialog(const std::string &title, const std::vector<hg::FileFilter> &filters, std::string &output, const std::string &initial_dir) {
	IFileOpenDialog *pFileOpen;

	// create the FileOpenDialog object
	CoInitialize(NULL);
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void **>(&pFileOpen));

	if (SUCCEEDED(hr)) {
		// set title
		if (!title.empty())
			pFileOpen->SetTitle(utf8_to_wchar(title).c_str());

		// set base folder
		if (!initial_dir.empty()) {
			auto win_path = initial_dir;
			replace_all(win_path, "/", "\\");

			IShellItem *psiFolder;
			hr = SHCreateItemFromParsingName(utf8_to_wchar(win_path).c_str(), NULL, IID_PPV_ARGS(&psiFolder));

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
		std::vector<COMDLG_FILTERSPEC> w32_filters;
		std::vector<std::wstring> wstring_buffer;
		if (!filters.empty()) {
			w32_filters.resize(filters.size());
			wstring_buffer.reserve(2 * filters.size());
			for (size_t i = 0; i < filters.size(); i++) {
				wstring_buffer.emplace_back(utf8_to_wchar(filters[i].name));
				w32_filters[i].pszName = wstring_buffer.back().c_str();

				wstring_buffer.emplace_back(utf8_to_wchar(filters[i].pattern));
				w32_filters[i].pszSpec = wstring_buffer.back().c_str();
			}
			pFileOpen->SetFileTypes(static_cast<UINT>(w32_filters.size()), w32_filters.data());
		}

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
					output = CleanPath(wchar_to_utf8(pszFilePath));
				pItem->Release();
			}
		}
		pFileOpen->Release();
	}
	return SUCCEEDED(hr);
}

bool SaveFileDialog(const std::string &title, const std::vector<hg::FileFilter> &filters, std::string &output, const std::string &initial_dir) {
	IFileSaveDialog *pFileSave;

	// create the FilSaveDialog object
	CoInitialize(NULL);
	HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void **>(&pFileSave));

	if (SUCCEEDED(hr)) {

		// set title
		if (!title.empty())
			pFileSave->SetTitle(utf8_to_wchar(title).c_str());

		// set file name
		if (!output.empty())
			pFileSave->SetFileName(utf8_to_wchar(output).c_str());

		// set base folder
		if (!initial_dir.empty()) {
			auto win_path = initial_dir;
			replace_all(win_path, "/", "\\");

			IShellItem *psiFolder;
			hr = SHCreateItemFromParsingName(utf8_to_wchar(win_path).c_str(), NULL, IID_PPV_ARGS(&psiFolder));

			if (SUCCEEDED(hr)) {
				hr = pFileSave->SetFolder(psiFolder);
				psiFolder->Release();
			} else {
				warn(format("SHCreateItemFromParsingName failed in SaveFileDialog: %1").arg(GetLastError_Win32()).c_str());
			}
		}

		// add filters
		std::vector<COMDLG_FILTERSPEC> w32_filters;
		std::vector<std::wstring> wstring_buffer;
		if (!filters.empty()) {
			w32_filters.resize(filters.size());
			wstring_buffer.reserve(2 * filters.size());
			for (size_t i = 0; i < filters.size(); i++) {
				wstring_buffer.emplace_back(utf8_to_wchar(filters[i].name));
				w32_filters[i].pszName = wstring_buffer.back().c_str();

				wstring_buffer.emplace_back(utf8_to_wchar(filters[i].pattern));
				w32_filters[i].pszSpec = wstring_buffer.back().c_str();
			}
			pFileSave->SetFileTypes(static_cast<UINT>(w32_filters.size()), w32_filters.data());
		}

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
					output = CleanPath(wchar_to_utf8(pszFilePath));
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
	LPWSTR err_win32 = NULL;

	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&err_win32, 0, NULL);

	std::string err;

	if (err_win32) {
		err = wchar_to_utf8(err_win32);
		LocalFree(err_win32);
	}

	return err;
}

} // namespace hg
