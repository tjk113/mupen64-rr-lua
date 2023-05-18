/***************************************************************************
                          config.c  -  description
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

  /*
  #if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0500)
  #undef _WIN32_WINNT
  #define _WIN32_WINNT 0x0500
  #endif
  */

#include <Windows.h>
#include <winuser.h>
#include <stdio.h>
#include "rombrowser.h"
#include "commandline.h"
#include "../../winproject/resource.h"
#include "config.h"

#include "../lua/Recent.h"
#include <vcr.h>
#include "translation.h"

#define CfgFileName "mupen64.cfg"

extern char AppPath[];
extern HWND mainHWND;
extern HWND hRomList;

extern int no_audio_delay;
extern int no_compiled_jump;

int input_delay;
int LUA_double_buffered;

NamedHotkey namedHotkeys[] = {
    {
        .name = "Fast-forward",
        .hotkeys = &Config.hotkeys[0],
    },
    {
        .name = "Speed up",
        .hotkeys = &Config.hotkeys[1],
    },
    {
        .name = "Speed down",
        .hotkeys = &Config.hotkeys[2],
    },
    {
        .name = "Frame advance",
        .hotkeys = &Config.hotkeys[3],
    },
    {
        .name = "Pause",
        .hotkeys = &Config.hotkeys[4],
    },
    {
        .name = "Toggle read-only mode",
        .hotkeys = &Config.hotkeys[5],
    },
    {
        .name = "Start movie playback",
        .hotkeys = &Config.hotkeys[6],
    },
    {
        .name = "Stop movie playback",
        .hotkeys = &Config.hotkeys[7],
    },
    {
        .name = "Start movie recording",
        .hotkeys = &Config.hotkeys[8],
    },
    {
        .name = "Stop movie recording",
        .hotkeys = &Config.hotkeys[9],
    },
    {
        .name = "Take screenshot",
        .hotkeys = &Config.hotkeys[10],
    },
    {
        .name = "Save to current slot",
        .hotkeys = &Config.hotkeys[11],
    },
    {
        .name = "Load from current slot",
        .hotkeys = &Config.hotkeys[12],
    },
    {
        .name = "Save to slot 1",
        .hotkeys = &Config.hotkeys[13],
    },
    {
        .name = "Save to slot 2",
        .hotkeys = &Config.hotkeys[14],
    },
    {
        .name = "Save to slot 3",
        .hotkeys = &Config.hotkeys[15],
    },
    {
        .name = "Save to slot 4",
        .hotkeys = &Config.hotkeys[16],
    },
    {
        .name = "Save to slot 5",
        .hotkeys = &Config.hotkeys[17],
    },
    {
        .name = "Save to slot 6",
        .hotkeys = &Config.hotkeys[18],
    },
    {
        .name = "Save to slot 7",
        .hotkeys = &Config.hotkeys[19],
    },
    {
        .name = "Save to slot 8",
        .hotkeys = &Config.hotkeys[20],
    },
    {
        .name = "Save to slot 9",
        .hotkeys = &Config.hotkeys[21],
    },
    {
        .name = "Save to slot 10",
        .hotkeys = &Config.hotkeys[22],
    },

    {
        .name = "Load from slot 1",
        .hotkeys = &Config.hotkeys[23],
    },
    {
        .name = "Load from slot 2",
        .hotkeys = &Config.hotkeys[24],
    },
    {
        .name = "Load from slot 3",
        .hotkeys = &Config.hotkeys[25],
    },
    {
        .name = "Load from slot 4",
        .hotkeys = &Config.hotkeys[26],
    },
    {
        .name = "Load from slot 5",
        .hotkeys = &Config.hotkeys[27],
    },
    {
        .name = "Load from slot 6",
        .hotkeys = &Config.hotkeys[28],
    },
    {
        .name = "Load from slot 7",
        .hotkeys = &Config.hotkeys[29],
    },
    {
        .name = "Load from slot 8",
        .hotkeys = &Config.hotkeys[30],
    },
    {
        .name = "Load from slot 9",
        .hotkeys = &Config.hotkeys[31],
    },
	{
		.name = "Load from slot 10",
		.hotkeys = &Config.hotkeys[32],
	},


    {
        .name = "Select slot 1",
        .hotkeys = &Config.hotkeys[33],
    },
    {
        .name = "Select slot 2",
        .hotkeys = &Config.hotkeys[34],
    },
    {
        .name = "Select slot 3",
        .hotkeys = &Config.hotkeys[36],
    },
    {
        .name = "Select slot 4",
        .hotkeys = &Config.hotkeys[37],
    },
    {
        .name = "Select slot 5",
        .hotkeys = &Config.hotkeys[38],
    },
    {
        .name = "Select slot 6",
        .hotkeys = &Config.hotkeys[39],
    },
    {
        .name = "Select slot 7",
        .hotkeys = &Config.hotkeys[40],
    },
    {
        .name = "Select slot 8",
        .hotkeys = &Config.hotkeys[41],
    },
    {
        .name = "Select slot 9",
        .hotkeys = &Config.hotkeys[42],
    },
	{
		.name = "Select slot 10",
		.hotkeys = &Config.hotkeys[43],
	},

};
const int namedHotkeyCount = sizeof(namedHotkeys) / sizeof(NamedHotkey);

