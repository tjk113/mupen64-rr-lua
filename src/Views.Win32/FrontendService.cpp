/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Config.h>
#include <FrontendService.h>



#include <capture/EncodingManager.h>
#include <gui/Loggers.h>
#include <gui/Main.h>
#include <gui/features/Dispatcher.h>
#include <gui/features/MGECompositor.h>
#include <gui/features/RomBrowser.h>
#include <gui/features/Statusbar.h>
#include <lua/LuaConsole.h>

size_t FrontendService::show_multiple_choice_dialog(const std::vector<std::wstring>& choices, const wchar_t* str, const wchar_t* title, core_dialog_type type, void* hwnd)
{
    std::vector<TASKDIALOG_BUTTON> buttons;

    buttons.reserve(choices.size());
    for (int i = 0; i < choices.size(); ++i)
    {
        buttons.push_back({i, choices[i].c_str()});
    }

    auto icon = TD_ERROR_ICON;
    switch (type)
    {
    case fsvc_error:
        icon = TD_ERROR_ICON;
        break;
    case fsvc_warning:
        icon = TD_WARNING_ICON;
        break;
    case fsvc_information:
        icon = TD_INFORMATION_ICON;
        break;
    }

    TASKDIALOGCONFIG task_dialog_config = {
    .cbSize = sizeof(TASKDIALOGCONFIG),
    .hwndParent = static_cast<HWND>(hwnd ? hwnd : g_main_hwnd),
    .pszWindowTitle = title,
    .pszMainIcon = icon,
    .pszContent = str,
    .cButtons = (UINT)buttons.size(),
    .pButtons = buttons.data(),
    };

    int pressed_button = -1;
    TaskDialogIndirect(&task_dialog_config, &pressed_button, NULL, NULL);

    g_view_logger->trace(L"[FrontendService] show_multiple_choice_dialog: '{}', answer: {}", str, pressed_button > 0 ? choices[pressed_button] : L"?");

    return pressed_button;
}

bool FrontendService::show_ask_dialog(const wchar_t* str, const wchar_t* title, bool warning, void* hwnd)
{
    g_view_logger->trace(L"[FrontendService] show_ask_dialog: '{}'", str);
    if (g_config.silent_mode)
    {
        return true;
    }
    return MessageBox(static_cast<HWND>(hwnd ? hwnd : g_main_hwnd), str, title, MB_YESNO | (warning ? MB_ICONWARNING : MB_ICONQUESTION)) == IDYES;
}

void FrontendService::show_dialog(const wchar_t* str, const wchar_t* title, core_dialog_type type, void* hwnd)
{
    int icon = 0;

    switch (type)
    {
    case fsvc_error:
        g_view_logger->error(L"[FrontendService] {}", str);
        icon = MB_ICONERROR;
        break;
    case fsvc_warning:
        g_view_logger->warn(L"[FrontendService] {}", str);
        icon = MB_ICONWARNING;
        break;
    case fsvc_information:
        g_view_logger->info(L"[FrontendService] {}", str);
        icon = MB_ICONINFORMATION;
        break;
    default:
        assert(false);
    }

    if (!g_config.silent_mode)
    {
        MessageBox(static_cast<HWND>(hwnd ? hwnd : g_main_hwnd), str, title, icon);
    }
}

void FrontendService::show_statusbar(const wchar_t* str)
{
    Statusbar::post(str);
}

std::filesystem::path FrontendService::get_app_path()
{
    return g_app_path;
}

