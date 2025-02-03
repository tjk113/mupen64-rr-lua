/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// ReSharper disable CppInconsistentNaming
#pragma once

#include <cstdint>

/**
 * \brief Represents a cheat.
 */
typedef struct CoreCheat {
    std::wstring name = L"Unnamed Cheat";
    std::wstring code;
    bool active = true;
    std::vector<std::tuple<bool, std::function<bool()>>> instructions;
} core_cheat;

/**
 * The tone of a dialog.
 */
typedef enum {
    fsvc_error,
    fsvc_warning,
    fsvc_information
} core_dialog_type;

/**
 * An enum containing results that can be returned by the core.
 */
typedef enum {
    // TODO: Maybe unify all Busy and Cancelled results?

    // The operation completed successfully
    Res_Ok,

#pragma region VCR
    // The provided data has an invalid format
    VCR_InvalidFormat,
    // The provided file is inaccessible or does not exist
    VCR_BadFile,
    // The user cancelled the operation
    VCR_Cancelled,
    // The controller configuration is invalid
    VCR_InvalidControllers,
    // The movie's savestate is missing or invalid
    VCR_InvalidSavestate,
    // The resulting frame is outside the bounds of the movie
    VCR_InvalidFrame,
    // There is no rom which matches this movie
    VCR_NoMatchingRom,
    // The callee is already performing another task
    VCR_Busy,
    // The VCR engine is idle, but must be active to complete this operation
    VCR_Idle,
    // The provided freeze buffer is not from the currently active movie
    VCR_NotFromThisMovie,
    // The movie's version is invalid
    VCR_InvalidVersion,
    // The movie's extended version is invalid
    VCR_InvalidExtendedVersion,
    // The operation requires a playback or recording task
    VCR_NeedsPlaybackOrRecording,
    // The provided start type is invalid.
    VCR_InvalidStartType,
    // Another warp modify operation is already running
    VCR_WarpModifyAlreadyRunning,
    // Warp modifications can only be performed during recording
    VCR_WarpModifyNeedsRecordingTask,
    // The provided input buffer is empty
    VCR_WarpModifyEmptyInputBuffer,
    // Another seek operation is already running
    VCR_SeekAlreadyRunning,
    // The seek operation could not be initiated due to a savestate not being loaded successfully
    VCR_SeekSavestateLoadFailed,
    // The seek operation can't be initiated because the seek savestate interval is 0
    VCR_SeekSavestateIntervalZero,
#pragma endregion

#pragma region VR
    // The callee is already performing another task
    VR_Busy,
    // Couldn't find a rom matching the provided movie
    VR_NoMatchingRom,
    // An error occured during plugin loading
    VR_PluginError,
    // The ROM or alternative rom source is invalid
    VR_RomInvalid,
    // The emulator isn't running yet
    VR_NotRunning,
    // Failed to open core streams
    VR_FileOpenFailed,
#pragma endregion

#pragma region Savestates
    // The core isn't launched
    ST_CoreNotLaunched,
    // The savestate file wasn't found
    ST_NotFound,
    // The savestate couldn't be written to disk
    ST_FileWriteError,
    // Couldn't decompress the savestate
    ST_DecompressionError,
    // The event queue was too long
    ST_EventQueueTooLong,
    // The user cancelled the operation
    ST_Cancelled,
#pragma endregion

#pragma region Plugins
    // The plugin library couldn't be loaded
    Pl_LoadLibraryFailed,
    // The plugin doesn't export a GetDllInfo function
    Pl_NoGetDllInfo,
#pragma endregion
} core_result;

typedef enum {
    plugin_video = 2,
    plugin_audio = 3,
    plugin_input = 4,
    plugin_rsp = 1,
} core_plugin_type;

typedef enum {
    ce_none = 1,
    ce_mempak = 2,
    ce_rumblepak = 3,
    ce_transferpak = 4,
    ce_raw = 5
} core_controller_extension;

typedef enum {
    sys_ntsc,
    sys_pal,
    sys_mpal
} core_system_type;

typedef struct
{
    uint16_t Version;
    uint16_t Type;
    char Name[100];

    /* If DLL supports memory these memory options then set them to TRUE or FALSE
       if it does not support it */
    int32_t NormalMemory; /* a normal uint8_t array */
    int32_t MemoryBswaped; /* a normal uint8_t array where the memory has been pre
                             bswap on a dword (32 bits) boundry */
} core_plugin_info;

typedef struct
{
    void* hInst;
    // Whether the memory has been pre-byteswapped on a uint32_t boundary
    int32_t MemoryBswaped;
    uint8_t* RDRAM;
    uint8_t* DMEM;
    uint8_t* IMEM;

    uint32_t* MI_INTR_REG;

    uint32_t* SP_MEM_ADDR_REG;
    uint32_t* SP_DRAM_ADDR_REG;
    uint32_t* SP_RD_LEN_REG;
    uint32_t* SP_WR_LEN_REG;
    uint32_t* SP_STATUS_REG;
    uint32_t* SP_DMA_FULL_REG;
    uint32_t* SP_DMA_BUSY_REG;
    uint32_t* SP_PC_REG;
    uint32_t* SP_SEMAPHORE_REG;

    uint32_t* DPC_START_REG;
    uint32_t* DPC_END_REG;
    uint32_t* DPC_CURRENT_REG;
    uint32_t* DPC_STATUS_REG;
    uint32_t* DPC_CLOCK_REG;
    uint32_t* DPC_BUFBUSY_REG;
    uint32_t* DPC_PIPEBUSY_REG;
    uint32_t* DPC_TMEM_REG;

    void(__cdecl* CheckInterrupts)(void);
    void(__cdecl* ProcessDlistList)(void);
    void(__cdecl* ProcessAlistList)(void);
    void(__cdecl* ProcessRdpList)(void);
    void(__cdecl* ShowCFB)(void);
} core_rsp_info;

