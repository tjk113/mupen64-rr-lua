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
#include "Config.hpp"
#include "configdialog.h"
#include "LuaCallbacks.h"
#include "features/CrashHelper.h"
#include "LuaConsole.h"
#include "messenger.h"
#include "timers.h"
#include "../guifuncs.h"
#include "../Plugin.hpp"
#include "../../r4300/rom.h"
#include "../savestates.h"
#include "../vcr.h"
#include "../../memory/memory.h"
#include "../../memory/pif.h"
#include "../../r4300/r4300.h"
#include "../../r4300/recomph.h"
#include "../../r4300/tracelog.h"
#include "../../winproject/resource.h"
#include "features/CoreDbg.h"
#include "features/MGECompositor.h"
#include "features/MovieDialog.h"
#include "features/RomBrowser.hpp"
#include "features/Seeker.h"
#include "features/Statusbar.hpp"
#include "ffmpeg_capture/ffmpeg_capture.hpp"
#include "helpers/collection_helpers.h"
#include "helpers/math_helpers.h"
#include "helpers/string_helpers.h"
#include "helpers/win_helpers.h"
#include "wrapper/PersistentPathDialog.h"

HANDLE audio_thread_handle;
static BOOL FullScreenMode = 0;
int last_wheel_delta = 0;

HANDLE loading_handle[4];
HANDLE EmuThreadHandle;
HWND hwnd_plug;
UINT update_screen_timer;

static DWORD WINAPI ThreadFunc(LPVOID lpParam);
constexpr char g_szClassName[] = "myWindowClass";

HWND mainHWND;
HMENU main_menu;
HMENU recent_roms_menu;
HMENU recent_movies_menu;
HMENU recent_lua_menu;
HINSTANCE app_instance;

std::string app_path = "";
std::filesystem::path rom_path;

// Whether the sound thread can call aiUpdate
bool sound_allowed = true;

std::deque<std::function<void()>> dispatcher_queue;

// Flag which tells close_rom start_rom to skip some broadcasting and other operations
bool is_restarting;

// Lock to prevent start/stop race conditions
std::mutex emu_start_cs;

// Lock to prevent reset race conditions
std::mutex emu_reset_cs;

bool paused_before_menu;
bool paused_before_focus;
bool in_menu_loop;
bool vis_since_input_poll_warning_dismissed;
bool emu_starting;

namespace Recent
{
	void build(std::vector<std::string>& vec, int first_menu_id, HMENU parent_menu, bool reset = false)
	{
		for (size_t i = 0; i < vec.size(); i++)
		{
			if (vec[i].empty())
			{
				continue;
			}
			DeleteMenu(main_menu, first_menu_id + i, MF_BYCOMMAND);
		}

		if (reset)
		{
			vec.clear();
			return;
		}

		MENUITEMINFO menu_info = {0};
		menu_info.cbSize = sizeof(MENUITEMINFO);
		menu_info.fMask = MIIM_TYPE | MIIM_ID;
		menu_info.fType = MFT_STRING;
		menu_info.fState = MFS_ENABLED;

		for (size_t i = 0; i < vec.size(); i++)
		{
			if (vec[i].empty())
			{
				continue;
			}
			menu_info.dwTypeData = (LPSTR)vec[i].c_str();
			menu_info.cch = strlen(menu_info.dwTypeData);
			menu_info.wID = first_menu_id + i;
			InsertMenuItem(parent_menu, i + 3, TRUE, &menu_info);
		}
	}

	void add(std::vector<std::string>& vec, std::string val, bool frozen, int first_menu_id, HMENU parent_menu)
	{
		if (frozen)
		{
			return;
		}
		if (vec.size() > 5)
		{
			vec.pop_back();
		}
		std::erase(vec, val);
		vec.insert(vec.begin(), val);
		build(vec, first_menu_id, parent_menu);
	}

	std::string element_at(std::vector<std::string> vec, int first_menu_id, int menu_id)
	{
		const int index = menu_id - first_menu_id;
		if (index >= 0 && index < vec.size()) {
			return vec[index];
		}
		return "";
	}

}