////////////////////// Service functions and structures ////////////////////////

CONFIG Config;

// is this the best way to handle this?
int* autoinc_save_slot = &Config.is_slot_autoincrement_enabled;


void hotkeyToString(HOTKEY* hotkeys, char* buf)
{
    int k = hotkeys->key;
    buf[0] = 0;

    if (!hotkeys->ctrl && !hotkeys->shift && !hotkeys->alt && !hotkeys->key)
    {
        strcpy(buf, "(nothing)");
        return;
    }

    if (hotkeys->ctrl)
        strcat(buf, "Ctrl ");
    if (hotkeys->shift)
        strcat(buf, "Shift ");
    if (hotkeys->alt)
        strcat(buf, "Alt ");
    if (k)
    {
        char buf2[32];
        if ((k >= '0' && k <= '9') || (k >= 'A' && k <= 'Z'))
            sprintf(buf2, "%c", (char)k);
        else if ((k >= VK_F1 && k <= VK_F24))
            sprintf(buf2, "F%d", k - (VK_F1 - 1));
        else if ((k >= VK_NUMPAD0 && k <= VK_NUMPAD9))
            sprintf(buf2, "Num%d", k - VK_NUMPAD0);
        else switch (k)
        {
        case VK_SPACE: strcpy(buf2, "Space"); break;
        case VK_BACK: strcpy(buf2, "Backspace"); break;
        case VK_TAB: strcpy(buf2, "Tab"); break;
        case VK_CLEAR: strcpy(buf2, "Clear"); break;
        case VK_RETURN: strcpy(buf2, "Enter"); break;
        case VK_PAUSE: strcpy(buf2, "Pause"); break;
        case VK_CAPITAL: strcpy(buf2, "Caps"); break;
        case VK_PRIOR: strcpy(buf2, "PageUp"); break;
        case VK_NEXT: strcpy(buf2, "PageDn"); break;
        case VK_END: strcpy(buf2, "End"); break;
        case VK_HOME: strcpy(buf2, "Home"); break;
        case VK_LEFT: strcpy(buf2, "Left"); break;
        case VK_UP: strcpy(buf2, "Up"); break;
        case VK_RIGHT: strcpy(buf2, "Right"); break;
        case VK_DOWN: strcpy(buf2, "Down"); break;
        case VK_SELECT: strcpy(buf2, "Select"); break;
        case VK_PRINT: strcpy(buf2, "Print"); break;
        case VK_SNAPSHOT: strcpy(buf2, "PrintScrn"); break;
        case VK_INSERT: strcpy(buf2, "Insert"); break;
        case VK_DELETE: strcpy(buf2, "Delete"); break;
        case VK_HELP: strcpy(buf2, "Help"); break;
        case VK_MULTIPLY: strcpy(buf2, "Num*"); break;
        case VK_ADD: strcpy(buf2, "Num+"); break;
        case VK_SUBTRACT: strcpy(buf2, "Num-"); break;
        case VK_DECIMAL: strcpy(buf2, "Num."); break;
        case VK_DIVIDE: strcpy(buf2, "Num/"); break;
        case VK_NUMLOCK: strcpy(buf2, "NumLock"); break;
        case VK_SCROLL: strcpy(buf2, "ScrollLock"); break;
        case /*VK_OEM_PLUS*/0xBB: strcpy(buf2, "=+"); break;
        case /*VK_OEM_MINUS*/0xBD: strcpy(buf2, "-_"); break;
        case /*VK_OEM_COMMA*/0xBC: strcpy(buf2, ","); break;
        case /*VK_OEM_PERIOD*/0xBE: strcpy(buf2, "."); break;
        case VK_OEM_7: strcpy(buf2, "'\""); break;
        case VK_OEM_6: strcpy(buf2, "]}"); break;
        case VK_OEM_5: strcpy(buf2, "\\|"); break;
        case VK_OEM_4: strcpy(buf2, "[{"); break;
        case VK_OEM_3: strcpy(buf2, "`~"); break;
        case VK_OEM_2: strcpy(buf2, "/?"); break;
        case VK_OEM_1: strcpy(buf2, ";:"); break;
        default:
            sprintf(buf2, "(%d)", k);
            break;
        }
        strcat(buf, buf2);
    }
}

void SetDlgItemHotkey(HWND hwnd, int idc, HOTKEY* hotkeys)
{
    char buf[64];
    hotkeyToString(hotkeys, buf);
    SetDlgItemText(hwnd, idc, buf);
}

void SetDlgItemHotkeyAndMenu(HWND hwnd, int idc, HOTKEY* hotkeys, HMENU hmenu, int menuItemID)
{
    char buf[64];
    hotkeyToString(hotkeys, buf);
    SetDlgItemText(hwnd, idc, buf);

    if (hmenu && menuItemID >= 0)
    {
        if (strcmp(buf, "(nothing)"))
            SetMenuAccelerator(hmenu, menuItemID, buf);
        else
            SetMenuAccelerator(hmenu, menuItemID, "");
    }
}

