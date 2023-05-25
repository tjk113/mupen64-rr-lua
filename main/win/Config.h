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

#define NUM_HOTKEYS (49)

typedef struct _NamedHotkey
{
    const char* name;
    HOTKEY* hotkeys;
} NamedHotkey;

typedef struct _CONFIG {

    /// <summary>
    /// The config protocol version
    /// </summary>
    unsigned char config_version;

    /// <summary>
    /// The selected language
    /// </summary>
    char language[MAX_PATH];

    /// <summary>
    /// Whether FPS are shown to the user
    /// </summary>
    BOOL show_fps;

    /// <summary>
    /// Whether VI/s are shown to the user 
    /// TODO: fix disabling this breaking emulation
    /// </summary>
    BOOL show_vis_per_second;

    /// <summary>
    /// Whether large or presumably hacked roms will not be loaded
    /// </summary>
    BOOL prevent_suspicious_rom_loading;

    /// <summary>
    /// Whether the user will be notified of savestate errors
    /// TODO: Deprecate or unify
    /// </summary>
    BOOL is_savestate_warning_enabled;

    /// <summary>
    /// Whether movie rom metadata is checked against the currently loaded rom
    /// </summary>
    BOOL is_rom_movie_compatibility_check_enabled;

    /// <summary>
    /// All user-defined hotkeys
    /// </summary>
    HOTKEY hotkeys[NUM_HOTKEYS];

    /// <summary>
    /// Whether the FPS are limited to the rom's specified FPS
    /// </summary>
    BOOL is_fps_limited;

    /// <summary>
    /// Whether the saved ini file 
    /// TODO: Deprecate
    /// </summary>
    BOOL is_ini_compressed;

    /// <summary>
    /// The currently selected core type
    /// <para/>
    /// 0 - Cached Interpreter
    /// 1 - Dynamic Recompiler
    /// 2 - Pure Interpreter
    /// </summary>
    int core_type;

    /// <summary>
    /// Whether the FPS modifier setting is respected
    /// </summary>
    BOOL is_fps_modifier_enabled;

    /// <summary>
    /// The target FPS modifier as a percentage 
    /// <para/>
    /// (100 = default)
    /// </summary>
    int fps_modifier;

    /// <summary>
    /// The frequency at which frames are skipped during fast-forward
    /// <para/>
    /// (n = every nth frame is skipped)
    /// </summary>
    int frame_skip_frequency;

    /// <summary>
    /// Whether a playing movie will loop upon ending
    /// </summary>
    BOOL is_movie_loop_enabled;

    /// <summary>
    /// Whether the visual frame count is indexed relative to zero
    /// </summary>
    BOOL is_frame_count_visual_zero_index;

    /// <summary>
    /// Whether movie backups are enabled
    /// </summary>
    BOOL is_movie_backup_enabled;

    /// <summary>
    /// The frequency at which movie backups are created
    /// TODO: Unify
    /// </summary>
    int movie_backup_level;

    /// <summary>
    /// The emulated CPU's clock speed multiplier
    /// <para/>
    /// 1 = original clock speed
    /// high number = approach native clock speed
    /// </summary>
    int cpu_clock_speed_multiplier;

    /// <summary>
    /// Whether the game will start in full-screen mode
    /// TODO: Deprecate, analyze usage
    /// </summary>
    BOOL is_fullscreen_start_enabled;

    /// <summary>
    /// Whether emulation will pause when the main window loses focus
    /// </summary>
    BOOL is_unfocused_pause_enabled;

    /// <summary>
    /// Whether the plugins specified by the rom will be ignored, and the currently selected ones will be picked
    /// TODO: Deprecate (this entire feature is extremely pointless and introduces extreme amounts of bloat)
    /// </summary>
    BOOL use_global_plugins;

    /// <summary>
    /// Whether the toolbar is enabled
    /// </summary>
    BOOL is_toolbar_enabled;

    /// <summary>
    /// Whether the statusbar is enabled
    /// </summary>
    BOOL is_statusbar_enabled;

    /// <summary>
    /// Whether the current savestate slot will increment (and wrap around) upon saving
    /// TODO: Deprecate, analyze usage
    /// </summary>
    BOOL is_slot_autoincrement_enabled;

    /// <summary>
    /// Whether savestate loading will be allowed regardless of any incompatiblities
    /// TODO: better represent how dangerous this is in the view layer
    /// </summary>
    BOOL is_state_independent_state_loading_allowed;

    /// <summary>
    /// Whether the rom "GoodName" rombrowser column is enabled
    /// </summary>
    BOOL is_good_name_column_enabled;

    /// <summary>
    /// Whether the internal rom name rombrowser column is enabled
    /// </summary>
    BOOL is_internal_name_column_enabled;

    /// <summary>
    /// Whether the rom country rombrowser column is enabled
    /// </summary>
    BOOL is_country_column_enabled;

    /// <summary>
    /// Whether the rom size rombrowser column is enabled
    /// </summary>
    BOOL is_size_column_enabled;

    /// <summary>
    /// Whether the rom comments rombrowser column is enabled
    /// </summary>
    BOOL is_comments_column_enabled;

    /// <summary>
    /// Whether the rom filename rombrowser column is enabled
    /// </summary>
    BOOL is_filename_column_enabled;

    /// <summary>
    /// Whether the rom md5 checksum rombrowser column is enabled
    /// </summary>
    BOOL is_md5_column_enabled;

    /// <summary>
    /// Whether the default plugins directory will be used (otherwise, falls back to <see cref="plugins_directory"/>)
    /// </summary>
    BOOL is_default_plugins_directory_used;

    /// <summary>
    /// Whether the default save directory will be used (otherwise, falls back to <see cref="saves_directory"/>)
    /// </summary>
    BOOL is_default_saves_directory_used;

    /// <summary>
    /// Whether the default screenshot directory will be used (otherwise, falls back to <see cref="screenshots_directory"/>)
    /// </summary>
    BOOL is_default_screenshots_directory_used;

    /// <summary>
    /// The plugin directory
    /// </summary>
    char plugins_directory[MAX_PATH];

    /// <summary>
    /// The save directory
    /// </summary>
    char saves_directory[MAX_PATH];

    /// <summary>
    /// The screenshot directory
    /// </summary>
    char screenshots_directory[MAX_PATH];

    /// <summary>
    /// The savestate directory
    /// </summary>
    char states_path[MAX_PATH];

    /// <summary>
    /// The recently opened roms' paths
    /// </summary>
    char recent_rom_paths[10][MAX_PATH];

    /// <summary>
    /// Whether recently opened rom path collection is paused
    /// </summary>
    BOOL is_recent_rom_paths_frozen;

    /// <summary>
    /// The recently opened movies' paths
    /// </summary>
    char recent_movie_paths[5][MAX_PATH];

    /// <summary>
    /// Whether recently opened movie path collection is paused
    /// </summary>
    BOOL is_recent_movie_paths_frozen;

    /// <summary>
    /// The main window's X position
    /// </summary>
    int window_x;

    /// <summary>
    /// The main window's Y position
    /// </summary>
    int window_y;

    /// <summary>
    /// The main window's width
    /// </summary>
    int window_width;

    /// <summary>
    /// The main window's height
    /// </summary>
    int window_height;

    /// <summary>
    /// The rom browser column index to sort by
    /// </summary>
    int rom_browser_sorted_column;

    /// <summary>
    /// The method to sort <see cref="rom_browser_sorted_column"/> by
    /// </summary>
    char rom_browser_sort_method[10];

    /// <summary>
    /// Whether the rom browser will recursively search for roms beginning in the specified directories
    /// </summary>
    BOOL is_rom_browser_recursion_enabled;

    /// <summary>
    /// Whether rom resets are not recorded in movies
    /// TODO: Negate related conditions and wording
    /// </summary>
    bool is_reset_recording_disabled;

    /// <summary>
    /// Whether video will forcibly be captured by mupen itself
    /// <para/>
    /// false = video capture might be performed by video plugin
    /// </summary>
    bool is_internal_capture_forced;

    /// <summary>
    /// Whether the video capture is performed by cropping the screen to mupen's client area
    /// </summary>
    bool is_capture_cropped_screen_dc;

    /// <summary>
    /// Whether selecting hotkeys inside the OEM-reserved range is allowed
    /// </summary>
    bool is_unknown_hotkey_selection_allowed;

    /// <summary>
    /// The avi capture path
    /// </summary>
    char avi_capture_path[MAX_PATH];

    /// <summary>
    /// The audio-video synchronization mode
    /// <para/>
    /// 0 - drop/duplicate frames to synchronize with audio
    /// 1 - insert empty audio samples to synchronize with video
    /// 2 - no synchronization
    /// </summary>
    int synchronization_mode;

    /// <summary>
    /// The lua script path
    /// </summary>
    char lua_script_path[MAX_PATH];

    /// <summary>
    /// The recently opened lua scripts' paths
    /// </summary>
    char recent_lua_script_paths[LUA_MAX_RECENT][MAX_PATH];

    /// <summary>
    /// Whether recently opened lua script path collection is paused
    /// </summary>
    BOOL is_recent_scripts_frozen;

    /// <summary>
    /// Whether the lua script dialog is simplified
    /// TODO: Deprecate, analyze usage
    /// </summary>
    BOOL is_lua_simple_dialog_enabled;

    /// <summary>
    /// Whether the lua script dialog will ask for confirmation before closing
    /// TODO: Deprecate, analyze usage
    /// </summary>
    BOOL is_lua_exit_confirm_enabled;

    /// <summary>
    /// Whether the statusbar will refresh every frame
    /// </summary>
    BOOL is_statusbar_frequent_refresh_enabled;

    /// <summary>
    /// Whether floats will be truncated when rounding
    /// </summary>
    BOOL is_round_towards_zero_enabled;

    /// <summary>
    /// Whether floating point exceptions will propagate and crash the emulator
    /// </summary>
    BOOL is_float_exception_propagation_enabled;

    /// <summary>
    /// Whether the input will be delayed by 1f
    /// TODO: Obsolete, remove (not implemented)
    /// </summary>
    BOOL is_input_delay_enabled;

    /// <summary>
    /// Whether lua drawing is double-buffered
    /// </summary>
    BOOL is_lua_double_buffered;
} CONFIG;

extern "C" CONFIG Config;

void ApplyHotkeys();
void hotkeyToString(HOTKEY* hotkeys, char* buf);

void WriteCfgString   (const char *Section, const char *Key,char *Value) ;
void WriteCfgInt      (const char *Section, const char *Key,int Value) ;
void ReadCfgString    (const char *Section, const char *Key, const char *DefaultValue,char *retValue) ;
int ReadCfgInt        (const char *Section, const char *Key,int DefaultValue) ;

extern NamedHotkey namedHotkeys[];
extern const int namedHotkeyCount;

void LoadConfig()  ;
void SaveConfig()  ; 

