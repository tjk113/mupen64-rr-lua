#pragma once

#include <filesystem>
#include <Windows.h>
#include <string>
#include <vector>

std::vector<std::string> get_files_with_extension_in_directory(
	const std::string &directory, const std::string &extension);

std::vector<std::string> get_files_in_subdirectories(
	const std::string& directory);

std::string strip_extension(const std::string& path);
std::wstring strip_extension(const std::wstring &path);
std::wstring get_extension(const std::wstring &path);

void copy_to_clipboard(HWND owner, const std::string &str);

std::wstring get_desktop_path();
bool is_file_accessible(const std::filesystem::path& path);

void vecwrite(std::vector<uint8_t>& vec, void* data, size_t len);
