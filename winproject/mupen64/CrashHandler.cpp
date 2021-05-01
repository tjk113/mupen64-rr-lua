#include "CrashHandler.h"
#include "main_win.h"
#include <stdio.h>
#include <Psapi.h>

void FindModuleName(void* addr)
{
    HMODULE hMods[1024];
    HANDLE hProcess = GetCurrentProcess();
    DWORD cbNeeded;
    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
    {
        for (int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
        {
            TCHAR szModName[MAX_PATH];

            // Get the full path to the module's file.

            if (GetModuleFileNameEx(hProcess, hMods[i], szModName,
                sizeof(szModName) / sizeof(TCHAR)))
            {
                // Print the module name and handle value.

                printf(TEXT("\t%s (0x%08X)\n"), szModName, hMods[i]);
            }
        }
    }
}

LONG WINAPI ExceptionReleaseTarget(_EXCEPTION_POINTERS* ExceptionInfo)
{
    int len = 0;
    char error[2048];
    void* addr = ExceptionInfo->ExceptionRecord->ExceptionAddress;
    FindModuleName(addr);
    len += sprintf(error, "Addr:%x\n", addr);
    len += sprintf(error, "Gfx:%x\n", gfx_name);
    len += sprintf(error, "Input:%x\n", input_name);
    len += sprintf(error, "Audio:%x\n", sound_name);
    len += sprintf(error, "rsp:%x\n", rsp_name);

    int code = MessageBox(0, "Emulator crashed, press OK to try to continue\n Crash log saved to " CRASH_LOG, "Exception", MB_OKCANCEL | MB_ICONERROR);
    return code == IDOK ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_EXECUTE_HANDLER;
}