/***************************************************************************
						  main_win.cpp  -  description
							 -------------------
	copyright C) 2003    : ShadowPrince (shadow@emulation64.com)
	modifications        : linker (linker@mail.bg)
	mupen64 author       : hacktarux (hacktarux@yahoo.fr)
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "main_win.h"

#include <cmath>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <Windows.h>
#include <Windowsx.h>
#include <commctrl.h>
#include <Shlwapi.h>
#include <gdiplus.h>

#include "Commandline.h"
#include "config.hpp"
#include "configdialog.h"
#include "LuaCallbacks.h"
#include "features/CrashHelper.h"
#include "LuaConsole.h"
#include "messenger.h"
#include "Recent.h"
#include "timers.h"
#include "../guifuncs.h"
#include "../plugin.hpp"
#include "../rom.h"
#include "../savestates.h"
#include "../vcr.h"
#include "../../memory/memory.h"
#include "../../memory/pif.h"
#include "../../r4300/r4300.h"
#include "../../r4300/recomph.h"
#include "../../r4300/tracelog.h"
#include "../../winproject/resource.h"
#include "features/CoreDbg.h"
#include "features/MovieDialog.h"
#include "features/RomBrowser.hpp"
#include "features/Statusbar.hpp"
#include "features/Toolbar.hpp"
#include "ffmpeg_capture/ffmpeg_capture.hpp"
#include "helpers/collection_helpers.h"
#include "helpers/string_helpers.h"
#include "helpers/win_helpers.h"
#include "wrapper/PersistentPathDialog.h"



DWORD start_rom_id;
DWORD close_rom_id;
DWORD audio_thread_id;
HANDLE sound_thread_handle;
static BOOL FullScreenMode = 0;
int last_wheel_delta = 0;

HANDLE loading_handle[4];
HANDLE EmuThreadHandle;
HWND hwnd_plug;
UINT update_screen_timer;

static DWORD WINAPI ThreadFunc(LPVOID lpParam);
DWORD WINAPI close_rom(LPVOID lpParam);
constexpr char g_szClassName[] = "myWindowClass";
char rom_path[MAX_PATH] = {0};
char LastSelectedRom[_MAX_PATH];
bool scheduled_restart = false;
BOOL really_restart_mode = 0;
BOOL clear_sram_on_restart_mode = 0;
BOOL continue_vcr_on_restart_mode = 0;
BOOL just_restarted_flag = 0;
static BOOL AutoPause = 0;
static BOOL MenuPaused = 0;

HWND mainHWND;
HMENU main_menu;
HINSTANCE app_instance;

std::string app_path = "";


/**
 * \brief List of lua environment map keys running before emulation stopped
 */
std::vector<HWND> previously_running_luas;

std::deque<std::function<void()>> dispatcher_queue;


