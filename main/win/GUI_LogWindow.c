/***************************************************************************
						  GUI_LogWindow.c  -  description
							 -------------------
	copyright            : (C) 2003 by ShadowPrince
	email                : shadow@emulation64.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <windows.h>
#include <stdio.h>
#include "GUI_LogWindow.h"
#include "DebugView.h"
#include "main_win.h"

int extLogger;
//HINSTANCE dv_hInst;
HMODULE DVHandle;
///////////////// External Logger DLL ////////////////////////////////////////// 

int CreateExtLogger() {

	char TempStr[MAX_PATH];

	sprintf(TempStr, "%s%s", AppPath, "debugview.dll");

	DVHandle = LoadLibrary(TempStr);

	if (DVHandle) {

		FileLog = (void(__cdecl*)(char*, ...)) GetProcAddress(DVHandle, "FileLog");
		OpenDV = (HWND(__cdecl*)(HINSTANCE, int)) GetProcAddress(DVHandle, "OpenDV");
		DVMsg = (void(__cdecl*)(int, char*, ...)) GetProcAddress(DVHandle, "DVMsg");
		CloseDV = (void(__cdecl*)(void)) GetProcAddress(DVHandle, "CloseDV");
		ShowDV = (void(__cdecl*)(int)) GetProcAddress(DVHandle, "ShowDV");
		SetUserIcon = (void(__cdecl*)(int, HICON)) GetProcAddress(DVHandle, "SetUserIcon");
		SetUserIconName = (void(__cdecl*)(int, char*)) GetProcAddress(DVHandle, "SetUserIconName");
		DVClear = (void(__cdecl*)(void)) GetProcAddress(DVHandle, "DVClear");


		OpenDV(DVHandle, DV_HIDE);
//        ShowDV( DV_HIDE ) ;

		return 1;
	} else {
		return 0;
	}
}

/* Show / Hide Management */
void ShowLogWindow() {
	if (extLogger) ShowDV(DV_SHOW);
}

void HideLogWindow() {
	if (extLogger) ShowDV(DV_HIDE);
}


void ShowHideLogWindow() {
	if (extLogger) ShowDV(DV_AUTO);
}



/* Add Text to Log */
void ShowInfo(const char* Str, ...) {
#ifndef _DEBUG
	return;
#endif
	int i;
	char Msg[800];

	va_list ap;
	va_start(ap, Str);
	_vsnprintf(Msg, sizeof(Msg), Str, ap);
	va_end(ap);


	for (i = 0; i < strlen(Msg); i++) {
		if (Msg[i] == '\n') Msg[i] = ' ';		//carrier retirn
		if (Msg[i] == '\r') Msg[i] = ' ';
		if (Msg[i] == '\t') Msg[i] = ' ';		//tab
	}
#if 0
	OutputDebugString(Msg);
	OutputDebugString("\n");
#else
	if (DVMsg) DVMsg(DV_INFO, Msg); //logger disabled
	else { printf("%s", Msg); printf("\n"); }
#endif
}

void ShowWarning(const char* Str, ...) {
	int i;
	char Msg[800];

	va_list ap;
	va_start(ap, Str);
	_vsnprintf(Msg, sizeof(Msg), Str, ap);
	va_end(ap);


	for (i = 0; i < (int) strlen(Msg); i++) {
		if (Msg[i] == '\n') Msg[i] = ' ';		//carrier retirn
		if (Msg[i] == '\r') Msg[i] = ' ';
		if (Msg[i] == '\t') Msg[i] = ' ';		//tab
	}

	if (DVMsg) DVMsg(DV_WARNING, Msg);
}

void ShowError(const char* Str, ...) {
	int i;
	char Msg[800];

	va_list ap;
	va_start(ap, Str);
	_vsnprintf(Msg, sizeof(Msg), Str, ap);
	va_end(ap);


	for (i = 0; i < (int) strlen(Msg); i++) {
		if (Msg[i] == '\n') Msg[i] = ' ';		//carrier retirn
		if (Msg[i] == '\r') Msg[i] = ' ';
		if (Msg[i] == '\t') Msg[i] = ' ';		//tab
	}

	if (DVMsg) DVMsg(DV_ERROR, Msg);
}


/* Clear Text in Log */
void ClearLogWindow() {
	if (extLogger) {
		if (DVClear) DVClear();
	}
}


int GUI_CreateLogWindow(HWND hwnd) {
	extLogger = CreateExtLogger();
	return extLogger;
}

void CloseLogWindow() {
	// if this closes once you can never open it again
	// probably this is just shitty programming because debugview source code isnt available
	if (extLogger) {
		if (CloseDV) CloseDV();
	}
}


