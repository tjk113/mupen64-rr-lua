#include <shared/services/IOService.h>
#include <string>
#include <vector>
#include <Windows.h>

std::vector<std::string> IOService::get_files_with_extension_in_directory(const std::string& directory,
                                                                          const std::string& extension)
{
	std::string dir = directory;

	if (directory.empty())
	{
		dir = ".\\";
	}

	if (directory.back() != '\\')
	{
		dir += "\\";
	}

	WIN32_FIND_DATA find_file_data;
	const HANDLE h_find = FindFirstFile((dir + "*." + extension).c_str(),
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
			paths.emplace_back(dir + find_file_data.cFileName);
		}
	}
	while (FindNextFile(h_find, &find_file_data) != 0);

	FindClose(h_find);

	return paths;
}