void FrontendService::set_default_hotkey_keys(cfg_view* config)
{
    config->fast_forward_hotkey.key = VK_TAB;

    config->gs_hotkey.key = 'G';

    config->speed_down_hotkey.key = VK_OEM_MINUS;

    config->speed_up_hotkey.key = VK_OEM_PLUS;
    
    config->speed_reset_hotkey.key = VK_OEM_PLUS;
    config->speed_reset_hotkey.ctrl = 1;

    config->frame_advance_hotkey.key = VK_OEM_5;
    
    config->multi_frame_advance_hotkey.key = VK_OEM_5;
    config->multi_frame_advance_hotkey.ctrl = 1;

    config->multi_frame_advance_inc_hotkey.key = 'E';
    config->multi_frame_advance_inc_hotkey.ctrl = 1;
    
    config->multi_frame_advance_dec_hotkey.key = 'Q';
    config->multi_frame_advance_dec_hotkey.ctrl = 1;

    config->multi_frame_advance_reset_hotkey.key = 'E';
    config->multi_frame_advance_reset_hotkey.ctrl = 1;
    config->multi_frame_advance_reset_hotkey.shift = 1;
    
    config->pause_hotkey.key = VK_PAUSE;

    config->toggle_read_only_hotkey.key = 'R';
    config->toggle_read_only_hotkey.shift = true;

    config->toggle_movie_loop_hotkey.key = 'L';
    config->toggle_movie_loop_hotkey.shift = true;

    config->start_movie_playback_hotkey.key = 'P';
    config->start_movie_playback_hotkey.ctrl = true;
    config->start_movie_playback_hotkey.shift = true;

    config->start_movie_recording_hotkey.key = 'R';
    config->start_movie_recording_hotkey.ctrl = true;
    config->start_movie_recording_hotkey.shift = true;

    config->stop_movie_hotkey.key = 'C';
    config->stop_movie_hotkey.ctrl = true;
    config->stop_movie_hotkey.shift = true;

    config->create_movie_backup_hotkey.key = 'B';
    config->create_movie_backup_hotkey.ctrl = true;
    config->create_movie_backup_hotkey.shift = true;

    config->take_screenshot_hotkey.key = VK_F12;

    config->play_latest_movie_hotkey.key = 'T';
    config->play_latest_movie_hotkey.ctrl = true;
    config->play_latest_movie_hotkey.shift = true;

    config->load_latest_script_hotkey.key = 'K';
    config->load_latest_script_hotkey.ctrl = true;
    config->load_latest_script_hotkey.shift = true;

    config->new_lua_hotkey.key = 'N';
    config->new_lua_hotkey.ctrl = true;

    config->close_all_lua_hotkey.key = 'W';
    config->close_all_lua_hotkey.ctrl = true;
    config->close_all_lua_hotkey.shift = true;

    config->load_rom_hotkey.key = 'O';
    config->load_rom_hotkey.ctrl = true;

    config->close_rom_hotkey.key = 'W';
    config->close_rom_hotkey.ctrl = true;

    config->reset_rom_hotkey.key = 'R';
    config->reset_rom_hotkey.ctrl = true;

    config->load_latest_rom_hotkey.key = 'O';
    config->load_latest_rom_hotkey.ctrl = true;
    config->load_latest_rom_hotkey.shift = true;

    config->fullscreen_hotkey.key = VK_RETURN;
    config->fullscreen_hotkey.alt = true;

    config->settings_hotkey.key = 'S';
    config->settings_hotkey.ctrl = true;

    config->toggle_statusbar_hotkey.key = 'S';
    config->toggle_statusbar_hotkey.alt = true;

    config->refresh_rombrowser_hotkey.key = VK_F5;
    config->refresh_rombrowser_hotkey.ctrl = true;

    config->seek_to_frame_hotkey.key = 'G';
    config->seek_to_frame_hotkey.ctrl = true;

    config->run_hotkey.key = 'P';
    config->run_hotkey.ctrl = true;

    config->cheats_hotkey.key = 'U';
    config->cheats_hotkey.ctrl = true;

    config->piano_roll_hotkey.key = 'K';
    config->piano_roll_hotkey.ctrl = true;

    config->save_current_hotkey.key = 'I';

    config->load_current_hotkey.key = 'P';

    config->save_as_hotkey.key = 'N';
    config->save_as_hotkey.ctrl = true;

    config->load_as_hotkey.key = 'M';
    config->load_as_hotkey.ctrl = true;

    config->undo_load_state_hotkey.key = 'Z';
    config->undo_load_state_hotkey.ctrl = true;

    config->save_to_slot_1_hotkey.key = '1';
    config->save_to_slot_1_hotkey.shift = true;

    config->save_to_slot_2_hotkey.key = '2';
    config->save_to_slot_2_hotkey.shift = true;

    config->save_to_slot_3_hotkey.key = '3';
    config->save_to_slot_3_hotkey.shift = true;

    config->save_to_slot_4_hotkey.key = '4';
    config->save_to_slot_4_hotkey.shift = true;

    config->save_to_slot_5_hotkey.key = '5';
    config->save_to_slot_5_hotkey.shift = true;

    config->save_to_slot_6_hotkey.key = '6';
    config->save_to_slot_6_hotkey.shift = true;

    config->save_to_slot_7_hotkey.key = '7';
    config->save_to_slot_7_hotkey.shift = true;

    config->save_to_slot_8_hotkey.key = '8';
    config->save_to_slot_8_hotkey.shift = true;

    config->save_to_slot_9_hotkey.key = '9';
    config->save_to_slot_9_hotkey.shift = true;

    config->save_to_slot_10_hotkey.key = '0';
    config->save_to_slot_10_hotkey.shift = true;

    config->load_from_slot_1_hotkey.key = VK_F1;

    config->load_from_slot_2_hotkey.key = VK_F2;

    config->load_from_slot_3_hotkey.key = VK_F3;

    config->load_from_slot_4_hotkey.key = VK_F4;

    config->load_from_slot_5_hotkey.key = VK_F5;

    config->load_from_slot_6_hotkey.key = VK_F6;

    config->load_from_slot_7_hotkey.key = VK_F7;

    config->load_from_slot_8_hotkey.key = VK_F8;

    config->load_from_slot_9_hotkey.key = VK_F9;

    config->load_from_slot_10_hotkey.key = VK_F10;

    config->select_slot_1_hotkey.key = '1';

    config->select_slot_2_hotkey.key = '2';

    config->select_slot_3_hotkey.key = '3';

    config->select_slot_4_hotkey.key = '4';

    config->select_slot_5_hotkey.key = '5';

    config->select_slot_6_hotkey.key = '6';

    config->select_slot_7_hotkey.key = '7';

    config->select_slot_8_hotkey.key = '8';

    config->select_slot_9_hotkey.key = '9';

    config->select_slot_10_hotkey.key = '0';
}

void FrontendService::update_screen()
{
    if (core_vr_get_mge_available())
    {
        MGECompositor::update_screen();
    }
    else
    {
        g_core.plugin_funcs.update_screen();
    }
}

void FrontendService::at_vi()
{
    if (!EncodingManager::is_capturing())
    {
        return;
    }

    EncodingManager::at_vi();
}

void FrontendService::ai_len_changed()
{
    if (!EncodingManager::is_capturing())
    {
        return;
    }

    EncodingManager::ai_len_changed();
}

std::wstring FrontendService::find_available_rom(const std::function<bool(const core_rom_header&)>& predicate)
{
    return RomBrowser::find_available_rom(predicate);
}