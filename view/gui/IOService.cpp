#include <shared/services/IOService.h>
#include <string>
#include <vector>
#include <Windows.h>

std::vector<std::string> IOService::get_files_with_extension_in_directory(std::string directory, const std::string& extension)
{
	if (directory.empty())
	{
		directory = ".\\";
	} else
	{
		if (directory.back() != '\\')
		{
			directory += "\\";
		}
	}


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