typedef struct
{
    void* hWnd; /* Render window */
    void* hStatusBar;
    /* if render window does not have a status bar then this is NULL */

    int32_t MemoryBswaped; // If this is set to TRUE, then the memory has been pre
    //   bswap on a dword (32 bits) boundry
    //	eg. the first 8 bytes are stored like this:
    //        4 3 2 1   8 7 6 5

    uint8_t* HEADER; // This is the rom header (first 40h bytes of the rom
    // This will be in the same memory format as the rest of the memory.
    uint8_t* RDRAM;
    uint8_t* DMEM;
    uint8_t* IMEM;

    uint32_t* MI_INTR_REG;

    uint32_t* DPC_START_REG;
    uint32_t* DPC_END_REG;
    uint32_t* DPC_CURRENT_REG;
    uint32_t* DPC_STATUS_REG;
    uint32_t* DPC_CLOCK_REG;
    uint32_t* DPC_BUFBUSY_REG;
    uint32_t* DPC_PIPEBUSY_REG;
    uint32_t* DPC_TMEM_REG;

    uint32_t* VI_STATUS_REG;
    uint32_t* VI_ORIGIN_REG;
    uint32_t* VI_WIDTH_REG;
    uint32_t* VI_INTR_REG;
    uint32_t* VI_V_CURRENT_LINE_REG;
    uint32_t* VI_TIMING_REG;
    uint32_t* VI_V_SYNC_REG;
    uint32_t* VI_H_SYNC_REG;
    uint32_t* VI_LEAP_REG;
    uint32_t* VI_H_START_REG;
    uint32_t* VI_V_START_REG;
    uint32_t* VI_V_BURST_REG;
    uint32_t* VI_X_SCALE_REG;
    uint32_t* VI_Y_SCALE_REG;

    void(__cdecl* CheckInterrupts)(void);
} core_gfx_info;

typedef struct
{
    void* hwnd;
    void* hinst;

    int32_t MemoryBswaped; // If this is set to TRUE, then the memory has been pre
    //   bswap on a dword (32 bits) boundry
    //	eg. the first 8 bytes are stored like this:
    //        4 3 2 1   8 7 6 5
    uint8_t* HEADER; // This is the rom header (first 40h bytes of the rom
    // This will be in the same memory format as the rest of the memory.
    uint8_t* RDRAM;
    uint8_t* DMEM;
    uint8_t* IMEM;

    uint32_t* MI_INTR_REG;

    uint32_t* AI_DRAM_ADDR_REG;
    uint32_t* AI_LEN_REG;
    uint32_t* AI_CONTROL_REG;
    uint32_t* AI_STATUS_REG;
    uint32_t* AI_DACRATE_REG;
    uint32_t* AI_BITRATE_REG;

    void(__cdecl* CheckInterrupts)(void);
} core_audio_info;

typedef struct
{
    int32_t Present;
    int32_t RawData;
    int32_t Plugin;
} core_controller;

typedef union {
    uint32_t Value;

    struct
    {
        unsigned R_DPAD : 1;
        unsigned L_DPAD : 1;
        unsigned D_DPAD : 1;
        unsigned U_DPAD : 1;
        unsigned START_BUTTON : 1;
        unsigned Z_TRIG : 1;
        unsigned B_BUTTON : 1;
        unsigned A_BUTTON : 1;

        unsigned R_CBUTTON : 1;
        unsigned L_CBUTTON : 1;
        unsigned D_CBUTTON : 1;
        unsigned U_CBUTTON : 1;
        unsigned R_TRIG : 1;
        unsigned L_TRIG : 1;
        unsigned Reserved1 : 1;
        unsigned Reserved2 : 1;

        signed Y_AXIS : 8;

        signed X_AXIS : 8;
    };
} core_buttons;

typedef struct
{
    void* hMainWindow;
    void* hinst;

    int32_t MemoryBswaped; // If this is set to TRUE, then the memory has been pre
    //   bswap on a dword (32 bits) boundry, only effects header.
    //	eg. the first 8 bytes are stored like this:
    //        4 3 2 1   8 7 6 5
    uint8_t* HEADER; // This is the rom header (first 40h bytes of the rom)
    core_controller* Controls; // A pointer to an array of 4 controllers .. eg:
    // CONTROL Controls[4];
} core_input_info;

typedef struct {
    uint8_t init_PI_BSB_DOM1_LAT_REG;
    uint8_t init_PI_BSB_DOM1_PGS_REG;
    uint8_t init_PI_BSB_DOM1_PWD_REG;
    uint8_t init_PI_BSB_DOM1_PGS_REG2;
    uint32_t ClockRate;
    uint32_t PC;
    uint32_t Release;
    uint32_t CRC1;
    uint32_t CRC2;
    uint32_t Unknown[2];
    uint8_t nom[20];
    uint32_t unknown;
    uint32_t Manufacturer_ID;
    uint16_t Cartridge_ID;
    uint16_t Country_code;
    uint32_t Boot_Code[1008];
} core_rom_header;

enum {
    MOVIE_START_FROM_SNAPSHOT = (1 << 0),
    MOVIE_START_FROM_NOTHING = (1 << 1),
    MOVIE_START_FROM_EEPROM = (1 << 2),
    MOVIE_START_FROM_EXISTING_SNAPSHOT = (1 << 3)
};

#define CONTROLLER_X_PRESENT(x) (1 << (x))
#define CONTROLLER_X_MEMPAK(x) (1 << ((x) + 4))
#define CONTROLLER_X_RUMBLE(x) (1 << ((x) + 8))

#pragma pack(push, 1)
/**
 * The movie flag bitfield for extended movies.
 */
typedef union ExtendedMovieFlags {
    uint8_t data;

    struct
    {
        /**
         * Whether the movie was recorded with WiiVC mode enabled.
         */
        bool wii_vc : 1;
        bool unused_1 : 1;
        bool unused_2 : 1;
        bool unused_3 : 1;
        bool unused_4 : 1;
        bool unused_5 : 1;
        bool unused_6 : 1;
        bool unused_7 : 1;
    };
} core_vcr_extended_movie_flags;

/**
 * Additional data for extended movies. Must be 32 bytes large.
 */