void update_titlebar()
{
	std::string text = MUPEN_VERSION;

	if (emu_starting)
	{
		text += " - Starting...";
	}

	if (emu_launched)
	{
		text += std::format(" - {}", reinterpret_cast<char*>(ROM_HEADER.nom));
	}

	if (m_task != e_task::idle)
	{
		char movie_filename[MAX_PATH] = {0};
		_splitpath(movie_path.string().c_str(), nullptr, nullptr, movie_filename, nullptr);
		text += std::format(" - {}", movie_filename);
	}

	SetWindowText(mainHWND, text.c_str());
}

#pragma region Change notifications

void on_movie_recording_or_playback_started(std::any data)
{
	auto value = std::any_cast<std::filesystem::path>(data);
	Recent::add(Config.recent_movie_paths, value.string(), Config.is_recent_movie_paths_frozen, ID_RECENTMOVIES_FIRST, recent_movies_menu);
}

void on_script_started(std::any data)
{
	auto value = std::any_cast<std::filesystem::path>(data);
	Recent::add(Config.recent_lua_script_paths, value.string(), Config.is_recent_scripts_frozen, ID_LUA_RECENT, recent_lua_menu);
}

void on_task_changed(std::any data)
{
	auto value = std::any_cast<e_task>(data);

	switch (value) {
	case e_task::idle:
		EnableMenuItem(main_menu, IDM_STOP_MOVIE_PLAYBACK, MF_GRAYED);
		EnableMenuItem(main_menu, IDM_STOP_MOVIE_RECORDING, MF_GRAYED);
		break;
	case e_task::start_recording:
	case e_task::start_recording_from_snapshot:
	case e_task::start_recording_from_existing_snapshot:
	case e_task::recording:
		EnableMenuItem(main_menu, IDM_STOP_MOVIE_PLAYBACK, MF_GRAYED);
		EnableMenuItem(main_menu, IDM_STOP_MOVIE_RECORDING, MF_ENABLED);
		break;
	case e_task::start_playback:
	case e_task::start_playback_from_snapshot:
	case e_task::playback:
		EnableMenuItem(main_menu, IDM_STOP_MOVIE_PLAYBACK, MF_ENABLED);
		EnableMenuItem(main_menu, IDM_STOP_MOVIE_RECORDING, MF_GRAYED);
		break;
	}

	EnableMenuItem(main_menu, IDM_SEEKER, (task_is_playback(value) && emu_launched) ? MF_ENABLED : MF_GRAYED);

	update_titlebar();
}

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
	EnableMenuItem(main_menu, IDM_SEEKER, value ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(main_menu, IDM_COREDBG, value && Config.core_type == 2 ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(main_menu, IDM_START_MOVIE_RECORDING, value ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(main_menu, IDM_STOP_MOVIE_RECORDING, MF_GRAYED);

	if (Config.is_statusbar_enabled) CheckMenuItem(
		main_menu, IDM_STATUSBAR, MF_BYCOMMAND | MF_CHECKED);
	else CheckMenuItem(main_menu, IDM_STATUSBAR, MF_BYCOMMAND | MF_UNCHECKED);
	if (Config.is_recent_movie_paths_frozen) CheckMenuItem(
		main_menu, IDM_FREEZE_RECENT_MOVIES, MF_BYCOMMAND | MF_CHECKED);
	if (Config.is_recent_scripts_frozen) CheckMenuItem(
		main_menu, IDM_FREEZE_RECENT_LUA, MF_BYCOMMAND | MF_CHECKED);
	if (Config.is_recent_rom_paths_frozen) CheckMenuItem(
		main_menu, IDM_FREEZE_RECENT_ROMS, MF_BYCOMMAND | MF_CHECKED);

	update_titlebar();
	// Some menu items, like movie ones, depend on both this and vcr task
	on_task_changed(m_task);

	Recent::add(Config.recent_rom_paths, rom_path.string(), Config.is_recent_rom_paths_frozen, ID_RECENTROMS_FIRST, recent_roms_menu);
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

	update_titlebar();
}

void on_speed_modifier_changed(std::any data)
{
	auto value = std::any_cast<int32_t>(data);

	Statusbar::post(std::format("Speed limit: {}%", value));
}

void on_emu_paused_changed(std::any data)
{
	auto value = std::any_cast<bool>(data);

	sound_allowed = !value;
	frame_changed = true;
}

void on_vis_since_input_poll_exceeded(std::any)
{
	if (vis_since_input_poll_warning_dismissed)
	{
		return;
	}

	if(!Config.crash_dialog || MessageBox(mainHWND, "An unusual execution pattern was detected. Continuing might leave the emulator in an unusable state.\r\nWould you like to terminate emulation?", "Warning",
			   MB_ICONWARNING | MB_YESNO) == IDYES)
	{
		std::thread([] { close_rom(); }).detach();
	}
	vis_since_input_poll_warning_dismissed = true;
}

void on_movie_loop_changed(std::any data)
{
	auto value = std::any_cast<bool>(data);

	CheckMenuItem(main_menu, IDM_LOOP_MOVIE,
				  MF_BYCOMMAND | (value
									  ? MFS_CHECKED
									  : MFS_UNCHECKED));

	Statusbar::post(value
							 ? "Movies restart after ending"
							 : "Movies stop after ending");
}

void on_readonly_changed(std::any data)
{
	auto value = std::any_cast<bool>(data);

	CheckMenuItem(main_menu, IDM_VCR_READONLY,
				  MF_BYCOMMAND | (value
									  ? MFS_CHECKED
									  : MFS_UNCHECKED));

	Statusbar::post(value
							 ? "Read-only"
							 : "Read/write");
}


BetterEmulationLock::BetterEmulationLock()
{
	was_paused = emu_paused;
	pause_emu();
}

BetterEmulationLock::~BetterEmulationLock()
{
	if (!was_paused)
		resume_emu();
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


int start_rom(std::filesystem::path path){

	// Kill any roms that are still running
	if (emu_launched) {
		close_rom();
	}

	std::unique_lock lock(emu_start_cs, std::try_to_lock);
	if(!lock.owns_lock()){
		return 0;
	}

	Messenger::broadcast(Messenger::Message::EmuStartingChanged, true);

	// If we get a movie instead of a rom, we try to search the available rom lists to find one matching the movie
	if (path.extension() == ".m64")
	{
		t_movie_header movie_header{};
		if (VCR::parse_header(path, &movie_header) != VCR::Result::Ok)
		{
			Messenger::broadcast(Messenger::Message::EmuStartingChanged, false);
			return 0;
		}

		const auto matching_rom = Rombrowser::find_available_rom([&movie_header] (auto header)
		{
			return movie_header.rom_crc1 == header.CRC1;
		});

		if (matching_rom.empty())
		{
			Messenger::broadcast(Messenger::Message::EmuStartingChanged, false);
			return 0;
		}

		path = matching_rom;
	}

	rom_path = path;
	auto start_time = std::chrono::high_resolution_clock::now();

	// TODO: keep plugins loaded and only unload and reload them when they actually change
	printf("Loading plugins\n");
	if (!load_plugins())
	{
		if (MessageBox(mainHWND,
		               "Plugins couldn't be loaded.\r\nDo you want to change the selected plugins?",
		               nullptr,
		               MB_ICONQUESTION | MB_YESNO) == IDYES)
		{
			SendMessage(mainHWND, WM_COMMAND, MAKEWPARAM(IDM_SETTINGS, 0), 0);
		}
		Messenger::broadcast(Messenger::Message::EmuStartingChanged, false);
		return 0;
	}

	// valid rom is required to start emulation
	if (!rom_load(path.string().c_str()))
	{
		MessageBox(mainHWND, "Failed to open ROM", "Error", MB_ICONERROR | MB_OK);
		unload_plugins();
		Messenger::broadcast(Messenger::Message::EmuStartingChanged, false);
		return 0;
	}

	timer_init(Config.fps_modifier, &ROM_HEADER);

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

	while (!emu_launched);

	// Between emu_launched being set to 1 and core finishing setup, there's some time we want to bridge
	Sleep(100);

	return 1;
}

void close_rom(bool stop_vcr)
{
	if (!emu_launched)
	{
		return;
	}

	std::unique_lock lock(emu_start_cs, std::try_to_lock);
	if(!lock.owns_lock()){
		return;
	}

	resume_emu();

	if (stop_vcr)
	{
		vcr_core_stopped();
	}

	printf("Closing emulation thread...\n");

	// we signal the core to stop, then wait until thread exits
	terminate_emu();

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

	if (!is_restarting)
	{
		Messenger::broadcast(Messenger::Message::EmuLaunchedChanged, false);
	}
}

void clear_save_data()
{
	{
		if (FILE* f = fopen(get_sram_path().string().c_str(), "wb"))
		{
			extern unsigned char sram[0x8000];
			for (unsigned char& i : sram) i = 0;
			fwrite(sram, 1, 0x8000, f);
			fclose(f);
		}
	}
	{
		if (FILE* f = fopen(get_eeprom_path().string().c_str(), "wb"))
		{
			extern unsigned char eeprom[0x8000];
			for (int i = 0; i < 0x800; i++) eeprom[i] = 0;
			fwrite(eeprom, 1, 0x800, f);
			fclose(f);
		}
	}
	{
		if (FILE* f = fopen(get_mempak_path().string().c_str(), "wb"))
		{
			extern unsigned char mempack[4][0x8000];
			for (auto& j : mempack)
			{
				for (int i = 0; i < 0x800; i++) j[i] = 0;
				fwrite(j, 1, 0x800, f);
			}
			fclose(f);
		}
	}
}

bool reset_rom(bool reset_save_data, bool stop_vcr)
{
	std::unique_lock lock(emu_reset_cs, std::try_to_lock);
	if(!lock.owns_lock()){
		return false;
	}

	if (!emu_launched)
		return false;

	// why is it so damned difficult to reset the game?
	// right now it's hacked to exit to the GUI then re-load the ROM,
	// but it should be possible to reset the game while it's still running
	// simply by clearing out some memory and maybe notifying the plugins...
	frame_advancing = false;
	is_restarting = true;

	close_rom(stop_vcr);
	if (reset_save_data)
	{
		clear_save_data();
	}
	start_rom(rom_path);

	is_restarting = false;
	Messenger::broadcast(Messenger::Message::ResetCompleted, nullptr);
	return true;
}

DWORD WINAPI audio_thread(LPVOID)
{
	printf("Sound thread entering...\n");
	while (emu_launched)
	{
		if(VCR::is_seeking())
		{
			Sleep(1);
			continue;
		}
		aiUpdate(1);
	}
	printf("Sound thread exiting...\n");
	ExitThread(0);
}

static DWORD WINAPI ThreadFunc(LPVOID lpParam)
{
	auto start_time = std::chrono::high_resolution_clock::now();
	init_memory();
	vis_since_input_poll_warning_dismissed = false;
	romOpen_gfx();
	romOpen_input();
	romOpen_audio();

	dynacore = Config.core_type;

	printf("Emu thread: Emulation started....\n");

	LuaCallbacks::call_reset();

	printf("emu thread entry %dms\n", static_cast<int>((std::chrono::high_resolution_clock::now() - start_time).count() / 1'000'000));
	emu_paused = 0;
	emu_launched = 1;
	audio_thread_handle = CreateThread(nullptr, 0, audio_thread, nullptr, 0, nullptr);

	Messenger::broadcast(Messenger::Message::EmuLaunchedChanged, true);
	Messenger::broadcast(Messenger::Message::EmuStartingChanged, false);

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
			auto drop = (HDROP)wParam;
			char fname[MAX_PATH] = {0};
			DragQueryFile(drop, 0, fname, sizeof(fname));

			std::filesystem::path path = fname;
			std::string extension = to_lower(path.extension().string());

			if (extension == ".n64" || extension == ".z64" || extension == ".v64" || extension == ".rom")
			{
				std::thread([path] { start_rom(path); }).detach();
			} else if (extension == ".m64")
			{
				Config.vcr_readonly = true;
				Messenger::broadcast(Messenger::Message::ReadonlyChanged, (bool)Config.vcr_readonly);
				std::thread([fname] { VCR::start_playback(fname); }).detach();
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
						// We only want to send it if the corresponding menu item exists and is enabled
						auto state = GetMenuState(main_menu, hotkey->down_cmd, MF_BYCOMMAND);
						if (state != -1 && (state & MF_DISABLED || state & MF_GRAYED))
						{
							printf("Dismissed %s (%d)\n", hotkey->identifier.c_str(), hotkey->down_cmd);
							continue;
						}
						printf("Sent down %s (%d)\n", hotkey->identifier.c_str(), hotkey->down_cmd);
						SendMessage(mainHWND, WM_COMMAND, hotkey->down_cmd, 0);
						hit = TRUE;
					}
				}
			}

			if (keyDown && emu_launched)
				keyDown(wParam, lParam);
			if (!hit)
				return DefWindowProc(hwnd, Message, wParam, lParam);
			break;
		}
	case WM_SYSKEYUP:
	case WM_KEYUP:
		{
			BOOL hit = FALSE;
			for (const t_hotkey* hotkey : hotkeys)
			{
				if (!hotkey->up_cmd)
				{
					continue;
				}

				if ((int)wParam == hotkey->key)
				{
					if (((GetKeyState(VK_SHIFT) & 0x8000) ? 1 : 0) == hotkey->
						shift
						&& ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0) ==
						hotkey->ctrl
						&& ((GetKeyState(VK_MENU) & 0x8000) ? 1 : 0) == hotkey->
						alt)
					{
						// We only want to send it if the corresponding menu item exists and is enabled
						auto state = GetMenuState(main_menu, hotkey->up_cmd, MF_BYCOMMAND);
						if (state != -1 && (state & MF_DISABLED || state & MF_GRAYED))
						{
							printf("Dismissed %s (%d)\n", hotkey->identifier.c_str(), hotkey->up_cmd);
							continue;
						}
						printf("Sent up %s (%d)\n", hotkey->identifier.c_str(), hotkey->up_cmd);
						SendMessage(mainHWND, WM_COMMAND, hotkey->up_cmd, 0);
						hit = TRUE;
					}
				}
			}

			if (keyUp && emu_launched)
				keyUp(wParam, lParam);
			if (!hit)
				return DefWindowProc(hwnd, Message, wParam, lParam);
			break;
		}
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
				SendMessage(Statusbar::hwnd(), WM_SIZE, 0, 0);
			}
			RECT rect{};
			GetClientRect(mainHWND, &rect);
			Messenger::broadcast(Messenger::Message::SizeChanged, rect);
			break;
		}
	case WM_FOCUS_MAIN_WINDOW:
		SetFocus(mainHWND);
		break;
	case WM_CREATE:
		main_menu = GetMenu(hwnd);
		GetModuleFileName(NULL, path_buffer, sizeof(path_buffer));
		update_screen_timer = SetTimer(hwnd, NULL, (uint32_t)(1000 / get_primary_monitor_refresh_rate()), NULL);
		MGECompositor::create(hwnd);
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

			if (frame_changed)
			{
				vcr_update_statusbar();
				frame_changed = false;
			}

			// NOTE: We don't invalidate the controls in their WM_PAINT, since that generates too many WM_PAINTs and fills up the message queue
			// Instead, we invalidate them driven by not so high-freq heartbeat like previously.
			for (auto map : hwnd_lua_map)
			{
				map.second->invalidate_visuals();
			}

			// We need to create a copy of these, as they might get mutated during our enumeration
			auto frame_times = new_frame_times;
			auto vi_times = new_vi_times;

			// We throttle FPS and VI/s visual updates to 1 per second, so no unstable values are displayed
			if (time - last_statusbar_update > std::chrono::seconds(1))
			{
				Statusbar::post(std::format("FPS: {:.1f}", get_rate_per_second_from_times(frame_times)), Statusbar::Section::FPS);
				Statusbar::post(std::format("VI/s: {:.1f}", get_rate_per_second_from_times(vi_times)), Statusbar::Section::VIs);
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
	case WM_INITMENU:
		{
			CheckMenuItem(main_menu, IDM_PAUSE, MF_BYCOMMAND | (paused_before_menu ? MF_CHECKED : MF_UNCHECKED));
		}
		break;
	case WM_ENTERMENULOOP:
		in_menu_loop = true;
		paused_before_menu = emu_paused;
		pause_emu();
		break;

	case WM_EXITMENULOOP:
		// This message is sent when we escape the blocking menu loop, including situations where the clicked menu spawns a dialog.
		// In those situations, we would unpause the game here (since this message is sent first), and then pause it again in the menu item message handler.
		// It's almost guaranteed that a game frame will pass between those messages, so we need to wait a bit on another thread before unpausing.
		std::thread([]
		{
			Sleep(60);
			in_menu_loop = false;
			if (!paused_before_menu)
			{
				resume_emu();
			}
		}).detach();
		break;
	case WM_ACTIVATE:
		UpdateWindow(hwnd);

		if (!Config.is_unfocused_pause_enabled)
		{
			break;
		}

		switch (LOWORD(wParam))
		{
		case WA_ACTIVE:
		case WA_CLICKACTIVE:
			if (!paused_before_focus)
			{
				resume_emu();
			}
			break;

		case WA_INACTIVE:
			paused_before_focus = emu_paused;
			pause_emu();
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
				if (!confirm_user_exit())
					break;
				std::thread([] { close_rom(); }).detach();
				break;
			case IDM_FASTFORWARD_ON:
				fast_forward = 1;
				break;
			case IDM_FASTFORWARD_OFF:
				fast_forward = 0;
				break;
			case IDM_PAUSE:
				{
					// FIXME: While this is a beautiful and clean solution, there has to be a better way to handle this
					// We're too close to release to care tho
					if (in_menu_loop)
					{
						if (paused_before_menu)
						{
							resume_emu();
							paused_before_menu = false;
							break;
						}
						paused_before_menu = true;
						pause_emu();
					} else
					{
						if (emu_paused)
						{
							resume_emu();
							break;
						}
						pause_emu();
					}

					break;
				}

			case IDM_FRAMEADVANCE:
				{
					frame_advancing = 1;
					resume_emu();
				}
				break;

			case IDM_VCR_READONLY:
				Config.vcr_readonly ^= true;
				Messenger::broadcast(Messenger::Message::ReadonlyChanged, (bool)Config.vcr_readonly);
				break;

			case IDM_LOOP_MOVIE:
				Config.is_movie_loop_enabled ^= true;
				Messenger::broadcast(Messenger::Message::MovieLoopChanged, (bool)Config.is_movie_loop_enabled);
				break;


			case EMU_PLAY:
				resume_emu();
				break;

			case IDM_RESET_ROM:
				if (!Config.is_reset_recording_enabled && confirm_user_exit())
					break;

				if (Config.is_reset_recording_enabled && vcr_is_recording())
				{
					Messenger::broadcast(Messenger::Message::ResetRequested, nullptr);
					break;
				}

				std::thread([] { reset_rom(); }).detach();
				break;

			case IDM_SETTINGS:
				{
					BetterEmulationLock lock;
					configdialog_show();
				}
				break;
			case IDM_ABOUT:
				{
					BetterEmulationLock lock;
					configdialog_about();
				}
				break;
			case IDM_COREDBG:
				CoreDbg::start();
				break;
			case IDM_SEEKER:
				{
					BetterEmulationLock lock;
					Seeker::show();
				}
				break;
			case IDM_RAMSTART:
				{
					BetterEmulationLock lock;

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
					break;
				}
			case IDM_LOAD_ROM:
				{
					BetterEmulationLock lock;

					const auto path = show_persistent_open_dialog("o_rom", mainHWND, L"*.n64;*.z64;*.v64;*.rom;*.bin;*.zip;*.usa;*.eur;*.jap");

					if (!path.empty())
					{
						std::thread([path] { start_rom(path); }).detach();
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
				savestates_do_slot(-1, e_st_job::save);
				break;
			case IDM_SAVE_STATE_AS:
				{
					BetterEmulationLock lock;

					auto path = show_persistent_save_dialog("s_savestate", hwnd, L"*.st;*.savestate");
					if (path.empty())
					{
						break;
					}

					savestates_do_file(path, e_st_job::save);
				}
				break;
			case IDM_LOAD_SLOT:
				savestates_do_slot(-1, e_st_job::load);
				break;
			case IDM_LOAD_STATE_AS:
				{
					BetterEmulationLock lock;

					auto path = show_persistent_open_dialog("o_state", hwnd, L"*.st;*.savestate;*.st0;*.st1;*.st2;*.st3;*.st4;*.st5;*.st6;*.st7;*.st8;*.st9,*.st10");
					if (path.empty())
					{
						break;
					}

					savestates_do_file(path, e_st_job::load);
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

					Config.pause_at_frame = result.pause_at;
					Config.pause_at_last_frame = result.pause_at_last;

					std::thread([result]{ VCR::start_playback(result.path); }).detach();
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
				Recent::build(Config.recent_rom_paths, ID_RECENTROMS_FIRST, recent_roms_menu, true);
				break;
			case IDM_RESET_RECENT_MOVIES:
				Recent::build(Config.recent_movie_paths, ID_RECENTMOVIES_FIRST, recent_movies_menu, true);
				break;
			case IDM_RESET_RECENT_LUA:
				Recent::build(Config.recent_lua_script_paths, ID_LUA_RECENT, recent_lua_menu, true);
				break;
			case IDM_FREEZE_RECENT_ROMS:
				CheckMenuItem(main_menu, IDM_FREEZE_RECENT_ROMS,
							  (Config.is_recent_rom_paths_frozen ^= 1)
								  ? MF_CHECKED
								  : MF_UNCHECKED);
				break;
			case IDM_FREEZE_RECENT_MOVIES:
				CheckMenuItem(main_menu, IDM_FREEZE_RECENT_MOVIES,
							  (Config.is_recent_movie_paths_frozen ^= 1)
								  ? MF_CHECKED
								  : MF_UNCHECKED);
				break;
			case IDM_FREEZE_RECENT_LUA:
				CheckMenuItem(main_menu, IDM_FREEZE_RECENT_LUA,
							  (Config.is_recent_scripts_frozen ^= 1)
								  ? MF_CHECKED
								  : MF_UNCHECKED);
				break;
			case IDM_LOAD_LATEST_LUA:
				SendMessage(mainHWND, WM_COMMAND, MAKEWPARAM(ID_LUA_RECENT, 0), 0);
				break;
			case IDM_LOAD_LATEST_ROM:
				SendMessage(mainHWND, WM_COMMAND, MAKEWPARAM(ID_RECENTROMS_FIRST, 0), 0);
				break;
			case IDM_PLAY_LATEST_MOVIE:
				SendMessage(mainHWND, WM_COMMAND, MAKEWPARAM(ID_RECENTMOVIES_FIRST, 0), 0);
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
				break;
			case IDC_RESET_MODIFIER:
				Config.fps_modifier = 100;
				timer_init(Config.fps_modifier, &ROM_HEADER);
				break;
			default:
				if (LOWORD(wParam) >= IDM_SELECT_1 && LOWORD(wParam)
					<= IDM_SELECT_10)
				{
					auto slot = LOWORD(wParam) - IDM_SELECT_1;
					savestates_set_slot(slot);

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
					auto path = Recent::element_at(Config.recent_rom_paths, ID_RECENTROMS_FIRST, LOWORD(wParam));
					if (path.empty())
						break;

					std::thread([path] { start_rom(path); }).detach();
				} else if (LOWORD(wParam) >= ID_RECENTMOVIES_FIRST &&
					LOWORD(wParam) < (ID_RECENTMOVIES_FIRST + Config.
						recent_movie_paths.size()))
				{
					auto path = Recent::element_at(Config.recent_movie_paths, ID_RECENTMOVIES_FIRST, LOWORD(wParam));
					if (path.empty())
						break;

					Config.vcr_readonly = true;
					Messenger::broadcast(Messenger::Message::ReadonlyChanged, (bool)Config.vcr_readonly);
					std::thread([path] { VCR::start_playback(path);}).detach();
				} else if (LOWORD(wParam) >= ID_LUA_RECENT && LOWORD(wParam) < (
					ID_LUA_RECENT + Config.recent_lua_script_paths.size()))
				{
					auto path = Recent::element_at(Config.recent_lua_script_paths, ID_LUA_RECENT, LOWORD(wParam));
					if (path.empty())
						break;

					lua_create_and_run(path.c_str());
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

	if (!Config.crash_dialog)
	{
		return EXCEPTION_EXECUTE_HANDLER;
	}

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

	Messenger::init();

	app_path = get_app_full_path();
	app_instance = hInstance;

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
	MGECompositor::init();

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

	recent_roms_menu = GetSubMenu(GetSubMenu(main_menu, 0), 5);
	recent_movies_menu = GetSubMenu(GetSubMenu(main_menu, 3), 6);
	recent_lua_menu = GetSubMenu(GetSubMenu(main_menu, 6), 2);

	RECT rect{};
	GetClientRect(mainHWND, &rect);

	Messenger::subscribe(Messenger::Message::EmuLaunchedChanged, on_emu_launched_changed);
	Messenger::subscribe(Messenger::Message::EmuPausedChanged, on_emu_paused_changed);
	Messenger::subscribe(Messenger::Message::CapturingChanged, on_capturing_changed);
	Messenger::subscribe(Messenger::Message::MovieLoopChanged, on_movie_loop_changed);
	Messenger::subscribe(Messenger::Message::ReadonlyChanged, on_readonly_changed);
	Messenger::subscribe(Messenger::Message::TaskChanged, on_task_changed);
	Messenger::subscribe(Messenger::Message::MovieRecordingStarted, on_movie_recording_or_playback_started);
	Messenger::subscribe(Messenger::Message::MoviePlaybackStarted, on_movie_recording_or_playback_started);
	Messenger::subscribe(Messenger::Message::ScriptStarted, on_script_started);
	Messenger::subscribe(Messenger::Message::SpeedModifierChanged, on_speed_modifier_changed);
	Messenger::subscribe(Messenger::Message::LagLimitExceeded, on_vis_since_input_poll_exceeded);
	Messenger::subscribe(Messenger::Message::EmuStartingChanged, [] (std::any data)
	{
		emu_starting = std::any_cast<bool>(data);
		update_titlebar();
	});

	// Rombrowser needs to be initialized *after* other components, since it depends on their state smh bru
	Statusbar::init();
	Rombrowser::init();
	VCR::init();
	Cli::init();
	Seeker::init();
	savestates_init();

	update_menu_hotkey_labels();

	Recent::build(Config.recent_rom_paths, ID_RECENTROMS_FIRST, recent_roms_menu);
	Recent::build(Config.recent_movie_paths, ID_RECENTMOVIES_FIRST, recent_movies_menu);
	Recent::build(Config.recent_lua_script_paths, ID_LUA_RECENT, recent_lua_menu);

	Messenger::broadcast(Messenger::Message::StatusbarVisibilityChanged, (bool)Config.is_statusbar_enabled);
	Messenger::broadcast(Messenger::Message::MovieLoopChanged, (bool)Config.is_movie_loop_enabled);
	Messenger::broadcast(Messenger::Message::ReadonlyChanged, (bool)Config.vcr_readonly);
	Messenger::broadcast(Messenger::Message::EmuLaunchedChanged, false);
	Messenger::broadcast(Messenger::Message::CapturingChanged, false);
	Messenger::broadcast(Messenger::Message::SizeChanged, rect);
	Messenger::broadcast(Messenger::Message::AppReady, nullptr);

	Rombrowser::build();

	//warning, this is ignored when debugger is attached (like visual studio)
	SetUnhandledExceptionFilter(ExceptionReleaseTarget);

	// raise noncontinuable exception (impossible to recover from it)
	//RaiseException(EXCEPTION_ACCESS_VIOLATION, EXCEPTION_NONCONTINUABLE, NULL, NULL);
	//
	// raise continuable exception
	//RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, NULL, NULL);

	while (GetMessage(&msg, NULL, 0, 0) != 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		for (t_hotkey* hotkey : hotkeys)
		{
			// modifier-only checks, cannot be obtained through windows messaging...
			if (!hotkey->key && (hotkey->shift || hotkey->ctrl || hotkey->alt))
			{
				if (((GetKeyState(VK_SHIFT) & 0x8000) ? 1 : 0) ==
						hotkey->shift
						&& ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0) ==
						hotkey->ctrl
						&& ((GetKeyState(VK_MENU) & 0x8000) ? 1 : 0) ==
						hotkey->alt)
				{
					SendMessage(mainHWND, WM_COMMAND, hotkey->down_cmd,
								0);
				}

			}

		}

	}


	return (int)msg.wParam;
}

