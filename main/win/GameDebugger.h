#pragma once


#ifdef _DEBUG
#define N64DEBUGGER_ALLOWED 1
#endif

#define N64DEBUG_PAUSE 0
#define N64DEBUG_RESUME 1


extern int debugger_cpuAllowed;

extern void DebuggerDialog();

