#include "CrashHandler.h"
#include "main_win.h"
#include <stdio.h>
#include <Psapi.h>
#include "../../winproject/resource.h" //MUPEN_VERSION
#include "vcr.h"

char GUICrashMessage[260];
BYTE ValueStatus;

_EXCEPTION_POINTERS* ExceptionInfoG;
BOOL actualCrash;

//Attempt to find crashing module, finds closest base address to crash point which should be the module (right?)
//error - points to the error msg buffer
//addr - where did it crash
//len - current error length so it can append properly
//returns length of appended text
int FindModuleName(char* error, void* addr, int len)
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

// TODO: find some library or something because this is cringe, no program should have to handle this manually
void GetExceptionCodeFriendlyName(char* str, _EXCEPTION_POINTERS* ExceptionInfo) {

    switch (ExceptionInfo->ExceptionRecord->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        strcpy(str, "Access violation");
        break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        strcpy(str, "Attempted to access out-of-bounds array index");
        break;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
        strcpy(str, "An operand in floating point operation was denormal");
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        strcpy(str, "Float division by zero");
        break;
    case EXCEPTION_STACK_OVERFLOW:
        strcpy(str, "Stack overflow");
        break;
    default:
        sprintf(str, "%d", ExceptionInfo->ExceptionRecord->ExceptionCode);
        break;
    }
}

VOID CrashLog(_EXCEPTION_POINTERS* ExceptionInfo) {
    int len = 0;
    char error[2048];
    void* addr = ExceptionInfo->ExceptionRecord->ExceptionAddress;
    len += FindModuleName(error, addr, len); //appends to error as well
    
    //emu info
#ifdef _DEBUG
    len += sprintf(error + len, "Version:" MUPEN_VERSION " DEBUG\n");
#else
    len += sprintf(error + len, "Version:" MUPEN_VERSION "\n");
#endif
    char exceptionCodeFriendly[2048] = { 0 };
    GetExceptionCodeFriendlyName(exceptionCodeFriendly, ExceptionInfo);
    len += sprintf(error + len, "Exception code: %s (0x%08x)\n", exceptionCodeFriendly, ExceptionInfo->ExceptionRecord->ExceptionCode);
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
}

BOOL CALLBACK ErrorDialogProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
    switch (Message) {
    case WM_INITDIALOG: {

        CrashLog(ExceptionInfoG);

        char TitleBar[MAX_PATH];
        sprintf(TitleBar, "%s - %s", MUPEN_VERSION, GUICrashMessage);
        SetWindowText(hwnd, TitleBar);

        strcat(GUICrashMessage, "\r\nA crash log was automatically generated\r\nPlease choose a way to proceed");
        SetWindowText(GetDlgItem(hwnd, IDC_ERRORTEXT), GUICrashMessage);

        //EnableWindow(GetDlgItem(hwnd, IDC_ERROR_PANIC_CLOSEMUPEN), actualCrash);
        //EnableWindow(GetDlgItem(hwnd, IDC_CRASHREPORT), actualCrash);

        DWORD exStyle = ::GetWindowLong(GetDlgItem(hwnd, IDC_ERROR_PICTUREBOX), GWL_EXSTYLE);

        SetWindowLong(GetDlgItem(hwnd, IDC_ERROR_PICTUREBOX), GWL_EXSTYLE, exStyle | WS_EX_LAYERED);

        HBITMAP hBitmap = reinterpret_cast<HBITMAP>(::LoadImage(
            GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_LOGO_BW), IMAGE_BITMAP,
            0, 0, LR_CREATEDIBSECTION));

        SendMessage(GetDlgItem(hwnd, IDC_ERROR_PICTUREBOX), STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);
        
        // for some reason windows holds pointer to this so cant dispose it now
        // this, of course, causes a memory leak because we should free upon calling EndDialog()
        //DeleteObject((HBITMAP)hBitmap);

        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_ERROR_PANIC_IGNORE:
            ValueStatus = EMU_STATUS_CONTINUE;
            EndDialog(hwnd, 0);
            break;
        case IDC_ERROR_PANIC_CLOSE:
            ValueStatus = EMU_STATUS_KILL;
            EndDialog(hwnd, 0);
            break;
        }
        break;
    default:
        return FALSE;
    
    }
    
}

// returns: close rom?
BOOL ErrorDialogEmuError() {
    actualCrash = FALSE;

    strcpy(GUICrashMessage, "An emulation problem has occured");

    DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_MUPENERROR), 0, ErrorDialogProc);

    return GETBIT(ValueStatus, 2);
}


//builds da error message
LONG WINAPI ExceptionReleaseTarget(_EXCEPTION_POINTERS* ExceptionInfo)
{
    actualCrash = TRUE;

    ExceptionInfoG = ExceptionInfo;

    strcpy(GUICrashMessage, "An exception has been thrown");

    DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_MUPENERROR), 0, ErrorDialogProc);

    return GETBIT(ValueStatus, 1) ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_EXECUTE_HANDLER;
}