typedef struct ExtendedMovieData {
    /**
     * Special authorship information, such as the program which created the movie.
     */
    uint8_t authorship_tag[4] = {0x4D, 0x55, 0x50, 0x4E};

    /**
     * Additional data regarding bruteforcing.
     */
    uint32_t bruteforce_extra_data;

    /**
     * The high word of the amount of loaded states during recording.
     */
    uint32_t rerecord_count;

    uint64_t unused_1;
    uint64_t unused_2;
    uint32_t unused_3;
} t_extended_movie_data;
#pragma pack(pop)

/**
 * \brief
 */
typedef struct CoreVCRMovieHeader {
    /**
     * \brief <c>M64\0x1a</c>
     */
    uint32_t magic;

    /**
     * \brief Currently <c>3</c>
     */
    uint32_t version;

    /**
     * \brief Movie creation time, used to correlate savestates to a movie
     */
    uint32_t uid;

    /**
     * \brief Amount of VIs in the movie
     */
    uint32_t length_vis;

    /**
     * \brief The low word of the amount of loaded states during recording.
     */
    uint32_t rerecord_count;

    /**
     * \brief The VIs per second of the rom this movie was recorded on
     * \remarks (unused)
     * \remarks This field makes no sense, as VI/s are already known via ROM cc
     */
    unsigned char vis_per_second;

    /**
     * \brief The amount of controllers connected during recording
     * \remarks (pretty much unused)
     * \remarks This field makes no sense, as this value can already be figured out by checking controller flags post-load
     */
    unsigned char num_controllers;

    /**
     * The extended format version. Old movies have it set to <c>0</c>.
     */
    uint8_t extended_version = 1;

    /**
     * The extended movie flags. Only valid if <c>extended_version != 0</c>.
     */
    core_vcr_extended_movie_flags extended_flags;

    /**
     * \brief Amount of input samples in the movie, ideally equal to <c>(file_length - 1024) / 4</c>
     */
    uint32_t length_samples;

    /**
     * \brief What state the movie is expected to start from
     * \remarks vcr.h:32
     */
    uint16_t startFlags;

    uint16_t reserved2;

    /**
     * \brief Bitfield of flags for each controller. Sequence of present, mempak and rumble bits with per-controller values respectively (c1,c2,c3,c4,r1,...)
     */
    uint32_t controller_flags;

    /**
     * The extended movie data. Only valid if <c>extended_version != 0</c>.
     */
    t_extended_movie_data extended_data{};

    /**
     * \brief The name of the movie's author
     * \remarks Used in .rec (version 2)
     */
    char old_author_info[48];

    /**
     * \brief A description of what the movie is about
     * \remarks Used in .rec (version 2)
     */
    char old_description[80];

    /**
     * \brief The internal name of the rom the movie was recorded on
     */
    char rom_name[32];

    /**
     * \brief The primary checksum of the rom the movie was recorded on
     */
    uint32_t rom_crc1;

    /**
     * \brief The rom country of the rom the movie was recorded on
     */
    uint16_t rom_country;

    char reserved_bytes[56];

    /**
     * \brief The name of the video plugin the movie was recorded with, as specified by the plugin
     */
    char video_plugin_name[64];

    /**
     * \brief The name of the audio plugin the movie was recorded with, as specified by the plugin
     */
    char audio_plugin_name[64];

    /**
     * \brief The name of the input plugin the movie was recorded with, as specified by the plugin
     */
    char input_plugin_name[64];

    /**
     * \brief The name of the RSP plugin the movie was recorded with, as specified by the plugin
     */
    char rsp_plugin_name[64];

    /**
     * \brief The name of the movie's author as a UTF-8 string
     */
    char author[222];

    /**
     * \brief A description of what the movie is about as a UTF-8 string
     */
    char description[256];
} core_vcr_movie_header;
#pragma pack(pop)

typedef enum {
    task_idle,
    task_start_recording_from_reset,
    task_start_recording_from_snapshot,
    task_start_recording_from_existing_snapshot,
    task_recording,
    task_start_playback_from_reset,
    task_start_playback_from_snapshot,
    task_playback
} core_vcr_task;

/**
 * \brief The movie freeze buffer, which is used to store the movie (with only essential data) associated with a savestate inside the savestate.
 */
typedef struct {
    uint32_t size;
    uint32_t uid;
    uint32_t current_sample;
    uint32_t current_vi;
    uint32_t length_samples;
    std::vector<core_buttons> input_buffer;
} core_vcr_freeze_info;

/**
 * \brief Action that can be triggered by a hotkey
 */
