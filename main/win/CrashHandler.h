#pragma once
#include <windows.h>

#define CRASH_LOG "crash.log"

LONG WINAPI ExceptionReleaseTarget(_EXCEPTION_POINTERS* ExceptionInfo)