void ApplyHotkeys() {

    SetDlgItemHotkey(mainHWND, IDC_HOT_FASTFORWARD, &Config.hotkeys[0]);
    SetDlgItemHotkey(mainHWND, IDC_HOT_SPEEDUP, &Config.hotkeys[1]);
    SetDlgItemHotkey(mainHWND, IDC_HOT_SPEEDDOWN, &Config.hotkeys[2]);
    SetDlgItemHotkeyAndMenu(mainHWND, IDC_HOT_FRAMEADVANCE, &Config.hotkeys[3], GetSubMenu(GetMenu(mainHWND), 1), 1);
    SetDlgItemHotkeyAndMenu(mainHWND, IDC_HOT_PAUSE, &Config.hotkeys[4], GetSubMenu(GetMenu(mainHWND), 1), 0);
    SetDlgItemHotkeyAndMenu(mainHWND, IDC_HOT_READONLY, &Config.hotkeys[5], GetSubMenu(GetMenu(mainHWND), 3), 15);
    SetDlgItemHotkeyAndMenu(mainHWND, IDC_HOT_PLAY, &Config.hotkeys[6], GetSubMenu(GetMenu(mainHWND), 3), 3);
    SetDlgItemHotkeyAndMenu(mainHWND, IDC_HOT_PLAYSTOP, &Config.hotkeys[7], GetSubMenu(GetMenu(mainHWND), 3), 4);
    SetDlgItemHotkeyAndMenu(mainHWND, IDC_HOT_RECORD, &Config.hotkeys[8], GetSubMenu(GetMenu(mainHWND), 3), 0);
    SetDlgItemHotkeyAndMenu(mainHWND, IDC_HOT_RECORDSTOP, &Config.hotkeys[9], GetSubMenu(GetMenu(mainHWND), 3), 1);
    SetDlgItemHotkeyAndMenu(mainHWND, IDC_HOT_SCREENSHOT, &Config.hotkeys[10], GetSubMenu(GetMenu(mainHWND), 1), 2);
    SetDlgItemHotkeyAndMenu(mainHWND, IDC_CSAVE, &Config.hotkeys[11], GetSubMenu(GetMenu(mainHWND), 1), 4);
    SetDlgItemHotkeyAndMenu(mainHWND, IDC_CLOAD, &Config.hotkeys[12], GetSubMenu(GetMenu(mainHWND), 1), 6);

    SetDlgItemHotkey(mainHWND, IDC_1SAVE, &Config.hotkeys[13]);
    SetDlgItemHotkey(mainHWND, IDC_2SAVE, &Config.hotkeys[14]);
    SetDlgItemHotkey(mainHWND, IDC_3SAVE, &Config.hotkeys[15]);
    SetDlgItemHotkey(mainHWND, IDC_4SAVE, &Config.hotkeys[16]);
    SetDlgItemHotkey(mainHWND, IDC_5SAVE, &Config.hotkeys[17]);
    SetDlgItemHotkey(mainHWND, IDC_6SAVE, &Config.hotkeys[18]);
    SetDlgItemHotkey(mainHWND, IDC_7SAVE, &Config.hotkeys[19]);
    SetDlgItemHotkey(mainHWND, IDC_8SAVE, &Config.hotkeys[20]);
    SetDlgItemHotkey(mainHWND, IDC_9SAVE, &Config.hotkeys[21]);
	SetDlgItemHotkey(mainHWND, IDC_10SAVE,&Config.hotkeys[22]);

    SetDlgItemHotkey(mainHWND, IDC_1LOAD, &Config.hotkeys[23]);
    SetDlgItemHotkey(mainHWND, IDC_2LOAD, &Config.hotkeys[24]);
    SetDlgItemHotkey(mainHWND, IDC_3LOAD, &Config.hotkeys[25]);
    SetDlgItemHotkey(mainHWND, IDC_4LOAD, &Config.hotkeys[26]);
    SetDlgItemHotkey(mainHWND, IDC_5LOAD, &Config.hotkeys[27]);
    SetDlgItemHotkey(mainHWND, IDC_6LOAD, &Config.hotkeys[28]);
    SetDlgItemHotkey(mainHWND, IDC_7LOAD, &Config.hotkeys[29]);
    SetDlgItemHotkey(mainHWND, IDC_8LOAD, &Config.hotkeys[30]);
    SetDlgItemHotkey(mainHWND, IDC_9LOAD, &Config.hotkeys[31]);
	SetDlgItemHotkey(mainHWND, IDC_10LOAD,&Config.hotkeys[32]);

    SetDlgItemHotkeyAndMenu(mainHWND, IDC_1SEL, &Config.hotkeys[33], GetSubMenu(GetSubMenu(GetMenu(mainHWND), 1), 9), 0);
    SetDlgItemHotkeyAndMenu(mainHWND, IDC_2SEL, &Config.hotkeys[34], GetSubMenu(GetSubMenu(GetMenu(mainHWND), 1), 9), 1);
    SetDlgItemHotkeyAndMenu(mainHWND, IDC_3SEL, &Config.hotkeys[35], GetSubMenu(GetSubMenu(GetMenu(mainHWND), 1), 9), 2);
    SetDlgItemHotkeyAndMenu(mainHWND, IDC_4SEL, &Config.hotkeys[36], GetSubMenu(GetSubMenu(GetMenu(mainHWND), 1), 9), 3);
    SetDlgItemHotkeyAndMenu(mainHWND, IDC_5SEL, &Config.hotkeys[37], GetSubMenu(GetSubMenu(GetMenu(mainHWND), 1), 9), 4);
    SetDlgItemHotkeyAndMenu(mainHWND, IDC_6SEL, &Config.hotkeys[38], GetSubMenu(GetSubMenu(GetMenu(mainHWND), 1), 9), 5);
    SetDlgItemHotkeyAndMenu(mainHWND, IDC_7SEL, &Config.hotkeys[39], GetSubMenu(GetSubMenu(GetMenu(mainHWND), 1), 9), 6);
    SetDlgItemHotkeyAndMenu(mainHWND, IDC_8SEL, &Config.hotkeys[40], GetSubMenu(GetSubMenu(GetMenu(mainHWND), 1), 9), 7);
    SetDlgItemHotkeyAndMenu(mainHWND, IDC_9SEL, &Config.hotkeys[41], GetSubMenu(GetSubMenu(GetMenu(mainHWND), 1), 9), 8);
	SetDlgItemHotkeyAndMenu(mainHWND, IDC_10SEL,&Config.hotkeys[42], GetSubMenu(GetSubMenu(GetMenu(mainHWND), 1), 9), 9);

}

