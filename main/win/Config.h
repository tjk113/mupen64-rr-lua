/***************************************************************************
                          config.h  -  description
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
#pragma once
// we are tightly coupled to windows everywhere, even in the core lmfa
#include <Windows.h>
#define LUA_MAX_RECENT 5

typedef struct _HOTKEY {
    int key;
    BOOL shift;
    BOOL ctrl;
    BOOL alt;
    int command;
} HOTKEY;

#define NUM_HOTKEYS (46)

typedef struct _NamedHotkey
{
    const char* name;
    HOTKEY* hotkey;
} NamedHotkey;

typedef struct _CONFIG {
    unsigned char ConfigVersion;

    //Language
    char DefaultLanguage[100];

    // Alert Messages variables
    BOOL showFPS;
    BOOL showVIS;
    BOOL manageBadRoms;
    BOOL savestateWarnings; // warnings about savestates
    BOOL moviesERRORS; // warnings about m64<->current rom issues

    // General vars
    BOOL limitFps;
    BOOL compressedIni;
    int guiDynacore;
    BOOL UseFPSmodifier;
    int FPSmodifier;
    int skipFrequency;
    BOOL loopMovie;
    BOOL zeroIndex;
    BOOL movieBackups;
    int movieBackupsLevel;
    int CPUClockSpeedMultiplier;

    // Advanced vars
    BOOL StartFullScreen;
    BOOL PauseWhenNotActive;
    BOOL OverwritePluginSettings;
    BOOL GuiToolbar;
    BOOL GuiStatusbar;
    BOOL AutoIncSaveSlot;
    //BOOL RoundToZero;
    BOOL allowArbitrarySavestateLoading;

    //Compatibility Options
    //BOOL NoAudioDelay;
    //BOOL NoCompiledJump;

    //Rom Browser Columns
    BOOL Column_GoodName;
    BOOL Column_InternalName;
    BOOL Column_Country;
    BOOL Column_Size;
    BOOL Column_Comments;
    BOOL Column_FileName;
    BOOL Column_MD5;

    // Directories
    BOOL DefaultPluginsDir;
    BOOL DefaultSavesDir;
    BOOL DefaultScreenshotsDir;
    char PluginsDir[MAX_PATH];
    char SavesDir[MAX_PATH];
    char ScreenshotsDir[MAX_PATH];
    char SaveLoadAsandSaveStateAsPath[MAX_PATH];

    // Recent Roms
    char RecentRoms[10][MAX_PATH];
    BOOL RecentRomsFreeze;

    // Recent Movies
    char RecentMovies[5][MAX_PATH];
    BOOL RecentMoviesFreeze;

    //Window
    int WindowWidth;
    int WindowHeight;
    int WindowPosX;
    int WindowPosY;

    //Rom Browser
    int RomBrowserSortColumn;
    char RomBrowserSortMethod[10];
    BOOL RomBrowserRecursion;

    //Recording options
    bool NoReset;

    //avi options
    bool forceInternalCapture;
    bool captureOtherWindows;
    bool allowUnknownHotkeySelection;
    HOTKEY hotkey[NUM_HOTKEYS];
    char AviCapturePath[MAX_PATH];

    //Lua
    char LuaScriptPath[MAX_PATH];
    char RecentScripts[LUA_MAX_RECENT][MAX_PATH];
    BOOL RecentScriptsFreeze;
    BOOL LuaSimpleDialog;
    BOOL LuaWarnOnClose;
    BOOL FrequentVCRUIRefresh;
    int SyncMode;
} CONFIG;

extern "C" CONFIG Config;

void ApplyHotkeys();
void hotkeyToString(HOTKEY* hotkey, char* buf);

void WriteCfgString   (const char *Section, const char *Key,char *Value) ;
void WriteCfgInt      (const char *Section, const char *Key,int Value) ;
void ReadCfgString    (const char *Section, const char *Key, const char *DefaultValue,char *retValue) ;
int ReadCfgInt        (const char *Section, const char *Key,int DefaultValue) ;

extern int round_to_zero;
extern int emulate_float_crashes;
extern int input_delay;
extern int LUA_double_buffered; 
extern NamedHotkey namedHotkeys[];
extern const int namedHotkeyCount;

void LoadConfig()  ;
void SaveConfig()  ; 
    
