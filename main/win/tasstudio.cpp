/*
--
----
Mupen64 TAS Studio
----
--
*/





#include <windows.h>
#include <winuser.h>
#include <stdio.h>
#include "../main/win/Config.h"
#include "../main/win/main_win.h"
#include "../../winproject/resource.h"
#include "../main/vcr.h"
#include "tasstudio.h"
#include <CommCtrl.h>
#include "tasstudio.h"
#include "../../library/zeegrid.h"
#include <guifuncs.h>
#include <tchar.h>
#include "../../winproject/mupen64/ops_helper.h"

HINSTANCE hInstt;
HMODULE hGridMod       = NULL;
HWND hWnd              = NULL;
HWND studioControlhWnd = NULL;
HWND statusControlhWnd = NULL;
SMovieHeader curMovie;
static DWORD tasStudioThreadID;
HANDLE tasStudioThread;
char iStructName[18][3] = { "D>", "D<", "Dv", "D^", "S", "Z", "B", "A", "C>", "C<", "Cv", "C^", "R", "L", "R1", "R2", "X", "Y" };
const char* classname = "tasStudioWindow"; // "fuckOff"
char* tasStudioinputbuffer;
int pendingData;
int tasStudioInitialised;

HWND CreateGridCtl()
{
    
    hGridMod = LoadLibrary("zeegrid.dll");
    if (!hGridMod)
    {
        MBOX_ERROR("Failed to load TAS Studio dependencies. zeegrid.dll in this directory might be missing.");
        return NULL;
    }

    studioControlhWnd = CreateWindowA("ZeeGrid", "TAS Studio", WS_CHILD | WS_VISIBLE, 0, 0, STUDIOCONTROL_W, STUDIOCONTROL_H, hWnd, (HMENU)0, hInstt, NULL);
    
    SendMessage(studioControlhWnd, ZGM_EMPTYGRID, 1, 0);
    SendMessage(studioControlhWnd, ZGM_DIMGRID, 18, 0);
    SendMessage(studioControlhWnd, ZGM_SHOWROWNUMBERS, TRUE, 0);
    SendMessage(studioControlhWnd, ZGM_SETDEFAULTTYPE, 2, 0);
    SendMessage(studioControlhWnd, ZGM_SETDEFAULTNUMPRECISION, 0, 0);
    SendMessage(studioControlhWnd, ZGM_SHOWROWNUMBERS, TRUE, 0);
    SendMessage(studioControlhWnd, ZGM_SHOWTOOLBAR, FALSE, 0);
    //SendMessage(studioControlhWnd, ZGM_ENABLEROWSELECT, FALSE, 0);
    
    for (char i = 0; i < 18; i++)
        SendMessage(studioControlhWnd, ZGM_SETCELLTEXT, i+1, (LPARAM)iStructName[i]);

    return studioControlhWnd;
}

void TASStudioSeek(int targetFrame) {
    // ahead - turn on fast mode and wait until frame is reached, then pause
    // behind - restart movie, turn on fast mode and wait until frame is reached, then pause

    TS_STATUS("Seeking...");

    if (targetFrame > m_currentSample) {
            
    }
    else {
        // later
    }
}
void TASStudioBuild() {

    if (m_task != ETask::Playback) {
        TS_STATUS("No movie is playing");
        return;
    }

    printf("rebuild!\n");

    TS_STATUS("Loading...");

    SendMessage(studioControlhWnd, ZGM_EMPTYGRID, TRUE, 0);

    for (UINT i = 0; i < curMovie.length_samples; i++)
    {
        //SendMessage(studioControlhWnd, ZGM_APPENDROW, 0, 0);

        char name[15] = { 0 };

        ZeroMemory(name, strlen(name)); // paranoia

        for (char j = 0; j < 18-2; j++)
        {
            
            if (BIT_GET(tasStudioinputbuffer[i], j)) 
                strcpy(name, iStructName[j]);
            else
                strcpy(name, " ");
            
            SendMessage(studioControlhWnd, ZGM_SETCELLTEXT, GETCELL(studioControlhWnd, i, j), (LPARAM)name);

        }


        sprintf(name, "%hi", (signed char)BYTE_GET(tasStudioinputbuffer[i], 2));
        //SendMessage(studioControlhWnd, ZGM_SETCELLTEXT, GETCELL(studioControlhWnd, i, 17), (LPARAM)name);
        SendMessage(studioControlhWnd, ZGM_SETCELLTYPE, GETCELL(studioControlhWnd, i, 17), (LPARAM)3);
        SendMessage(studioControlhWnd, ZGM_SETCELLTEXT, GETCELL(studioControlhWnd, i, 17), (LPARAM)name);

        sprintf(name, "%hi", (signed char)BYTE_GET(tasStudioinputbuffer[i], 3));
        //SendMessage(studioControlhWnd, ZGM_SETCELLTEXT, GETCELL(studioControlhWnd, i, 18), (LPARAM)name);
        SendMessage(studioControlhWnd, ZGM_SETCELLTYPE, GETCELL(studioControlhWnd, i, 18), (LPARAM)3);
        SendMessage(studioControlhWnd, ZGM_SETCELLTEXT, GETCELL(studioControlhWnd, i, 18), (LPARAM)name);

    }

    for (char i = 0; i < 18; i++)
        SendMessage(studioControlhWnd, ZGM_SETCELLTEXT, i + 1, (LPARAM)iStructName[i]);

    UpdateWindow(hWnd);
    TS_STATUS("Idle");
}

LRESULT CALLBACK TASStudioWndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam){
    switch (Message)
    {
    case WM_INITDIALOG:
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_REBUILD:
            TASStudioBuild();
            break;
        case IDC_CLOSETASSTUDIO:
            TASStudioWindow(0); // dont close just hide
            break;

        
        }
        break;
    default:
        return DefWindowProc(hwnd, Message, wParam, lParam);
    }
    return FALSE;
}

void TASStudioRebuildHandlesCache() {

    if (!studioControlhWnd)
        studioControlhWnd = GetDlgItem(hWnd, IDC_STUDIOCONTROL);

    if(!statusControlhWnd)
        statusControlhWnd = GetDlgItem(hWnd, IDC_TASSTUDIOSTAT);
}

void TASStudioWindow(int flag) {

    
    ShowWindow(hWnd, flag ? SW_SHOW : SW_HIDE);

    UpdateWindow(hWnd);

    TASStudioRebuildHandlesCache();

}

DWORD WINAPI TASStudioThread(LPVOID tParam) {

    
    MSG Msg;

    hWnd = CreateDialog(hInstt, MAKEINTRESOURCE(IDD_TASSTUDIO_DIALOG), NULL, (DLGPROC)TASStudioWndProc);

    if (hWnd == NULL)
    {
        MessageBox(NULL, "TAS Studio failed to initialize", "Error!", MB_ICONERROR | MB_OK);
        return 0;
    }

    SetWindowLong(hWnd, GWL_STYLE,
        GetWindowLong(hWnd, GWL_STYLE) & ~WS_MINIMIZEBOX);

    studioControlhWnd = CreateGridCtl();

    TASStudioRebuildHandlesCache();
    
    TASStudioBuild();

    TS_STATUS("Idle");

    tasStudioInitialised = TRUE;

    while (GetMessage(&Msg, NULL, 0, 0))
    {
        if (!IsDialogMessage(hWnd, &Msg))
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
    }

    ExitThread(0);
}


void LoadTASStudio() {
    tasStudioThread = CreateThread(NULL, 0, TASStudioThread, NULL, 0, &tasStudioThreadID);
}
