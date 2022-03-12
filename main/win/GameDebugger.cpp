#include <LuaConsole.h>

#include "../resource.h"
#include "../main/win/CrashHandler.h"
#include "../main/win/main_win.h"
#include "GameDebugger.h"
#include <stdio.h>
#include <Psapi.h>
#include "vcr.h"
#include <Windows.h>
#include "../../r4300/r4300.h"
#include "../../memory/memory.h"

// shitty crappy n64 debugger* by aurumaker 

int debugger_cpuAllowed = 1;
int debugger_step = 0;
int debugger_cartridgeTilt = 0;
HWND hwndd;
extern unsigned long op;
BOOL diecdmode;

DWORD(__cdecl* ORIGINAL_doRspCycles)(DWORD Cycles) = NULL;
static DWORD __cdecl fake_doRspCycles(DWORD Cycles) { return Cycles; };


unsigned long _gaddr() {
    return Config.guiDynacore ? PC->addr : interp_addr;
}
unsigned long _gsrc() {
    return Config.guiDynacore ? PC->src : op;
}



void DebuggerSet(int debuggerFlag) {

    if (!Config.guiDynacore) {
        //N64DEBUG_MBOX(N64DEBUG_NAME " might not work with (pure) interpreter cpu core.");
    }

    debugger_cpuAllowed = debuggerFlag;

    if (debuggerFlag == N64DEBUG_RESUME) debugger_step = 0;

    if (debuggerFlag == N64DEBUG_PAUSE) {

        // MEMORY LEAK: this leaks instrstr string and more inside instrstr functions and also crashes after some iterations
        // FIXME 
        // i dont have time or care about this so i fix it later?

        diecdmode = IsDlgButtonChecked(hwndd, IDC_DEBUGGER_INSTDECODEMODE);

        char* instrStr = (char*)malloc(255);
        char addr[100];
        char op[100];

        diecdmode ? instrStr1(_gaddr(), _gsrc(), instrStr) : instrStr2(_gaddr(), _gsrc(), instrStr);

        sprintf(addr, "%lu", _gaddr());
        sprintf(op, "%lu", _gsrc());

        SetWindowText(GetDlgItem(hwndd, IDC_DEBUGGER_INSTRUCTION), instrStr);
        SetWindowText(GetDlgItem(hwndd, IDC_DEBUGGER_PRECOMPADDR), addr);
        SetWindowText(GetDlgItem(hwndd, IDC_DEBUGGER_PRECOMPOP), op);

        free(instrStr);
        //if(Config.guiDynacore)
        //PC->s_ops();
    }

}

BOOL CALLBACK DebuggerDialogProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
    switch (Message) {
    case WM_INITDIALOG: {

        hwndd = hwnd;
            
        CheckDlgButton(hwndd, IDC_DEBUGGER_RSP_TOGGLE, 1);

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

        SetWindowText(hwnd, N64DEBUG_NAME " - " MUPEN_VERSION);

        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_DEBUGGER_DMA_R_TOGGLE:
            debugger_cartridgeTilt = IsDlgButtonChecked(hwnd, IDC_DEBUGGER_DMA_R_TOGGLE);
            break;
        case IDC_DEBUGGER_DUMPRDRAM:
            N64DEBUG_MBOX("You are about to write 32 megabytes to a file.");
            FILE* f;
            f = fopen("rdram.bin", "wb");
            fwrite(&rdram, sizeof(unsigned long), sizeof(rdram), f);
            fflush(f); fclose(f);
            break;
        case IDC_DEBUGGER_RSP_TOGGLE:
            if (!ORIGINAL_doRspCycles) {
                ORIGINAL_doRspCycles = doRspCycles;
            }
            if (IsDlgButtonChecked(hwnd, IDC_DEBUGGER_RSP_TOGGLE))
                doRspCycles = ORIGINAL_doRspCycles;
            else
                doRspCycles = fake_doRspCycles;

            break;
        case IDC_DEBUGGER_STEP:
            if (!debugger_cpuAllowed) {
                debugger_step = 1;
            }
            break;
        case IDC_DEBUGGER_INSTDECODEMODE:
            if (!debugger_cpuAllowed) DebuggerSet(0);
            break;
        case IDC_DEBUGGER_PAUSE:
            DebuggerSet(N64DEBUG_PAUSE);

            
            break;
        case IDC_DEBUGGER_RESUME:
            DebuggerSet(N64DEBUG_RESUME);

            
            break;
        case IDOK:

            // some shitty slower checks for everything

            printf(" --- N64 Debugger End ---\n");
            printf("cpu allowed: %d\n", debugger_cpuAllowed);
            printf("step: %d\n", debugger_step);
            printf("rsp allowed: %d\n", IsDlgButtonChecked(hwnd, IDC_DEBUGGER_RSP_TOGGLE));
            printf("dma read allowed: %d\n", debugger_cartridgeTilt);
            printf(" ------------------------\n");

            if (!debugger_cpuAllowed) {
                N64DEBUG_MBOX("The debugger paused the r4300. Unpause it before quitting the Debugger.");
                break;
            }
            if (debugger_step) {
                N64DEBUG_MBOX("The debugger paused the r4300. Unpause it before quitting the Debugger.");
                break;
            }
            if (!IsDlgButtonChecked(hwnd, IDC_DEBUGGER_RSP_TOGGLE) || debugger_cartridgeTilt) {
                N64DEBUG_MBOX("The debugger has changed the original game. Undo your changed settings before closing the debugger.");
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