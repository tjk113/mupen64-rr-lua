/***************************************************************************
						  configdialog.c  -  description
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

#include <shlobj.h>
#include <stdio.h>
#include "../lua/LuaConsole.h"
#include "main_win.h"
#include "../../winproject/resource.h"
#include "../plugin.hpp"
#include "RomBrowser.hpp"
#include "../guifuncs.h"
#include "../md5.h"
#include "timers.h"
#include "translation.h"
#include "Config.hpp"
#include "../rom.h"
#include "../main/win/wrapper/ReassociatingFileDialog.h"
#include "../main/helpers/string_helpers.h"


#include "configdialog.h"
#include <vcr.h>
#include <cassert>
// ughh msvc-only code once again
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"propsys.lib")
#pragma comment(lib,"shlwapi.lib")

#ifdef _MSC_VER
#define snprintf	_snprintf
#define strcasecmp	_stricmp
#define strncasecmp	_strnicmp
#endif

ReassociatingFileDialog fdSelectRomFolder;
ReassociatingFileDialog fdSelectPluginsFolder;
ReassociatingFileDialog fdSelectSavesFolder;
ReassociatingFileDialog fdSelectScreenshotsFolder;

//HWND romInfoHWND;
static DWORD dwExitCode;
static DWORD Id;
BOOL stopScan = FALSE;
//HWND __stdcall CreateTrackbar(HWND hwndDlg, HMENU hMenu, UINT iMin, UINT iMax, UINT iSelMin, UINT iSelMax, UINT x, UINT y, UINT w); // winapi macro was very confusing

HWND hwndTrackMovieBackup;

void SwitchMovieBackupModifier(HWND hwnd)
{
    if (ReadCheckBoxValue(hwnd, IDC_MOVIEBACKUPS))
    {
        EnableWindow(hwndTrackMovieBackup, TRUE);
    }
    else
    {
        EnableWindow(hwndTrackMovieBackup, FALSE);
    }
}

BOOL CALLBACK OtherOptionsProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    int index;
    NMHDR FAR* l_nmhdr = nullptr;
    memcpy(&l_nmhdr, &lParam, sizeof(NMHDR FAR*));
    switch (Message)
    {
    case WM_INITDIALOG:
        {
            WriteCheckBoxValue(hwnd, IDC_ALERTMOVIESERRORS, Config.is_rom_movie_compatibility_check_enabled);

            static const char* clockSpeedMultiplierNames[] = {
                "1 - Legacy Mupen Lag Emulation", "2 - 'Lagless'", "3", "4", "5", "6"
            };

            // Populate CPU Clock Speed Multiplier Dropdown Menu
            for (int i = 0; i < (int)(sizeof(clockSpeedMultiplierNames) / sizeof(clockSpeedMultiplierNames[0])); i++)
            {
                SendDlgItemMessage(hwnd, IDC_COMBO_CLOCK_SPD_MULT, CB_ADDSTRING, 0,
                                   (LPARAM)clockSpeedMultiplierNames[i]);
            }
            index = SendDlgItemMessage(hwnd, IDC_COMBO_CLOCK_SPD_MULT, CB_FINDSTRINGEXACT, -1,
                                       (LPARAM)clockSpeedMultiplierNames[Config.cpu_clock_speed_multiplier - 1]);
            SendDlgItemMessage(hwnd, IDC_COMBO_CLOCK_SPD_MULT, CB_SETCURSEL, index, 0);

            // todo
            //hwndTrackMovieBackup = CreateTrackbar(hwnd, (HMENU)ID_MOVIEBACKUP_TRACKBAR, 1, 3, Config.movieBackupsLevel, 3, 200, 55, 100);
            SwitchMovieBackupModifier(hwnd);

            switch (Config.synchronization_mode)
            {
            case VCR_SYNC_AUDIO_DUPL:
                CheckDlgButton(hwnd, IDC_AV_AUDIOSYNC, BST_CHECKED);
                break;
            case VCR_SYNC_VIDEO_SNDROP:
                CheckDlgButton(hwnd, IDC_AV_VIDEOSYNC, BST_CHECKED);
                break;
            case VCR_SYNC_NONE:
                CheckDlgButton(hwnd, IDC_AV_NOSYNC, BST_CHECKED);
                break;
            default:
                break;
            }


            return TRUE;
        }
    case WM_COMMAND:
        {
            char buf[50];
            // dame tu xorita mamacita
            switch (LOWORD(wParam))
            {
            case IDC_AV_AUDIOSYNC:
                if (!VCR_isCapturing())
                    Config.synchronization_mode = VCR_SYNC_AUDIO_DUPL;
                break;
            case IDC_AV_VIDEOSYNC:
                if (!VCR_isCapturing())
                    Config.synchronization_mode = VCR_SYNC_VIDEO_SNDROP;
                break;
            case IDC_AV_NOSYNC:
                if (!VCR_isCapturing())
                    Config.synchronization_mode = VCR_SYNC_NONE;
                break;
            case IDC_COMBO_CLOCK_SPD_MULT:
                ReadComboBoxValue(hwnd, IDC_COMBO_CLOCK_SPD_MULT, buf);
                Config.cpu_clock_speed_multiplier = atoi(&buf[0]);
                break;
            case IDC_MOVIEBACKUPS:
                SwitchMovieBackupModifier(hwnd);
                break;
            default:
                break;
            }

            break;
        }

    case WM_NOTIFY:
        {
            if (l_nmhdr->code == NM_RELEASEDCAPTURE)
            {
                // could potentially show value here, if necessary
            }

            if (l_nmhdr->code == PSN_APPLY)
            {
                Config.is_rom_movie_compatibility_check_enabled = ReadCheckBoxValue(hwnd, IDC_ALERTMOVIESERRORS);
                update_toolbar_visibility();
                update_statusbar_visibility();
                rombrowser_build();
                LoadConfigExternals();
            }
        }
        break;

    default:
        return FALSE;
    }
    return TRUE;
}


void WriteCheckBoxValue(HWND hwnd, int resourceID, int value)
{
    if (value)
    {
        SendMessage(GetDlgItem(hwnd, resourceID), BM_SETCHECK, BST_CHECKED, 0);
    }
}

int ReadCheckBoxValue(HWND hwnd, int resourceID)
{
    return SendDlgItemMessage(hwnd, resourceID, BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE;
}

void WriteComboBoxValue(HWND hwnd, int ResourceID, char* PrimaryVal, char* DefaultVal)
{
    int index;
    index = SendDlgItemMessage(hwnd, ResourceID, CB_FINDSTRINGEXACT, 0, (LPARAM)PrimaryVal);
    if (index != CB_ERR)
    {
        SendDlgItemMessage(hwnd, ResourceID, CB_SETCURSEL, index, 0);
        return;
    }
    index = SendDlgItemMessage(hwnd, ResourceID, CB_FINDSTRINGEXACT, 0, (LPARAM)DefaultVal);
    if (index != CB_ERR)
    {
        SendDlgItemMessage(hwnd, ResourceID, CB_SETCURSEL, index, 0);
        return;
    }
    SendDlgItemMessage(hwnd, ResourceID, CB_SETCURSEL, 0, 0);
}

void ReadComboBoxValue(HWND hwnd, int ResourceID, char* ret)
{
    int index;
    index = SendDlgItemMessage(hwnd, ResourceID, CB_GETCURSEL, 0, 0);
    SendDlgItemMessage(hwnd, ResourceID, CB_GETLBTEXT, index, (LPARAM)ret);
}

void ChangeSettings(HWND hwndOwner)
{
    PROPSHEETPAGE psp[6]{};
    PROPSHEETHEADER psh{};
    char ConfigStr[200], DirectoriesStr[200], titleStr[200], settingsStr[200];
    char AdvSettingsStr[200], HotkeysStr[200], OtherStr[200];
    psp[0].dwSize = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags = PSP_USETITLE;
    psp[0].hInstance = app_hInstance;
    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_MAIN);
    psp[0].pfnDlgProc = PluginsCfg;
    TranslateDefault("Plugins", "Plugins", ConfigStr);
    psp[0].pszTitle = ConfigStr;
    psp[0].lParam = 0;
    psp[0].pfnCallback = NULL;

    psp[1].dwSize = sizeof(PROPSHEETPAGE);
    psp[1].dwFlags = PSP_USETITLE;
    psp[1].hInstance = app_hInstance;
    psp[1].pszTemplate = MAKEINTRESOURCE(IDD_DIRECTORIES);
    psp[1].pfnDlgProc = DirectoriesCfg;
    TranslateDefault("Directories", "Directories", DirectoriesStr);
    psp[1].pszTitle = DirectoriesStr;
    psp[1].lParam = 0;
    psp[1].pfnCallback = NULL;

    psp[2].dwSize = sizeof(PROPSHEETPAGE);
    psp[2].dwFlags = PSP_USETITLE;
    psp[2].hInstance = app_hInstance;
    psp[2].pszTemplate = MAKEINTRESOURCE(IDD_MESSAGES);
    psp[2].pfnDlgProc = GeneralCfg;
    TranslateDefault("General", "General", settingsStr);
    psp[2].pszTitle = settingsStr;
    psp[2].lParam = 0;
    psp[2].pfnCallback = NULL;

    psp[3].dwSize = sizeof(PROPSHEETPAGE);
    psp[3].dwFlags = PSP_USETITLE;
    psp[3].hInstance = app_hInstance;
    psp[3].pszTemplate = MAKEINTRESOURCE(IDD_ADVANCED_OPTIONS);
    psp[3].pfnDlgProc = AdvancedSettingsProc;
    TranslateDefault("Advanced", "Advanced", AdvSettingsStr);
    psp[3].pszTitle = AdvSettingsStr;
    psp[3].lParam = 0;
    psp[3].pfnCallback = NULL;

    psp[4].dwSize = sizeof(PROPSHEETPAGE);
    psp[4].dwFlags = PSP_USETITLE;
    psp[4].hInstance = app_hInstance;
    psp[4].pszTemplate = MAKEINTRESOURCE(IDD_NEW_HOTKEY_DIALOG);
    psp[4].pfnDlgProc = HotkeysProc;
    TranslateDefault("Hotkeys", "Hotkeys", HotkeysStr);
    psp[4].pszTitle = HotkeysStr;
    psp[4].lParam = 0;
    psp[4].pfnCallback = NULL;

    psp[5].dwSize = sizeof(PROPSHEETPAGE);
    psp[5].dwFlags = PSP_USETITLE;
    psp[5].hInstance = app_hInstance;
    psp[5].pszTemplate = MAKEINTRESOURCE(IDD_OTHER_OPTIONS_DIALOG);
    psp[5].pfnDlgProc = OtherOptionsProc;
    TranslateDefault("Other", "Other", OtherStr);
    psp[5].pszTitle = OtherStr;
    psp[5].lParam = 0;
    psp[5].pfnCallback = NULL;

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP;
    psh.hwndParent = hwndOwner;
    psh.hInstance = app_hInstance;
    TranslateDefault("Settings", "Settings", titleStr);
    psh.pszCaption = (LPTSTR)titleStr;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = (LPCPROPSHEETPAGE)&psp;
    psh.pfnCallback = NULL;

    CONFIG old_config = Config;

    if (PropertySheet(&psh))
        save_config();
    else
    {
        Config = old_config;
    }

    return;
}


BOOL CALLBACK AboutDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
    case WM_INITDIALOG:
        SendDlgItemMessage(hwnd, IDB_LOGO, STM_SETIMAGE, IMAGE_BITMAP,
                           (LPARAM)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_LOGO),
                                             IMAGE_BITMAP, 0, 0, 0));

    //            MessageBox( hwnd, "", "", MB_OK );
        return TRUE;

    case WM_CLOSE:
        EndDialog(hwnd, IDOK);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            EndDialog(hwnd, IDOK);
            break;

        case IDC_WEBSITE:
            ShellExecute(0, 0, "http://mupen64.emulation64.com", 0, 0, SW_SHOW);
            break;
        case IDC_GITREPO:
            ShellExecute(0, 0, "https://github.com/mkdasher/mupen64-rr-lua-/", 0, 0, SW_SHOW);
            break;
        default:
            break;
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

void build_rom_browser_path_list(HWND dialog_hwnd)
{
    HWND hwnd = GetDlgItem(dialog_hwnd, IDC_ROMBROWSER_DIR_LIST);

    SendMessage(hwnd, LB_RESETCONTENT, 0, 0);

    for (std::string str : Config.rombrowser_rom_paths)
    {
        SendMessage(hwnd, LB_ADDSTRING, 0,
                    (LPARAM)str.c_str());
    }
}

BOOL CALLBACK DirectoriesCfg(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    char Directory[MAX_PATH];
    LPITEMIDLIST pidl{};
    BROWSEINFO bi{};
    char RomBrowserDir[_MAX_PATH];
    HWND RomBrowserDirListBox;
    auto l_nmhdr = (NMHDR*)&lParam;
    switch (Message)
    {
    case WM_INITDIALOG:

        build_rom_browser_path_list(hwnd);

        SendMessage(GetDlgItem(hwnd, IDC_RECURSION), BM_SETCHECK,
                    Config.is_rombrowser_recursion_enabled ? BST_CHECKED : BST_UNCHECKED, 0);

        if (Config.is_default_plugins_directory_used)
        {
            SendMessage(GetDlgItem(hwnd, IDC_DEFAULT_PLUGINS_CHECK), BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(GetDlgItem(hwnd, IDC_PLUGINS_DIR), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_CHOOSE_PLUGINS_DIR), FALSE);
        }
        if (Config.is_default_saves_directory_used)
        {
            SendMessage(GetDlgItem(hwnd, IDC_DEFAULT_SAVES_CHECK), BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(GetDlgItem(hwnd, IDC_SAVES_DIR), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_CHOOSE_SAVES_DIR), FALSE);
        }
        if (Config.is_default_screenshots_directory_used)
        {
            SendMessage(GetDlgItem(hwnd, IDC_DEFAULT_SCREENSHOTS_CHECK), BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(GetDlgItem(hwnd, IDC_SCREENSHOTS_DIR), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_CHOOSE_SCREENSHOTS_DIR), FALSE);
        }

        SetDlgItemText(hwnd, IDC_PLUGINS_DIR, Config.plugins_directory.c_str());
        SetDlgItemText(hwnd, IDC_SAVES_DIR, Config.saves_directory.c_str());
        SetDlgItemText(hwnd, IDC_SCREENSHOTS_DIR, Config.screenshots_directory.c_str());

        break;
    case WM_NOTIFY:
        if (l_nmhdr->code == PSN_APPLY)
        {
            int selected = SendDlgItemMessage(hwnd, IDC_DEFAULT_PLUGINS_CHECK, BM_GETCHECK, 0, 0) == BST_CHECKED
                               ? TRUE
                               : FALSE;
            GetDlgItemText(hwnd, IDC_PLUGINS_DIR, TempMessage, 200);
            Config.plugins_directory = std::string(TempMessage);
            Config.is_default_plugins_directory_used = selected;

            selected = SendDlgItemMessage(hwnd, IDC_DEFAULT_SAVES_CHECK, BM_GETCHECK, 0, 0) == BST_CHECKED
                           ? TRUE
                           : FALSE;
            GetDlgItemText(hwnd, IDC_SAVES_DIR, TempMessage, MAX_PATH);
            Config.saves_directory = std::string(TempMessage);
            Config.is_default_saves_directory_used = selected;

            selected = SendDlgItemMessage(hwnd, IDC_DEFAULT_SCREENSHOTS_CHECK, BM_GETCHECK, 0, 0) == BST_CHECKED
                           ? TRUE
                           : FALSE;
            GetDlgItemText(hwnd, IDC_SCREENSHOTS_DIR, TempMessage, MAX_PATH);
            Config.screenshots_directory = std::string(TempMessage);
            Config.is_default_screenshots_directory_used = selected;

            search_plugins();
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_RECURSION:
            Config.is_rombrowser_recursion_enabled = SendDlgItemMessage(hwnd, IDC_RECURSION, BM_GETCHECK, 0, 0) ==
                BST_CHECKED;
            break;
        case IDC_ADD_BROWSER_DIR:
            {
                if (fdSelectRomFolder.ShowFolderDialog(Directory, sizeof(Directory) / sizeof(char), hwnd))
                {
                    int len = (int)strlen(Directory);
                    Config.rombrowser_rom_paths.push_back(std::string(Directory));
                }
                build_rom_browser_path_list(hwnd);
                break;
            }
        case IDC_REMOVE_BROWSER_DIR:
            {
                int32_t selected_index = SendMessage(GetDlgItem(hwnd, IDC_ROMBROWSER_DIR_LIST), LB_GETCURSEL, 0, 0);
                if (selected_index != -1)
                {
                    Config.rombrowser_rom_paths.erase(Config.rombrowser_rom_paths.begin() + selected_index);
                }
                build_rom_browser_path_list(hwnd);

                break;
            }

        case IDC_REMOVE_BROWSER_ALL:
            Config.rombrowser_rom_paths.clear();
            build_rom_browser_path_list(hwnd);
            break;

        case IDC_DEFAULT_PLUGINS_CHECK:
            {
                Config.is_default_plugins_directory_used = SendMessage(GetDlgItem(hwnd, IDC_DEFAULT_PLUGINS_CHECK),
                                                                       BM_GETCHECK, 0, 0);
                EnableWindow(GetDlgItem(hwnd, IDC_PLUGINS_DIR), !Config.is_default_plugins_directory_used);
                EnableWindow(GetDlgItem(hwnd, IDC_CHOOSE_PLUGINS_DIR), !Config.is_default_plugins_directory_used);
            }
            break;
        case IDC_PLUGIN_DIRECTORY_HELP:
            {
                MessageBox(hwnd, "Changing the plugin directory may introduce bugs to some plugins.", "Info",
                           MB_ICONINFORMATION | MB_OK);
            }
            break;
        case IDC_CHOOSE_PLUGINS_DIR:
            {
                if (fdSelectPluginsFolder.ShowFolderDialog(Directory, sizeof(Directory) / sizeof(char), hwnd))
                {
                    Config.plugins_directory = std::string(Directory) + "\\";
                }
                SetDlgItemText(hwnd, IDC_PLUGINS_DIR, Config.plugins_directory.c_str());
            }


            break;
        case IDC_DEFAULT_SAVES_CHECK:
            {
                Config.is_default_saves_directory_used = SendMessage(GetDlgItem(hwnd, IDC_DEFAULT_SAVES_CHECK),
                                                                     BM_GETCHECK, 0, 0);
                EnableWindow(GetDlgItem(hwnd, IDC_SAVES_DIR), !Config.is_default_saves_directory_used);
                EnableWindow(GetDlgItem(hwnd, IDC_CHOOSE_SAVES_DIR), !Config.is_default_saves_directory_used);
            }
            break;
        case IDC_CHOOSE_SAVES_DIR:
            {
                if (fdSelectSavesFolder.ShowFolderDialog(Directory, sizeof(Directory) / sizeof(char), hwnd))
                {
                    Config.saves_directory = std::string(Directory) + "\\";
                    SetDlgItemText(hwnd, IDC_SAVES_DIR, Config.saves_directory.c_str());
                }
            }
            break;
        case IDC_DEFAULT_SCREENSHOTS_CHECK:
            {
                Config.is_default_screenshots_directory_used = SendMessage(
                    GetDlgItem(hwnd, IDC_DEFAULT_SCREENSHOTS_CHECK), BM_GETCHECK, 0, 0);
                EnableWindow(GetDlgItem(hwnd, IDC_SCREENSHOTS_DIR), !Config.is_default_screenshots_directory_used);
                EnableWindow(GetDlgItem(hwnd, IDC_CHOOSE_SCREENSHOTS_DIR),
                             !Config.is_default_screenshots_directory_used);
            }
            break;
        case IDC_CHOOSE_SCREENSHOTS_DIR:
            {
                if (fdSelectScreenshotsFolder.ShowFolderDialog(Directory, sizeof(Directory) / sizeof(char), hwnd))
                {
                    Config.screenshots_directory = std::string(Directory) + "\\";
                    SetDlgItemText(hwnd, IDC_SCREENSHOTS_DIR, Config.screenshots_directory.c_str());
                }
            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return FALSE;
}

void sync_plugin_option(HWND hwnd, int32_t id, std::string& selected_plugin_name)
{
    size_t index = SendDlgItemMessage(hwnd, id, CB_FINDSTRINGEXACT, 0, (LPARAM)selected_plugin_name.c_str());
    if (index != CB_ERR)
    {
        SendDlgItemMessage(hwnd, id, CB_SETCURSEL, index, 0);
    }
    else
    {
        SendDlgItemMessage(hwnd, id, CB_SETCURSEL, 0, 0);
        char str[260] = {0};
        SendDlgItemMessage(hwnd, id, CB_GETLBTEXT, 0, (LPARAM)str);
        selected_plugin_name = std::string(str);
    }
}

BOOL CALLBACK PluginsCfg(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    char path_buffer[_MAX_PATH];
    int index;
    NMHDR FAR* l_nmhdr = nullptr;
    memcpy(&l_nmhdr, &lParam, sizeof(NMHDR FAR*));
    switch (Message)
    {
    case WM_CLOSE:
        EndDialog(hwnd, IDOK);
        break;
    case WM_INITDIALOG:
        search_plugins();

        for (auto& plugin : plugins)
        {
            int32_t id = 0;

            switch (plugin->type)
            {
            case plugin_type::video:
                id = IDC_COMBO_GFX;
                break;
            case plugin_type::audio:
                id = IDC_COMBO_SOUND;
                break;
            case plugin_type::input:
                id = IDC_COMBO_INPUT;
                break;
            case plugin_type::rsp:
                id = IDC_COMBO_RSP;
                break;
            default:
                assert(false);
                break;
            }
            SendDlgItemMessage(hwnd, id, CB_ADDSTRING, 0, (LPARAM)plugin->name.c_str());
        }

        sync_plugin_option(hwnd, IDC_COMBO_GFX, Config.selected_video_plugin_name);
        sync_plugin_option(hwnd, IDC_COMBO_SOUND, Config.selected_audio_plugin_name);
        sync_plugin_option(hwnd, IDC_COMBO_INPUT, Config.selected_input_plugin_name);
        sync_plugin_option(hwnd, IDC_COMBO_RSP, Config.selected_rsp_plugin_name);

        EnableWindow(GetDlgItem(hwnd, IDC_COMBO_GFX), !emu_launched);
        EnableWindow(GetDlgItem(hwnd, IDC_COMBO_INPUT), !emu_launched);
        EnableWindow(GetDlgItem(hwnd, IDC_COMBO_SOUND), !emu_launched);
        EnableWindow(GetDlgItem(hwnd, IDC_COMBO_RSP), !emu_launched);

    //Show the images
        SendDlgItemMessage(hwnd, IDB_DISPLAY, STM_SETIMAGE, IMAGE_BITMAP,
                           (LPARAM)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_DISPLAY),
                                             IMAGE_BITMAP, 0, 0, 0));
        SendDlgItemMessage(hwnd, IDB_CONTROL, STM_SETIMAGE, IMAGE_BITMAP,
                           (LPARAM)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_CONTROL),
                                             IMAGE_BITMAP, 0, 0, 0));
        SendDlgItemMessage(hwnd, IDB_SOUND, STM_SETIMAGE, IMAGE_BITMAP,
                           (LPARAM)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_SOUND),
                                             IMAGE_BITMAP, 0, 0, 0));
        SendDlgItemMessage(hwnd, IDB_RSP, STM_SETIMAGE, IMAGE_BITMAP,
                           (LPARAM)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_RSP),
                                             IMAGE_BITMAP, 0, 0, 0));
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDGFXCONFIG:
            hwnd_plug = hwnd;
            ReadComboBoxValue(hwnd, IDC_COMBO_GFX, path_buffer);
            plugin_config(get_plugin_by_name(path_buffer));
            break;
        case IDGFXTEST:
            hwnd_plug = hwnd;
            ReadComboBoxValue(hwnd, IDC_COMBO_GFX, path_buffer);
            plugin_test(get_plugin_by_name(path_buffer));
            break;
        case IDGFXABOUT:
            hwnd_plug = hwnd;
            ReadComboBoxValue(hwnd, IDC_COMBO_GFX, path_buffer);
            plugin_about(get_plugin_by_name(path_buffer));
            break;
        case IDINPUTCONFIG:
            hwnd_plug = hwnd;
            ReadComboBoxValue(hwnd, IDC_COMBO_INPUT, path_buffer);
            plugin_config(get_plugin_by_name(path_buffer));
            break;
        case IDINPUTTEST:
            hwnd_plug = hwnd;
            ReadComboBoxValue(hwnd, IDC_COMBO_INPUT, path_buffer);
            plugin_test(get_plugin_by_name(path_buffer));
            break;
        case IDINPUTABOUT:
            hwnd_plug = hwnd;
            ReadComboBoxValue(hwnd, IDC_COMBO_INPUT, path_buffer);
            plugin_about(get_plugin_by_name(path_buffer));
            break;
        case IDSOUNDCONFIG:
            hwnd_plug = hwnd;
            ReadComboBoxValue(hwnd, IDC_COMBO_SOUND, path_buffer);
            plugin_config(get_plugin_by_name(path_buffer));
            break;
        case IDSOUNDTEST:
            hwnd_plug = hwnd;
            ReadComboBoxValue(hwnd, IDC_COMBO_SOUND, path_buffer);
            plugin_test(get_plugin_by_name(path_buffer));
            break;
        case IDSOUNDABOUT:
            hwnd_plug = hwnd;
            ReadComboBoxValue(hwnd, IDC_COMBO_SOUND, path_buffer);
            plugin_about(get_plugin_by_name(path_buffer));
            break;
        case IDRSPCONFIG:
            hwnd_plug = hwnd;
            ReadComboBoxValue(hwnd, IDC_COMBO_RSP, path_buffer);
            plugin_config(get_plugin_by_name(path_buffer));
            break;
        case IDRSPTEST:
            hwnd_plug = hwnd;
            ReadComboBoxValue(hwnd, IDC_COMBO_RSP, path_buffer);
            plugin_test(get_plugin_by_name(path_buffer));
            break;
        case IDRSPABOUT:
            hwnd_plug = hwnd;
            ReadComboBoxValue(hwnd, IDC_COMBO_RSP, path_buffer);
            plugin_about(get_plugin_by_name(path_buffer));
            break;
        default:
            break;
        }
        break;
    case WM_NOTIFY:
        if (l_nmhdr->code == PSN_APPLY)
        {
            char str[260] = {0};

            ReadComboBoxValue(hwnd, IDC_COMBO_GFX, str);
            Config.selected_video_plugin_name = std::string(str);

            ReadComboBoxValue(hwnd, IDC_COMBO_SOUND, str);
            Config.selected_audio_plugin_name = std::string(str);

            ReadComboBoxValue(hwnd, IDC_COMBO_INPUT, str);
            Config.selected_input_plugin_name = std::string(str);

            ReadComboBoxValue(hwnd, IDC_COMBO_RSP, str);
            Config.selected_rsp_plugin_name = std::string(str);
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

BOOL CALLBACK GeneralCfg(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR* l_nmhdr = nullptr;
    memcpy(&l_nmhdr, &lParam, sizeof(NMHDR FAR*));

    switch (Message)
    {
    case WM_INITDIALOG:
        WriteCheckBoxValue(hwnd, IDC_SHOWFPS, Config.show_fps);
        WriteCheckBoxValue(hwnd, IDC_SHOWVIS, Config.show_vis_per_second);
        WriteCheckBoxValue(hwnd, IDC_MANAGEBADROM, Config.allow_suspicious_rom_loading);
        WriteCheckBoxValue(hwnd, IDC_ALERTSAVESTATEWARNINGS, Config.is_savestate_warning_enabled);
        SetDlgItemInt(hwnd, IDC_SKIPFREQ, Config.frame_skip_frequency, 0);
        WriteCheckBoxValue(hwnd, IDC_ALLOW_ARBITRARY_SAVESTATE_LOADING,
                           Config.is_state_independent_state_loading_allowed);

        CheckDlgButton(hwnd, IDC_INTERP, Config.core_type == 0 ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, IDC_RECOMP, Config.core_type == 1 ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, IDC_PURE_INTERP, Config.core_type == 2 ? BST_CHECKED : BST_UNCHECKED);

        EnableWindow(GetDlgItem(hwnd, IDC_INTERP), !emu_launched);
        EnableWindow(GetDlgItem(hwnd, IDC_RECOMP), !emu_launched);
        EnableWindow(GetDlgItem(hwnd, IDC_PURE_INTERP), !emu_launched);

        TranslateGeneralDialog(hwnd);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_SKIPFREQUENCY_HELP:
            MessageBox(hwnd, "0 = Skip all frames, 1 = Show all frames, n = show every nth frame", "Info",
                       MB_OK | MB_ICONINFORMATION);
            break;
        case IDC_INTERP:
            if (!emu_launched)
            {
                Config.core_type = 0;
            }
            break;
        case IDC_RECOMP:
            if (!emu_launched)
            {
                Config.core_type = 1;
            }
            break;
        case IDC_PURE_INTERP:
            if (!emu_launched)
            {
                Config.core_type = 2;
            }
            break;
        default:
            break;
        }
        break;

    case WM_NOTIFY:
        if (l_nmhdr->code == PSN_APPLY)
        {
            Config.show_fps = ReadCheckBoxValue(hwnd, IDC_SHOWFPS);
            Config.show_vis_per_second = ReadCheckBoxValue(hwnd, IDC_SHOWVIS);
            Config.allow_suspicious_rom_loading = ReadCheckBoxValue(hwnd, IDC_MANAGEBADROM);
            Config.is_savestate_warning_enabled = ReadCheckBoxValue(hwnd, IDC_ALERTSAVESTATEWARNINGS);
            Config.frame_skip_frequency = (int)GetDlgItemInt(hwnd, IDC_SKIPFREQ, 0, 0);
            Config.is_state_independent_state_loading_allowed = ReadCheckBoxValue(
                hwnd, IDC_ALLOW_ARBITRARY_SAVESTATE_LOADING);

            if (emu_launched) SetStatusMode(2);
            else SetStatusMode(0);
            InitTimer();
            SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)"");
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}


BOOL CALLBACK AdvancedSettingsProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR* l_nmhdr = nullptr;
    memcpy(&l_nmhdr, &lParam, sizeof(NMHDR FAR*));
    switch (Message)
    {
    case WM_INITDIALOG:
        WriteCheckBoxValue(hwnd, IDC_PAUSENOTACTIVE, Config.is_unfocused_pause_enabled);
        WriteCheckBoxValue(hwnd, IDC_GUI_TOOLBAR, Config.is_toolbar_enabled);
        WriteCheckBoxValue(hwnd, IDC_GUI_STATUSBAR, Config.is_statusbar_enabled);
        WriteCheckBoxValue(hwnd, IDC_ROUNDTOZERO, Config.is_round_towards_zero_enabled);
        WriteCheckBoxValue(hwnd, IDC_EMULATEFLOATCRASHES, Config.is_float_exception_propagation_enabled);
        WriteCheckBoxValue(hwnd, IDC_CLUADOUBLEBUFFER, Config.is_lua_double_buffered);
        WriteCheckBoxValue(hwnd, IDC_NO_AUDIO_DELAY, !Config.is_audio_delay_enabled);
        WriteCheckBoxValue(hwnd, IDC_NO_COMPILED_JUMP, !Config.is_compiled_jump_enabled);

        WriteCheckBoxValue(hwnd, IDC_NORESET, !Config.is_reset_recording_disabled);

        WriteCheckBoxValue(hwnd, IDC_FORCEINTERNAL, Config.is_internal_capture_forced);

        TranslateAdvancedDialog(hwnd);
        return TRUE;


    case WM_NOTIFY:
        if (l_nmhdr->code == PSN_APPLY)
        {
            Config.is_unfocused_pause_enabled = ReadCheckBoxValue(hwnd, IDC_PAUSENOTACTIVE);
            Config.is_toolbar_enabled = ReadCheckBoxValue(hwnd, IDC_GUI_TOOLBAR);
            Config.is_statusbar_enabled = ReadCheckBoxValue(hwnd, IDC_GUI_STATUSBAR);
            Config.is_round_towards_zero_enabled = ReadCheckBoxValue(hwnd, IDC_ROUNDTOZERO);
            Config.is_float_exception_propagation_enabled = ReadCheckBoxValue(hwnd, IDC_EMULATEFLOATCRASHES);

            Config.is_lua_double_buffered = ReadCheckBoxValue(hwnd, IDC_CLUADOUBLEBUFFER);
            Config.is_audio_delay_enabled = !ReadCheckBoxValue(hwnd, IDC_NO_AUDIO_DELAY);
            Config.is_compiled_jump_enabled = !ReadCheckBoxValue(hwnd, IDC_NO_COMPILED_JUMP);

            Config.is_reset_recording_disabled = ReadCheckBoxValue(hwnd, IDC_NORESET) == 0;
            Config.is_internal_capture_forced = ReadCheckBoxValue(hwnd, IDC_FORCEINTERNAL);

            update_toolbar_visibility();
            update_statusbar_visibility();
            rombrowser_build();
            LoadConfigExternals();
        }
        break;

    default:
        return FALSE;
    }
    return TRUE;
}


static void KillMessages()
{
    MSG Msg;
    int i = 0;
    while (GetMessage(&Msg, NULL, 0, 0) > 0 && i < 20)
    {
        //		TranslateMessage(&Msg);
        //		DispatchMessage(&Msg);
        i++;
    }
}

std::string hotkey_to_string_overview(t_hotkey* hotkey)
{
    return hotkey->identifier + " (" + hotkey_to_string(hotkey) + ")"; 
}

int32_t get_hotkey_array_index_from_selected_identifier(std::string selected_identifier)
{
    int32_t indexInHotkeyArray = -1;
    for (size_t i = 0; i < hotkeys.size(); i++)
    {
        if (selected_identifier == hotkey_to_string_overview(hotkeys[i]))
        {
            indexInHotkeyArray = i;
            break;
        }
    }
    return indexInHotkeyArray;
}

void update_selected_hotkey_view(const HWND dialog_hwnd)
{
    HWND list_hwnd = GetDlgItem(dialog_hwnd, IDC_HOTKEY_LIST);
    HWND selected_hotkey_edit_hwnd = GetDlgItem(dialog_hwnd, IDC_SELECTED_HOTKEY_TEXT);

    char selected_identifier[MAX_PATH] = {0};
    SendMessage(list_hwnd, LB_GETTEXT, SendMessage(list_hwnd, LB_GETCURSEL, 0, 0), (LPARAM)selected_identifier);

    const int index_in_hotkey_array = get_hotkey_array_index_from_selected_identifier(std::string(selected_identifier));

    if (index_in_hotkey_array >= 0 && index_in_hotkey_array < hotkeys.size())
    {
        SetWindowText(selected_hotkey_edit_hwnd, hotkey_to_string(hotkeys[index_in_hotkey_array]).c_str());
    }
    else
    {
        SetDlgItemText(dialog_hwnd, IDC_HOTKEY_ASSIGN_SELECTED, "Assign...");
        SetDlgItemText(dialog_hwnd, IDC_SELECTED_HOTKEY_TEXT, "");
    }
}

void build_hotkey_list(HWND list_hwnd, std::string search_query)
{
    SendMessage(list_hwnd, LB_RESETCONTENT, 0, 0);

    for (size_t i = 0; i < hotkeys.size(); i++)
    {
        std::string hotkey_string = hotkey_to_string_overview(hotkeys[i]);

        if (!search_query.empty())
        {
            if (!contains(to_lower(hotkey_string), to_lower(search_query))
                && !contains(to_lower(hotkeys[i]->identifier), to_lower(search_query)))
            {
                continue;
            }
        }

        SendMessage(list_hwnd, LB_ADDSTRING, 0,
                    (LPARAM)hotkey_string.c_str());
    }
}

BOOL CALLBACK HotkeysProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
    case WM_INITDIALOG:
        {
            SetDlgItemText(hwnd, IDC_HOTKEY_SEARCH, "");
            update_selected_hotkey_view(hwnd);
            return TRUE;
        }
    case WM_COMMAND:
        {
            int id = LOWORD(wParam);
            int event = HIWORD(wParam);

            if (id == IDC_HOTKEY_LIST && event == LBN_SELCHANGE)
            {
                update_selected_hotkey_view(hwnd);
            }

            if (id == IDC_HOTKEY_ASSIGN_SELECTED)
            {
                HWND listHwnd = GetDlgItem(hwnd, IDC_HOTKEY_LIST);
                char selected_identifier[MAX_PATH];
                SendMessage(listHwnd, LB_GETTEXT, SendMessage(listHwnd, LB_GETCURSEL, 0, 0),
                            (LPARAM)selected_identifier);

                int32_t index = get_hotkey_array_index_from_selected_identifier(std::string(selected_identifier));

                if (index >= 0 && index < hotkeys.size())
                {
                    SetDlgItemText(hwnd, id, "...");

                    get_user_hotkey(hotkeys[index]);
                    update_selected_hotkey_view(hwnd);

                    char search_query[MAX_PATH] = {0};
                    GetDlgItemText(hwnd, IDC_HOTKEY_SEARCH, search_query, std::size(search_query));
                    build_hotkey_list(GetDlgItem(hwnd, IDC_HOTKEY_LIST), search_query);
                }
            }

            if (id == IDC_HOTKEY_SEARCH)
            {
                char search_query[MAX_PATH] = {0};
                GetDlgItemText(hwnd, IDC_HOTKEY_SEARCH, search_query, std::size(search_query));
                build_hotkey_list(GetDlgItem(hwnd, IDC_HOTKEY_LIST), search_query);
            }
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}


HWND WINAPI CreateTrackbar(
    HWND hwndDlg, // handle of dialog box (parent window)
    HMENU hMenu, // handle of trackbar to use as identifier
    UINT iMin, // minimum value in trackbar range 
    UINT iMax, // maximum value in trackbar range 
    UINT iSelMin, // minimum value in trackbar selection 
    UINT iSelMax, // maximum value in trackbar selection 
    UINT x, // x pos
    UINT y, // y pos
    UINT w
) // width
{
    HWND hwndTrack = CreateWindowEx(
        0, // no extended styles 
        TRACKBAR_CLASS, // class name 
        "Trackbar Control", // title (caption) 
        WS_CHILD | WS_VISIBLE |
        /*TBS_TOOLTIPS |*/ TBS_FIXEDLENGTH | TBS_TOOLTIPS, // style 
        (int)x, (int)y, // position 
        (int)w, 30, // size 
        hwndDlg, // parent window 
        hMenu, // control identifier 
        app_hInstance, // instance 
        NULL); // no WM_CREATE parameter 


    SendMessage(hwndTrack, TBM_SETRANGE,
                (WPARAM)TRUE, // redraw flag 
                (LPARAM)MAKELONG(iMin, iMax)); // min. & max. positions 

    LPARAM pageSize;
    if (iMax < 10)
    {
        pageSize = 1;
        SendMessage(hwndTrack, TBM_SETTIC,
                    0, 2);
    }
    else
    {
        pageSize = 10;
        SendMessage(hwndTrack, TBM_SETTIC,
                    0, 100);
    }
    SendMessage(hwndTrack, TBM_SETPAGESIZE,
                0, pageSize); // new page size 
    SendMessage(hwndTrack, TBM_SETLINESIZE,
                0, pageSize);
    /*
        SendMessage(hwndTrack, TBM_SETSEL,
            (WPARAM) FALSE,                  // redraw flag
            (LPARAM) MAKELONG(iSelMin, iSelMax));
    */
    SendMessage(hwndTrack, TBM_SETPOS,
                (WPARAM)TRUE, // redraw flag 
                (LPARAM)iSelMin);

    SetFocus(hwndTrack);

    return hwndTrack;
}