typedef enum {
    ACTION_NONE,
    ACTION_FASTFORWARD_ON,
    ACTION_FASTFORWARD_OFF,
    ACTION_GAMESHARK_ON,
    ACTION_GAMESHARK_OFF,
    ACTION_SPEED_DOWN,
    ACTION_SPEED_UP,
    ACTION_FRAME_ADVANCE,
    ACTION_PAUSE,
    ACTION_TOGGLE_READONLY,
    ACTION_TOGGLE_MOVIE_LOOP,
    ACTION_START_MOVIE_PLAYBACK,
    ACTION_START_MOVIE_RECORDING,
    ACTION_STOP_MOVIE,
    ACTION_CREATE_MOVIE_BACKUP,
    ACTION_TAKE_SCREENSHOT,
    ACTION_PLAY_LATEST_MOVIE,
    ACTION_LOAD_LATEST_SCRIPT,
    ACTION_NEW_LUA,
    ACTION_CLOSE_ALL_LUA,
    ACTION_LOAD_ROM,
    ACTION_CLOSE_ROM,
    ACTION_RESET_ROM,
    ACTION_LOAD_LATEST_ROM,
    ACTION_FULLSCREEN,
    ACTION_SETTINGS,
    ACTION_TOGGLE_STATUSBAR,
    ACTION_REFRESH_ROM_BROWSER,
    ACTION_OPEN_SEEKER,
    ACTION_OPEN_RUNNER,
    ACTION_OPEN_PIANO_ROLL,
    ACTION_OPEN_CHEATS,
    ACTION_SAVE_SLOT,
    ACTION_LOAD_SLOT,
    ACTION_SAVE_AS,
    ACTION_LOAD_AS,
    ACTION_UNDO_LOAD_STATE,
    ACTION_SAVE_SLOT1,
    ACTION_SAVE_SLOT2,
    ACTION_SAVE_SLOT3,
    ACTION_SAVE_SLOT4,
    ACTION_SAVE_SLOT5,
    ACTION_SAVE_SLOT6,
    ACTION_SAVE_SLOT7,
    ACTION_SAVE_SLOT8,
    ACTION_SAVE_SLOT9,
    ACTION_SAVE_SLOT10,
    ACTION_LOAD_SLOT1,
    ACTION_LOAD_SLOT2,
    ACTION_LOAD_SLOT3,
    ACTION_LOAD_SLOT4,
    ACTION_LOAD_SLOT5,
    ACTION_LOAD_SLOT6,
    ACTION_LOAD_SLOT7,
    ACTION_LOAD_SLOT8,
    ACTION_LOAD_SLOT9,
    ACTION_LOAD_SLOT10,
    ACTION_SELECT_SLOT1,
    ACTION_SELECT_SLOT2,
    ACTION_SELECT_SLOT3,
    ACTION_SELECT_SLOT4,
    ACTION_SELECT_SLOT5,
    ACTION_SELECT_SLOT6,
    ACTION_SELECT_SLOT7,
    ACTION_SELECT_SLOT8,
    ACTION_SELECT_SLOT9,
    ACTION_SELECT_SLOT10,
} core_action;

typedef enum {
    PRESENTER_DCOMP,
    PRESENTER_GDI
} core_presenter_type;

typedef enum {
    ENCODER_VFW,
    ENCODER_FFMPEG
} core_encoder_type;

/**
 * \brief The statusbar layout preset.
 */
typedef enum {
    /**
     * The legacy layout.
     */
    LAYOUT_CLASSIC,

    /**
     * The new layout containing additional information.
     */
    LAYOUT_MODERN,

    /**
     * The new layout, but with a section for read-only status.
     */
    LAYOUT_MODERN_WITH_READONLY,
} core_statusbar_layout;

typedef struct Hotkey {
    std::wstring identifier;
    int32_t key;
    int32_t ctrl;
    int32_t shift;
    int32_t alt;
    core_action down_cmd;
    core_action up_cmd;
} core_hotkey;

