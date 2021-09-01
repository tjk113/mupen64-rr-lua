#include "../resource.h"
#include "../main/win/CrashHandler.h"
#include "../main/win/main_win.h"
#include "GameDebugger.h"
#include <stdio.h>
#include <Psapi.h>
#include "vcr.h"
#include <Windows.h>


int debugger_cpuAllowed = 1;
// shitty crappy n64 debugger* by aurumaker 

HWND hwndd;


VOID DebuggerSet(INT debuggerFlag) {

    debugger_cpuAllowed = debuggerFlag;

    if (debuggerFlag == N64DEBUG_PAUSE) {
        
    }
}
BOOL CALLBACK DebuggerDialogProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
    switch (Message) {
    case WM_INITDIALOG: {

        
            
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