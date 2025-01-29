/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Windows.h>
#include <spdlog/logger.h>
#include <gui/features/Dispatcher.h>
#include <core_api.h>
#include <Plugin.h>

#ifdef _DEBUG
#define BUILD_TARGET_INFO L"-debug"
#else
#define BUILD_TARGET_INFO L""
#endif

#ifdef UNICODE
#define CHARSET_INFO L""
#else
#define CHARSET_INFO L"-a"
#endif

#ifdef _M_X64
#define ARCH_INFO L"-x64"
#else
#define ARCH_INFO L""
#endif

#define BASE_NAME L"Mupen 64 "
#define CURRENT_VERSION L"1.1.9-9"

#define MUPEN_VERSION BASE_NAME CURRENT_VERSION ARCH_INFO CHARSET_INFO BUILD_TARGET_INFO

#define WM_FOCUS_MAIN_WINDOW (WM_USER + 17)
#define WM_EXECUTE_DISPATCHER (WM_USER + 18)

extern BOOL CALLBACK CfgDlgProc(HWND hwnd, UINT Message, WPARAM wParam,
                                LPARAM lParam);

extern core_params g_core;
extern bool g_frame_changed;

extern DWORD g_ui_thread_id;

extern int g_last_wheel_delta;

extern HWND g_main_hwnd;
extern HINSTANCE g_app_instance;

extern HWND g_hwnd_plug;
extern DWORD start_rom_id;

extern std::filesystem::path g_app_path;
extern std::shared_ptr<Dispatcher> g_main_window_dispatcher;

/**
 * \brief Currently loaded ROM's path, or empty.
 */
extern std::filesystem::path g_rom_path;

/**
 * \brief Whether the statusbar needs to be updated with new input information
 */
extern bool is_primary_statusbar_invalidated;

/**
 * The view ff flag. Combined with other flags to determine the core fast-forward value.
 */
extern bool g_fast_forward;

/**
 * \brief Pauses the emulation during the object's lifetime, resuming it if previously paused upon being destroyed
 */
struct BetterEmulationLock {
private:
    bool was_paused;

public:
    BetterEmulationLock();
    ~BetterEmulationLock();
};

typedef struct
{
    long width;
    long height;
    long statusbar_height;
} t_window_info;

extern t_window_info window_info;

static bool task_is_playback(vcr_task task)
{
    return task == task_playback || task == task_start_playback_from_reset || task ==
        task_start_playback_from_snapshot;
}

static bool vcr_is_task_recording(vcr_task task)
{
    return task == task_recording || task == task_start_recording_from_reset || task == task_start_recording_from_existing_snapshot || task ==
        task_start_recording_from_snapshot;
}

constexpr uint32_t AddrMask = 0x7FFFFF;
#define S8 3
#define S16 2
#define Sh16 1

template <typename T>
static uint32_t ToAddr(uint32_t addr)
{
    return sizeof(T) == 4
               ? addr
               : sizeof(T) == 2
               ? addr ^ S16
               : sizeof(T) == 1
               ? addr ^ S8
               : throw"ToAddr: sizeof(T)";
}

/**
 * \brief Gets the value at the specified address from RDRAM
 * \tparam T The value's type
 * \param addr The start address of the value
 * \return The value at the address
 */
template <typename T>
static T LoadRDRAMSafe(uint32_t addr)
{
    return *((T*)((uint8_t*)g_core.rdram + ((ToAddr<T>(addr) & AddrMask))));
}

/**
 * \brief Sets the value at the specified address in RDRAM
 * \tparam T The value's type
 * \param addr The start address of the value
 * \param value The value to set
 */
template <typename T>
static void StoreRDRAMSafe(uint32_t addr, T value)
{
    *((T*)((uint8_t*)g_core.rdram + ((ToAddr<T>(addr) & AddrMask)))) = value;
}


t_window_info get_window_info();

/**
 * \brief Gets the string representation of a hotkey
 * \param hotkey The hotkey to convert
 * \return The hotkey as a string
 */
std::wstring hotkey_to_string(const t_hotkey* hotkey);

/**
 * \brief Demands user confirmation for an exit action
 * \return Whether the action is allowed
 * \remarks If the user has chosen to not use modals, this function will return true by default
 */
bool confirm_user_exit();

/**
 * \brief Whether the current execution is on the UI thread
 */
bool is_on_gui_thread();

/**
 * Shows an error dialog for a core result. If the result indicates no error, no work is done.
 * \param result The result to show an error dialog for.
 * \param hwnd The parent window handle for the spawned dialog. If null, the main window is used.
 * \returns Whether the function was able to show an error dialog.
 */
bool show_error_dialog_for_result(core_result result, void* hwnd = nullptr);

/**
 * Represents the result of a plugin discovery operation.
 */
typedef struct
{
    /**
     * The discovered plugins matching the plugin API surface.
     */
    std::vector<std::unique_ptr<Plugin>> plugins;

    /**
     * Vector of discovered plugins and their results.
     */
    std::vector<std::pair<std::filesystem::path, std::wstring>> results;

} t_plugin_discovery_result;

/**
 * Performs a plugin discovery operation.
 */
t_plugin_discovery_result do_plugin_discovery();
