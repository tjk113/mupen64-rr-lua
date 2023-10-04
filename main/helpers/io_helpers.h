#pragma once

#include <Windows.h>
#include <string>
#include <vector>

std::vector<std::string> get_files_with_extension_in_directory(
	std::string directory, std::string extension);

std::vector<std::string> get_files_in_subdirectories(
	std::string directory);

std::wstring strip_extension(std::wstring path);
std::wstring get_extension(std::wstring path);

void copy_to_clipboard(HWND owner, std::string str);

std::wstring get_desktop_path();
