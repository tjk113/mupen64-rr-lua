#pragma once

#ifdef _DEBUG
#define N64DEBUGGER_ALLOWED
#endif

//#ifdef _DEBUG

#define N64DEBUG_NAME "N64 Debugger"

#define N64DEBUG_ALLOWED 1

#define N64DEBUG_PAUSE 0
#define N64DEBUG_RESUME 1

#define N64_DEBUG_STEP_DURATION 10

#define N64DEBUG_MBOX(str) MessageBox(0,str,N64DEBUG_NAME,MB_ICONASTERISK)

//#endif



extern int debugger_cpuAllowed;
extern int debugger_step;
extern int debugger_cartridgeTilt;

extern void DebuggerSet(int debuggerFlag);
extern void DebuggerDialog();

