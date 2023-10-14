#include "io_helpers.h"
#include <cstdio>
#include <Windows.h>
#include <shlobj.h>
#include <string>
#include <vector>

std::vector<std::string> get_files_with_extension_in_directory(
	const std::string &directory, const std::string &extension)
{
	WIN32_FIND_DATA find_file_data;
	const HANDLE h_find = FindFirstFile((directory + "*." + extension).c_str(),
	                                    &find_file_data);
	if (h_find == INVALID_HANDLE_VALUE)
	{
		return {};
	}

	std::vector<std::string> paths;

	do
	{
		if (!(find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			paths.emplace_back(directory + find_file_data.cFileName);
		}
	}
	while (FindNextFile(h_find, &find_file_data) != 0);

	FindClose(h_find);

	return paths;
}

std::vector<std::string> get_files_in_subdirectories(
	const std::string &directory)
{
	WIN32_FIND_DATA find_file_data;
	const HANDLE h_find = FindFirstFile((directory + "*").c_str(),
	                                    &find_file_data);
	if (h_find == INVALID_HANDLE_VALUE)
	{
		return {};
	}

	std::vector<std::string> paths;

	do
	{
		if (strcmp(find_file_data.cFileName, ".") != 0 && strcmp(
			find_file_data.cFileName, "..") != 0)
		{
			std::string full_path = directory + find_file_data.cFileName;
			if (find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				for (const auto& path : get_files_in_subdirectories(
					     full_path + "\\"))
				{
					paths.push_back(path);
				}
			} else
			{
				paths.push_back(full_path);
			}
		}
	}
	while (FindNextFile(h_find, &find_file_data) != 0);

	FindClose(h_find);

	return paths;
}

std::string strip_extension(const std::string& path) {
	size_t i = path.find_last_of('.');

	if (i != std::string::npos) {
		return path.substr(0, i);
	}
	return path;
}
std::wstring strip_extension(const std::wstring &path)
{
	size_t i = path.find_last_of('.');

	if (i != std::string::npos) {
		return path.substr(0, i);
	}
	return path;
}
std::wstring get_extension(const std::wstring &path)
{
	size_t i = path.find_last_of('.');

	if (i != std::string::npos) {
		return path.substr(i, path.size() - i);
	}
	return path;
}

void copy_to_clipboard(HWND owner, const std::string &str)
{
	OpenClipboard(owner);
	EmptyClipboard();
	HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, str.size() + 1);
	if (hg)
	{
		memcpy(GlobalLock(hg), str.c_str(), str.size() + 1);
		GlobalUnlock(hg);
		SetClipboardData(CF_TEXT, hg);
		CloseClipboard();
		GlobalFree(hg);
	} else
	{
		printf("Failed to copy\n");
		CloseClipboard();
	}
}

std::wstring get_desktop_path()
{
	wchar_t path[MAX_PATH + 1] = {0};
	SHGetSpecialFolderPathW(HWND_DESKTOP, path, CSIDL_DESKTOP, FALSE);
	return path;
}