#pragma pack(push, 1)
typedef struct CoreCfg {
#pragma region Hotkeys
    core_hotkey fast_forward_hotkey;
    core_hotkey gs_hotkey;
    core_hotkey speed_down_hotkey;
    core_hotkey speed_up_hotkey;
    core_hotkey frame_advance_hotkey;
    core_hotkey pause_hotkey;
    core_hotkey toggle_read_only_hotkey;
    core_hotkey toggle_movie_loop_hotkey;
    core_hotkey start_movie_playback_hotkey;
    core_hotkey start_movie_recording_hotkey;
    core_hotkey stop_movie_hotkey;
    core_hotkey create_movie_backup_hotkey;
    core_hotkey take_screenshot_hotkey;
    core_hotkey play_latest_movie_hotkey;
    core_hotkey load_latest_script_hotkey;
    core_hotkey new_lua_hotkey;
    core_hotkey close_all_lua_hotkey;
    core_hotkey load_rom_hotkey;
    core_hotkey close_rom_hotkey;
    core_hotkey reset_rom_hotkey;
    core_hotkey load_latest_rom_hotkey;
    core_hotkey fullscreen_hotkey;
    core_hotkey settings_hotkey;
    core_hotkey toggle_statusbar_hotkey;
    core_hotkey refresh_rombrowser_hotkey;
    core_hotkey seek_to_frame_hotkey;
    core_hotkey run_hotkey;
    core_hotkey piano_roll_hotkey;
    core_hotkey cheats_hotkey;
    core_hotkey save_current_hotkey;
    core_hotkey load_current_hotkey;
    core_hotkey save_as_hotkey;
    core_hotkey load_as_hotkey;
    core_hotkey undo_load_state_hotkey;
    core_hotkey save_to_slot_1_hotkey;
    core_hotkey save_to_slot_2_hotkey;
    core_hotkey save_to_slot_3_hotkey;
    core_hotkey save_to_slot_4_hotkey;
    core_hotkey save_to_slot_5_hotkey;
    core_hotkey save_to_slot_6_hotkey;
    core_hotkey save_to_slot_7_hotkey;
    core_hotkey save_to_slot_8_hotkey;
    core_hotkey save_to_slot_9_hotkey;
    core_hotkey save_to_slot_10_hotkey;
    core_hotkey load_from_slot_1_hotkey;
    core_hotkey load_from_slot_2_hotkey;
    core_hotkey load_from_slot_3_hotkey;
    core_hotkey load_from_slot_4_hotkey;
    core_hotkey load_from_slot_5_hotkey;
    core_hotkey load_from_slot_6_hotkey;
    core_hotkey load_from_slot_7_hotkey;
    core_hotkey load_from_slot_8_hotkey;
    core_hotkey load_from_slot_9_hotkey;
    core_hotkey load_from_slot_10_hotkey;
    core_hotkey select_slot_1_hotkey;
    core_hotkey select_slot_2_hotkey;
    core_hotkey select_slot_3_hotkey;
    core_hotkey select_slot_4_hotkey;
    core_hotkey select_slot_5_hotkey;
    core_hotkey select_slot_6_hotkey;
    core_hotkey select_slot_7_hotkey;
    core_hotkey select_slot_8_hotkey;
    core_hotkey select_slot_9_hotkey;
    core_hotkey select_slot_10_hotkey;

#pragma endregion

    /// <summary>
    /// The config protocol version
    /// </summary>
    int32_t version = 5;

    /// <summary>
    /// The new version of Mupen64 currently ignored by the update checker.
    /// <para></para>
    /// L"" means no ignored version.
    /// </summary>
    std::wstring ignored_version;

    /// <summary>
    /// Statistic - Amount of state loads during recording
    /// </summary>
    uint64_t total_rerecords;

    /// <summary>
    /// Statistic - Amount of emulated frames
    /// </summary>
    uint64_t total_frames;

    /// <summary>
    /// The currently selected core type
    /// <para/>
    /// 0 - Cached Interpreter
    /// 1 - Dynamic Recompiler
    /// 2 - Pure Interpreter
    /// </summary>
    int32_t core_type = 1;

    /// <summary>
    /// The target FPS modifier as a percentage
    /// <para/>
    /// (100 = default)
    /// </summary>
    int32_t fps_modifier = 100;

    /// <summary>
    /// The frequency at which frames are skipped during fast-forward
    /// <para/>
    /// 0 = skip all frames
    /// 1 = skip no frames
    /// >0 = every nth frame is skipped
    /// </summary>
    int32_t frame_skip_frequency = 1;

    /// <summary>
    /// The current savestate slot index (0-9).
    /// </summary>
    int32_t st_slot;

    /// <summary>
    /// Whether fast-forward will mute audio
    /// This option improves performance by skipping additional do_rsp_cycles calls, but may cause issues
    /// </summary>
    int32_t fastforward_silent;

    /// <summary>
    /// Whether lag frames will cause updateScreen to be invoked
    /// </summary>
    int32_t skip_rendering_lag;

    /// <summary>
    /// Throttles game rendering to 60 FPS.
    /// </summary>
    int32_t render_throttling = 1;

    /// <summary>
    /// Maximum number of entries into the rom cache
    /// <para/>
    /// 0 = disabled
    /// </summary>
    int32_t rom_cache_size;

    /// <summary>
    /// Saves video buffer to savestates, slow!
    /// </summary>
    int32_t st_screenshot;

    /// <summary>
    /// Whether a playing movie will loop upon ending
    /// </summary>
    int32_t is_movie_loop_enabled;

    /// <summary>
    /// The CPU's counter factor. Higher values will generate less lag frames in-game at the cost of higher native CPU usage.
    /// </summary>
    int32_t counter_factor = 1;

    /// <summary>
    /// Whether emulation will pause when the main window loses focus
    /// </summary>
    int32_t is_unfocused_pause_enabled;

    /// <summary>
    /// Whether the statusbar is enabled
    /// </summary>
    int32_t is_statusbar_enabled = 1;

    /// <summary>
    /// Whether the statusbar is allowed to scale its segments down.
    /// </summary>
    int32_t statusbar_scale_down = 1;

    /// <summary>
    /// Whether the statusbar is allowed to scale its segments up.
    /// </summary>
    int32_t statusbar_scale_up;

    /// <summary>
    /// The statusbar layout.
    /// </summary>
    int32_t statusbar_layout = LAYOUT_MODERN;

    /// <summary>
    /// Whether plugins discovery is performed asynchronously. Removes potential waiting times in the config dialog.
    /// </summary>
    int32_t plugin_discovery_async = 1;

    /// <summary>
    /// Whether the default plugins directory will be used (otherwise, falls back to <see cref="plugins_directory"/>)
    /// </summary>
    int32_t is_default_plugins_directory_used = 1;

    /// <summary>
    /// Whether the default save directory will be used (otherwise, falls back to <see cref="saves_directory"/>)
    /// </summary>
    int32_t is_default_saves_directory_used = 1;

    /// <summary>
    /// Whether the default screenshot directory will be used (otherwise, falls back to <see cref="screenshots_directory"/>)
    /// </summary>
    int32_t is_default_screenshots_directory_used = 1;

    /// <summary>
    /// Whether the default backups directory will be used (otherwise, falls back to <see cref="backups_directory"/>)
    /// </summary>
    int32_t is_default_backups_directory_used = 1;

    /// <summary>
    /// The plugin directory
    /// </summary>
    std::wstring plugins_directory;

    /// <summary>
    /// The save directory
    /// </summary>
    std::wstring saves_directory;

    /// <summary>
    /// The screenshot directory
    /// </summary>
    std::wstring screenshots_directory;

    /// <summary>
    /// The savestate directory
    /// </summary>
    std::wstring states_path;

    /// <summary>
    /// The movie backups directory
    /// </summary>
    std::wstring backups_directory;

    /// <summary>
    /// The recently opened roms' paths
    /// </summary>
    std::vector<std::wstring> recent_rom_paths;

    /// <summary>
    /// Whether recently opened rom path collection is paused
    /// </summary>
    int32_t is_recent_rom_paths_frozen;

    /// <summary>
    /// The recently opened movies' paths
    /// </summary>
    std::vector<std::wstring> recent_movie_paths;

    /// <summary>
    /// Whether recently opened movie path collection is paused
    /// </summary>
    int32_t is_recent_movie_paths_frozen;

    /// <summary>
    /// Whether the rom browser will recursively search for roms beginning in the specified directories
    /// </summary>
    int32_t is_rombrowser_recursion_enabled;

    /// <summary>
    /// The paths to directories which are searched for roms
    /// </summary>
    std::vector<std::wstring> rombrowser_rom_paths;

    /// <summary>
    /// Whether rom resets are not recorded in movies
    /// </summary>
    int32_t is_reset_recording_enabled;

    /// <summary>
    /// The strategy to use when capturing video
    /// <para/>
    /// 0 = Use the video plugin's readScreen or read_video (MGE)
    /// 1 = Internal capture of window
    /// 2 = Internal capture of screen cropped to window
    /// 3 = Use the video plugin's readScreen or read_video (MGE) composited with lua scripts
    /// </summary>
    int32_t capture_mode = 3;

    /// <summary>
    /// The presenter to use for Lua scripts
    /// </summary>
    int32_t presenter_type = PRESENTER_DCOMP;

    /// <summary>
    /// Enables lazy Lua renderer initialization. Greatly speeds up start and stop times for certain scripts.
    /// </summary>
    int32_t lazy_renderer_init = 1;

    /// <summary>
    /// The encoder to use for capturing.
    /// </summary>
    int32_t encoder_type = ENCODER_VFW;

    /// <summary>
    /// The delay (in milliseconds) before capturing the window
    /// <para/>
    /// May be useful when capturing other windows alongside mupen
    /// </summary>
    int32_t capture_delay;

    /// <summary>
    /// FFmpeg post-stream option format string which is used when capturing using the FFmpeg encoder type
    /// </summary>
    std::wstring ffmpeg_final_options =
    L"-y -f rawvideo -pixel_format bgr24 -video_size %dx%d -framerate %d -i %s "
    L"-f s16le -sample_rate %d -ac 2 -channel_layout stereo -i %s "
    L"-c:v libx264 -preset fast -crf 23 -c:a copy -b:a 128k -vf \"vflip\" -f mp4 %s";

    /// <summary>
    /// FFmpeg binary path
    /// </summary>
    std::wstring ffmpeg_path = L"C:\\ffmpeg\\bin\\ffmpeg.exe";

    /// <summary>
    /// The audio-video synchronization mode
    /// <para/>
    /// 0 - No Sync
    /// 1 - Audio Sync
    /// 2 - Video Sync
    /// </summary>
    int32_t synchronization_mode = 1;

    /// <summary>
    /// When enabled, mupen won't change the working directory to its current path at startup
    /// </summary>
    int32_t keep_default_working_directory;

    /// <summary>
    /// Whether the async executor is used for async calls. If disabled, a new thread is spawned for each call (legacy behaviour).
    /// </summary>
    int32_t use_async_executor;

    /// <summary>
    /// Whether the async executor will apply concurrency fuzzing. When enabled, task execution will be delayed to expose delayed task execution handling deficiencies at the callsite.
    /// </summary>
    int32_t async_executor_cuzz;

    /// <summary>
    /// Whether the plugin discovery process is artificially lengthened.
    /// </summary>
    int32_t plugin_discovery_delayed;

    /// <summary>
    /// The lua script path
    /// </summary>
    std::wstring lua_script_path;

    /// <summary>
    /// The recently opened lua scripts' paths
    /// </summary>
    std::vector<std::wstring> recent_lua_script_paths;

    /// <summary>
    /// Whether recently opened lua script path collection is paused
    /// </summary>
    int32_t is_recent_scripts_frozen;

    /// <summary>
    /// The save interval for warp modify savestates in frames
    /// </summary>
    int32_t seek_savestate_interval = 0;

    /// <summary>
    /// The maximum amount of warp modify savestates to keep in memory
    /// </summary>
    int32_t seek_savestate_max_count = 20;

    /// <summary>
    /// Whether piano roll edits are constrained to the column they started on
    /// </summary>
    int32_t piano_roll_constrain_edit_to_column;

    /// <summary>
    /// Maximum size of the undo/redo stack.
    /// </summary>
    int32_t piano_roll_undo_stack_size = 100;

    /// <summary>
    /// Whether the piano roll will try to keep the selection visible when the frame changes
    /// </summary>
    int32_t piano_roll_keep_selection_visible;

    /// <summary>
    /// Whether the piano roll will try to keep the playhead visible when the frame changes
    /// </summary>
    int32_t piano_roll_keep_playhead_visible;

    /// <summary>
    /// Whether undo savestate load functionality is enabled.
    /// </summary>
    int32_t st_undo_load = 1;

    /// <summary>
    /// SD card emulation
    /// </summary>
    int32_t use_summercart;

    /// <summary>
    /// Whether WiiVC emulation is enabled. Causes truncation instead of rounding when performing certain casts.
    /// </summary>
    int32_t wii_vc_emulation;

    /// <summary>
    /// Whether floating point exceptions will propagate and crash the emulator
    /// </summary>
    int32_t is_float_exception_propagation_enabled;

    /// <summary>
    /// Whether audio interrupts will be delayed
    /// </summary>
    int32_t is_audio_delay_enabled = 1;

    /// <summary>
    /// Whether jmp instructions will cause the following code to be JIT'd by dynarec
    /// </summary>
    int32_t is_compiled_jump_enabled = 1;

    /// <summary>
    /// The path of the currently selected video plugin
    /// </summary>
    std::wstring selected_video_plugin;

    /// <summary>
    /// The path of the currently selected audio plugin
    /// </summary>
    std::wstring selected_audio_plugin;

    /// <summary>
    /// The path of the currently selected input plugin
    /// </summary>
    std::wstring selected_input_plugin;

    /// <summary>
    /// The path of the currently selected RSP plugin
    /// </summary>
    std::wstring selected_rsp_plugin;

    /// <summary>
    /// The last known value of the record movie dialog's "start type" field
    /// </summary>
    int32_t last_movie_type = 1; // (MOVIE_START_FROM_SNAPSHOT)

    /// <summary>
    /// The last known value of the record movie dialog's "author" field
    /// </summary>
    std::wstring last_movie_author;

    /// <summary>
    /// The main window's X position
    /// </summary>
    int32_t window_x = ((int)0x80000000);

    /// <summary>
    /// The main window's Y position
    /// </summary>
    int32_t window_y = ((int)0x80000000);

    /// <summary>
    /// The main window's width
    /// </summary>
    int32_t window_width = 640;

    /// <summary>
    /// The main window's height
    /// </summary>
    int32_t window_height = 480;

    /// <summary>
    /// The width of rombrowser columns by index
    /// </summary>
    std::vector<std::int32_t> rombrowser_column_widths = {24, 240, 240, 120};

    /// <summary>
    /// The index of the currently sorted column, or -1 if none is sorted
    /// </summary>
    int32_t rombrowser_sorted_column;

    /// <summary>
    /// Whether the selected column is sorted in an ascending order
    /// </summary>
    int32_t rombrowser_sort_ascending = 1;

    /// <summary>
    /// A map of persistent path dialog IDs and the respective value
    /// </summary>
    std::map<std::wstring, std::wstring> persistent_folder_paths;

    /// <summary>
    /// The last selected settings tab's index.
    /// </summary>
    int32_t settings_tab;

    /// <summary>
    /// Resets the emulator faster, but may cause issues
    /// </summary>
    int32_t fast_reset;

    /// <summary>
    /// Whether VCR displays frame information relative to frame 0, not 1
    /// </summary>
    int32_t vcr_0_index;

    /// <summary>
    /// Increments the current slot when saving savestate to slot
    /// </summary>
    int32_t increment_slot;

    /// <summary>
    /// The movie frame to automatically pause at
    /// -1 none
    /// </summary>
    int32_t pause_at_frame = -1;

    /// <summary>
    /// Whether to pause at the last movie frame
    /// </summary>
    int32_t pause_at_last_frame;

    /// <summary>
    /// Whether VCR operates in read-only mode
    /// </summary>
    int32_t vcr_readonly;

    /// <summary>
    /// Whether VCR creates movie backups
    /// </summary>
    int32_t vcr_backups = 1;

    /// <summary>
    /// Whether movies are written using the new extended format.
    /// If disabled, the extended format sections are set to 0.
    /// </summary>
    int32_t vcr_write_extended_format = 1;

    /// <summary>
    /// Whether automatic update checking is enabled.
    /// </summary>
    int32_t automatic_update_checking;

    /// <summary>
    /// Whether mupen will avoid showing modals and other elements which require user interaction
    /// </summary>
    int32_t silent_mode = 0;

    /// <summary>
    /// The maximum amount of VIs allowed to be generated since the last input poll before a warning dialog is shown
    /// 0 - no warning
    /// </summary>
    int32_t max_lag = 480;

    /// <summary>
    /// The current seeker input value
    /// </summary>
    std::wstring seeker_value;
} core_cfg;
#pragma pack(pop)

