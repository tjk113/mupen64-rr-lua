#pragma once
#include <Windows.h>

/// <summary>
/// A helper for generating or parsing exception info
/// </summary>
namespace CrashHelper
{
	void generate_log(const _EXCEPTION_POINTERS* exception_pointers_ptr, char* log_ptr);
	void get_exception_code_friendly_name(const _EXCEPTION_POINTERS* exception_pointers_ptr, char* exception_code_string_ptr);
	int find_module_name(char* error, void* addr, int len);
}