char* CfgFilePath()
{
    static char* cfgpath = NULL;
    if (cfgpath == NULL)
    {
        cfgpath = (char*)malloc(strlen(AppPath) + 1 + strlen(CfgFileName));
        strcpy(cfgpath, AppPath);
        strcat(cfgpath, CfgFileName);
    }
    return cfgpath;
}


void WriteCfgString(const char* Section, const char* Key, char* Value)
{
    WritePrivateProfileString(Section, Key, Value, CfgFilePath());
}


void WriteCfgInt(const char* Section, const char* Key, int Value)
{
    static char TempStr[20];
    sprintf(TempStr, "%d", Value);
    WriteCfgString(Section, Key, TempStr);
}


void ReadCfgString(const char* Section, const char* Key, const char* DefaultValue, char* retValue)
{
    GetPrivateProfileString(Section, Key, DefaultValue, retValue, MAX_PATH, CfgFilePath());
}


int ReadCfgInt(const char* Section, const char* Key, int DefaultValue)
{
    return GetPrivateProfileInt(Section, Key, DefaultValue, CfgFilePath());
}

//////////////////////////// Load and Save Config //////////////////////////////

// Loads recent roms from .cfg
void LoadRecentRoms()
{
    int i;
    char tempStr[32];

    Config.is_recent_rom_paths_frozen = ReadCfgInt("Recent Roms", "Freeze", 0);
    for (i = 0; i < MAX_RECENT_ROMS; i++)
    {

        sprintf(tempStr, "RecentRom%d", i);
        ReadCfgString("Recent Roms", tempStr, "", Config.recent_rom_paths[i]);
        //        FILE* f = fopen(tempStr, "wb");
        //        fputs(Config.recent_rom_paths[i], f);
        //        fclose(f);
    }

}

//Loads recent scripts from .cfg
void LoadRecentScripts()
{
    char tempStr[32];
    Config.is_recent_scripts_frozen = ReadCfgInt("Recent Scripts", "Freeze", 0);
    for (unsigned i = 0; i < LUA_MAX_RECENT; i++)
    {

        sprintf(tempStr, "RecentLua%d", i);
        ReadCfgString("Recent Scripts", tempStr, "", Config.recent_lua_script_paths[i]);
    }

}

// Loads recent movies from .cfg
void LoadRecentMovies()
{
    char tempStr[32];
    Config.is_recent_movie_paths_frozen = ReadCfgInt("Recent Movies", "Freeze", 0);
    for (unsigned i = 0; i < MAX_RECENT_MOVIE; i++)
    {
        sprintf(tempStr, "RecentMovie%d", i);
        ReadCfgString("Recent Movies", tempStr, "", Config.recent_movie_paths[i]);
    }
}

void ReadHotkeyConfig(int n, const  char* name, int cmd, int def) {
    HOTKEY* h;
    char t[128];
    h = &Config.hotkeys[n];
    sprintf(t, "%s Key", name);
    h->key = ReadCfgInt("Hotkeys", t, def & 0xFF);
    sprintf(t, "%s Shift", name);
    h->shift = ReadCfgInt("Hotkeys", t, def & 0x100 ? 1 : 0);
    sprintf(t, "%s Ctrl", name);
    h->ctrl = ReadCfgInt("Hotkeys", t, def & 0x200 ? 1 : 0);
    sprintf(t, "%s Alt", name);
    h->alt = ReadCfgInt("Hotkeys", t, def & 0x400 ? 1 : 0);
    h->command = cmd;
}
void WriteHotkeyConfig(int n, const char* name) {
    HOTKEY* h;
    char t[128];
    h = &Config.hotkeys[n];
    sprintf(t, "%s Key", name);
    WriteCfgInt("Hotkeys", t, h->key);
    sprintf(t, "%s Shift", name);
    WriteCfgInt("Hotkeys", t, h->shift);
    sprintf(t, "%s Ctrl", name);
    WriteCfgInt("Hotkeys", t, h->ctrl);
    sprintf(t, "%s Alt", name);
    WriteCfgInt("Hotkeys", t, h->alt);
}

