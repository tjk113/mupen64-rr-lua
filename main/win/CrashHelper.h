#pragma once
#include <Windows.h>

/// <summary>
/// A helper class for generating or parsing exception info
/// </summary>
class crash_helper
{
public:
	static void generate_log(const _EXCEPTION_POINTERS* exception_pointers_ptr,
	                         char* log_ptr);
	static void get_exception_code_friendly_name(
		const _EXCEPTION_POINTERS* exception_pointers_ptr,
		char* exception_code_string_ptr);
	static int find_module_name(char* error, void* addr, int len);
};
