#pragma once

#define TAS_STUDIO_NAME "TAS Studio"

extern void TASStudioWindow(int flag);
extern void LoadTASStudio();
extern void TASStudioBuild();
extern SMovieHeader curMovie;
extern char* tasStudioinputbuffer;
extern int pendingData;
extern int tasStudioInitialised;

// made this because i kept getting confused
#define LV_SetCellText(hwnd, col, row, text) ListView_SetItemText(hwnd, row, col, TEXT(text));
#define LV_DeleteAll(hwnd) ListView_DeleteAllItems(hwnd); //for (unsigned int i = 0; i < 18; i++) ListView_DeleteColumn(hwnd, i);
#define SELCELL(hwnd, cellptr, row, col) SetCell(cellptr, row, col)
#define GETCELL(hwnd, row,col) SendMessage(hwnd, ZGM_GETCELLINDEX, row, col)

#define STUDIOCONTROL_W (600)
#define STUDIOCONTROL_H (300)

#define TS_STATUS(str) SetWindowText(statusControlhWnd, str)

