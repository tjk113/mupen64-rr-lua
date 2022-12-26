#include "CrashHelper.h"
#include "main_win.h"
#include <vcr.h>
#include <Psapi.h>


int CrashHelper::FindModuleName(char* error, void* addr, int len)
{
    HMODULE hMods[1024];
    HANDLE hProcess = GetCurrentProcess();
    DWORD cbNeeded;
    printf("addr: %p\n", addr);
    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
    {
        HMODULE maxbase = 0;
        for (int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
        {
            //find closest addr
            if (hMods[i] > maxbase && hMods[i] < addr) {
                maxbase = hMods[i];
                char modname[MAX_PATH];
                GetModuleBaseName(hProcess, maxbase, modname, sizeof(modname) / sizeof(char));
                printf("%s: %p\n", modname, maxbase);
            }
        }
        // Get the full path to the module's file.
        char modname[MAX_PATH];
        if (GetModuleBaseName(hProcess, maxbase, modname, sizeof(modname) / sizeof(char)))
            // write the address with module
            return sprintf(error + len, "Addr:0x%p (%s 0x%p)\n", addr, modname, maxbase);
    }
    return 0; //what
}

void CrashHelper::GetExceptionCodeFriendlyName(_EXCEPTION_POINTERS* exceptionPointersPtr, char* exceptionCodeStringPtr) {

    switch (exceptionPointersPtr->ExceptionRecord->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        strcpy(exceptionCodeStringPtr, "Access violation");
        break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        strcpy(exceptionCodeStringPtr, "Attempted to access out-of-bounds array index");
        break;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
        strcpy(exceptionCodeStringPtr, "An operand in floating point operation was denormal");
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        strcpy(exceptionCodeStringPtr, "Float division by zero");
        break;
    case EXCEPTION_STACK_OVERFLOW:
        strcpy(exceptionCodeStringPtr, "Stack overflow");
        break;
    default:
        sprintf(exceptionCodeStringPtr, "%d", exceptionPointersPtr->ExceptionRecord->ExceptionCode);
        break;
    }
}


void CrashHelper::GenerateLog(_EXCEPTION_POINTERS* exceptionPointersPtr, char* logPtr) {
    int len = 0;
    const int maxLength = 1024 * 4;

    char error[maxLength] = {0};

    if (exceptionPointersPtr != nullptr) {
        void* addr = exceptionPointersPtr->ExceptionRecord->ExceptionAddress;
        len += FindModuleName(error, addr, len); //appends to error as well

        //emu info
#ifdef _DEBUG
        len += sprintf(error + len, "Version:" MUPEN_VERSION " DEBUG\n");
#else
        len += sprintf(error + len, "Version:" MUPEN_VERSION "\n");
#endif
        char exceptionCodeFriendly[maxLength] = { 0 };
        GetExceptionCodeFriendlyName(exceptionPointersPtr, exceptionCodeFriendly);
        len += sprintf(error + len, "Exception code: %s (0x%08x)\n", exceptionCodeFriendly, exceptionPointersPtr->ExceptionRecord->ExceptionCode);
    }
    else {
        //emu info
#ifdef _DEBUG
        len += sprintf(error + len, "Version:" MUPEN_VERSION " DEBUG\n");
#else
        len += sprintf(error + len, "Version:" MUPEN_VERSION "\n");
#endif
        len += sprintf(error + len, "Exception code: unknown (no exception thrown, was crash log called manually?)\n");
    }
    len += sprintf(error + len, "Gfx:%s\n", gfx_name);
    len += sprintf(error + len, "Input:%s\n", input_name);
    len += sprintf(error + len, "Audio:%s\n", sound_name);
    len += sprintf(error + len, "rsp:%s\n", rsp_name);
    extern int m_task;
    //some flags
    len += sprintf(error + len, "m_task:%d\n", m_task);
    len += sprintf(error + len, "emu_launched:%d\n", emu_launched);
    len += sprintf(error + len, "is_capturing_avi:%d\n", VCR_isCapturing());

    strcpy(logPtr, error);
}


