#pragma once
#include <windows.h>

extern char GUICrashMessage[260];

#define CRASH_LOG "crash.log"

#define EMU_STATUS_CONTINUE 0b10
#define EMU_STATUS_KILL     0b00
#define EMU_STATUS_ROMCLOSE 0b11
// 1st bit on: Continue
// 1st bit off: Die

// 2nd bit on: Close rom
// 2nd bit off: Nothing

#define GETBIT(val,n) ((val >> n) & 1)

LONG WINAPI ExceptionReleaseTarget(_EXCEPTION_POINTERS* ExceptionInfo);
BOOL CALLBACK ErrorDialogProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

extern BYTE ValueStatus;
extern BOOL actualCrash;
extern char GUICrashMessage[260];