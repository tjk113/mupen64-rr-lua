#pragma once

#include <filesystem>
#include <shlobj_core.h>
#include <span>
#include <string>
#include <vector>
#include <Windows.h>

/**
 * \brief Records the execution time of a scope
 */
class ScopeTimer
{
public:
	ScopeTimer(std::string name)
	{
		m_name = name;
		m_start_time = std::chrono::high_resolution_clock::now();
	}

	~ScopeTimer()
	{
		print_duration();
	}

	void print_duration()
	{
		printf("[ScopeTimer] %s took %dms\n", m_name.c_str(), static_cast<int>((std::chrono::high_resolution_clock::now() - m_start_time).count() / 1'000'000));
	}

private:
	std::string m_name;
	std::chrono::time_point<std::chrono::steady_clock> m_start_time;
};

/**
 * \brief Gets all files under all subdirectory of a specific directory, including the directory's shallow files
 * \param directory The path joiner-terminated directory
 */
static std::vector<std::string> get_files_in_subdirectories(
	std::string directory)
{
	if (directory.back() != '\\')
	{
		directory += "\\";
	}
	WIN32_FIND_DATA find_file_data;
	const HANDLE h_find = FindFirstFile((directory + "*").c_str(),
	                                    &find_file_data);
	if (h_find == INVALID_HANDLE_VALUE)
	{
		return {};
	}

	std::vector<std::string> paths;
	std::string fixed_path = directory;
	do
	{
		if (strcmp(find_file_data.cFileName, ".") != 0 && strcmp(
			find_file_data.cFileName, "..") != 0)
		{
			std::string full_path = directory + find_file_data.cFileName;
			if (find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (directory[directory.size() - 2] == '\0')
				{
					if (directory.back() == '\\')
					{
						fixed_path.pop_back();
						fixed_path.pop_back();
					}
				}
				if (directory.back() != '\\')
				{
					fixed_path.push_back('\\');
				}
				full_path = fixed_path + find_file_data.cFileName;
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

/**
 * \brief Removes the extension from a path
 * \param path The path to remove the extension from
 * \return The path without an extension
 */
static std::string strip_extension(const std::string& path)
{
	size_t i = path.find_last_of('.');

	if (i != std::string::npos)
	{
		return path.substr(0, i);
	}
	return path;
}

/**
 * \brief Removes the extension from a path
 * \param path The path to remove the extension from
 * \return The path without an extension
 */
static std::wstring strip_extension(const std::wstring& path)
{
	size_t i = path.find_last_of('.');

	if (i != std::string::npos)
	{
		return path.substr(0, i);
	}
	return path;
}

/**
 * \brief Gets the path to the current user's desktop
 */
static std::wstring get_desktop_path()
{
	wchar_t path[MAX_PATH + 1] = {0};
	SHGetSpecialFolderPathW(HWND_DESKTOP, path, CSIDL_DESKTOP, FALSE);
	return path;
}

/**
 * \brief Creates a new path with a different name, keeping the extension and preceeding path intact
 * \param path A path
 * \param name The new name
 * \return The modified path
 */
static std::filesystem::path with_name(std::filesystem::path path, std::string name)
{
	char drive[MAX_PATH] = {0};
	char dir[MAX_PATH] = {0};
	char filename[MAX_PATH] = {0};
	_splitpath(path.string().c_str(), drive, dir, filename, nullptr);

	return std::filesystem::path(std::string(drive) + std::string(dir) + name + path.extension().string());
}

/**
 * \brief Gets the name (filename without extension) of a path
 * \param path A path
 * \return The path's name
 */
static std::string get_name(std::filesystem::path path)
{
	char filename[MAX_PATH] = {0};
	_splitpath(path.string().c_str(), nullptr, nullptr, filename, nullptr);
	return filename;
}
