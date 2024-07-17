#pragma once
#include <string>
#include <Windows.h>

/// <summary>
/// A helper for generating exception logs
/// </summary>
namespace CrashHelper
{
	std::string generate_log(const _EXCEPTION_POINTERS* exception_pointers_ptr);
}
