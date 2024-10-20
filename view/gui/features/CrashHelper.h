#pragma once
#include <string>
#include <Windows.h>

/// <summary>
/// A helper for generating exception logs
/// </summary>
namespace CrashHelper
{
    /**
     * Logs a crash to the view logger.
     * \param exception_pointers_ptr The exception pointers
     */
    void log_crash(_EXCEPTION_POINTERS* exception_pointers_ptr);
}
