/***************************************************************************
						  main_win.h  -  description
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

 //for max recent
#include "../lua/Recent.h"
#include "Config.hpp"

#ifndef MAIN_WIN_H
#define MAIN_WIN_H

#define MUPEN_VERSION     "Mupen 64 1.1.4"

extern BOOL CALLBACK CfgDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
extern void ShowMessage(const char* lpszMessage);
extern void EnableToolbar();
extern void CreateStatusBarWindow(HWND hwnd);
extern void SetStatusMode(int mode);
extern char* getExtension(char* str);

/********* Global Variables **********/
extern char TempMessage[200];
extern int emu_launched; // emu_emulating
extern int emu_paused;
extern int recording;
extern HWND hTool, mainHWND, hStatus;
extern HINSTANCE app_hInstance;
extern BOOL manualFPSLimit;
extern char statusmsg[800];
extern int shouldSave;

extern HWND hwnd_plug;
extern HANDLE EmuThreadHandle;

extern char AppPath[MAX_PATH];

extern void EnableEmulationMenuItems(BOOL flag);
BOOL IsMenuItemEnabled(HMENU hMenu, UINT uId);
BOOL StartRom(const char* fullRomPath);
extern void resetEmu();
extern void resumeEmu(BOOL quiet);
extern void pauseEmu(BOOL quiet);
extern void EnableStatusbar();
extern void OpenMoviePlaybackDialog();
extern void OpenMovieRecordDialog();
extern void LoadConfigExternals();

extern BOOL forceIgnoreRSP;
extern BOOL continue_vcr_on_restart_mode;
extern BOOL ignoreErrorEmulation;

void exit_emu(int postquit);

#define IGNORE_RSP (((!Config.is_fps_limited || !manualFPSLimit) && !VCR_isCapturing() && (!Config.frame_skip_frequency || (frame++ % Config.frame_skip_frequency)))) //if frame advancing and either skipfreq is 0 or modulo is 0

#define RESET_TITLEBAR char tmpwndtitle[200]; sprintf(tmpwndtitle, MUPEN_VERSION " - %s", ROM_HEADER->nom); SetWindowText(mainHWND, tmpwndtitle);

#endif