typedef void(__cdecl* GETDLLINFO)(core_plugin_info*);
typedef void(__cdecl* DLLCONFIG)(void*);
typedef void(__cdecl* DLLTEST)(void*);
typedef void(__cdecl* DLLABOUT)(void*);

typedef void(__cdecl* CHANGEWINDOW)();
typedef void(__cdecl* CLOSEDLL_GFX)();
typedef int32_t(__cdecl* INITIATEGFX)(core_gfx_info);
typedef void(__cdecl* PROCESSDLIST)();
typedef void(__cdecl* PROCESSRDPLIST)();
typedef void(__cdecl* ROMCLOSED_GFX)();
typedef void(__cdecl* ROMOPEN_GFX)();
typedef void(__cdecl* SHOWCFB)();
typedef void(__cdecl* UPDATESCREEN)();
typedef void(__cdecl* VISTATUSCHANGED)();
typedef void(__cdecl* VIWIDTHCHANGED)();
typedef void(__cdecl* READSCREEN)(void**, int32_t*, int32_t*);
typedef void(__cdecl* DLLCRTFREE)(void*);
typedef void(__cdecl* MOVESCREEN)(int32_t, int32_t);
typedef void(__cdecl* CAPTURESCREEN)(char*);
typedef void(__cdecl* GETVIDEOSIZE)(int32_t*, int32_t*);
typedef void(__cdecl* READVIDEO)(void**);
typedef void(__cdecl* FBREAD)(uint32_t);
typedef void(__cdecl* FBWRITE)(uint32_t addr, uint32_t size);
typedef void(__cdecl* FBGETFRAMEBUFFERINFO)(void*);