#pragma region Change notifications
void on_emu_launched_changed(std::any data)
{
	auto value = std::any_cast<bool>(data);

	if (value)
	{
		SetWindowLong(mainHWND, GWL_STYLE,
					  GetWindowLong(mainHWND, GWL_STYLE) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX));
	} else
	{
		SetWindowLong(mainHWND, GWL_STYLE,
			  GetWindowLong(mainHWND, GWL_STYLE) | WS_THICKFRAME | WS_MAXIMIZEBOX);
	}

	EnableMenuItem(main_menu, IDM_STATUSBAR, !value ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(main_menu, IDM_TOOLBAR, !value ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(main_menu, IDM_START_FFMPEG_CAPTURE, value ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(main_menu, ID_AUDIT_ROMS, !value ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(main_menu, IDM_REFRESH_ROMBROWSER, !value ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(main_menu, IDM_RESET_ROM, value ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(main_menu, IDM_SCREENSHOT, value ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(main_menu, IDM_LOAD_STATE_AS, value ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(main_menu, IDM_LOAD_SLOT, value ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(main_menu, IDM_SAVE_STATE_AS, value ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(main_menu, IDM_SAVE_SLOT, value ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(main_menu, IDM_FULLSCREEN, value ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(main_menu, IDM_FRAMEADVANCE, value ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(main_menu, IDM_PAUSE, value ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(main_menu, EMU_PLAY, value ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(main_menu, IDM_CLOSE_ROM, value ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(main_menu, IDM_TRACELOG, value ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(main_menu, IDM_COREDBG, value && Config.core_type == 2 ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(main_menu, IDM_START_MOVIE_PLAYBACK, value ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(main_menu, IDM_STOP_MOVIE_PLAYBACK, value ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(main_menu, IDM_START_MOVIE_RECORDING, value ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(main_menu, IDM_STOP_MOVIE_RECORDING, value ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(main_menu, IDM_PLAY_LATEST_MOVIE, value ? MF_ENABLED : MF_GRAYED);

	if (Config.is_toolbar_enabled) CheckMenuItem(
		main_menu, IDM_TOOLBAR, MF_BYCOMMAND | MF_CHECKED);
	else CheckMenuItem(main_menu, IDM_TOOLBAR, MF_BYCOMMAND | MF_UNCHECKED);
	if (Config.is_statusbar_enabled) CheckMenuItem(
		main_menu, IDM_STATUSBAR, MF_BYCOMMAND | MF_CHECKED);
	else CheckMenuItem(main_menu, IDM_STATUSBAR, MF_BYCOMMAND | MF_UNCHECKED);
	if (Config.is_movie_loop_enabled) CheckMenuItem(
		main_menu, IDM_PLAY_LATEST_MOVIE, MF_BYCOMMAND | MF_CHECKED);
	else CheckMenuItem(main_menu, IDM_PLAY_LATEST_MOVIE, MF_BYCOMMAND | MF_UNCHECKED);
	if (Config.is_recent_movie_paths_frozen) CheckMenuItem(
		main_menu, IDM_FREEZE_RECENT_MOVIES, MF_BYCOMMAND | MF_CHECKED);
	if (Config.is_recent_scripts_frozen) CheckMenuItem(
		main_menu, IDM_FREEZE_RECENT_LUA, MF_BYCOMMAND | MF_CHECKED);
	if (Config.is_recent_rom_paths_frozen) CheckMenuItem(
		main_menu, IDM_FREEZE_RECENT_ROMS, MF_BYCOMMAND | MF_CHECKED);
	//
	// toolbar_on_emu_launched_changed(value, value);
}

void on_capturing_changed(std::any data)
{
	auto value = std::any_cast<bool>(data);

	if (value)
	{
		SetWindowLong(mainHWND, GWL_STYLE, GetWindowLong(mainHWND, GWL_STYLE) & ~WS_MINIMIZEBOX);
		// we apply WS_EX_LAYERED to fix off-screen blitting (off-screen window portions are not included otherwise)
		SetWindowLong(mainHWND, GWL_EXSTYLE, GetWindowLong(mainHWND, GWL_EXSTYLE) | WS_EX_LAYERED);

		EnableMenuItem(main_menu, IDM_START_CAPTURE, MF_GRAYED);
		EnableMenuItem(main_menu, IDM_START_CAPTURE_PRESET,
					   MF_GRAYED);
		EnableMenuItem(main_menu, IDM_START_FFMPEG_CAPTURE, MF_GRAYED);
		EnableMenuItem(main_menu, IDM_STOP_CAPTURE, MF_ENABLED);
		EnableMenuItem(main_menu, IDM_FULLSCREEN, MF_GRAYED);
	} else
	{
		SetWindowLong(mainHWND, GWL_STYLE, GetWindowLong(mainHWND, GWL_STYLE) | WS_MINIMIZEBOX);
        // we remove WS_EX_LAYERED again, because dwm sucks at dealing with layered top-level windows
        SetWindowLong(mainHWND, GWL_EXSTYLE, GetWindowLong(mainHWND, GWL_EXSTYLE) & ~WS_EX_LAYERED);

		SetWindowPos(mainHWND, HWND_TOP, 0, 0, 0, 0,
								 SWP_NOMOVE | SWP_NOSIZE);
		EnableMenuItem(main_menu, IDM_STOP_CAPTURE, MF_GRAYED);
		EnableMenuItem(main_menu, IDM_START_CAPTURE, MF_ENABLED);
		EnableMenuItem(main_menu, IDM_START_FFMPEG_CAPTURE, MF_ENABLED);
		EnableMenuItem(main_menu, IDM_START_CAPTURE_PRESET, MF_ENABLED);
	}
}


void on_emu_paused_changed(bool value)
{

}


BetterEmulationLock::BetterEmulationLock()
{
	was_paused = emu_paused && !MenuPaused;
	MenuPaused = FALSE;
	if (emu_launched && !emu_paused)
		pauseEmu(FALSE);
}

BetterEmulationLock::~BetterEmulationLock()
{
	if (emu_launched && emu_paused && !was_paused)
		resumeEmu(FALSE);
}

#pragma endregion

void main_dispatcher_invoke(const std::function<void()>& func) {
	dispatcher_queue.push_back(func);
	SendMessage(mainHWND, WM_EXECUTE_DISPATCHER, 0, 0);
}

void main_dispatcher_process()
{
	while (!dispatcher_queue.empty()) {
		dispatcher_queue.front()();
		dispatcher_queue.pop_front();
	}
}

void ClearButtons()
{
	BUTTONS zero = {0};
	for (int i = 0; i < 4; i++)
	{
		setKeys(i, zero);
	}
}

std::string get_app_full_path()
{
	char ret[MAX_PATH] = {0};
	char drive[_MAX_DRIVE], dirn[_MAX_DIR];
	char path_buffer[_MAX_DIR];
	GetModuleFileName(nullptr, path_buffer, sizeof(path_buffer));
	_splitpath(path_buffer, drive, dirn, nullptr, nullptr);
	strcpy(ret, drive);
	strcat(ret, dirn);

	return ret;
}

void set_is_movie_loop_enabled(bool value)
{
	Config.is_movie_loop_enabled = value;

	CheckMenuItem(GetMenu(mainHWND), IDM_PLAY_LATEST_MOVIE,
				  MF_BYCOMMAND | (Config.is_movie_loop_enabled
									  ? MFS_CHECKED
									  : MFS_UNCHECKED));

	if (emu_launched)
		Statusbar::post(Config.is_movie_loop_enabled
								 ? "Movies restart after ending"
								 : "Movies stop after ending");
}

static void gui_ChangeWindow()
{
	if (FullScreenMode)
	{
		ShowCursor(FALSE);
		changeWindow();
	} else
	{
		changeWindow();
		ShowCursor(TRUE);
	}
}

void resumeEmu(BOOL quiet)
{
	BOOL wasPaused = emu_paused;
	if (emu_launched)
	{
		emu_paused = 0;
		ResumeThread(sound_thread_handle);
		if (!quiet)
			Statusbar::post("Emulation started");
	}

	// toolbar_on_emu_launched_changed(emu_launched, 1);

	if (emu_paused != wasPaused && !quiet)
		CheckMenuItem(GetMenu(mainHWND), IDM_PAUSE,
		              MF_BYCOMMAND | (emu_paused
			                              ? MFS_CHECKED
			                              : MFS_UNCHECKED));
}


void pauseEmu(BOOL quiet)
{
	BOOL wasPaused = emu_paused;
	if (emu_launched)
	{
		frame_changed = true;
		emu_paused = 1;
		if (!quiet)
			// HACK (not a typo) seems to help avoid a race condition that permanently disables sound when doing frame advance
			SuspendThread(sound_thread_handle);
		if (!quiet)
			Statusbar::post("Emulation paused");
	} else
	{
		CheckMenuItem(GetMenu(mainHWND), IDM_PAUSE,
		              MF_BYCOMMAND | MFS_UNCHECKED);
	}

	// toolbar_on_emu_launched_changed(emu_launched, 0);

	if (emu_paused != wasPaused && !MenuPaused)
		CheckMenuItem(GetMenu(mainHWND), IDM_PAUSE,
		              MF_BYCOMMAND | (emu_paused
			                              ? MFS_CHECKED
			                              : MFS_UNCHECKED));
}

DWORD WINAPI start_rom(LPVOID lpParam)
{
	// maybe make no access violations, probably can be just
	// replaced with rom_path instead of rom_path_local
	char* rom_path_local = {};
	if (lpParam != nullptr) {
		rom_path_local = (char*)lpParam;
	} else {
		rom_path_local = rom_path;
	}
	auto start_time = std::chrono::high_resolution_clock::now();

	// Kill any roms that are still running
	if (emu_launched) {
		WaitForSingleObject(CreateThread(NULL, 0, close_rom, NULL, 0, &close_rom_id), 10'000);
	}

	// TODO: keep plugins loaded and only unload and reload them when they actually change
	printf("Loading plugins\n");
	if (!load_plugins())
	{
		MessageBox(mainHWND, "Invalid plugins selected", nullptr,
		   MB_ICONERROR | MB_OK);
		return 0;
	}

	// valid rom is required to start emulation
	if (rom_read(rom_path_local))
	{
		MessageBox(mainHWND, "Failed to open ROM", "Error",
				   MB_ICONERROR | MB_OK);
		unload_plugins();
		return 0;
	}

	// TODO: investigate wtf this is
	strcpy(LastSelectedRom, rom_path_local);

	// notify ui of emu state change
	main_recent_roms_add(rom_path_local);
	Messenger::broadcast(Messenger::Message::EmuLaunchedChanged, true);
	timer_init(Config.fps_modifier, &ROM_HEADER);
	on_speed_modifier_changed(Config.fps_modifier);

	if (m_task == e_task::idle) {
		SetWindowText(mainHWND, std::format("{} - {}", std::string(MUPEN_VERSION), std::string((char*)ROM_HEADER.nom)).c_str());
	}

	// HACK: We sleep between each plugin load, as that seems to remedy various plugins failing to initialize correctly.
	auto gfx_thread = std::thread(load_gfx, video_plugin->handle);
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	auto audio_thread = std::thread(load_audio, audio_plugin->handle);
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	auto input_thread = std::thread(load_input, input_plugin->handle);
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	auto rsp_thread = std::thread(load_rsp, rsp_plugin->handle);

	gfx_thread.join();
	audio_thread.join();
	input_thread.join();
	rsp_thread.join();

	printf("start_rom entry %dms\n", static_cast<int>((std::chrono::high_resolution_clock::now() - start_time).count() / 1'000'000));
	EmuThreadHandle = CreateThread(NULL, 0, ThreadFunc, NULL, 0, nullptr);

	return 1;
}

static int shut_window = 0;

DWORD WINAPI close_rom(LPVOID lpParam)
{
	//printf("gen interrupt: %lld ns/vi", (total_vi/frame_count));
	if (emu_launched) {

		if (emu_paused) {
			MenuPaused = FALSE;
			resumeEmu(FALSE);
		}

		if (vcr_is_capturing() && !continue_vcr_on_restart_mode) {
			// we need to stop capture before closing rom because rombrowser might show up in recording otherwise lol
			if (vcr_stop_capture() != 0)
				MessageBox(NULL, "Couldn't stop capturing", "VCR", MB_OK);
			else {
				SetWindowPos(mainHWND, HWND_TOP, 0, 0, 0, 0,
							 SWP_NOMOVE | SWP_NOSIZE);
				Statusbar::post("Stopped AVI capture");
			}
		}

		// remember all running lua scripts' HWNDs
		for (const auto key : hwnd_lua_map | std::views::keys)
		{
			previously_running_luas.push_back(key);
		}

		printf("Closing emulation thread...\n");

		// we signal the core to stop, then wait until thread exits
		stop_it();
		main_dispatcher_invoke(stop_all_scripts);
		stop = 1;
		DWORD result = WaitForSingleObject(EmuThreadHandle, 10'000);
		if (result == WAIT_TIMEOUT) {
			MessageBox(mainHWND, "Emu thread didn't exit in time", NULL,
					   MB_ICONERROR | MB_OK);
		}

		emu_launched = 0;
		emu_paused = 1;


		rom = NULL;
		free(rom);

		free_memory();

		Messenger::broadcast(Messenger::Message::EmuLaunchedChanged, false);
		// toolbar_on_emu_launched_changed(0, 0);

		if (m_task == e_task::idle) {
			SetWindowText(mainHWND, MUPEN_VERSION);
			// TODO: look into why this is done
			Statusbar::post(" ", 1);
		}

		if (shut_window) {
			SendMessage(mainHWND, WM_CLOSE, 0, 0);
			return 0;
		}

		Statusbar::post("Emulation stopped");

		if (really_restart_mode) {
			if (clear_sram_on_restart_mode) {
				vcr_clear_save_data();
				clear_sram_on_restart_mode = FALSE;
			}

			really_restart_mode = FALSE;
			if (m_task != e_task::idle)
				just_restarted_flag = TRUE;

			main_dispatcher_invoke([] {
				strcpy(rom_path, LastSelectedRom);
				CreateThread(NULL, 0, start_rom, rom_path, 0, &start_rom_id);
				//if (!start_rom(nullptr)) {
					//close_rom(NULL);
					//MessageBox(mainHWND, "Failed to open ROM", NULL,
							//   MB_ICONERROR | MB_OK);
				//}
			});
		}


		continue_vcr_on_restart_mode = FALSE;
		ExitThread(0);
	}
	ExitThread(0);
}

void resetEmu()
{
	// why is it so damned difficult to reset the game?
	// right now it's hacked to exit to the GUI then re-load the ROM,
	// but it should be possible to reset the game while it's still running
	// simply by clearing out some memory and maybe notifying the plugins...
	if (emu_launched)
	{
		frame_advancing = false;
		really_restart_mode = TRUE;
		MenuPaused = FALSE;
		CreateThread(NULL, 0, close_rom, NULL, 0, &close_rom_id);
	}
}

int pauseAtFrame = -1;

LRESULT CALLBACK PlayMovieProc(HWND hwnd, UINT Message, WPARAM wParam,
                               LPARAM lParam)
{
	char tempbuf[MAX_PATH];

	HWND descriptionDialog;
	HWND authorDialog;
	static char path_buffer[_MAX_PATH];
	switch (Message)
	{
	case WM_INITDIALOG:
		descriptionDialog = GetDlgItem(hwnd, IDC_INI_DESCRIPTION);
		authorDialog = GetDlgItem(hwnd, IDC_INI_AUTHOR);

		SendMessage(descriptionDialog, EM_SETLIMITTEXT,
		            MOVIE_DESCRIPTION_DATA_SIZE, 0);
		SendMessage(authorDialog, EM_SETLIMITTEXT, MOVIE_AUTHOR_DATA_SIZE, 0);

		sprintf(tempbuf, "%s (%s)", (char*)ROM_HEADER.nom, country_code_to_country_name(ROM_HEADER.Country_code).c_str());
		strcat(tempbuf, ".m64");
		SetDlgItemText(hwnd, IDC_INI_MOVIEFILE, tempbuf);

		SetDlgItemText(hwnd, IDC_ROM_INTERNAL_NAME2, (CHAR*)ROM_HEADER.nom);

		SetDlgItemText(hwnd, IDC_ROM_COUNTRY2, country_code_to_country_name(ROM_HEADER.Country_code).c_str());

		sprintf(tempbuf, "%X", (unsigned int)ROM_HEADER.CRC1);
		SetDlgItemText(hwnd, IDC_ROM_CRC3, tempbuf);

		SetDlgItemText(hwnd, IDC_MOVIE_VIDEO_TEXT2,
					   video_plugin->name.c_str());
		SetDlgItemText(hwnd, IDC_MOVIE_INPUT_TEXT2,
					   input_plugin->name.c_str());
		SetDlgItemText(hwnd, IDC_MOVIE_SOUND_TEXT2,
					   audio_plugin->name.c_str());
		SetDlgItemText(hwnd, IDC_MOVIE_RSP_TEXT2,
					   rsp_plugin->name.c_str());

		strcpy(tempbuf, Controls[0].Present ? "Present" : "Disconnected");
		if (Controls[0].Present && Controls[0].Plugin ==
			controller_extension::mempak)
			strcat(tempbuf, " with mempak");
		if (Controls[0].Present && Controls[0].Plugin ==
			controller_extension::rumblepak)
			strcat(tempbuf, " with rumble");
		SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER1_TEXT2, tempbuf);

		strcpy(tempbuf, Controls[1].Present ? "Present" : "Disconnected");
		if (Controls[1].Present && Controls[1].Plugin ==
			controller_extension::mempak)
			strcat(tempbuf, " with mempak");
		if (Controls[1].Present && Controls[1].Plugin ==
			controller_extension::rumblepak)
			strcat(tempbuf, " with rumble pak");
		SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER2_TEXT2, tempbuf);

		strcpy(tempbuf, Controls[2].Present ? "Present" : "Disconnected");
		if (Controls[2].Present && Controls[2].Plugin ==
			controller_extension::mempak)
			strcat(tempbuf, " with mempak");
		if (Controls[2].Present && Controls[2].Plugin ==
			controller_extension::rumblepak)
			strcat(tempbuf, " with rumble pak");
		SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER3_TEXT2, tempbuf);

		strcpy(tempbuf, Controls[3].Present ? "Present" : "Disconnected");
		if (Controls[3].Present && Controls[3].Plugin ==
			controller_extension::mempak)
			strcat(tempbuf, " with mempak");
		if (Controls[3].Present && Controls[3].Plugin ==
			controller_extension::rumblepak)
			strcat(tempbuf, " with rumble pak");
		SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER4_TEXT2, tempbuf);

		CheckDlgButton(hwnd, IDC_MOVIE_READONLY, vcr_get_read_only());


		SetFocus(GetDlgItem(hwnd, IDC_INI_MOVIEFILE));


		goto refresh;
	// better than making it a macro or zillion-argument function

	case WM_CLOSE:
		EndDialog(hwnd, IDOK);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_OK:
		case IDOK:
			{
				vcr_core_stopped();
				{
					BOOL success;
					unsigned int num = GetDlgItemInt(
						hwnd, IDC_PAUSEAT_FIELD, &success, TRUE);
					if (((signed int)num) >= 0 && success)
						pauseAtFrame = (int)num;
					else
						pauseAtFrame = -1;
				}
				GetDlgItemText(hwnd, IDC_INI_MOVIEFILE, tempbuf, MAX_PATH);

				// turn WCHAR into UTF8
				WCHAR authorWC[MOVIE_AUTHOR_DATA_SIZE];
				char authorUTF8[MOVIE_AUTHOR_DATA_SIZE * 4];
				if (GetDlgItemTextW(hwnd, IDC_INI_AUTHOR, authorWC,
				                    MOVIE_AUTHOR_DATA_SIZE))
					WideCharToMultiByte(CP_UTF8, 0, authorWC, -1, authorUTF8,
					                    sizeof(authorUTF8), NULL, NULL);
				else
					GetDlgItemTextA(hwnd, IDC_INI_AUTHOR, authorUTF8,
					                MOVIE_AUTHOR_DATA_SIZE);

				WCHAR descriptionWC[MOVIE_DESCRIPTION_DATA_SIZE];
				char descriptionUTF8[MOVIE_DESCRIPTION_DATA_SIZE * 4];
				if (GetDlgItemTextW(hwnd, IDC_INI_DESCRIPTION, descriptionWC,
				                    MOVIE_DESCRIPTION_DATA_SIZE))
					WideCharToMultiByte(CP_UTF8, 0, descriptionWC, -1,
					                    descriptionUTF8,
					                    sizeof(descriptionUTF8), NULL, NULL);
				else
					GetDlgItemTextA(hwnd, IDC_INI_DESCRIPTION, descriptionUTF8,
					                MOVIE_DESCRIPTION_DATA_SIZE);

				vcr_set_read_only(
					(BOOL)IsDlgButtonChecked(hwnd, IDC_MOVIE_READONLY));

				auto playbackResult = vcr_start_playback(
					tempbuf, authorUTF8, descriptionUTF8);

				if (tempbuf[0] == '\0' || playbackResult !=
					VCR_PLAYBACK_SUCCESS)
				{
					char errorString[MAX_PATH];

					sprintf(errorString, "Failed to start movie \"%s\" ",
					        tempbuf);

					switch (playbackResult)
					{
					case VCR_PLAYBACK_ERROR:
						strcat(errorString, " - unknown error");
						break;
					case VCR_PLAYBACK_SAVESTATE_MISSING:
						strcat(errorString, " - savestate is missing");
						break;
					case VCR_PLAYBACK_FILE_BUSY:
						strcat(errorString, " - file is locked");
						break;
					case VCR_PLAYBACK_INCOMPATIBLE:
						strcat(errorString, " - configuration incompatibility");
						break;
					default: break;
					}

					MessageBox(hwnd, errorString, "VCR", MB_OK);
					break;
				} else
				{
					resumeEmu(TRUE); // Unpause emu if it was paused before
				}
				EndDialog(hwnd, IDOK);
			}
			break;
		case IDC_CANCEL:
		case IDCANCEL:
			EndDialog(hwnd, IDOK);
			break;
		case IDC_MOVIE_BROWSE:
			{
				const auto path = show_persistent_open_dialog("o_movie", hwnd, L"*.m64;*.rec");
				if (path.size() == 0)
				{
					break;
				}
				SetDlgItemText(hwnd, IDC_INI_MOVIEFILE, wstring_to_string(path).c_str());
			}
			goto refresh;
		case IDC_MOVIE_REFRESH:
			goto refresh;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return FALSE;

refresh:

	GetDlgItemText(hwnd, IDC_INI_MOVIEFILE, tempbuf, MAX_PATH);
	t_movie_header m_header = vcr_get_header_info(tempbuf);

	SetDlgItemText(hwnd, IDC_ROM_INTERNAL_NAME, m_header.rom_name);

 	SetDlgItemText(hwnd, IDC_ROM_COUNTRY, country_code_to_country_name(m_header.rom_country).c_str());

	sprintf(tempbuf, "%X", (unsigned int)m_header.rom_crc1);
	SetDlgItemText(hwnd, IDC_ROM_CRC, tempbuf);

	SetDlgItemText(hwnd, IDC_MOVIE_VIDEO_TEXT, m_header.video_plugin_name);
	SetDlgItemText(hwnd, IDC_MOVIE_INPUT_TEXT, m_header.input_plugin_name);
	SetDlgItemText(hwnd, IDC_MOVIE_SOUND_TEXT, m_header.audio_plugin_name);
	SetDlgItemText(hwnd, IDC_MOVIE_RSP_TEXT, m_header.rsp_plugin_name);

	strcpy(tempbuf, (m_header.controller_flags & CONTROLLER_1_PRESENT)
		                ? "Present"
		                : "Disconnected");
	if (m_header.controller_flags & CONTROLLER_1_MEMPAK)
		strcat(tempbuf, " with mempak");
	if (m_header.controller_flags & CONTROLLER_1_RUMBLE)
		strcat(tempbuf, " with rumble");
	SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER1_TEXT, tempbuf);

	strcpy(tempbuf, (m_header.controller_flags & CONTROLLER_2_PRESENT)
		                ? "Present"
		                : "Disconnected");
	if (m_header.controller_flags & CONTROLLER_2_MEMPAK)
		strcat(tempbuf, " with mempak");
	if (m_header.controller_flags & CONTROLLER_2_RUMBLE)
		strcat(tempbuf, " with rumble");
	SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER2_TEXT, tempbuf);

	strcpy(tempbuf, (m_header.controller_flags & CONTROLLER_3_PRESENT)
		                ? "Present"
		                : "Disconnected");
	if (m_header.controller_flags & CONTROLLER_3_MEMPAK)
		strcat(tempbuf, " with mempak");
	if (m_header.controller_flags & CONTROLLER_3_RUMBLE)
		strcat(tempbuf, " with rumble");
	SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER3_TEXT, tempbuf);

	strcpy(tempbuf, (m_header.controller_flags & CONTROLLER_4_PRESENT)
		                ? "Present"
		                : "Disconnected");
	if (m_header.controller_flags & CONTROLLER_4_MEMPAK)
		strcat(tempbuf, " with mempak");
	if (m_header.controller_flags & CONTROLLER_4_RUMBLE)
		strcat(tempbuf, " with rumble");
	SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER4_TEXT, tempbuf);

	SetDlgItemText(hwnd, IDC_FROMSNAPSHOT_TEXT,
	               (m_header.startFlags & MOVIE_START_FROM_SNAPSHOT)
		               ? "Savestate"
		               : "Start");
	if (m_header.startFlags & MOVIE_START_FROM_EEPROM)
	{
		SetDlgItemTextA(hwnd, IDC_FROMSNAPSHOT_TEXT, "EEPROM");
	}

	sprintf(tempbuf, "%u  (%u input)", (int)m_header.length_vis,
	        (int)m_header.length_samples);
	SetDlgItemText(hwnd, IDC_MOVIE_FRAMES, tempbuf);

	if (m_header.vis_per_second == 0)
		m_header.vis_per_second = 60;

	double seconds = (double)m_header.length_vis / (double)m_header.
		vis_per_second;
	double minutes = seconds / 60.0;
	if ((bool)seconds)
		seconds = fmod(seconds, 60.0);
	double hours = minutes / 60.0;
	if ((bool)minutes)
		minutes = fmod(minutes, 60.0);

	if (hours >= 1.0)
		sprintf(tempbuf, "%d hours and %.1f minutes", (unsigned int)hours,
		        (float)minutes);
	else if (minutes >= 1.0)
		sprintf(tempbuf, "%d minutes and %.0f seconds", (unsigned int)minutes,
		        (float)seconds);
	else if (m_header.length_vis != 0)
		sprintf(tempbuf, "%.1f seconds", (float)seconds);
	else
		strcpy(tempbuf, "0 seconds");
	SetDlgItemText(hwnd, IDC_MOVIE_LENGTH, tempbuf);

	sprintf(tempbuf, "%lu", m_header.rerecord_count);
	SetDlgItemText(hwnd, IDC_MOVIE_RERECORDS, tempbuf);

	{
		// convert utf8 metadata to windows widechar
		WCHAR wszMeta[MOVIE_MAX_METADATA_SIZE];
		if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
		                        m_header.author, -1, wszMeta,
		                        MOVIE_AUTHOR_DATA_SIZE))
		{
			SetLastError(0);
			SetWindowTextW(GetDlgItem(hwnd, IDC_INI_AUTHOR), wszMeta);
			if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
			{
				// not implemented on this system - convert as best we can to 1-byte characters and set with that
				// TODO: load unicows.dll instead so SetWindowTextW won't fail even on Win98/ME
				char ansiStr[MOVIE_AUTHOR_DATA_SIZE];
				WideCharToMultiByte(CP_ACP, 0, wszMeta, -1, ansiStr,
				                    MOVIE_AUTHOR_DATA_SIZE, NULL, NULL);
				SetWindowTextA(GetDlgItem(hwnd, IDC_INI_AUTHOR), ansiStr);

				if (ansiStr[0] == '\0')
					SetWindowTextA(GetDlgItem(hwnd, IDC_INI_AUTHOR),
					               "(too lazy to type name)");

				SetLastError(0);
			} else
			{
				if (wszMeta[0] == '\0')
					SetWindowTextW(GetDlgItem(hwnd, IDC_INI_AUTHOR),
					               L"(too lazy to type name)");
			}
		}
		if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
		                        m_header.description, -1, wszMeta,
		                        MOVIE_DESCRIPTION_DATA_SIZE))
		{
			SetWindowTextW(GetDlgItem(hwnd, IDC_INI_DESCRIPTION), wszMeta);
			if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
			{
				char ansiStr[MOVIE_DESCRIPTION_DATA_SIZE];
				WideCharToMultiByte(CP_ACP, 0, wszMeta, -1, ansiStr,
				                    MOVIE_DESCRIPTION_DATA_SIZE, NULL, NULL);
				SetWindowTextA(GetDlgItem(hwnd, IDC_INI_DESCRIPTION), ansiStr);

				if (ansiStr[0] == '\0')
					SetWindowTextA(GetDlgItem(hwnd, IDC_INI_DESCRIPTION),
					               "(no description entered)");

				SetLastError(0);
			} else
			{
				if (wszMeta[0] == '\0')
					SetWindowTextW(GetDlgItem(hwnd, IDC_INI_DESCRIPTION),
					               L"(no description entered)");
			}
		}
	}

	return FALSE;
}



static DWORD WINAPI SoundThread(LPVOID lpParam)
{
	while (emu_launched)
	{
		aiUpdate(1);
	}
	ExitThread(0);
}

static DWORD WINAPI ThreadFunc(LPVOID lpParam)
{
	auto start_time = std::chrono::high_resolution_clock::now();
	init_memory();
	romOpen_gfx();
	romOpen_input();
	romOpen_audio();

	dynacore = Config.core_type;

	emu_paused = 0;
	emu_launched = 1;

	sound_thread_handle = CreateThread(NULL, 0, SoundThread, NULL, 0,
				                         &audio_thread_id);
	printf("Emu thread: Emulation started....\n");

	// start movies, st and lua scripts
	commandline_load_st();
	commandline_start_lua();
	commandline_start_movie();

	// HACK:
	// starting capture immediately won't work, since sample rate will change at game startup, thus terminating the capture
	// as a workaround, we wait a bit before starting the capture
	std::thread([]
	{
		Sleep(1000);
		commandline_start_capture();
	}).detach();

	LuaCallbacks::call_reset();

	if (pauseAtFrame == 0 && vcr_is_starting_and_just_restarted())
	{
		while (emu_paused)
		{
			Sleep(10);
		}
		pauseEmu(FALSE);
		pauseAtFrame = -1;
	}
	main_dispatcher_invoke([]
	{
		for (const HWND hwnd : previously_running_luas)
		{
			// click start button
			SendMessage(hwnd, WM_COMMAND,
					MAKEWPARAM(IDC_BUTTON_LUASTATE, BN_CLICKED),
					(LPARAM)GetDlgItem(hwnd, IDC_BUTTON_LUASTATE));
		}

		previously_running_luas.clear();
	});

	printf("emu thread entry %dms\n", static_cast<int>((std::chrono::high_resolution_clock::now() - start_time).count() / 1'000'000));

	go();

	romClosed_gfx();
	romClosed_audio();
	romClosed_input();
	romClosed_RSP();

	closeDLL_gfx();
	closeDLL_audio();
	closeDLL_input();
	closeDLL_RSP();

	printf("Unloading plugins\n");
	unload_plugins();

	ExitThread(0);
}

void main_recent_roms_build(int32_t reset)
{
	for (size_t i = 0; i < Config.recent_rom_paths.size(); i++)
	{
		if (Config.recent_rom_paths[i].empty())
		{
			continue;
		}
		DeleteMenu(main_menu, ID_RECENTROMS_FIRST + i, MF_BYCOMMAND);
	}

	if (reset)
	{
		Config.recent_rom_paths.clear();
	}

	HMENU h_sub_menu = GetSubMenu(main_menu, 0);
	h_sub_menu = GetSubMenu(h_sub_menu, 5);

	MENUITEMINFO menu_info = {0};
	menu_info.cbSize = sizeof(MENUITEMINFO);
	menu_info.fMask = MIIM_TYPE | MIIM_ID;
	menu_info.fType = MFT_STRING;
	menu_info.fState = MFS_ENABLED;

	for (size_t i = 0; i < Config.recent_rom_paths.size(); i++)
	{
		if (Config.recent_rom_paths[i].empty())
		{
			continue;
		}
		menu_info.dwTypeData = (LPSTR)Config.recent_rom_paths[i].c_str();
		menu_info.cch = strlen(menu_info.dwTypeData);
		menu_info.wID = ID_RECENTROMS_FIRST + i;
		InsertMenuItem(h_sub_menu, i + 3, TRUE, &menu_info);
	}
}

void main_recent_roms_add(const std::string& path)
{
	if (Config.is_recent_rom_paths_frozen)
	{
		return;
	}
	if (Config.recent_rom_paths.size() > 5)
	{
		Config.recent_rom_paths.pop_back();
	}
	std::erase(Config.recent_rom_paths, path);
	Config.recent_rom_paths.insert(Config.recent_rom_paths.begin(), path);
	main_recent_roms_build();
}

int32_t main_recent_roms_run(uint16_t menu_item_id)
{
	const int index = menu_item_id - ID_RECENTROMS_FIRST;
	if (index >= 0 && index < Config.recent_rom_paths.size()) {
		strcpy(rom_path, Config.recent_rom_paths[index].c_str());
		CreateThread(NULL, 0, start_rom, rom_path, 0, &start_rom_id);
			return 	1;
	}
	return 0;
}



void update_titlebar()
{
	std::string text = MUPEN_VERSION;

	if (emu_launched)
	{
		text += std::format(" - {}", reinterpret_cast<char*>(ROM_HEADER.nom));
	}

	if (!movie_path.empty())
	{
		char movie_filename[MAX_PATH] = {0};
		_splitpath(movie_path.string().c_str(), nullptr, nullptr, movie_filename, nullptr);
		text += std::format(" - {}", movie_filename);
	}

	SetWindowText(mainHWND, text.c_str());
}

void on_speed_modifier_changed(int32_t value)
{
	Statusbar::post(std::format("Speed limit: {}%", Config.fps_modifier));
}

void ProcessToolTips(LPARAM lParam, HWND hWnd)
{
	LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT)lParam;

	lpttt->hinst = app_instance;

	// Specify the resource identifier of the descriptive
	// text for the given button.

	switch (lpttt->hdr.idFrom)
	{
	case IDM_LOAD_ROM:
		strcpy(lpttt->lpszText, "Load ROM...");
		break;
	case EMU_PLAY:
		strcpy(lpttt->lpszText, "Resume");
		break;
	case IDM_PAUSE:
		strcpy(lpttt->lpszText, "Pause");
		break;
	case IDM_CLOSE_ROM:
		strcpy(lpttt->lpszText, "Stop");
		break;
	case IDM_FULLSCREEN:
		strcpy(lpttt->lpszText, "Fullscreen");
		break;
	case IDM_VIDEO_SETTINGS:
		strcpy(lpttt->lpszText, "Video Settings...");
		break;
	case IDM_AUDIO_SETTINGS:
		strcpy(lpttt->lpszText, "Audio Settings...");
		break;
	case IDM_INPUT_SETTINGS:
		strcpy(lpttt->lpszText, "Input Settings...");
		break;
	case IDM_RSP_SETTINGS:
		strcpy(lpttt->lpszText, "RSP Settings...");
		break;
	case IDM_SETTINGS:
		strcpy(lpttt->lpszText, "Settings...");
		break;
	default:
		break;
	}
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	char path_buffer[_MAX_PATH];
	LuaCallbacks::call_window_message(hwnd, Message, wParam, lParam);

	switch (Message)
	{
	case WM_EXECUTE_DISPATCHER:
		main_dispatcher_process();
		break;
	case WM_DROPFILES:
		{
			HDROP h_file = (HDROP)wParam;
			char fname[MAX_PATH] = {0};
			DragQueryFile(h_file, 0, fname, sizeof(fname));
			LocalFree(h_file);

			std::filesystem::path path = fname;
			std::string extension = to_lower(path.extension().string());

			if (extension == ".n64" || extension == ".z64" || extension == ".v64" || extension == ".rom")
			{
				strcpy(rom_path, fname);
				CreateThread(NULL, 0, start_rom, nullptr, 0, &start_rom_id);
			} else if (extension == ".m64")
			{
				if (!emu_launched) break;
				if (!vcr_get_read_only()) vcr_toggle_read_only();
				if (vcr_start_playback(fname, nullptr, nullptr) < 0)
				{
					printf(
						"[VCR]: Drag drop Failed to start playback of %s",
						fname);
					break;
				}
			}else if (extension == ".st" || extension == ".savestate")
			{
				if (!emu_launched) break;
				savestates_do_file(fname, e_st_job::load);
			} else if(extension == ".lua")
			{
				lua_create_and_run(path.string().c_str());
			}
			break;
		}
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		{
			BOOL hit = FALSE;
			if (!fast_forward)
			{
				if ((int)wParam == Config.fast_forward_hotkey.key)
				// fast-forward on
				{
					if (((GetKeyState(VK_SHIFT) & 0x8000) ? 1 : 0) == Config.
						fast_forward_hotkey.shift
						&& ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0) ==
						Config.fast_forward_hotkey.ctrl
						&& ((GetKeyState(VK_MENU) & 0x8000) ? 1 : 0) == Config.
						fast_forward_hotkey.alt)
					{
						fast_forward = 1;
						hit = TRUE;
					}
				}
			}
			for (const t_hotkey* hotkey : hotkeys)
			{
				if ((int)wParam == hotkey->key)
				{
					if (((GetKeyState(VK_SHIFT) & 0x8000) ? 1 : 0) == hotkey->
						shift
						&& ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0) ==
						hotkey->ctrl
						&& ((GetKeyState(VK_MENU) & 0x8000) ? 1 : 0) == hotkey->
						alt)
					{
						printf("Sent %s (%d)\n", hotkey->identifier.c_str(), hotkey->command);
						SendMessage(mainHWND, WM_COMMAND, hotkey->command, 0);
						hit = TRUE;
					}
				}
			}

			if (emu_launched)
				keyDown(wParam, lParam);
			if (!hit)
				return DefWindowProc(hwnd, Message, wParam, lParam);
		}
		break;
	case WM_SYSKEYUP:
	case WM_KEYUP:
		if ((int)wParam == Config.fast_forward_hotkey.key) // fast-forward off
		{
			fast_forward = 0;
		}
		if (emu_launched)
			keyUp(wParam, lParam);
		return DefWindowProc(hwnd, Message, wParam, lParam);
	case WM_MOUSEWHEEL:
		last_wheel_delta = GET_WHEEL_DELTA_WPARAM(wParam);
		break;
	case WM_NOTIFY:
		{
			LPNMHDR l_header = (LPNMHDR)lParam;

			if (wParam == IDC_ROMLIST)
			{
				Rombrowser::notify(lParam);
			}
			switch ((l_header)->code)
			{
			case TTN_NEEDTEXT:
				ProcessToolTips(lParam, hwnd);
				break;
			}
			return 0;
		}
	case WM_MOVE:
		{
			if (emu_launched && !FullScreenMode)
			{
				moveScreen((int)wParam, lParam);
			}
			RECT rect = {0};
			GetWindowRect(mainHWND, &rect);
			Config.window_x = rect.left;
			Config.window_y = rect.top;
			Config.window_width = rect.right - rect.left;
			Config.window_height = rect.bottom - rect.top;
			break;
		}
	case WM_SIZE:
		{
			if (!FullScreenMode)
			{
				SendMessage(Toolbar::hwnd(), TB_AUTOSIZE, 0, 0);
				SendMessage(Statusbar::hwnd(), WM_SIZE, 0, 0);
			}
			RECT rect{};
			GetClientRect(mainHWND, &rect);
			Messenger::broadcast(Messenger::Message::SizeChanged, rect);
			break;
		}
	case WM_USER + 17: SetFocus(mainHWND);
		break;
	case WM_CREATE:
		main_menu = GetMenu(hwnd);
		GetModuleFileName(NULL, path_buffer, sizeof(path_buffer));
		update_screen_timer = SetTimer(hwnd, NULL, (uint32_t)(1000 / get_primary_monitor_refresh_rate()), NULL);
		commandline_start_rom();
		return TRUE;
	case WM_DESTROY:
		save_config();
		KillTimer(mainHWND, update_screen_timer);

		PostQuitMessage(0);
		break;
	case WM_CLOSE:
		if (confirm_user_exit())
		{
			DestroyWindow(mainHWND);
			break;
		}
		return 0;
	case WM_TIMER:
		{
			static std::chrono::high_resolution_clock::time_point last_statusbar_update = std::chrono::high_resolution_clock::now();
			std::chrono::high_resolution_clock::time_point time = std::chrono::high_resolution_clock::now();

			LuaCallbacks::call_updatescreen();

			if (frame_changed)
			{
				vcr_update_statusbar();
				frame_changed = false;
			}

			// We need to create a copy of these, as they might get mutated during our enumeration
			auto frame_times = new_frame_times;
			auto vi_times = new_vi_times;

			// We throttle FPS and VI/s visual updates to 1 per second, so no unstable values are displayed
			if (time - last_statusbar_update > std::chrono::seconds(1))
			{
				// TODO: This is just a quick proof of concept, must reduce code duplication
				if (Config.show_fps && !frame_times.empty())
				{
					long long fps = 0;
					for (int i = 1; i < frame_times.size(); ++i)
					{
						fps += (frame_times[i] - frame_times[i - 1]).count();
					}
					Statusbar::post(std::format("FPS: {:.1f}", 1000.0f / (float)((fps / frame_times.size()) / 1'000'000.0f)), 2);
				}
				if (Config.show_vis_per_second && !vi_times.empty())
				{
					long long vis = 0;
					for (int i = 1; i < vi_times.size(); ++i)
					{
						vis += (vi_times[i] - vi_times[i - 1]).count();
					}
					Statusbar::post(std::format("VI/s: {:.1f}", 1000.0f / (float)((vis / vi_times.size()) / 1'000'000.0f)), 3);
				}
				last_statusbar_update = time;
			}
		}
		break;
	case WM_WINDOWPOSCHANGING: //allow gfx plugin to set arbitrary size
		return 0;
	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
			lpMMI->ptMinTrackSize.x = MIN_WINDOW_W;
			lpMMI->ptMinTrackSize.y = MIN_WINDOW_H;
			// this might break small res with gfx plugin!!!
		}
		break;
	case WM_ENTERMENULOOP:
		AutoPause = emu_paused;
		if (!emu_paused)
		{
			MenuPaused = TRUE;
			pauseEmu(FALSE);
		}
		break;

	case WM_EXITMENULOOP:
		// This message is sent when we escape the blocking menu loop, including situations where the clicked menu spawns a dialog.
		// In those situations, we would unpause the game here (since this message is sent first), and then pause it again in the menu item message handler.
		// It's almost guaranteed that a game frame will pass between those messages, so we need to wait a bit on another thread before unpausing.
		std::thread([]
		{
			Sleep(60);
			if (emu_paused && !AutoPause && MenuPaused)
			{
				resumeEmu(FALSE);
			}
			MenuPaused = FALSE;
		}).detach();
		break;
	case WM_ACTIVATE:
		UpdateWindow(hwnd);

		switch (LOWORD(wParam))
		{
		case WA_ACTIVE:
		case WA_CLICKACTIVE:
			if (Config.is_unfocused_pause_enabled && emu_paused && !AutoPause)
			{
				resumeEmu(FALSE);
				AutoPause = emu_paused;
			}
			break;

		case WA_INACTIVE:
			AutoPause = emu_paused && !MenuPaused;
			if (Config.is_unfocused_pause_enabled && !emu_paused
				/*(&& minimize*/ && !FullScreenMode)
			{
				MenuPaused = FALSE;
				pauseEmu(FALSE);
			} else if (Config.is_unfocused_pause_enabled && MenuPaused)
			{
				MenuPaused = FALSE;
			}
			break;
		default:
			break;
		}
		break;
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDM_VIDEO_SETTINGS:
			case IDM_INPUT_SETTINGS:
			case IDM_AUDIO_SETTINGS:
			case IDM_RSP_SETTINGS:
				{
					// If emu isn't launched, we don't have any loaded plugins, so we gotta load them first
					if (!emu_launched)
					{
						load_plugins();
					}

					hwnd_plug = mainHWND;
					t_plugin* plugin = video_plugin;
					switch (LOWORD(wParam))
					{
					case IDM_INPUT_SETTINGS:
						plugin = input_plugin;
						break;
					case IDM_AUDIO_SETTINGS:
						plugin = audio_plugin;
						break;
					case IDM_RSP_SETTINGS:
						plugin = rsp_plugin;
						break;
					}

					// plugin can be null when no plugins are available, but it's handled internally
					plugin_config(plugin);

					if (!emu_launched)
					{
						unload_plugins();
					}
				}
				break;
			case IDM_LOAD_LUA:
				{
					lua_create();
				}
				break;
			case IDM_FREEZE_RECENT_LUA:
				CheckMenuItem(main_menu, IDM_FREEZE_RECENT_LUA,
				              (Config.is_recent_scripts_frozen ^= 1)
					              ? MF_CHECKED
					              : MF_UNCHECKED);
				break;
			case IDM_RESET_RECENT_LUA:
				lua_recent_scripts_build(1);
				break;
			case IDM_LOAD_LATEST_LUA:
				lua_recent_scripts_run(ID_LUA_RECENT);
				break;
			case IDM_CLOSE_ALL_LUA:
				close_all_scripts();
				break;
			case IDM_TRACELOG:
				{
					if (tracelog::active())
					{
						tracelog::stop();
						ModifyMenu(main_menu, IDM_TRACELOG, MF_BYCOMMAND | MF_STRING, IDM_TRACELOG, "Start &Trace Logger...");
						break;
					}

					auto path = show_persistent_save_dialog("s_tracelog", mainHWND, L"*.log");

					if (path.empty())
					{
						break;
					}

					auto result = MessageBox(mainHWND, "Should the trace log be generated in a binary format?", "Trace Logger", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);

					tracelog::start(wstring_to_string(path).c_str(), result == IDYES);
					ModifyMenu(main_menu, IDM_TRACELOG, MF_BYCOMMAND | MF_STRING, IDM_TRACELOG, "Stop &Trace Logger");
				}
				break;
			case IDM_CLOSE_ROM:
				MenuPaused = FALSE;
				if (!confirm_user_exit())
					break;
				if (emu_launched)
				{
					//close_rom(&Id);
					CreateThread(NULL, 0, close_rom, (LPVOID)1, 0, &close_rom_id);

				}
				break;

			case IDM_PAUSE:
				{
					if (!emu_paused)
					{
						pauseEmu(vcr_is_active());
					} else if (MenuPaused)
					{
						MenuPaused = FALSE;
						CheckMenuItem(GetMenu(mainHWND), IDM_PAUSE,
						              MF_BYCOMMAND | MFS_CHECKED);
					} else
					{
						resumeEmu(vcr_is_active());
					}
					break;
				}

			case IDM_FRAMEADVANCE:
				{
					MenuPaused = FALSE;
					frame_advancing = 1;
					// vis_per_second = 0;
					// prevent old VI value from showing error if running at super fast speeds
					resumeEmu(TRUE); // maybe multithreading unsafe
				}
				break;

			case IDM_TOGGLE_READONLY:
				vcr_toggle_read_only();
				break;

			case IDM_LOOP_MOVIE:
				set_is_movie_loop_enabled(!Config.is_movie_loop_enabled);
				break;
			case IDM_PLAY_LATEST_MOVIE:
				if (!emu_launched || Config.recent_movie_paths.empty())
				{
					break;
				}
				vcr_set_read_only(TRUE);
				vcr_start_playback(Config.recent_movie_paths[0], nullptr, nullptr);
				break;
			case IDM_FREEZE_RECENT_MOVIES:
				CheckMenuItem(main_menu, IDM_FREEZE_RECENT_MOVIES,
				              (Config.is_recent_movie_paths_frozen ^= 1)
					              ? MF_CHECKED
					              : MF_UNCHECKED);
				break;
			case IDM_RESET_RECENT_MOVIES:
				vcr_recent_movies_build(1);
				break;
			case EMU_PLAY:
				if (emu_launched)
				{
					if (emu_paused)
					{
						resumeEmu(FALSE);
					}
				} else
				{
					// TODO: reimplement
					// RomList_OpenRom();
				}
				break;

			case IDM_RESET_ROM:
				if (!Config.is_reset_recording_enabled && confirm_user_exit())
					break;
				if (vcr_is_recording() && Config.is_reset_recording_enabled)
				{
					scheduled_restart = true;
					continue_vcr_on_restart_mode = true;
					Statusbar::post("Writing restart to movie");
					break;
				}
				resetEmu();
				break;

			case IDM_SETTINGS:
				{
					BetterEmulationLock lock;
					configdialog_show();
				}
				break;
			case IDM_ABOUT:
				{
					BOOL wasMenuPaused = MenuPaused;
					MenuPaused = FALSE;
					configdialog_about();
					if (wasMenuPaused)
					{
						resumeEmu(TRUE);
					}
				}
				break;
			case IDM_COREDBG:
				CoreDbg::start();
				break;
			case IDM_RAMSTART:
				{
					BOOL wasMenuPaused = MenuPaused;
					MenuPaused = FALSE;

					pauseEmu(TRUE);

					char ram_start[20] = {0};
					sprintf(ram_start, "0x%p", static_cast<void*>(rdram));

					char proc_name[MAX_PATH] = {0};
					GetModuleFileName(NULL, proc_name, MAX_PATH);
					_splitpath(proc_name, 0, 0, proc_name, 0);

					char stroop_c[1024] = {0};
					sprintf(stroop_c,
					        "<Emulator name=\"Mupen 5.0 RR\" processName=\"%s\" ramStart=\"%s\" endianness=\"little\"/>",
					        proc_name, ram_start);

					const auto stroop_str = std::string(stroop_c);
					if (MessageBoxA(mainHWND,
					                "Do you want to copy the generated STROOP config line to your clipboard?",
					                "STROOP",
					                MB_ICONINFORMATION | MB_TASKMODAL |
					                MB_YESNO) == IDYES)
					{
						copy_to_clipboard(mainHWND, stroop_str);
					}
					if (wasMenuPaused)
					{
						resumeEmu(TRUE);
					}
					break;
				}
			case IDM_LOAD_ROM:
				{
					BOOL wasMenuPaused = MenuPaused;
					MenuPaused = FALSE;

					const auto path = show_persistent_open_dialog("o_rom", mainHWND, L"*.n64;*.z64;*.v64;*.rom;*.bin;*.zip;*.usa;*.eur;*.jap");

					if (!path.empty())
					{
						strcpy(rom_path, wstring_to_string(path).c_str());
						CreateThread(nullptr, 0, start_rom, nullptr, NULL, &start_rom_id);
					}

					if (wasMenuPaused)
					{
						resumeEmu(TRUE);
					}
				}
				break;
			case IDM_EXIT:
				DestroyWindow(mainHWND);
				break;
			case IDM_FULLSCREEN:
				if (emu_launched && !vcr_is_capturing())
				{
					FullScreenMode = 1 - FullScreenMode;
					gui_ChangeWindow();
				}
				break;
			case IDM_REFRESH_ROMBROWSER:
				if (!emu_launched)
				{
					Rombrowser::build();
				}
				break;
			case IDM_SAVE_SLOT:
				savestates_do_slot(st_slot, e_st_job::save);
				break;
			case IDM_SAVE_STATE_AS:
				{
					BOOL wasMenuPaused = MenuPaused;
					MenuPaused = FALSE;

					auto path = show_persistent_save_dialog("s_savestate", hwnd, L"*.st;*.savestate");
					if (path.empty())
					{
						break;
					}

					savestates_do_file(path, e_st_job::save);

					if (wasMenuPaused)
					{
						resumeEmu(TRUE);
					}
				}
				break;
			case IDM_LOAD_SLOT:
				savestates_do_slot(st_slot, e_st_job::load);
				break;
			case IDM_LOAD_STATE_AS:
				{
					BOOL wasMenuPaused = MenuPaused;
					MenuPaused = FALSE;

					auto path = show_persistent_open_dialog("o_state", hwnd, L"*.st;*.savestate;*.st0;*.st1;*.st2;*.st3;*.st4;*.st5;*.st6;*.st7;*.st8;*.st9,*.st10");
					if (path.size() == 0)
					{
						break;
					}

					savestates_do_file(path, e_st_job::load);

					if (wasMenuPaused)
					{
						resumeEmu(TRUE);
					}
				}
				break;
			case IDM_START_MOVIE_RECORDING:
				{
					BetterEmulationLock lock;

					auto result = MovieDialog::show(false);

					if (result.path.empty())
					{
						break;
					}

					if (vcr_start_record(
							result.path.string().c_str(), result.start_flag, result.author.c_str(), result.description.c_str()) < 0)
					{
						show_modal_info(std::format("Couldn't start recording of {}", result.path.string()).c_str(), nullptr);
						break;
					}

					Config.last_movie_author = result.author;

					Statusbar::post("Recording replay");
				}
				break;
			case IDM_STOP_MOVIE_RECORDING:
				if (!vcr_is_recording())
					break;
				if (vcr_stop_record() < 0)
				{
					show_modal_info("Couldn't stop movie recording", nullptr);
					break;
				}
				ClearButtons();
				Statusbar::post("Recording stopped");
				break;
			case IDM_START_MOVIE_PLAYBACK:
				{
					BetterEmulationLock lock;

					auto result = MovieDialog::show(true);

					if (result.path.empty())
					{
						break;
					}

					auto playbackResult = vcr_start_playback(
						result.path.string(), nullptr, nullptr);

					if (playbackResult == VCR_PLAYBACK_SUCCESS)
						break;

					char err[MAX_PATH];

					sprintf(err, "Failed to start movie \"%s\" ",
					        result.path.string().c_str());

					switch (playbackResult)
					{
					case VCR_PLAYBACK_ERROR:
						strcat(err, " - unknown error");
						break;
					case VCR_PLAYBACK_SAVESTATE_MISSING:
						strcat(err, " - savestate is missing");
						break;
					case VCR_PLAYBACK_FILE_BUSY:
						strcat(err, " - file is locked");
						break;
					case VCR_PLAYBACK_INCOMPATIBLE:
						strcat(err, " - configuration incompatibility");
						break;
					default: break;
					}

					show_modal_info(err, nullptr);
				}
				break;
			case IDM_STOP_MOVIE_PLAYBACK:
				if (vcr_is_playing())
				{
					if (vcr_stop_playback() < 0); // fail quietly
					//                        MessageBox(NULL, "Couldn't stop playback.", "VCR", MB_OK);
					else
					{
						ClearButtons();
						Statusbar::post("Playback stopped");
					}
				}
				break;

			case IDM_START_FFMPEG_CAPTURE:
				{
					auto err = vcr_start_f_fmpeg_capture(
						"ffmpeg_out.mp4",
						"-pixel_format yuv420p -loglevel debug -y");
					if (err == INIT_SUCCESS)
					{
						Statusbar::post("Recording AVI with FFmpeg");
					} else
						printf("Start capture error: %d\n", err);
					break;
				}

			case IDM_START_CAPTURE_PRESET:
			case IDM_START_CAPTURE:
				if (emu_launched)
				{
					BetterEmulationLock lock;

					auto path = show_persistent_save_dialog("s_capture", hwnd, L"*.avi");
					if (path.empty())
					{
						break;
					}

					// pass false to startCapture when "last preset" option was choosen
					if (vcr_start_capture(wstring_to_string(path).c_str(), LOWORD(wParam) == IDM_START_CAPTURE) >= 0)
					{
						Statusbar::post("Recording AVI");
					}
				}

				break;
			case IDM_STOP_CAPTURE:
				if (vcr_stop_capture() < 0)
					MessageBox(NULL, "Couldn't stop capturing.", "VCR", MB_OK);
				else
				{

					Statusbar::post("Capture stopped");
				}
				break;
			case IDM_SCREENSHOT: // take/capture a screenshot
				if (Config.is_default_screenshots_directory_used)
				{
					sprintf(path_buffer, "%sScreenShots\\", app_path.c_str());
					CaptureScreen(path_buffer);
				} else
				{
					sprintf(path_buffer, "%s",
					        Config.screenshots_directory.c_str());
					CaptureScreen(path_buffer);
				}
				break;
			case IDM_RESET_RECENT_ROMS:
				main_recent_roms_build(1);
				break;
			case IDM_FREEZE_RECENT_ROMS:
				CheckMenuItem(main_menu, IDM_FREEZE_RECENT_ROMS,
							  (Config.is_recent_rom_paths_frozen ^= 1)
								  ? MF_CHECKED
								  : MF_UNCHECKED);
				break;
			case IDM_LOAD_LATEST_ROM:
				main_recent_roms_run(ID_RECENTROMS_FIRST);
				break;
			case IDM_TOOLBAR:
				Config.is_toolbar_enabled ^= true;
				Messenger::broadcast(Messenger::Message::ToolbarVisibilityChanged, (bool)Config.is_toolbar_enabled);
				CheckMenuItem(
					main_menu, IDM_TOOLBAR, MF_BYCOMMAND | (Config.is_toolbar_enabled ? MF_CHECKED : MF_UNCHECKED));
				break;
			case IDM_STATUSBAR:
				Config.is_statusbar_enabled ^= true;
				Messenger::broadcast(Messenger::Message::StatusbarVisibilityChanged, (bool)Config.is_statusbar_enabled);
				CheckMenuItem(
					main_menu, IDM_STATUSBAR, MF_BYCOMMAND | (Config.is_statusbar_enabled ? MF_CHECKED : MF_UNCHECKED));
				break;
			case IDC_INCREASE_MODIFIER:
				if (Config.fps_modifier < 50)
					Config.fps_modifier = Config.fps_modifier + 5;
				else if (Config.fps_modifier < 100)
					Config.fps_modifier = Config.fps_modifier + 10;
				else if (Config.fps_modifier < 200)
					Config.fps_modifier = Config.fps_modifier + 25;
				else if (Config.fps_modifier < 1000)
					Config.fps_modifier = Config.fps_modifier + 50;
				if (Config.fps_modifier > 1000)
					Config.fps_modifier = 1000;
				timer_init(Config.fps_modifier, &ROM_HEADER);
				on_speed_modifier_changed(Config.fps_modifier);
				break;
			case IDC_DECREASE_MODIFIER:
				if (Config.fps_modifier > 200)
					Config.fps_modifier = Config.fps_modifier - 50;
				else if (Config.fps_modifier > 100)
					Config.fps_modifier = Config.fps_modifier - 25;
				else if (Config.fps_modifier > 50)
					Config.fps_modifier = Config.fps_modifier - 10;
				else if (Config.fps_modifier > 5)
					Config.fps_modifier = Config.fps_modifier - 5;
				if (Config.fps_modifier < 5)
					Config.fps_modifier = 5;
				timer_init(Config.fps_modifier, &ROM_HEADER);
				on_speed_modifier_changed(Config.fps_modifier);
				break;
			case IDC_RESET_MODIFIER:
				Config.fps_modifier = 100;
				timer_init(Config.fps_modifier, &ROM_HEADER);
				on_speed_modifier_changed(Config.fps_modifier);
				break;
			default:
				if (LOWORD(wParam) >= IDM_SELECT_1 && LOWORD(wParam)
					<= IDM_SELECT_10)
				{
					auto slot = LOWORD(wParam) - IDM_SELECT_1;
					st_slot = slot;

					// set checked state for only the currently selected save
					for (int i = IDM_SELECT_1; i < IDM_SELECT_10; ++i)
					{
						CheckMenuItem(main_menu, i, MF_UNCHECKED);
					}
					CheckMenuItem(main_menu, LOWORD(wParam), MF_CHECKED);

				} else if (LOWORD(wParam) >= ID_SAVE_1 && LOWORD(wParam) <=
					ID_SAVE_10)
				{
					auto slot = LOWORD(wParam) - ID_SAVE_1;
					// if emu is paused and no console state is changing, we can safely perform st op instantly
					savestates_do_slot(slot, e_st_job::save);
				} else if (LOWORD(wParam) >= ID_LOAD_1 && LOWORD(wParam) <=
					ID_LOAD_10)
				{
					auto slot = LOWORD(wParam) - ID_LOAD_1;
					savestates_do_slot(slot, e_st_job::load);
				} else if (LOWORD(wParam) >= ID_RECENTROMS_FIRST &&
					LOWORD(wParam) < (ID_RECENTROMS_FIRST + Config.
						recent_rom_paths.size()))
				{
					main_recent_roms_run(LOWORD(wParam));
				} else if (LOWORD(wParam) >= ID_RECENTMOVIES_FIRST &&
					LOWORD(wParam) < (ID_RECENTMOVIES_FIRST + Config.
						recent_movie_paths.size()))
				{
					if (vcr_recent_movies_play(LOWORD(wParam)) != SUCCESS)
					{
						Statusbar::post("Couldn't load movie");
						break;
					}

					if (!emu_paused || !emu_launched)
						Statusbar::post("Playback started");
					else
						Statusbar::post("Playback started while paused");
				} else if (LOWORD(wParam) >= ID_LUA_RECENT && LOWORD(wParam) < (
					ID_LUA_RECENT + Config.recent_lua_script_paths.size()))
				{
					printf("run recent script\n");
					lua_recent_scripts_run(LOWORD(wParam));
				}
				break;
			}
		}
		break;
	default:
		return DefWindowProc(hwnd, Message, wParam, lParam);
	}

	return TRUE;
}

// kaboom
LONG WINAPI ExceptionReleaseTarget(_EXCEPTION_POINTERS* ExceptionInfo)
{
	// generate crash log

	char crash_log[1024 * 4] = {0};
	CrashHelper::generate_log(ExceptionInfo, crash_log);

	FILE* f = fopen("crash.log", "w+");
	fputs(crash_log, f);
	fclose(f);

	bool is_continuable = !(ExceptionInfo->ExceptionRecord->ExceptionFlags &
		EXCEPTION_NONCONTINUABLE);

	int result = 0;

	if (is_continuable) {
		TaskDialog(mainHWND, app_instance, L"Error",
			L"An error has occured", L"A crash log has been automatically generated. You can choose to continue program execution.", TDCBF_RETRY_BUTTON | TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, &result);
	} else {
		TaskDialog(mainHWND, app_instance, L"Error",
			L"An error has occured", L"A crash log has been automatically generated. The program will now exit.", TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, &result);
	}

	if (result == IDCLOSE) {
		return EXCEPTION_EXECUTE_HANDLER;
	}
	return EXCEPTION_CONTINUE_EXECUTION;
}


void on_task_changed(e_task task)
{
	printf("on_task_changed: %d\n", task);
}

int WINAPI WinMain(
	HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#ifdef _DEBUG
	AllocConsole();
	FILE* f = 0;
	freopen_s(&f, "CONIN$", "r", stdin);
	freopen_s(&f, "CONOUT$", "w", stdout);
	freopen_s(&f, "CONOUT$", "w", stderr);
#endif

	app_path = get_app_full_path();
	app_instance = hInstance;

	commandline_set();

	// ensure folders exist!
	CreateDirectory((app_path + "save").c_str(), NULL);
	CreateDirectory((app_path + "Mempaks").c_str(), NULL);
	CreateDirectory((app_path + "Lang").c_str(), NULL);
	CreateDirectory((app_path + "ScreenShots").c_str(), NULL);
	CreateDirectory((app_path + "plugin").c_str(), NULL);

	emu_launched = 0;
	emu_paused = 1;

	load_config();
	lua_init();
	vcr_init(on_task_changed);
	setup_dummy_info();

	WNDCLASSEX wc = {0};
	MSG msg;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(app_instance, MAKEINTRESOURCE(IDI_M64ICONBIG));
	wc.hIconSm = LoadIcon(app_instance, MAKEINTRESOURCE(IDI_M64ICONSMALL));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = g_szClassName;
	wc.lpfnWndProc = WndProc;
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MYMENU);

	RegisterClassEx(&wc);

	HACCEL accelerators = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCEL));

	Messenger::init();

	mainHWND = CreateWindowEx(
		0,
		g_szClassName,
		MUPEN_VERSION,
		WS_OVERLAPPEDWINDOW | WS_EX_COMPOSITED,
		Config.window_x, Config.window_y, Config.window_width,
		Config.window_height,
		NULL, NULL, hInstance, NULL);

	ShowWindow(mainHWND, nCmdShow);
	SetWindowLong(mainHWND, GWL_EXSTYLE, WS_EX_ACCEPTFILES);

	RECT rect{};
	GetClientRect(mainHWND, &rect);

	Messenger::subscribe(Messenger::Message::EmuLaunchedChanged, on_emu_launched_changed);
	Messenger::subscribe(Messenger::Message::CapturingChanged, on_capturing_changed);

	// Rombrowser needs to be initialized *after* toolbar, since it depends on its state smh bru
	Toolbar::init();
	Rombrowser::init();
	Statusbar::init();

	update_menu_hotkey_labels();

	set_is_movie_loop_enabled(Config.is_movie_loop_enabled);

	vcr_recent_movies_build();
	lua_recent_scripts_build();
	main_recent_roms_build();

	Messenger::broadcast(Messenger::Message::ToolbarVisibilityChanged, (bool)Config.is_toolbar_enabled);
	Messenger::broadcast(Messenger::Message::StatusbarVisibilityChanged, (bool)Config.is_statusbar_enabled);
	Messenger::broadcast(Messenger::Message::EmuLaunchedChanged, false);
	Messenger::broadcast(Messenger::Message::CapturingChanged, false);
	Messenger::broadcast(Messenger::Message::SizeChanged, rect);

	Rombrowser::build();

	//warning, this is ignored when debugger is attached (like visual studio)
	SetUnhandledExceptionFilter(ExceptionReleaseTarget);

	// raise noncontinuable exception (impossible to recover from it)
	//RaiseException(EXCEPTION_ACCESS_VIOLATION, EXCEPTION_NONCONTINUABLE, NULL, NULL);
	//
	// raise continuable exception
	//RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, NULL, NULL);

	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		if (!TranslateAccelerator(mainHWND, accelerators, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		for (t_hotkey* hotkey : hotkeys)
		{
			// modifier-only checks, cannot be obtained through windows messaging...
			if (!hotkey->key && (hotkey->shift || hotkey->ctrl || hotkey->alt))
			{
				// special treatment for fast-forward
				if (hotkey->identifier == Config.fast_forward_hotkey.identifier)
				{
					if (!frame_advancing)
					{
						// dont allow fastforward+frameadvance
						if (((GetKeyState(VK_SHIFT) & 0x8000) ? 1 : 0) ==
							hotkey->shift
							&& ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0)
							== hotkey->ctrl
							&& ((GetKeyState(VK_MENU) & 0x8000) ? 1 : 0) ==
							hotkey->alt)
						{
							fast_forward = 1;
						} else
						{
							fast_forward = 0;
						}
					}
					continue;
				}
				if (((GetKeyState(VK_SHIFT) & 0x8000) ? 1 : 0) ==
						hotkey->shift
						&& ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0) ==
						hotkey->ctrl
						&& ((GetKeyState(VK_MENU) & 0x8000) ? 1 : 0) ==
						hotkey->alt)
				{
					SendMessage(mainHWND, WM_COMMAND, hotkey->command,
								0);
				}

			}

		}

	}


	return (int)msg.wParam;
}

