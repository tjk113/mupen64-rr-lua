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
#include "Config.h"

#ifndef MAIN_WIN_H
#define MAIN_WIN_H

#define MUPEN_VERSION     "Mupen 64 1.1.3"

extern BOOL CALLBACK CfgDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
extern void ShowMessage(const char* lpszMessage) ;
extern void EnableToolbar();
extern void CreateStatusBarWindow( HWND hwnd );
extern void SetStatusMode( int mode );
extern char *getPluginName( char *pluginpath, int plugintype);
extern char* getExtension(char *str);

/********* Global Variables **********/
extern char TempMessage[200];
extern int emu_launched; // emu_emulating
extern int emu_paused;
extern int recording;
extern HWND hTool, mainHWND, hStatus, hRomList, hStatusProgress;
extern HINSTANCE app_hInstance;
extern BOOL manualFPSLimit;
extern char statusmsg[800];
extern int shouldSave;

extern char gfx_name[255];
extern char input_name[255];
extern char sound_name[255];
extern char rsp_name[255];

extern HWND hwnd_plug;
extern HANDLE EmuThreadHandle;

extern char AppPath[MAX_PATH];

extern void EnableEmulationMenuItems(BOOL flag);
BOOL IsMenuItemEnabled(HMENU hMenu, UINT uId);
extern BOOL StartRom(char *fullRomPath);
extern void resetEmu() ;
extern void resumeEmu(BOOL quiet);
extern void pauseEmu(BOOL quiet);
//extern void closeRom();
extern void search_plugins();
extern void rewind_plugin();
extern int get_plugin_type();
extern char *next_plugin();
extern void exec_config(char *name);
extern void exec_test(char *name);
extern void exec_about(char *name);
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