typedef void(__cdecl* AIDACRATECHANGED)(int32_t system_type);
typedef void(__cdecl* AILENCHANGED)();
typedef uint32_t(__cdecl* AIREADLENGTH)();
typedef void(__cdecl* CLOSEDLL_AUDIO)();
typedef int32_t(__cdecl* INITIATEAUDIO)(core_audio_info);
typedef void(__cdecl* PROCESSALIST)();
typedef void(__cdecl* ROMCLOSED_AUDIO)();
typedef void(__cdecl* ROMOPEN_AUDIO)();
typedef void(__cdecl* AIUPDATE)(int32_t wait);

typedef void(__cdecl* CLOSEDLL_INPUT)();
typedef void(__cdecl* CONTROLLERCOMMAND)(int32_t controller, unsigned char* command);
typedef void(__cdecl* GETKEYS)(int32_t controller, core_buttons* keys);
typedef void(__cdecl* SETKEYS)(int32_t controller, core_buttons keys);
typedef void(__cdecl* OLD_INITIATECONTROLLERS)(void* hwnd, core_controller controls[4]);
typedef void(__cdecl* INITIATECONTROLLERS)(core_input_info control_info);
typedef void(__cdecl* READCONTROLLER)(int32_t controller, unsigned char* command);
typedef void(__cdecl* ROMCLOSED_INPUT)();
typedef void(__cdecl* ROMOPEN_INPUT)();
typedef void(__cdecl* KEYDOWN)(uint32_t wParam, int32_t lParam);
typedef void(__cdecl* KEYUP)(uint32_t wParam, int32_t lParam);

typedef void(__cdecl* CLOSEDLL_RSP)();
typedef uint32_t(__cdecl* DORSPCYCLES)(uint32_t);
typedef void(__cdecl* INITIATERSP)(core_rsp_info rsp_info, uint32_t* cycles);
typedef void(__cdecl* ROMCLOSED_RSP)();

// frame buffer plugin spec extension
typedef struct
{
    uint32_t addr;
    uint32_t size;
    uint32_t width;
    uint32_t height;
} core_fb_info;

typedef struct CoreScript {
    // Whether the script code gets executed
    bool m_resumed = true;

    // Pair 1st element tells us whether instruction is a conditional. That's required for special treatment of buggy kaze blj anywhere code
    std::vector<std::tuple<bool, std::function<bool()>>> m_instructions;

    // The script's name. FIXME: This should be read from the script
    std::wstring m_name = L"Unnamed Script";

    // The script's code. Can't be mutated after creation.
    std::wstring m_code;
} core_script;