void LoadConfig()
{
    LoadRecentRoms();
    LoadRecentScripts();
    LoadRecentMovies();

    // Language
    ReadCfgString("Language", "Default", "English", Config.language);

    //Window position and size 
    Config.window_width = ReadCfgInt("Window", "Width", 640);
    Config.window_height = ReadCfgInt("Window", "Height", 480);
    Config.window_x = ReadCfgInt("Window", "X", (GetSystemMetrics(SM_CXSCREEN) - Config.window_width) / 2);
    Config.window_y = ReadCfgInt("Window", "Y", (GetSystemMetrics(SM_CYSCREEN) - Config.window_height) / 2);
    //if mupen was closed by minimising
    if (Config.window_x < MIN_WINDOW_W-1 || Config.window_width < MIN_WINDOW_H-1) {
        printf("window too small. attempting to fix\n");
        Config.window_width = 800;
        Config.window_height = 600;
        Config.window_x = (GetSystemMetrics(SM_CXSCREEN) - Config.window_width) / 2;
        Config.window_y = (GetSystemMetrics(SM_CYSCREEN) - Config.window_height) / 2;
    }

    //General Vars
    Config.show_fps = ReadCfgInt("General", "Show FPS", 1);
    Config.show_vis_per_second = ReadCfgInt("General", "Show VIS", 1);
    Config.prevent_suspicious_rom_loading = ReadCfgInt("General", "Manage Bad ROMs", 1);
    Config.is_savestate_warning_enabled = ReadCfgInt("General", "Alert Saves errors", 1);
    Config.is_fps_limited = ReadCfgInt("General", "Limit Fps", 1);
    Config.is_ini_compressed = ReadCfgInt("General", "Compressed Ini", 1);
    Config.is_fps_modifier_enabled = ReadCfgInt("General", "Use Fps Modifier", 1);
    if (Config.fps_modifier < 5) {
        Config.fps_modifier = 5;
    }
    if (Config.fps_modifier > 1000) {
        Config.fps_modifier = 1000;
    }
    Config.fps_modifier = ReadCfgInt("General", "Fps Modifier", 100);
	Config.frame_skip_frequency = ReadCfgInt("General", "Skip Frequency", 8);
	Config.is_movie_loop_enabled = ReadCfgInt("General", "Loop Movie", 0);
    Config.is_frame_count_visual_zero_index = ReadCfgInt("General", "Zero index", 0);


    Config.core_type = ReadCfgInt("CPU", "Core", 1);


#define NAMEOF(v) ((void*)&v, #v)
    printf("%s\n", NAMEOF(Config.is_capture_cropped_screen_dc));
    // TODO: make config section and entry r/w automatic
    
    //Advanced vars
    Config.is_fullscreen_start_enabled = ReadCfgInt("Advanced", "Start Full Screen", 0);
    Config.is_unfocused_pause_enabled = ReadCfgInt("Advanced", "Pause when not active", 1);
    Config.use_global_plugins = ReadCfgInt("Advanced", "Overwrite Plugins Settings", 0);
    Config.is_toolbar_enabled = ReadCfgInt("Advanced", "Use Toolbar", 0);
    Config.is_statusbar_enabled = ReadCfgInt("Advanced", "Use Statusbar", 1);
    Config.is_slot_autoincrement_enabled = ReadCfgInt("Advanced", "Auto Increment Save Slot", 0);
    Config.is_round_towards_zero_enabled = ReadCfgInt("Advanced", "Round To Zero", 0);
    Config.is_float_exception_propagation_enabled = ReadCfgInt("Advanced", "Emulate Float Crashes", 0);
    Config.is_input_delay_enabled = ReadCfgInt("Advanced", "Old Input Delay", 0);
    Config.is_lua_double_buffered = ReadCfgInt("Advanced", "LUA Double Buffer", 1);
	Config.is_state_independent_state_loading_allowed = ReadCfgInt("Advanced", "Allow arbitrary savestate loading", 0);

    //Compatibility Settings
    no_audio_delay = ReadCfgInt("Compatibility", "No Audio Delay", 0);
    no_compiled_jump = ReadCfgInt("Compatibility", "No Compiled Jump", 0);

    //RomBrowser Columns
    Config.is_good_name_column_enabled = ReadCfgInt("Rom Browser Columns", "Good Name", 1);
    Config.is_internal_name_column_enabled = ReadCfgInt("Rom Browser Columns", "Internal Name", 0);
    Config.is_country_column_enabled = ReadCfgInt("Rom Browser Columns", "Country", 1);
    Config.is_size_column_enabled = ReadCfgInt("Rom Browser Columns", "Size", 1);
    Config.is_comments_column_enabled = ReadCfgInt("Rom Browser Columns", "Comments", 1);
    Config.is_filename_column_enabled = ReadCfgInt("Rom Browser Columns", "File Name", 0);
    Config.is_md5_column_enabled = ReadCfgInt("Rom Browser Columns", "MD5", 0);

    // Directories
    Config.is_default_plugins_directory_used = ReadCfgInt("Directories", "Default Plugins Directory", 1);
    Config.is_default_saves_directory_used = ReadCfgInt("Directories", "Default Saves Directory", 1);
    Config.is_default_screenshots_directory_used = ReadCfgInt("Directories", "Default Screenshots Directory", 1);

    sprintf(Config.plugins_directory, "%sPlugin\\", AppPath);
    ReadCfgString("Directories", "Plugins Directory", Config.plugins_directory, Config.plugins_directory);

    sprintf(Config.saves_directory, "%sSave\\", AppPath);
    ReadCfgString("Directories", "Saves Directory", Config.saves_directory, Config.saves_directory);

    sprintf(Config.screenshots_directory, "%sScreenShots\\", AppPath);
    ReadCfgString("Directories", "Screenshots Directory", Config.screenshots_directory, Config.screenshots_directory);
    ReadCfgString("Directories", "SaveLoadAsandSaveStateAsPath", Config.states_path, Config.states_path);

    // Rom Browser
    Config.rom_browser_sorted_column = ReadCfgInt("Rom Browser", "Sort Column", 0);
    Config.is_rom_browser_recursion_enabled = ReadCfgInt("Rom Browser", "Recursion", 0);
    ReadCfgString("Rom Browser", "Sort Method", "ASC", Config.rom_browser_sort_method);

    // Recording Options
    Config.is_reset_recording_disabled = ReadCfgInt("Recording Options", "No reset recording", 1);

    //avi options
    Config.is_internal_capture_forced = ReadCfgInt("Avi Options", "Force internal capture", 0);
    Config.is_capture_cropped_screen_dc = ReadCfgInt("Avi Options", "Capture other windows", 0);
    ReadCfgString("Avi Options", "Avi Capture Path", "", Config.avi_capture_path);

    // other
    Config.is_lua_exit_confirm_enabled = ReadCfgInt("Other", "Ask on lua close", 0);
    Config.is_lua_simple_dialog_enabled = ReadCfgInt("Other", "Simplified lua", 0);
    Config.is_movie_backup_enabled = ReadCfgInt("Other", "Autobackup", 0);
    Config.movie_backup_level = ReadCfgInt("Other", "Autobackup level", 0);
    Config.is_statusbar_frequent_refresh_enabled = ReadCfgInt("Other", "Fast statusbar updates", 0);
    Config.synchronization_mode = ReadCfgInt("Other", "Synchronisation Mode", VCR_SYNC_AUDIO_DUPL);
    Config.is_rom_movie_compatibility_check_enabled = ReadCfgInt("General", "Alert Movies errors", 1); //moviesERRORS == 1 -> the errors are shown
    Config.cpu_clock_speed_multiplier = ReadCfgInt("Other", "CPU Clock Speed Multiplier", 1);

    // Load A Whole Whackton Of Hotkeys:
    /*
    OR'd Hex Codes Legend:
    0x100 = Ctrl
    0x200 = Shift
    0x400 = Alt
    0x300 = Ctrl + Shift
    0x600 = Ctrl + Alt
    */

    ReadHotkeyConfig(0, "Fast Forward", 0, VK_TAB);	// handled specially
    ReadHotkeyConfig(1, "Speed Up", IDC_INCREASE_MODIFIER, /*VK_OEM_PLUS*/0xBB);
    ReadHotkeyConfig(2, "Slow Down", IDC_DECREASE_MODIFIER, /*VK_OEM_MINUS*/0xBD);
    ReadHotkeyConfig(3, "Frame Advance", EMU_FRAMEADVANCE, VK_OEM_5);
    ReadHotkeyConfig(4, "Pause Resume", EMU_PAUSE, VK_PAUSE);
    ReadHotkeyConfig(5, "ReadOnly", EMU_VCRTOGGLEREADONLY, '8' | 0x100);
    ReadHotkeyConfig(6, "Play", ID_START_PLAYBACK, 'P' | 0x300);
    ReadHotkeyConfig(7, "PlayStop", ID_STOP_PLAYBACK, 'S' | 0x300);
    ReadHotkeyConfig(8, "Record", ID_START_RECORD, 'R' | 0x300);
    ReadHotkeyConfig(9, "RecordStop", ID_STOP_RECORD, 'S' | 0x300);
    ReadHotkeyConfig(10, "Screenshot", GENERATE_BITMAP, VK_F12);
    ReadHotkeyConfig(11, "Save Current", STATE_SAVE, VK_F5 | 0x200);
    ReadHotkeyConfig(12, "Load Current", STATE_RESTORE, VK_F7 | 0x200);

    // Save/Load Hotkeys
    {
        int i;
        char str[128];

        for (i = 1; i <= 10; i++)
        {
            sprintf(str, "Save %d", i);
            ReadHotkeyConfig(12 + i, str, (ID_SAVE_1 - 1) + i, ((VK_F1 - 1) + i) | 0x100);
        }
        for (i = 1; i <= 10; i++)
        {
            sprintf(str, "Load %d", i);
            ReadHotkeyConfig(22 + i, str, (ID_LOAD_1 - 1) + i, (VK_F1 - 1) + i);
        }
        for (i = 1; i <= 10; i++)
        {
            sprintf(str, "Select %d", i);
            ReadHotkeyConfig(32 + i, str, (ID_CURRENTSAVE_1 - 1) + i, '0' + i);
        }
    }
    //Lua
    //ダイアログに追加するの面倒くさい
    ReadCfgString("Lua", "Script Path", "", Config.lua_script_path);
    ReadHotkeyConfig(43, "Lua Script Reload", ID_LUA_RELOAD, VK_F3 | 0x200);
    ReadHotkeyConfig(44, "Lua Script CloseAll", ID_MENU_LUASCRIPT_CLOSEALL, VK_F4 | 0x200);
    ReadHotkeyConfig(48, "Load Latest Lua Script", ID_LUA_LOAD_LATEST, 'L' | 0x600);

    // some new misc ones
    ReadHotkeyConfig(45, "Start From Beginning", ID_RESTART_MOVIE, 'H' | 0x300);
    ReadHotkeyConfig(46, "Load Latest Movie", ID_REPLAY_LATEST, 'G' | 0x300);
    ReadHotkeyConfig(47, "Load Latest ROM", ID_LOAD_LATEST, 'L' | 0x300);


}

