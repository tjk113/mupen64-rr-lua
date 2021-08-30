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

VOID CrashLog(_EXCEPTION_POINTERS* ExceptionInfo) {
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
}

BOOL CALLBACK ErrorDialogProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
    switch (Message) {
    case WM_INITDIALOG: {

        

        //SendDlgItemMessage(hwnd, IDB_LOGO_BW, STM_SETIMAGE, IMAGE_BITMAP,
        //    (LPARAM)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_LOGO_BW),
        //        IMAGE_BITMAP, 0, 0, 0));

        char TitleBar[300];
        sprintf(TitleBar, "%s - %s", MUPEN_VERSION, GUICrashMessage);
        SetWindowText(hwnd, TitleBar);

        strcat(GUICrashMessage, "\r\nPress \'Stop Emulation\' to close the current rom\r\nPress \'Dump crash log\' to generate a crash log.\r\nPress \'Exit\' to kill the emulator.\r\nPress \'Continue\' to attempt to continue.");
        SetWindowText(GetDlgItem(hwnd, IDC_ERRORTEXT), GUICrashMessage);


        EnableWindow(GetDlgItem(hwnd, IDC_ERROR_PANIC_CLOSEMUPEN), actualCrash);
        EnableWindow(GetDlgItem(hwnd, IDC_CRASHREPORT), actualCrash);

        HBITMAP hBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_LOGO_BW));
        SendMessage(GetDlgItem(hwnd, IDC_ERROR_PICTUREBOX), STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);
        //DeleteObject((HBITMAP)hBitmap);

        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_ERROR_CONTINUE:
            ValueStatus = EMU_STATUS_CONTINUE;
            EndDialog(hwnd, 0);
            break;
        case IDC_ERROR_PANIC_CLOSEMUPEN:
            ValueStatus = EMU_STATUS_KILL;
            EndDialog(hwnd, 0);
            break;
        case IDC_ERROR_PANIC_CLOSEROM:
            ValueStatus = EMU_STATUS_ROMCLOSE;
            EndDialog(hwnd, 0);
            break;
        case IDC_CRASHREPORT:
            CrashLog(ExceptionInfoG);
            break;
        }
    case WM_PAINT: {

        
        break;
    }
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