typedef struct {
    uint32_t rdram_config;
    uint32_t rdram_device_id;
    uint32_t rdram_delay;
    uint32_t rdram_mode;
    uint32_t rdram_ref_interval;
    uint32_t rdram_ref_row;
    uint32_t rdram_ras_interval;
    uint32_t rdram_min_interval;
    uint32_t rdram_addr_select;
    uint32_t rdram_device_manuf;
} core_rdram_reg;

typedef struct {
    uint32_t sp_mem_addr_reg;
    uint32_t sp_dram_addr_reg;
    uint32_t sp_rd_len_reg;
    uint32_t sp_wr_len_reg;
    uint32_t w_sp_status_reg;
    uint32_t sp_status_reg;
    char halt;
    char broke;
    char dma_busy;
    char dma_full;
    char io_full;
    char single_step;
    char intr_break;
    char signal0;
    char signal1;
    char signal2;
    char signal3;
    char signal4;
    char signal5;
    char signal6;
    char signal7;
    uint32_t sp_dma_full_reg;
    uint32_t sp_dma_busy_reg;
    uint32_t sp_semaphore_reg;
} core_sp_reg;

typedef struct {
    uint32_t rsp_pc;
    uint32_t rsp_ibist;
} core_rsp_reg;

typedef struct {
    uint32_t dpc_start;
    uint32_t dpc_end;
    uint32_t dpc_current;
    uint32_t w_dpc_status;
    uint32_t dpc_status;
    char xbus_dmem_dma;
    char freeze;
    char flush;
    char start_glck;
    char tmem_busy;
    char pipe_busy;
    char cmd_busy;
    char cbuf_busy;
    char dma_busy;
    char end_valid;
    char start_valid;
    uint32_t dpc_clock;
    uint32_t dpc_bufbusy;
    uint32_t dpc_pipebusy;
    uint32_t dpc_tmem;
} core_dpc_reg;

typedef struct {
    uint32_t dps_tbist;
    uint32_t dps_test_mode;
    uint32_t dps_buftest_addr;
    uint32_t dps_buftest_data;
} core_dps_reg;

typedef struct {
    uint32_t w_mi_init_mode_reg;
    uint32_t mi_init_mode_reg;
    char init_length;
    char init_mode;
    char ebus_test_mode;
    char RDRAM_reg_mode;
    uint32_t mi_version_reg;
    uint32_t mi_intr_reg;
    uint32_t mi_intr_mask_reg;
    uint32_t w_mi_intr_mask_reg;
    char SP_intr_mask;
    char SI_intr_mask;
    char AI_intr_mask;
    char VI_intr_mask;
    char PI_intr_mask;
    char DP_intr_mask;
} core_mips_reg;

typedef struct {
    uint32_t vi_status;
    uint32_t vi_origin;
    uint32_t vi_width;
    uint32_t vi_v_intr;
    uint32_t vi_current;
    uint32_t vi_burst;
    uint32_t vi_v_sync;
    uint32_t vi_h_sync;
    uint32_t vi_leap;
    uint32_t vi_h_start;
    uint32_t vi_v_start;
    uint32_t vi_v_burst;
    uint32_t vi_x_scale;
    uint32_t vi_y_scale;
    uint32_t vi_delay;
} core_vi_reg;

typedef struct {
    uint32_t ai_dram_addr;
    // source address (in rdram) of sound sample to be played
    uint32_t ai_len; // amount of bytes(?) to be played
    uint32_t ai_control;
    uint32_t ai_status; // info about whether dma active and is fifo full
    uint32_t ai_dacrate;
    // clock rate / audio rate, tells sound controller how to interpret the audio samples
    uint32_t ai_bitrate;
    // possible values 2 to 16, bits per sample?, this is always (dacRate / 66)-1 (by libultra)
    uint32_t next_delay;
    uint32_t next_len;
    uint32_t current_delay;
    uint32_t current_len;
} core_ai_reg;

typedef struct {
    uint32_t pi_dram_addr_reg;
    uint32_t pi_cart_addr_reg;
    uint32_t pi_rd_len_reg;
    uint32_t pi_wr_len_reg;
    uint32_t read_pi_status_reg;
    uint32_t pi_bsd_dom1_lat_reg;
    uint32_t pi_bsd_dom1_pwd_reg;
    uint32_t pi_bsd_dom1_pgs_reg;
    uint32_t pi_bsd_dom1_rls_reg;
    uint32_t pi_bsd_dom2_lat_reg;
    uint32_t pi_bsd_dom2_pwd_reg;
    uint32_t pi_bsd_dom2_pgs_reg;
    uint32_t pi_bsd_dom2_rls_reg;
} core_pi_reg;

typedef struct {
    uint32_t ri_mode;
    uint32_t ri_config;
    uint32_t ri_current_load;
    uint32_t ri_select;
    uint32_t ri_refresh;
    uint32_t ri_latency;
    uint32_t ri_error;
    uint32_t ri_werror;
} core_ri_reg;

typedef struct {
    uint32_t si_dram_addr;
    uint32_t si_pif_addr_rd64b;
    uint32_t si_pif_addr_wr64b;
    uint32_t si_status;
} core_si_reg;

typedef enum {
    // A save operation
    core_st_job_save,
    // A load operation
    core_st_job_load
} core_st_job;

typedef enum {
    // The target medium is a slot (0-9).
    core_st_medium_slot,
    // The target medium is a file with a path.
    core_st_medium_path,
    // The target medium is in-memory.
    core_st_medium_memory,
} core_st_medium;

using core_st_callback = std::function<void(core_result result, const std::vector<uint8_t>&)>;

typedef std::common_type_t<std::chrono::duration<int64_t, std::ratio<1, 1000000000>>, std::chrono::duration<int64_t, std::ratio<1, 1000000000>>> core_timer_delta;

typedef struct
{
    uint32_t opcode;
    uint32_t address;
} core_dbg_cpu_state;

constexpr uint8_t core_timer_max_deltas = 60;
