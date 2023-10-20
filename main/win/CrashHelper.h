#pragma once
#include <Windows.h>

/// <summary>
/// A helper class for generating or parsing exception info
/// </summary>
class CrashHelper
{
public:
	static void GenerateLog(_EXCEPTION_POINTERS* exceptionPointersPtr,
	                        char* logStringPtr);
	static void GetExceptionCodeFriendlyName(
		_EXCEPTION_POINTERS* exceptionPointersPtr,
		char* exceptionCodeStringPtr);
	static int FindModuleName(char* error, void* addr, int len);
};