/////////////////////////////////////////////////////////////////////////////////

void saveWindowSettings()
{
    RECT rcMain;
    GetWindowRect(mainHWND, &rcMain);
    Config.window_x = rcMain.left;
    Config.window_y = rcMain.top;
    Config.window_width = rcMain.right - rcMain.left;
    Config.window_height = rcMain.bottom - rcMain.top;
    WriteCfgInt("Window", "Width", Config.window_width);
    WriteCfgInt("Window", "Height", Config.window_height);
    WriteCfgInt("Window", "X", Config.window_x);
    WriteCfgInt("Window", "Y", Config.window_y);
}

void saveBrowserSettings()
{
    int Column, ColWidth, index;
    index = 0;
    for (Column = 0; Column < ROM_COLUMN_FIELDS; Column++) {
        if (isFieldInBrowser(Column)) {
            ColWidth = ListView_GetColumnWidth(hRomList, index);
            WriteCfgInt("Rom Browser", getFieldName(Column), ColWidth);
            index++;
        }
    }

    WriteCfgInt("Rom Browser", "Sort Column", Config.rom_browser_sorted_column);
    WriteCfgInt("Rom Browser", "Recursion", Config.is_rom_browser_recursion_enabled);
    WriteCfgString("Rom Browser", "Sort Method", Config.rom_browser_sort_method);

}

