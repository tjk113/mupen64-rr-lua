#include "../resource.h"
#include "../main/win/CrashHandler.h"
#include "../main/win/main_win.h"
#include "GameDebugger.h"
#include <stdio.h>
#include <Psapi.h>
#include "vcr.h"
#include <Windows.h>
#include "../../r4300/r4300.h"
#include <LuaConsole.h>

// shitty crappy n64 debugger* by aurumaker 


typedef struct _DebuggedData DebuggerData;
struct _DebuggedData
{
    unsigned long Address;
    unsigned long Source;


    char* DecodedInstructionText;
};
    
DebuggerData* debuggerData = NULL;
int debugger_cpuAllowed = 1;
HWND hwndd;
extern unsigned long op;
BOOL diecdmode;

int _gaddr() {
    return Config.guiDynacore ? PC->addr : interp_addr;
}
int _gsrc() {
    return Config.guiDynacore ? PC->src : op;
}



VOID DebuggerSet(INT debuggerFlag) {

    debugger_cpuAllowed = debuggerFlag;

    if (debuggerFlag == N64DEBUG_PAUSE) {
        
        char* instrstr = (char*)calloc(100, sizeof(char));

        diecdmode = IsDlgButtonChecked(hwndd, IDC_DEBUGGER_INSTDECODEMODE);

        if(diecdmode)
            instrStr1(_gaddr(), _gsrc(), instrstr);
        else
            instrStr2(_gaddr(), _gsrc(), instrstr);
        

        SetWindowText(GetDlgItem(hwndd, IDC_DEBUGGER_INSTRUCTION), instrstr);


        //if(Config.guiDynacore)
        //PC->s_ops();
    }

}

BOOL CALLBACK DebuggerDialogProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
    switch (Message) {
    case WM_INITDIALOG: {

        hwndd = hwnd;
            
        //SendMessage(GetDlgItem(hwnd, IDC_DEBUGGER_PAUSE),
        //    (UINT)BM_SETIMAGE,
        //    (WPARAM)IMAGE_BITMAP,
        //    (LPARAM)LoadBitmap(GetModuleHandle(0), MAKEINTRESOURCE(IDB_PAUSE)));
        //
        //SendMessage(GetDlgItem(hwnd, IDC_DEBUGGER_RESUME),
        //    (UINT)BM_SETIMAGE,
        //    (WPARAM)IMAGE_BITMAP,
        //    (LPARAM)LoadBitmap(GetModuleHandle(0), MAKEINTRESOURCE(IDB_RESUME)));
        //

        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_DEBUGGER_PAUSE:
            DebuggerSet(N64DEBUG_PAUSE);

            
            break;
        case IDC_DEBUGGER_RESUME:
            DebuggerSet(N64DEBUG_RESUME);

            
            break;
        case IDOK:
            if (!debugger_cpuAllowed) {
                MessageBox(0, "The debugger still paused cpu. Unpause it and then close this.", "No", MB_ICONASTERISK);
                break;
            }
            EndDialog(hwnd,0);
            break;
        }
    default:
        return FALSE;

        return DebuggerDialogProc(hwnd, Message, wParam, lParam);
    }
}

DWORD WINAPI DebuggerThread(LPVOID lpParam) {
    DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_GAMEDEBUGGERDIALOG), 0, DebuggerDialogProc);
    ExitThread(0);
}
void DebuggerDialog() {

    DWORD troll;
    CreateThread(NULL, 0, DebuggerThread, NULL, 0, &troll);

    

}