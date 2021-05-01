#include "CrashHandler.h"
#include "main_win.h"
#include <stdio.h>
#include <Psapi.h>
#include "../../winproject/resource.h" //MUPEN_VERSION
#include "vcr.h"

//Attempt to find the base module, might be faulty with crazy crashes where everything corrupts
//error - points to the error msg buffer
//addr - where did it crash
//len - current error length so it can append properly
//returns length of appended text
int FindModuleName(char *error, void* addr, int len)
{
    HMODULE hMods[1024];
    HANDLE hProcess = GetCurrentProcess();
    DWORD cbNeeded;
    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
    {
        for (int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
        {
            char modname[MAX_PATH];
            // Get the full path to the module's file.

            if (GetModuleBaseName(hProcess, hMods[i], modname,
                sizeof(modname) / sizeof(char)))
            {

                // write the address with module
                if (hMods[i]<addr)
                    return sprintf(error+len,"Addr:0x%p (%s)\n", addr,modname);
            }
        }
    }
    return 0; //what
}

//builds da error message
LONG WINAPI ExceptionReleaseTarget(_EXCEPTION_POINTERS* ExceptionInfo)
{
    int len = 0;
    char error[2048];
    void* addr = ExceptionInfo->ExceptionRecord->ExceptionAddress;
    len += FindModuleName(error, addr, len); //appends to error as well

    //emu info
    len += sprintf(error + len, "Version:" MUPEN_VERSION "\n");
    len += sprintf(error + len, "Gfx:%s\n", gfx_name);
    len += sprintf(error + len, "Input:%s\n", input_name);
    len += sprintf(error + len, "Audio:%s\n", sound_name);
    len += sprintf(error + len, "rsp:%s\n", rsp_name);
    extern int m_task;
    //some flags
    len += sprintf(error + len, "m_task:%d\n", m_task);
    len += sprintf(error + len, "emu_launched:%d\n", emu_launched);
    len += sprintf(error + len, "is_capturing_avi:%d\n", VCR_isCapturing());

    FILE* f = fopen(CRASH_LOG, "w+"); //overwrite
    fprintf(f, error);
    fclose(f);

    int code = MessageBox(0, "Emulator crashed, press OK to try to continue\n Crash log saved to " CRASH_LOG, "Exception", MB_OKCANCEL | MB_ICONERROR);
    return code == IDOK ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_EXECUTE_HANDLER;
}