void SaveRecentRoms()
{
    int i;
    char tempStr[50];

    WriteCfgInt("Recent Roms", "Freeze", Config.is_recent_rom_paths_frozen);
    for (i = 0; i < MAX_RECENT_ROMS; i++)
    {
        sprintf(tempStr, "RecentRom%d", i);
        WriteCfgString("Recent Roms", tempStr, Config.recent_rom_paths[i]);
    }
}

void SaveRecentScripts()
{
    char tempStr[32];
    WriteCfgInt("Recent Scripts", "Freeze", Config.is_recent_scripts_frozen);
    for (unsigned i = 0; i < LUA_MAX_RECENT; i++)
    {
        sprintf(tempStr, "RecentLua%d", i);
        WriteCfgString("Recent Scripts", tempStr, Config.recent_lua_script_paths[i]);
    }
}

void SaveRecentMovies()
{
	char tempStr[32];
	WriteCfgInt("Recent Movies", "Freeze", Config.is_recent_movie_paths_frozen);
	for (unsigned i = 0; i < MAX_RECENT_MOVIE; i++)
	{
		sprintf(tempStr, "RecentMovie%d", i);
		WriteCfgString("Recent Movies", tempStr, Config.recent_movie_paths[i]);
	}
}

void SaveConfig()
{
    saveWindowSettings();

    if (!cmdlineNoGui) {
        saveBrowserSettings();
        SaveRecentRoms();
        SaveRecentScripts();
        SaveRecentMovies();
    }

    //Language
    WriteCfgString("Language", "Default", Config.language);

    //General Vars
    WriteCfgInt("General", "Show FPS", Config.show_fps);
    WriteCfgInt("General", "Show VIS", Config.show_vis_per_second);
    WriteCfgInt("General", "Manage Bad ROMs", Config.prevent_suspicious_rom_loading);
    WriteCfgInt("General", "Alert Saves errors", Config.is_savestate_warning_enabled);
    WriteCfgInt("General", "Alert Movies errors", Config.is_rom_movie_compatibility_check_enabled);
    WriteCfgInt("General", "Limit Fps", Config.is_fps_limited);
    WriteCfgInt("General", "Compressed Ini", Config.is_ini_compressed);
    WriteCfgInt("General", "Fps Modifier", Config.fps_modifier);
    WriteCfgInt("General", "Use Fps Modifier", Config.is_fps_modifier_enabled);
	WriteCfgInt("General", "Skip Frequency", Config.frame_skip_frequency);
	WriteCfgInt("General", "Loop Movie", Config.is_movie_loop_enabled);
    WriteCfgInt("General", "Zero Index", Config.is_frame_count_visual_zero_index);

    //Advanced Vars
    WriteCfgInt("Advanced", "Start Full Screen", Config.is_fullscreen_start_enabled);
    WriteCfgInt("Advanced", "Pause when not active", Config.is_unfocused_pause_enabled);
    WriteCfgInt("Advanced", "Overwrite Plugins Settings", Config.use_global_plugins);
    WriteCfgInt("Advanced", "Use Toolbar", Config.is_toolbar_enabled);
    WriteCfgInt("Advanced", "Use Statusbar", Config.is_statusbar_enabled);
    WriteCfgInt("Advanced", "Auto Increment Save Slot", Config.is_slot_autoincrement_enabled);
    WriteCfgInt("Advanced", "Round To Zero", Config.is_round_towards_zero_enabled);
    WriteCfgInt("Advanced", "Emulate Float Crashes", Config.is_float_exception_propagation_enabled);
    WriteCfgInt("Advanced", "Old Input Delay", Config.is_input_delay_enabled);
	WriteCfgInt("Advanced", "LUA Double Buffer", Config.is_lua_double_buffered);
    WriteCfgInt("Advanced", "Allow arbitrary savestate loading", Config.is_state_independent_state_loading_allowed);
    
    WriteCfgInt("CPU", "Core", Config.core_type);

    //Compatibility Settings
    WriteCfgInt("Compatibility", "No Audio Delay", no_audio_delay);
    WriteCfgInt("Compatibility", "No Compiled Jump", no_compiled_jump);

    //Rom Browser columns
    WriteCfgInt("Rom Browser Columns", "Good Name", Config.is_good_name_column_enabled);
    WriteCfgInt("Rom Browser Columns", "Internal Name", Config.is_internal_name_column_enabled);
    WriteCfgInt("Rom Browser Columns", "Country", Config.is_country_column_enabled);
    WriteCfgInt("Rom Browser Columns", "Size", Config.is_size_column_enabled);
    WriteCfgInt("Rom Browser Columns", "Comments", Config.is_comments_column_enabled);
    WriteCfgInt("Rom Browser Columns", "File Name", Config.is_filename_column_enabled);
    WriteCfgInt("Rom Browser Columns", "MD5", Config.is_md5_column_enabled);

    // Directories
    WriteCfgInt("Directories", "Default Plugins Directory", Config.is_default_plugins_directory_used);
    WriteCfgString("Directories", "Plugins Directory", Config.plugins_directory);
    WriteCfgInt("Directories", "Default Saves Directory", Config.is_default_saves_directory_used);
    WriteCfgString("Directories", "Saves Directory", Config.saves_directory);
    WriteCfgInt("Directories", "Default Screenshots Directory", Config.is_default_screenshots_directory_used);
    WriteCfgString("Directories", "Screenshots Directory", Config.screenshots_directory);
    WriteCfgString("Directories", "SaveLoadAsandSaveStateAsPath", Config.states_path);

    WriteCfgInt("Recording Options", "No reset recording", Config.is_reset_recording_disabled);

    WriteCfgInt("Avi Options", "Force internal capture", Config.is_internal_capture_forced);
    WriteCfgInt("Avi Options", "Capture other windows", Config.is_capture_cropped_screen_dc);
    WriteCfgString("Avi Options", "Avi Capture Path", Config.avi_capture_path);

    WriteCfgInt("Other", "Ask on lua close", Config.is_lua_exit_confirm_enabled);
    WriteCfgInt("Other", "Simplified lua", Config.is_lua_simple_dialog_enabled);
    WriteCfgInt("Other", "Autobackup", Config.is_movie_backup_enabled);
    WriteCfgInt("Other", "Autobackup level", Config.movie_backup_level);
    WriteCfgInt("Other", "Fast statusbar updates", Config.is_statusbar_frequent_refresh_enabled);
    WriteCfgInt("Other", "Synchronisation Mode", Config.synchronization_mode);
    WriteCfgInt("Other", "CPU Clock Speed Multiplier", Config.cpu_clock_speed_multiplier);

    // Save A Whole Whackton Of Hotkeys:

    const char* settingStrings[13] = { "Fast Forward", "Speed Up", "Slow Down", "Frame Advance", "Pause Resume",
        "ReadOnly", "Play", "PlayStop", "Record",
        "RecordStop", "Screenshot", "Save Current", "Load Current" };

    for (int i = 0; i <= 12; i++)
    {
        WriteHotkeyConfig(i, settingStrings[i]);
    }

    // Save/Load Hotkeys
        int i;
        char str[128];

        for (i = 1; i <= 10; i++)
        {
            sprintf(str, "Save %d", i);
            WriteHotkeyConfig(12 + i, str);
            sprintf(str, "Load %d", i);
            WriteHotkeyConfig(22 + i, str);
            sprintf(str, "Select %d", i);
            WriteHotkeyConfig(32 + i, str);
        }
    //Lua
    WriteCfgString("Lua", "Script Path", Config.lua_script_path);
    WriteHotkeyConfig(43, "Lua Script Reload");
    WriteHotkeyConfig(44, "Lua Script CloseAll");
    WriteHotkeyConfig(48, "Load Latest Lua Script");

    // some new misc ones
    WriteHotkeyConfig(45, "Start From Beginning");
    WriteHotkeyConfig(46, "Load Latest Movie");
    WriteHotkeyConfig(47, "Load Latest ROM");
    
}


