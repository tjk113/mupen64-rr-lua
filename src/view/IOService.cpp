/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <IOService.h>


std::vector<std::wstring> IOService::get_files_with_extension_in_directory(std::wstring directory, const std::wstring& extension)
{
    if (directory.empty())
    {
        directory = L".\\";
    }
    else
    {
        if (directory.back() != L'\\')
        {
            directory += L"\\";
        }
    }

    WIN32_FIND_DATA find_file_data;
    const HANDLE h_find = FindFirstFile((directory + L"*." + extension).c_str(), &find_file_data);
    if (h_find == INVALID_HANDLE_VALUE)
    {
        return {};
    }

    std::vector<std::wstring> paths;

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
