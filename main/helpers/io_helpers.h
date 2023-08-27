#include <memory>
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <functional>

#pragma once

inline static std::vector<std::string> get_files_with_extension_in_directory(
	std::string directory, std::string extension) {
	WIN32_FIND_DATA find_file_data;
	const HANDLE h_find = FindFirstFile((directory + "*." + extension).c_str(),
										&find_file_data);
	if (h_find == INVALID_HANDLE_VALUE) {
		return {};
	}

	std::vector<std::string> paths;

	do {
		if (!(find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			paths.push_back(directory + find_file_data.cFileName);
		}
	} while (FindNextFile(h_find, &find_file_data) != 0);

	FindClose(h_find);

	return paths;
}

inline static std::vector<std::string> get_files_in_subdirectories(std::string directory)
{
	WIN32_FIND_DATA find_file_data;
	const HANDLE h_find = FindFirstFile((directory + "*").c_str(),
										&find_file_data);
	if (h_find == INVALID_HANDLE_VALUE) {
		return {};
	}

	std::vector<std::string> paths;

	do {
		if (!(find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			paths.push_back(directory + find_file_data.cFileName);
		}
	} while (FindNextFile(h_find, &find_file_data) != 0);

	FindClose(h_find);

	return paths;
}


inline static void copy_to_clipboard(HWND owner, std::string str)
{
	OpenClipboard(owner);
	EmptyClipboard();
	HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, str.size() + 1);
	if (hg) {
		memcpy(GlobalLock(hg), str.c_str(), str.size() + 1);
		GlobalUnlock(hg);
		SetClipboardData(CF_TEXT, hg);
		CloseClipboard();
		GlobalFree(hg);
	} else { printf("Failed to copy\n"); CloseClipboard(); }
}
