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


#include "LuaConsole.h"

#include "Recent.h"
#include "ffmpeg_capture/ffmpeg_capture.hpp"

#if defined(__cplusplus) && !defined(_MSC_VER)
extern "C" {
#endif

#include <windows.h> // TODO: Include Windows.h not windows.h and see if it breaks
#include <Shlwapi.h>
#ifndef _WIN32_IE
#define _WIN32_IE 0x0500
#endif
#include <commctrl.h>
#include <stdlib.h>
#include <math.h>
#include <filesystem>
#ifndef _MSC_VER
#include <dirent.h>
#endif
#include "../../winproject/resource.h"
#include "../plugin.hpp"
#include "../rom.h"
#include "../../r4300/r4300.h"
#include "../../memory/memory.h"
#include "translation.h"
#include "RomBrowser.hpp"
#include "main_win.h"
#include "configdialog.h"
#include "../guifuncs.h"
#include "../mupenIniApi.h"
#include "../savestates.h"
#include "timers.h"
#include "config.hpp"
#include "RomSettings.h"
#include "commandline.h"
#include "CrashHandlerDialog.h"
#include "CrashHelper.h"
#include "wrapper/ReassociatingFileDialog.h"
#include "../vcr.h"
#include "../../r4300/recomph.h"

#define EMULATOR_MAIN_CPP_DEF
#include "../../memory/pif.h"
#undef EMULATOR_MAIN_CPP_DEF

#include <gdiplus.h>
#include "../main/win/GameDebugger.h"

#pragma comment (lib,"Gdiplus.lib")

	void StartMovies();
	void StartLuaScripts();
	void StartSavestate();

	typedef std::string String;

	bool ffup = false;
	BOOL forceIgnoreRSP = false;

#if defined(__cplusplus) && !defined(_MSC_VER)
}
#endif



#ifdef _MSC_VER
#define snprintf	_snprintf
#define strcasecmp	stricmp
#define strncasecmp	strnicmp
#endif


static DWORD Id;
static DWORD SOUNDTHREADID;
static HANDLE SoundThreadHandle;
static BOOL FullScreenMode = 0;
DWORD WINAPI closeRom(LPVOID lpParam);

HANDLE EmuThreadHandle;
HWND hwnd_plug;

static int currentSaveState = 1;

static DWORD WINAPI ThreadFunc(LPVOID lpParam);

const char g_szClassName[] = "myWindowClass";
char LastSelectedRom[_MAX_PATH];
bool scheduled_restart = false;
static BOOL restart_mode = 0;
BOOL really_restart_mode = 0;
BOOL clear_sram_on_restart_mode = 0;
BOOL continue_vcr_on_restart_mode = 0;
BOOL just_restarted_flag = 0;
static BOOL AutoPause = 0;
static BOOL MenuPaused = 0;
static HWND hStaticHandle; //Handle for static place

char TempMessage[200];
int emu_launched; // emu_emulating
int emu_paused;
int recording;
HWND hTool, mainHWND, hStatus;
HINSTANCE app_hInstance;
BOOL manualFPSLimit = TRUE;
BOOL ignoreErrorEmulation = FALSE;
char statusmsg[800];

char correctedPath[260];
#define INCOMPATIBLE_PLUGINS_AMOUNT 1 // this is so bad
const char pluginBlacklist[INCOMPATIBLE_PLUGINS_AMOUNT][256] = {"Azimer\'s Audio v0.7"};

ReassociatingFileDialog fdBrowseMovie;
ReassociatingFileDialog fdBrowseMovie2;
ReassociatingFileDialog fdLoadRom;
ReassociatingFileDialog fdSaveLoadAs;
ReassociatingFileDialog fdSaveStateAs;
ReassociatingFileDialog fdStartCapture;

TCHAR CoreNames[3][30] = {TEXT("Interpreter"), TEXT("Dynamic Recompiler"), TEXT("Pure Interpreter")};

char AppPath[MAX_PATH];

void ClearButtons() {
	BUTTONS zero = {0};
	for (int i = 0; i < 4; i++) {
		setKeys(i, zero);
	}
}
void getAppFullPath(char* ret) {
	char drive[_MAX_DRIVE], dirn[_MAX_DIR];
	char fname[_MAX_FNAME], ext[_MAX_EXT];
	char path_buffer[_MAX_DIR];

	GetModuleFileName(NULL, path_buffer, sizeof(path_buffer));
	_splitpath(path_buffer, drive, dirn, fname, ext);
	strcpy(ret, drive);
	strcat(ret, dirn);
}



static void gui_ChangeWindow() {
	if (FullScreenMode) {
		EnableStatusbar();
		EnableWindow(hTool, FALSE);
		ShowWindow(hTool, SW_HIDE);
		ShowWindow(hStatus, SW_HIDE);
		ShowCursor(FALSE);
		changeWindow();
	} else {
		changeWindow();
		ShowWindow(hTool, SW_SHOW);
		EnableWindow(hTool, TRUE);
		ShowWindow(hStatus, SW_SHOW);
		ShowCursor(TRUE);
	}

}

static int LastState = ID_CURRENTSAVE_1;
void SelectState(HWND hWnd, int StateID) {
	HMENU hMenu = GetMenu(hWnd);
	CheckMenuItem(hMenu, LastState, MF_BYCOMMAND | MFS_UNCHECKED);
	CheckMenuItem(hMenu, StateID, MF_BYCOMMAND | MFS_CHECKED);
	currentSaveState = StateID - (ID_CURRENTSAVE_1 - 1);
	LastState = StateID;
	savestates_select_slot(currentSaveState);
}

void SaveTheState(HWND hWnd, int StateID) {
	SelectState(hWnd, (StateID - ID_SAVE_1) + ID_CURRENTSAVE_1);

//	SendMessage(hWnd, WM_COMMAND, STATE_SAVE, 0);
	if (!emu_paused || MenuPaused)
		savestates_job = SAVESTATE;
	else if (emu_launched)
		savestates_save();
}

void LoadTheState(HWND hWnd, int StateID) {
	SelectState(hWnd, (StateID - ID_LOAD_1) + ID_CURRENTSAVE_1);
	if (emu_launched) {
		savestates_job = LOADSTATE;
	}
}

char* getExtension(char* str) {
	if (strlen(str) > 3) return str + strlen(str) - 3;
	else return NULL;
}

void WaitEmuThread() {
	DWORD ExitCode;
	int count;

	for (count = 0; count < 400; count++) {
		SleepEx(50, TRUE);
		GetExitCodeThread(EmuThreadHandle, &ExitCode);
		if (ExitCode != STILL_ACTIVE) {
			EmuThreadHandle = NULL;
			break;
		}
	}
	if (EmuThreadHandle != NULL || ExitCode != 0) {

		if (EmuThreadHandle != NULL) {
			printf("Abnormal emu thread termination!\n");
			TerminateThread(EmuThreadHandle, 0);
			EmuThreadHandle = NULL;
		}

		MessageBox(NULL, "There was a problem with emulating.", "Warning", MB_OK);
	}
	emu_launched = 0;
	emu_paused = 1;
}


void resumeEmu(BOOL quiet) {
	BOOL wasPaused = emu_paused;
	if (emu_launched) {
		emu_paused = 0;
		ResumeThread(SoundThreadHandle);
		if (!quiet)
			SetStatusTranslatedString(hStatus, 0, "Emulation started");
		SendMessage(hTool, TB_CHECKBUTTON, EMU_PAUSE, 0);
		SendMessage(hTool, TB_CHECKBUTTON, EMU_PLAY, 1);
	} else {
		SendMessage(hTool, TB_CHECKBUTTON, EMU_PLAY, 0);
	}

	if (emu_paused != wasPaused && !quiet)
		CheckMenuItem(GetMenu(mainHWND), EMU_PAUSE, MF_BYCOMMAND | (emu_paused ? MFS_CHECKED : MFS_UNCHECKED));
}


void pauseEmu(BOOL quiet) {
	BOOL wasPaused = emu_paused;
	if (emu_launched) {
		VCR_updateFrameCounter();
		emu_paused = 1;
		if (!quiet) // HACK (not a typo) seems to help avoid a race condition that permanently disables sound when doing frame advance
			SuspendThread(SoundThreadHandle);
		if (!quiet)
			SetStatusTranslatedString(hStatus, 0, "Emulation paused");

		SendMessage(hTool, TB_CHECKBUTTON, EMU_PAUSE, 1);
		SendMessage(hTool, TB_CHECKBUTTON, EMU_PLAY, 0);
		SendMessage(hTool, TB_CHECKBUTTON, EMU_STOP, 0);
	} else {
		SendMessage(hTool, TB_CHECKBUTTON, EMU_PAUSE, 0);
		CheckMenuItem(GetMenu(mainHWND), EMU_PAUSE, MF_BYCOMMAND | MFS_UNCHECKED);
	}

	if (emu_paused != wasPaused && !MenuPaused)
		CheckMenuItem(GetMenu(mainHWND), EMU_PAUSE, MF_BYCOMMAND | (emu_paused ? MFS_CHECKED : MFS_UNCHECKED));
}

BOOL StartRom(const char* fullRomPath) {
	LONG winstyle;
	if (emu_launched) {
		really_restart_mode = TRUE;
		strcpy(LastSelectedRom, fullRomPath);
		CreateThread(NULL, 0, closeRom, NULL, 0, &Id);
	}
	else {
		winstyle = GetWindowLong(mainHWND, GWL_STYLE);
		winstyle = winstyle & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
		SetWindowLong(mainHWND, GWL_STYLE, winstyle);
		SetWindowPos(mainHWND, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);  //Set on top

		if (!restart_mode) {
			std::vector<plugin_type> missing_plugin_types = get_missing_plugin_types();

			if (missing_plugin_types.size() > 0) {

				if (MessageBox(mainHWND, "Can't start emulation prior to selecting plugins.\nDo you want to select plugins in the settings?", "Question", MB_YESNO | MB_ICONQUESTION) == IDYES) {
					ChangeSettings(mainHWND);
				}
				return 1;
			}

			SetStatusMode(1);

			SetStatusTranslatedString(hStatus, 0, "Loading ROM...");
			SendMessage(hStatus, SB_SETTEXT, 2, (LPARAM)fullRomPath);

			if (rom_read(fullRomPath)) {
				emu_launched = 0;
				SetStatusMode(0);
				SetStatusTranslatedString(hStatus, 0, "Failed to open rom");
				return TRUE;
			}

			sprintf(LastSelectedRom, "%s", fullRomPath);
			// TODO: reimplement
			// AddToRecentList(mainHWND, fullRomPath);

			InitTimer();

			EnableEmulationMenuItems(TRUE);

			rombrowser_set_visibility(0);
		}

		SetStatusMode(2);

		printf("Creating emulation thread...\n");
		EmuThreadHandle = CreateThread(NULL, 0, ThreadFunc, NULL, 0, &Id);

		extern int m_task;
		if (m_task == 0) {
			sprintf(TempMessage, MUPEN_VERSION " - %s", (char *) ROM_HEADER->nom);
			SetWindowText(mainHWND, TempMessage);
		}
		SendMessage(hTool, TB_CHECKBUTTON, EMU_PLAY, 1);
		SetStatusTranslatedString(hStatus, 0, "Emulation started");
		printf("Thread created\n");
		EnableEmulationMenuItems(TRUE);
		return FALSE;
	}
	return 0;
}

void exit_emu2();
static int shut_window = 0;

//void closeRom()
DWORD WINAPI closeRom(LPVOID lpParam) //lpParam - treated as bool, show romlist?
{
	LONG winstyle;                                //Used for setting new style to the window
 //   int browserwidths[] = {400, -1};              //Rombrowser statusbar

	if (emu_launched) {
		if (emu_paused) {
			MenuPaused = FALSE;
			resumeEmu(FALSE);
		}

		if (recording && !continue_vcr_on_restart_mode) {
		   //Sleep(1000); // HACK - seems to help crash on closing ROM during capture...?
			if (VCR_stopCapture() != 0) // but it never returns non-zero ???
				MessageBox(NULL, "Couldn't stop capturing", "VCR", MB_OK);
			else {
				SetWindowPos(mainHWND, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				SetStatusTranslatedString(hStatus, 0, "Stopped AVI capture");
				recording = FALSE;
			}
		}


		printf("Closing emulation thread...\n");
		stop_it();

		WaitEmuThread();

		if (!restart_mode) {
			free(rom);
			rom = NULL;
			free(ROM_HEADER);
			ROM_HEADER = NULL;
			free_memory();

			EnableEmulationMenuItems(FALSE);
			rombrowser_set_visibility(!really_restart_mode);

			extern int m_task;
			if (m_task == 0) {
				SetWindowText(mainHWND, MUPEN_VERSION);
				SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)" ");
			}
		}

		if (shut_window) {
			SendMessage(mainHWND, WM_CLOSE, 0, 0);
			return 0;
		}
	}

	SetStatusMode(0);
	SetStatusTranslatedString(hStatus, 0, "Rom Closed");

	//Makes window resizable
	winstyle = GetWindowLong(mainHWND, GWL_STYLE);
	winstyle |= WS_THICKFRAME;
	SetWindowLong(mainHWND, GWL_STYLE, winstyle);
	SetWindowPos(mainHWND, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);  //Set on top

	{
		if (really_restart_mode) {


			if (clear_sram_on_restart_mode) {
				VCR_clearAllSaveData();
				clear_sram_on_restart_mode = FALSE;
			}

			really_restart_mode = FALSE;
			extern int m_task;
			if (m_task != 0)
				just_restarted_flag = TRUE;
			if (StartRom(LastSelectedRom)) { // If rom loading fails
				closeRom(lpParam);
				// TODO: reimplement
				// ShowRomBrowser(TRUE, TRUE);
				SetStatusTranslatedString(hStatus, 0, "Failed to open rom");
			}
		}
	}

	continue_vcr_on_restart_mode = FALSE;

	return 0;
}

void resetEmu() {
	 // why is it so damned difficult to reset the game?
	 // right now it's hacked to exit to the GUI then re-load the ROM,
	 // but it should be possible to reset the game while it's still running
	 // simply by clearing out some memory and maybe notifying the plugins...
	if (emu_launched) {
		extern int frame_advancing;
		frame_advancing = false;
		restart_mode = 0;
		really_restart_mode = TRUE;
		MenuPaused = FALSE;
		CreateThread(NULL, 0, closeRom, NULL, 0, &Id);
	}

}

void ShowMessage(const char* lpszMessage) {
	MessageBox(NULL, lpszMessage, "Info", MB_OK);
}

void CreateToolBarWindow(HWND hwnd) {
	TBBUTTON tbButtons[] =
	{
		{0, IDLOAD, TBSTATE_ENABLED, TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
		{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0, 0}, 0, 0},
		{1, EMU_PLAY, TBSTATE_ENABLED, TBSTYLE_CHECK | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
		{2, EMU_PAUSE, TBSTATE_ENABLED, TBSTYLE_CHECK | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
		{3, EMU_STOP, TBSTATE_ENABLED, TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
		{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0, 0}, 0, 0},
		{4, FULL_SCREEN, TBSTATE_ENABLED, TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
		{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0, 0}, 0, 0},
		{5, IDGFXCONFIG, TBSTATE_ENABLED, TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
		{6, IDSOUNDCONFIG, TBSTATE_ENABLED, TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
		{7, IDINPUTCONFIG, TBSTATE_ENABLED, TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
		{8, IDRSPCONFIG, TBSTATE_ENABLED, TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
		{9, ID_LOAD_CONFIG, TBSTATE_ENABLED, TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
	};
	auto tbButtonsCount = sizeof(tbButtons) / sizeof(TBBUTTON);
	hTool = CreateToolbarEx(hwnd,
		WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS /* | */
		/*CS_ADJUSTABLE    no this causes a bug...*/,
		IDC_TOOLBAR, 10, app_hInstance, IDB_TOOLBAR,
		tbButtons, (int) tbButtonsCount, 16, 16, (int) tbButtonsCount * 16, 16, sizeof(TBBUTTON));

	if (hTool == NULL)
		MessageBox(hwnd, "Could not create tool bar.", "Error", MB_OK | MB_ICONERROR);


	if (emu_launched) {
		if (emu_paused) {
			SendMessage(hTool, TB_CHECKBUTTON, EMU_PAUSE, 1);
		} else {
			SendMessage(hTool, TB_CHECKBUTTON, EMU_PLAY, 1);
		}
	} else {
		// TODO: reimplement
		// getSelectedRom();
		SendMessage(hTool, TB_ENABLEBUTTON, EMU_STOP, FALSE);
		SendMessage(hTool, TB_ENABLEBUTTON, EMU_PAUSE, FALSE);
		SendMessage(hTool, TB_ENABLEBUTTON, FULL_SCREEN, FALSE);
	}

}

void CreateStatusBarWindow(HWND hwnd) {
   //Create Status bar
	hStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL,
		WS_CHILD | WS_VISIBLE /*| SBARS_SIZEGRIP*/, 0, 0, 0, 0,
		hwnd, (HMENU)IDC_MAIN_STATUS, GetModuleHandle(NULL), NULL);

	if (emu_launched) SetStatusMode(2);
	else SetStatusMode(0);
}

int pauseAtFrame = -1;

/// <summary>
/// Helper function because this is repeated multiple times
/// </summary>
void SetStatusPlaybackStarted() {
	HMENU hMenu = GetMenu(mainHWND);
	EnableMenuItem(hMenu, ID_STOP_RECORD, MF_GRAYED);
	EnableMenuItem(hMenu, ID_STOP_PLAYBACK, MF_ENABLED);

	if (!emu_paused || !emu_launched)
		SetStatusTranslatedString(hStatus, 0, "Playback started...");
	else
		SetStatusTranslatedString(hStatus, 0, "Playback started. (Paused)");
}

LRESULT CALLBACK PlayMovieProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	char tempbuf[MAX_PATH];
	char tempbuf2[MAX_PATH];

	HWND descriptionDialog;
	HWND authorDialog;

//    md5_byte_t digest[16];
//    ROM_INFO *pRomInfo;
//    char tempname[100];
//    HWND hwndPB;
//    pRomInfo = getSelectedRom();
//    DEFAULT_ROM_SETTINGS TempRomSettings;

	static char path_buffer[_MAX_PATH];

//    if (pRomInfo==NULL) {
//       EndDialog(hwnd, IDOK);
//       return FALSE;
//      }
	switch (Message) {
		case WM_INITDIALOG:
			descriptionDialog = GetDlgItem(hwnd, IDC_INI_DESCRIPTION);
			authorDialog = GetDlgItem(hwnd, IDC_INI_AUTHOR);

			SendMessage(descriptionDialog, EM_SETLIMITTEXT, MOVIE_DESCRIPTION_DATA_SIZE, 0);
			SendMessage(authorDialog, EM_SETLIMITTEXT, MOVIE_AUTHOR_DATA_SIZE, 0);

			country_code_to_country_name(ROM_HEADER->Country_code, tempbuf2);
			sprintf(tempbuf, "%s (%s)", (char *) ROM_HEADER->nom, tempbuf2);
			strcat(tempbuf, ".m64");
			SetDlgItemText(hwnd, IDC_INI_MOVIEFILE, tempbuf);

			SetDlgItemText(hwnd, IDC_ROM_INTERNAL_NAME2, (CHAR*)ROM_HEADER->nom);

			country_code_to_country_name(ROM_HEADER->Country_code, tempbuf);
			SetDlgItemText(hwnd, IDC_ROM_COUNTRY2, tempbuf);

			sprintf(tempbuf, "%X", (unsigned int)ROM_HEADER->CRC1);
			SetDlgItemText(hwnd, IDC_ROM_CRC3, tempbuf);

			SetDlgItemText(hwnd, IDC_MOVIE_VIDEO_TEXT2, Config.selected_video_plugin_name.c_str());
			SetDlgItemText(hwnd, IDC_MOVIE_INPUT_TEXT2, Config.selected_input_plugin_name.c_str());
			SetDlgItemText(hwnd, IDC_MOVIE_SOUND_TEXT2, Config.selected_audio_plugin_name.c_str());
			SetDlgItemText(hwnd, IDC_MOVIE_RSP_TEXT2, Config.selected_rsp_plugin_name.c_str());

			strcpy(tempbuf, Controls[0].Present ? "Present" : "Disconnected");
			if (Controls[0].Present && Controls[0].Plugin == controller_extension::mempak)
				strcat(tempbuf, " with mempak");
			if (Controls[0].Present && Controls[0].Plugin == controller_extension::rumblepak)
				strcat(tempbuf, " with rumble");
			SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER1_TEXT2, tempbuf);

			strcpy(tempbuf, Controls[1].Present ? "Present" : "Disconnected");
			if (Controls[1].Present && Controls[1].Plugin == controller_extension::mempak)
				strcat(tempbuf, " with mempak");
			if (Controls[1].Present && Controls[1].Plugin == controller_extension::rumblepak)
				strcat(tempbuf, " with rumble pak");
			SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER2_TEXT2, tempbuf);

			strcpy(tempbuf, Controls[2].Present ? "Present" : "Disconnected");
			if (Controls[2].Present && Controls[2].Plugin == controller_extension::mempak)
				strcat(tempbuf, " with mempak");
			if (Controls[2].Present && Controls[2].Plugin == controller_extension::rumblepak)
				strcat(tempbuf, " with rumble pak");
			SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER3_TEXT2, tempbuf);

			strcpy(tempbuf, Controls[3].Present ? "Present" : "Disconnected");
			if (Controls[3].Present && Controls[3].Plugin == controller_extension::mempak)
				strcat(tempbuf, " with mempak");
			if (Controls[3].Present && Controls[3].Plugin == controller_extension::rumblepak)
				strcat(tempbuf, " with rumble pak");
			SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER4_TEXT2, tempbuf);

			CheckDlgButton(hwnd, IDC_MOVIE_READONLY, VCR_getReadOnly());


			SetFocus(GetDlgItem(hwnd, IDC_INI_MOVIEFILE));


			goto refresh; // better than making it a macro or zillion-argument function

	case WM_CLOSE:
			EndDialog(hwnd, IDOK);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_OK:
				case IDOK:
				{
					VCR_coreStopped();
					{
						BOOL success;
						unsigned int num = GetDlgItemInt(hwnd, IDC_PAUSEAT_FIELD, &success, TRUE);
						if (((signed int)num) >= 0 && success)
							pauseAtFrame = (int) num;
						else
							pauseAtFrame = -1;
					}
					GetDlgItemText(hwnd, IDC_INI_MOVIEFILE, tempbuf, MAX_PATH);

					// turn WCHAR into UTF8
					WCHAR authorWC[MOVIE_AUTHOR_DATA_SIZE];
					char authorUTF8[MOVIE_AUTHOR_DATA_SIZE * 4];
					if (GetDlgItemTextW(hwnd, IDC_INI_AUTHOR, authorWC, MOVIE_AUTHOR_DATA_SIZE))
						WideCharToMultiByte(CP_UTF8, 0, authorWC, -1, authorUTF8, sizeof(authorUTF8), NULL, NULL);
					else
						GetDlgItemTextA(hwnd, IDC_INI_AUTHOR, authorUTF8, MOVIE_AUTHOR_DATA_SIZE);

					WCHAR descriptionWC[MOVIE_DESCRIPTION_DATA_SIZE];
					char descriptionUTF8[MOVIE_DESCRIPTION_DATA_SIZE * 4];
					if (GetDlgItemTextW(hwnd, IDC_INI_DESCRIPTION, descriptionWC, MOVIE_DESCRIPTION_DATA_SIZE))
						WideCharToMultiByte(CP_UTF8, 0, descriptionWC, -1, descriptionUTF8, sizeof(descriptionUTF8), NULL, NULL);
					else
						GetDlgItemTextA(hwnd, IDC_INI_DESCRIPTION, descriptionUTF8, MOVIE_DESCRIPTION_DATA_SIZE);

					VCR_setReadOnly((BOOL) IsDlgButtonChecked(hwnd, IDC_MOVIE_READONLY));

					auto playbackResult = VCR_startPlayback(tempbuf, authorUTF8, descriptionUTF8);

					if (strlen(tempbuf) == 0 || playbackResult != VCR_PLAYBACK_SUCCESS) {
						char errorString[MAX_PATH];

						sprintf(errorString, "Failed to start movie \"%s\" ", tempbuf);

						switch (playbackResult) {
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
					} else {
						SetStatusPlaybackStarted();
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
				    // The default directory we open the file dialog window in is
				    // the parent directory of the last movie that the user ran
					GetDefaultFileDialogPath(path_buffer, Config.recent_movie_paths[0].c_str());

					if (fdBrowseMovie.ShowFileDialog(path_buffer, L"*.m64;*.rec", TRUE, FALSE, hwnd))
						SetDlgItemText(hwnd, IDC_INI_MOVIEFILE, path_buffer);
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
	SMovieHeader m_header = VCR_getHeaderInfo(tempbuf);

	SetDlgItemText(hwnd, IDC_ROM_INTERNAL_NAME, m_header.romNom);

	country_code_to_country_name(m_header.romCountry, tempbuf);
	SetDlgItemText(hwnd, IDC_ROM_COUNTRY, tempbuf);

	sprintf(tempbuf, "%X", (unsigned int)m_header.romCRC);
	SetDlgItemText(hwnd, IDC_ROM_CRC, tempbuf);

	SetDlgItemText(hwnd, IDC_MOVIE_VIDEO_TEXT, m_header.videoPluginName);
	SetDlgItemText(hwnd, IDC_MOVIE_INPUT_TEXT, m_header.inputPluginName);
	SetDlgItemText(hwnd, IDC_MOVIE_SOUND_TEXT, m_header.soundPluginName);
	SetDlgItemText(hwnd, IDC_MOVIE_RSP_TEXT, m_header.rspPluginName);

	strcpy(tempbuf, (m_header.controllerFlags & CONTROLLER_1_PRESENT) ? "Present" : "Disconnected");
	if (m_header.controllerFlags & CONTROLLER_1_MEMPAK)
		strcat(tempbuf, " with mempak");
	if (m_header.controllerFlags & CONTROLLER_1_RUMBLE)
		strcat(tempbuf, " with rumble");
	SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER1_TEXT, tempbuf);

	strcpy(tempbuf, (m_header.controllerFlags & CONTROLLER_2_PRESENT) ? "Present" : "Disconnected");
	if (m_header.controllerFlags & CONTROLLER_2_MEMPAK)
		strcat(tempbuf, " with mempak");
	if (m_header.controllerFlags & CONTROLLER_2_RUMBLE)
		strcat(tempbuf, " with rumble");
	SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER2_TEXT, tempbuf);

	strcpy(tempbuf, (m_header.controllerFlags & CONTROLLER_3_PRESENT) ? "Present" : "Disconnected");
	if (m_header.controllerFlags & CONTROLLER_3_MEMPAK)
		strcat(tempbuf, " with mempak");
	if (m_header.controllerFlags & CONTROLLER_3_RUMBLE)
		strcat(tempbuf, " with rumble");
	SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER3_TEXT, tempbuf);

	strcpy(tempbuf, (m_header.controllerFlags & CONTROLLER_4_PRESENT) ? "Present" : "Disconnected");
	if (m_header.controllerFlags & CONTROLLER_4_MEMPAK)
		strcat(tempbuf, " with mempak");
	if (m_header.controllerFlags & CONTROLLER_4_RUMBLE)
		strcat(tempbuf, " with rumble");
	SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER4_TEXT, tempbuf);

	SetDlgItemText(hwnd, IDC_FROMSNAPSHOT_TEXT, (m_header.startFlags & MOVIE_START_FROM_SNAPSHOT) ? "Savestate" : "Start");
	if (m_header.startFlags & MOVIE_START_FROM_EEPROM) {
		SetDlgItemTextA(hwnd, IDC_FROMSNAPSHOT_TEXT, "EEPROM");
	}

	sprintf(tempbuf, "%u  (%u input)", (int)m_header.length_vis, (int)m_header.length_samples);
	SetDlgItemText(hwnd, IDC_MOVIE_FRAMES, tempbuf);

	if (m_header.vis_per_second == 0)
		m_header.vis_per_second = 60;

	double seconds = (double)m_header.length_vis / (double)m_header.vis_per_second;
	double minutes = seconds / 60.0;
	if ((bool) seconds)
		seconds = fmod(seconds, 60.0);
	double hours = minutes / 60.0;
	if ((bool) minutes)
		minutes = fmod(minutes, 60.0);

	if (hours >= 1.0)
		sprintf(tempbuf, "%d hours and %.1f minutes", (unsigned int)hours, (float)minutes);
	else if (minutes >= 1.0)
		sprintf(tempbuf, "%d minutes and %.0f seconds", (unsigned int)minutes, (float)seconds);
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
		if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, m_header.authorInfo, -1, wszMeta, MOVIE_AUTHOR_DATA_SIZE)) {
			SetLastError(0);
			SetWindowTextW(GetDlgItem(hwnd, IDC_INI_AUTHOR), wszMeta);
			if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {
				// not implemented on this system - convert as best we can to 1-byte characters and set with that
				// TODO: load unicows.dll instead so SetWindowTextW won't fail even on Win98/ME
				char ansiStr[MOVIE_AUTHOR_DATA_SIZE];
				WideCharToMultiByte(CP_ACP, 0, wszMeta, -1, ansiStr, MOVIE_AUTHOR_DATA_SIZE, NULL, NULL);
				SetWindowTextA(GetDlgItem(hwnd, IDC_INI_AUTHOR), ansiStr);

				if (strlen(ansiStr) == 0)
					SetWindowTextA(GetDlgItem(hwnd, IDC_INI_AUTHOR), "(too lazy to type name)");

				SetLastError(0);
			} else {
				if (wcslen(wszMeta) == 0)
					SetWindowTextW(GetDlgItem(hwnd, IDC_INI_AUTHOR), L"(too lazy to type name)");
			}
		}
		if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, m_header.description, -1, wszMeta, MOVIE_DESCRIPTION_DATA_SIZE)) {
			SetWindowTextW(GetDlgItem(hwnd, IDC_INI_DESCRIPTION), wszMeta);
			if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {
				char ansiStr[MOVIE_DESCRIPTION_DATA_SIZE];
				WideCharToMultiByte(CP_ACP, 0, wszMeta, -1, ansiStr, MOVIE_DESCRIPTION_DATA_SIZE, NULL, NULL);
				SetWindowTextA(GetDlgItem(hwnd, IDC_INI_DESCRIPTION), ansiStr);

				if (strlen(ansiStr) == 0)
					SetWindowTextA(GetDlgItem(hwnd, IDC_INI_DESCRIPTION), "(no description entered)");

				SetLastError(0);
			} else {
				if (wcslen(wszMeta) == 0)
					SetWindowTextW(GetDlgItem(hwnd, IDC_INI_DESCRIPTION), L"(no description entered)");
			}
		}
	}

	return FALSE;
}

LRESULT CALLBACK RecordMovieProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	char tempbuf[MAX_PATH];
	char tempbuf2[MAX_PATH];
	int checked_movie_type;
	HWND descriptionDialog;
	HWND authorDialog;

	switch (Message) {
		case WM_INITDIALOG:

			checked_movie_type = Config.last_movie_type;
			descriptionDialog = GetDlgItem(hwnd, IDC_INI_DESCRIPTION);
			authorDialog = GetDlgItem(hwnd, IDC_INI_AUTHOR);

			SendMessage(descriptionDialog, EM_SETLIMITTEXT, MOVIE_DESCRIPTION_DATA_SIZE, 0);
			SendMessage(authorDialog, EM_SETLIMITTEXT, MOVIE_AUTHOR_DATA_SIZE, 0);

			SetDlgItemText(hwnd, IDC_INI_AUTHOR, Config.last_movie_author.c_str());
			SetDlgItemText(hwnd, IDC_INI_DESCRIPTION, "");

			CheckRadioButton(hwnd, IDC_FROMSNAPSHOT_RADIO, IDC_FROMSTART_RADIO, checked_movie_type);

			country_code_to_country_name(ROM_HEADER->Country_code, tempbuf2);
			sprintf(tempbuf, "%s (%s)", (char *) ROM_HEADER->nom, tempbuf2);
			strcat(tempbuf, ".m64");
			SetDlgItemText(hwnd, IDC_INI_MOVIEFILE, tempbuf);

			SetDlgItemText(hwnd, IDC_ROM_INTERNAL_NAME2, (CHAR*) ROM_HEADER->nom);

			country_code_to_country_name(ROM_HEADER->Country_code, tempbuf);
			SetDlgItemText(hwnd, IDC_ROM_COUNTRY2, tempbuf);

			sprintf(tempbuf, "%X", (unsigned int)ROM_HEADER->CRC1);
			SetDlgItemText(hwnd, IDC_ROM_CRC3, tempbuf);

			SetDlgItemText(hwnd, IDC_MOVIE_VIDEO_TEXT2, Config.selected_video_plugin_name.c_str());
			SetDlgItemText(hwnd, IDC_MOVIE_INPUT_TEXT2, Config.selected_input_plugin_name.c_str());
			SetDlgItemText(hwnd, IDC_MOVIE_SOUND_TEXT2, Config.selected_audio_plugin_name.c_str());
			SetDlgItemText(hwnd, IDC_MOVIE_RSP_TEXT2, Config.selected_rsp_plugin_name.c_str());

			strcpy(tempbuf, Controls[0].Present ? "Present" : "Disconnected");
			if (Controls[0].Present && Controls[0].Plugin == controller_extension::mempak)
				strcat(tempbuf, " with mempak");
			if (Controls[0].Present && Controls[0].Plugin == controller_extension::rumblepak)
				strcat(tempbuf, " with rumble");
			SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER1_TEXT2, tempbuf);

			strcpy(tempbuf, Controls[1].Present ? "Present" : "Disconnected");
			if (Controls[1].Present && Controls[1].Plugin == controller_extension::mempak)
				strcat(tempbuf, " with mempak");
			if (Controls[1].Present && Controls[1].Plugin == controller_extension::rumblepak)
				strcat(tempbuf, " with rumble pak");
			SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER2_TEXT2, tempbuf);

			strcpy(tempbuf, Controls[2].Present ? "Present" : "Disconnected");
			if (Controls[2].Present && Controls[2].Plugin == controller_extension::mempak)
				strcat(tempbuf, " with mempak");
			if (Controls[2].Present && Controls[2].Plugin == controller_extension::rumblepak)
				strcat(tempbuf, " with rumble pak");
			SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER3_TEXT2, tempbuf);

			strcpy(tempbuf, Controls[3].Present ? "Present" : "Disconnected");
			if (Controls[3].Present && Controls[3].Plugin == controller_extension::mempak)
				strcat(tempbuf, " with mempak");
			if (Controls[3].Present && Controls[3].Plugin == controller_extension::rumblepak)
				strcat(tempbuf, " with rumble pak");
			SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER4_TEXT2, tempbuf);

			EnableWindow(GetDlgItem(hwnd, IDC_EXTSAVESTATE), 0); // workaround because initial selected button is "Start"

			SetFocus(GetDlgItem(hwnd, IDC_INI_AUTHOR));

			return FALSE;
		case WM_CLOSE:
			EndDialog(hwnd, IDOK);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_OK:
				case IDOK:
				{

					// turn WCHAR into UTF8
					WCHAR authorWC[MOVIE_AUTHOR_DATA_SIZE];
					char authorUTF8[MOVIE_AUTHOR_DATA_SIZE * 4];
					if (GetDlgItemTextW(hwnd, IDC_INI_AUTHOR, authorWC, MOVIE_AUTHOR_DATA_SIZE))
						WideCharToMultiByte(CP_UTF8, 0, authorWC, -1, authorUTF8, sizeof(authorUTF8), NULL, NULL);
					else
						GetDlgItemTextA(hwnd, IDC_INI_AUTHOR, authorUTF8, MOVIE_AUTHOR_DATA_SIZE);

					Config.last_movie_author = std::string(authorUTF8);

					WCHAR descriptionWC[MOVIE_DESCRIPTION_DATA_SIZE];
					char descriptionUTF8[MOVIE_DESCRIPTION_DATA_SIZE * 4];
					if (GetDlgItemTextW(hwnd, IDC_INI_DESCRIPTION, descriptionWC, MOVIE_DESCRIPTION_DATA_SIZE))
						WideCharToMultiByte(CP_UTF8, 0, descriptionWC, -1, descriptionUTF8, sizeof(descriptionUTF8), NULL, NULL);
					else
						GetDlgItemTextA(hwnd, IDC_INI_DESCRIPTION, descriptionUTF8, MOVIE_DESCRIPTION_DATA_SIZE);


					GetDlgItemText(hwnd, IDC_INI_MOVIEFILE, tempbuf, MAX_PATH);

					// big
					checked_movie_type = IsDlgButtonChecked(hwnd, IDC_FROMSNAPSHOT_RADIO) ? IDC_FROMSNAPSHOT_RADIO : IsDlgButtonChecked(hwnd, IDC_FROMSTART_RADIO)
						? IDC_FROMSTART_RADIO : IsDlgButtonChecked(hwnd, IDC_FROMEEPROM_RADIO) ? IDC_FROMEEPROM_RADIO : IDC_FROMEXISTINGSNAPSHOT_RADIO;
					unsigned short flag = checked_movie_type == IDC_FROMSNAPSHOT_RADIO ? MOVIE_START_FROM_SNAPSHOT : checked_movie_type == IDC_FROMSTART_RADIO
						? MOVIE_START_FROM_NOTHING : checked_movie_type == IDC_FROMEEPROM_RADIO ? MOVIE_START_FROM_EEPROM : MOVIE_START_FROM_EXISTING_SNAPSHOT;
					Config.last_movie_type = checked_movie_type;

					bool allowClosing = true;
					if (flag == MOVIE_START_FROM_EXISTING_SNAPSHOT) {
						// The default directory we open the file dialog window in is the
						// parent directory of the last savestate that the user saved or loaded
						std::filesystem::path path = Config.states_path;
						GetDefaultFileDialogPath(tempbuf, Config.states_path.c_str());

						if (fdBrowseMovie2.ShowFileDialog(tempbuf, L"*.st;*.savestate", TRUE, FALSE, hwnd)) {
							savestates_select_filename(tempbuf);
							path = tempbuf;
							path.replace_extension("m64");
							strncpy(tempbuf, path.string().c_str(), MAX_PATH);

							if (std::filesystem::exists(path)) {
								sprintf(tempbuf2, "\"%s\" already exists. Are you sure want to overwrite this movie?", tempbuf);
								if (MessageBox(hwnd, tempbuf2, "VCR", MB_YESNO) == IDNO)
									break;
							}
						} else
							allowClosing = false;
					}

					if (allowClosing) {
						if (strlen(tempbuf) == 0 || VCR_startRecord(tempbuf, flag, authorUTF8, descriptionUTF8, !IsDlgButtonChecked(hwnd, IDC_EXTSAVESTATE)) < 0) {
							sprintf(tempbuf2, "Couldn't start recording\nof \"%s\".", tempbuf);
							MessageBox(hwnd, tempbuf2, "VCR", MB_OK);
							break;
						} else {
							HMENU hMenu = GetMenu(mainHWND);
							EnableMenuItem(hMenu, ID_STOP_RECORD, MF_ENABLED);
							EnableMenuItem(hMenu, ID_STOP_PLAYBACK, MF_GRAYED);
							SetStatusTranslatedString(hStatus, 0, "Recording replay...");
						}

						EndDialog(hwnd, IDOK);
					}
				} break;
				case IDC_CANCEL:
				case IDCANCEL:
					EndDialog(hwnd, IDOK);
					break;
				case IDC_MOVIE_BROWSE:
				{
					// The default directory we open the file dialog window in is
					// the parent directory of the last movie that the user ran
					GetDefaultFileDialogPath(tempbuf, Config.recent_movie_paths[0].c_str()); // if the first movie ends up being deleted this is fucked

					if (fdBrowseMovie2.ShowFileDialog(tempbuf, L"*.m64;*.rec", TRUE, FALSE, hwnd)) {
						if (strlen(tempbuf) > 0 && (strlen(tempbuf) < 4 || _stricmp(tempbuf + strlen(tempbuf) - 4, ".m64") != 0))
							strcat(tempbuf, ".m64");
						SetDlgItemText(hwnd, IDC_INI_MOVIEFILE, tempbuf);
					}
				}
				break;

				case IDC_FROMEEPROM_RADIO:
					EnableWindow(GetDlgItem(hwnd, IDC_EXTSAVESTATE), 0);
					EnableWindow(GetDlgItem(hwnd, IDC_MOVIE_BROWSE), 1);
					EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE), 1);
					EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE_TEXT), 1);
					break;
				case IDC_FROMSNAPSHOT_RADIO:
					EnableWindow(GetDlgItem(hwnd, IDC_EXTSAVESTATE), 1);
					EnableWindow(GetDlgItem(hwnd, IDC_MOVIE_BROWSE), 1);
					EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE), 1);
					EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE_TEXT), 1);
					break;
				case IDC_FROMEXISTINGSNAPSHOT_RADIO:
					EnableWindow(GetDlgItem(hwnd, IDC_EXTSAVESTATE), 1);
					EnableWindow(GetDlgItem(hwnd, IDC_MOVIE_BROWSE), 0);
					EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE), 0);
					EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE_TEXT), 0);
					break;
				case IDC_FROMSTART_RADIO:
					EnableWindow(GetDlgItem(hwnd, IDC_EXTSAVESTATE), 0);
					EnableWindow(GetDlgItem(hwnd, IDC_MOVIE_BROWSE), 1);
					EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE), 1);
					EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE_TEXT), 1);
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

void OpenMoviePlaybackDialog() {
	BOOL wasPaused = emu_paused && !MenuPaused;
	MenuPaused = FALSE;
	if (emu_launched && !emu_paused)
		pauseEmu(FALSE);

	DialogBox(GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDD_MOVIE_PLAYBACK_DIALOG), mainHWND, (DLGPROC)PlayMovieProc);

	if (emu_launched && emu_paused && !wasPaused)
		resumeEmu(FALSE);
}
void OpenMovieRecordDialog() {
	BOOL wasPaused = emu_paused && !MenuPaused;
	MenuPaused = FALSE;
	if (emu_launched && !emu_paused)
		pauseEmu(FALSE);

	DialogBox(GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDD_MOVIE_RECORD_DIALOG), mainHWND, (DLGPROC)RecordMovieProc);

	if (emu_launched && emu_paused && !wasPaused)
		resumeEmu(FALSE);
}


void SetStatusMode(int mode) {
	RECT rcClient;                                    //Client area of parent window
	const int loadingwidths[] = {200, 300, -1};       //Initial statusbar
	const int emulatewidthsFPSVIS[] = {230, 300, 370, 440, -1};//Emulating statusbar with FPS and VIS
	const int emulatewidthsFPS[] = {230, 300, 370, -1};    //Emulating statusbar with FPS
	const int emulatewidths[] = {230, 300, -1};            //Emulating statusbar
	const int browserwidths[] = {400, -1};	          //Initial statusbar
	int parts;


	if (hStaticHandle)   DestroyWindow(hStaticHandle);

	//Setting status widths
	if (Config.is_statusbar_enabled) {
		switch (mode) {
			case 0:                 //Rombrowser Statusbar
				SendMessage(hStatus, SB_SETPARTS, sizeof(browserwidths) / sizeof(int), (LPARAM)browserwidths);
				SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)statusmsg);
				SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)"");
				break;

			case 1:                 //Loading Statusbar
				SendMessage(hStatus, SB_SETPARTS, sizeof(loadingwidths) / sizeof(int), (LPARAM)loadingwidths);
				SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)statusmsg);
				SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)"");
				SendMessage(hStatus, SB_SETTEXT, 2, (LPARAM)"");

				GetClientRect(hStatus, &rcClient);
				break;

			case 2:                    //Emulating Statusbar
				if (Config.show_fps && Config.show_vis_per_second) {
					SendMessage(hStatus, SB_SETPARTS, sizeof(emulatewidthsFPSVIS) / sizeof(int), (LPARAM)emulatewidthsFPSVIS);
					SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)""); //rr
					SendMessage(hStatus, SB_SETTEXT, 2, (LPARAM)""); //FPS
					SendMessage(hStatus, SB_SETTEXT, 3, (LPARAM)""); //VIS
					parts = 5;
				} else if (Config.show_fps) {
					SendMessage(hStatus, SB_SETPARTS, sizeof(emulatewidthsFPS) / sizeof(int), (LPARAM)emulatewidthsFPS);
					SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)"");
					SendMessage(hStatus, SB_SETTEXT, 2, (LPARAM)"");
					parts = 4;
				} else if (Config.show_vis_per_second) {
					SendMessage(hStatus, SB_SETPARTS, sizeof(emulatewidthsFPS) / sizeof(int), (LPARAM)emulatewidthsFPS);
					SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)"");
					SendMessage(hStatus, SB_SETTEXT, 2, (LPARAM)"");
					parts = 4;
				} else {
					SendMessage(hStatus, SB_SETPARTS, sizeof(emulatewidths) / sizeof(int), (LPARAM)emulatewidths);
					SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)"");
					parts = 3;
				}

				SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)statusmsg);
				sprintf(TempMessage, "%s", ROM_SETTINGS.goodname);
				SendMessage(hStatus, SB_SETTEXT, parts - 1, (LPARAM)TempMessage);
				break;
default:
	break;
		}        //Switch
	}        //If
}

void EnableEmulationMenuItems(BOOL emulationRunning) {
	extern BOOL continue_vcr_on_restart_mode;

	HMENU hMenu = GetMenu(mainHWND);

#ifdef _DEBUG
	EnableMenuItem(hMenu, ID_CRASHHANDLERDIALOGSHOW, MF_ENABLED);
#endif

	if (emulationRunning) {
		EnableMenuItem(hMenu, EMU_STOP, MF_ENABLED);
		EnableMenuItem(hMenu, EMU_PAUSE, MF_ENABLED);
		EnableMenuItem(hMenu, EMU_FRAMEADVANCE, MF_ENABLED);
		EnableMenuItem(hMenu, EMU_PLAY, MF_ENABLED);
		EnableMenuItem(hMenu, FULL_SCREEN, MF_ENABLED);
		EnableMenuItem(hMenu, STATE_SAVE, MF_ENABLED);
		EnableMenuItem(hMenu, STATE_SAVEAS, MF_ENABLED);
		EnableMenuItem(hMenu, STATE_RESTORE, MF_ENABLED);
		EnableMenuItem(hMenu, STATE_LOAD, MF_ENABLED);
		EnableMenuItem(hMenu, GENERATE_BITMAP, MF_ENABLED);
		EnableMenuItem(hMenu, EMU_RESET, MF_ENABLED);
		EnableMenuItem(hMenu, REFRESH_ROM_BROWSER, MF_GRAYED);
		EnableMenuItem(hMenu, ID_RESTART_MOVIE, MF_ENABLED);
		EnableMenuItem(hMenu, ID_AUDIT_ROMS, MF_GRAYED);
		EnableMenuItem(hMenu, ID_FFMPEG_START, MF_DISABLED);

	#ifdef GAME_DEBUGGER
		EnableMenuItem(hMenu, ID_GAMEDEBUGGER, MF_ENABLED);
	#endif
		if (dynacore)
			EnableMenuItem(hMenu, ID_TRACELOG, MF_DISABLED);
		else
			EnableMenuItem(hMenu, ID_TRACELOG, MF_ENABLED);


		if (!continue_vcr_on_restart_mode) {
			EnableMenuItem(hMenu, ID_START_RECORD, MF_ENABLED);
			EnableMenuItem(hMenu, ID_STOP_RECORD, VCR_isRecording() ? MF_ENABLED : MF_GRAYED);
			EnableMenuItem(hMenu, ID_START_PLAYBACK, MF_ENABLED);
			EnableMenuItem(hMenu, ID_STOP_PLAYBACK, (VCR_isRestarting() || VCR_isPlaying()) ? MF_ENABLED : MF_GRAYED);
			EnableMenuItem(hMenu, ID_START_CAPTURE, MF_ENABLED);
			EnableMenuItem(hMenu, ID_START_CAPTURE_PRESET, MF_ENABLED);
			EnableMenuItem(hMenu, ID_END_CAPTURE, VCR_isCapturing() ? MF_ENABLED : MF_GRAYED);
		}

		if (Config.is_toolbar_enabled) {
			SendMessage(hTool, TB_ENABLEBUTTON, EMU_PLAY, TRUE);
			SendMessage(hTool, TB_ENABLEBUTTON, EMU_STOP, TRUE);
			SendMessage(hTool, TB_ENABLEBUTTON, EMU_PAUSE, TRUE);
			if (recording && !externalReadScreen)
				SendMessage(hTool, TB_ENABLEBUTTON, FULL_SCREEN, FALSE);
			else
				SendMessage(hTool, TB_ENABLEBUTTON, FULL_SCREEN, TRUE);
		}
	} else {
		EnableMenuItem(hMenu, EMU_STOP, MF_GRAYED);
		EnableMenuItem(hMenu, IDLOAD, MF_ENABLED);
		EnableMenuItem(hMenu, EMU_PAUSE, MF_GRAYED);
		EnableMenuItem(hMenu, EMU_FRAMEADVANCE, MF_GRAYED);
		EnableMenuItem(hMenu, EMU_PLAY, MF_GRAYED);
		EnableMenuItem(hMenu, FULL_SCREEN, MF_GRAYED);
		EnableMenuItem(hMenu, STATE_SAVE, MF_GRAYED);
		EnableMenuItem(hMenu, STATE_SAVEAS, MF_GRAYED);
		EnableMenuItem(hMenu, STATE_RESTORE, MF_GRAYED);
		EnableMenuItem(hMenu, STATE_LOAD, MF_GRAYED);
		EnableMenuItem(hMenu, GENERATE_BITMAP, MF_GRAYED);
		// TODO: reimplement
		// EnableRecentROMsMenu(hMenu, TRUE);
		EnableMenuItem(hMenu, EMU_RESET, MF_GRAYED);
		EnableMenuItem(hMenu, REFRESH_ROM_BROWSER, MF_ENABLED);
		EnableMenuItem(hMenu, ID_RESTART_MOVIE, MF_GRAYED);
		EnableMenuItem(hMenu, ID_GAMEDEBUGGER, MF_GRAYED);
		EnableMenuItem(hMenu, ID_TRACELOG, MF_DISABLED);
		EnableMenuItem(hMenu, ID_AUDIT_ROMS, MF_ENABLED);
		EnableMenuItem(hMenu, ID_FFMPEG_START, MF_GRAYED);

		if (!continue_vcr_on_restart_mode) {
			EnableMenuItem(hMenu, ID_START_RECORD, MF_GRAYED);
			EnableMenuItem(hMenu, ID_STOP_RECORD, MF_GRAYED);
			EnableMenuItem(hMenu, ID_START_PLAYBACK, MF_GRAYED);
			EnableMenuItem(hMenu, ID_STOP_PLAYBACK, MF_GRAYED);
			EnableMenuItem(hMenu, ID_START_CAPTURE, MF_GRAYED);
			EnableMenuItem(hMenu, ID_START_CAPTURE_PRESET, MF_GRAYED);
			EnableMenuItem(hMenu, ID_END_CAPTURE, MF_GRAYED);
			LONG winstyle;
			winstyle = GetWindowLong(mainHWND, GWL_STYLE);
			winstyle |= WS_MAXIMIZEBOX;
			SetWindowLong(mainHWND, GWL_STYLE, winstyle);
			SetWindowPos(mainHWND, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);  //Set on top
		}
		if (Config.is_toolbar_enabled) {
			// TODO: reimplement
			// getSelectedRom();
			//SendMessage( hTool, TB_ENABLEBUTTON, EMU_PLAY, FALSE );
			SendMessage(hTool, TB_ENABLEBUTTON, EMU_STOP, FALSE);
			SendMessage(hTool, TB_ENABLEBUTTON, EMU_PAUSE, FALSE);
			SendMessage(hTool, TB_ENABLEBUTTON, FULL_SCREEN, FALSE);
		}
	}

	if (Config.is_toolbar_enabled) CheckMenuItem(hMenu, IDC_GUI_TOOLBAR, MF_BYCOMMAND | MF_CHECKED);
	else CheckMenuItem(hMenu, IDC_GUI_TOOLBAR, MF_BYCOMMAND | MF_UNCHECKED);
	if (Config.is_statusbar_enabled) CheckMenuItem(hMenu, IDC_GUI_STATUSBAR, MF_BYCOMMAND | MF_CHECKED);
	else CheckMenuItem(hMenu, IDC_GUI_STATUSBAR, MF_BYCOMMAND | MF_UNCHECKED);
	if (Config.is_movie_loop_enabled) CheckMenuItem(hMenu, ID_LOOP_MOVIE, MF_BYCOMMAND | MF_CHECKED);
	else CheckMenuItem(hMenu, ID_LOOP_MOVIE, MF_BYCOMMAND | MF_UNCHECKED);
	if (Config.is_recent_movie_paths_frozen) CheckMenuItem(hMenu, ID_RECENTMOVIES_FREEZE, MF_BYCOMMAND | MF_CHECKED);
	if (Config.is_recent_scripts_frozen) CheckMenuItem(hMenu, ID_LUA_RECENT_FREEZE, MF_BYCOMMAND | MF_CHECKED);
}

static DWORD WINAPI SoundThread(LPVOID lpParam) {
	while (emu_launched) {
		aiUpdate(1);
	}
	ExitThread(0);
}

static DWORD WINAPI StartMoviesThread(LPVOID lpParam) {
	Sleep(2000);
	StartMovies();
	ExitThread(0);
}

static DWORD WINAPI ThreadFunc(LPVOID lpParam) {
	init_memory();
	load_plugins();
	romOpen_gfx();
	romOpen_input();
	romOpen_audio();

	dynacore = Config.core_type;

	emu_paused = 0;
	emu_launched = 1;
	restart_mode = 0;

	SoundThreadHandle = CreateThread(NULL, 0, SoundThread, NULL, 0, &SOUNDTHREADID);
	printf("Emu thread: Emulation started....\n");
	CreateThread(NULL, 0, StartMoviesThread, NULL, 0, NULL);
	StartLuaScripts();
	StartSavestate();
	AtResetCallback();
	if (pauseAtFrame == 0 && VCR_isStartingAndJustRestarted()) {
		while (emu_paused) {
			Sleep(10);
		}
		pauseEmu(FALSE);
		pauseAtFrame = -1;
	}
	go();

	romClosed_gfx();
	romClosed_audio();
	romClosed_input();
	romClosed_RSP();

	closeDLL_gfx();
	closeDLL_audio();
	closeDLL_input();
	closeDLL_RSP();

	destroy_plugins();
	search_plugins();

	ExitThread(0);
}

void exit_emu(int postquit) {

	save_config();

	if (postquit) {
		if (!cmdlineMode || cmdlineSave) {
			ini_updateFile();
			// TODO: reimplement
			// if (!cmdlineNoGui)
			// 	SaveRomBrowserCache();
		}
		ini_closeFile();
	} else {
		CreateThread(NULL, 0, closeRom, NULL, 0, &Id);
	}

	if (postquit) {
		// TODO: reimplement
		// freeRomDirList();
		// freeRomList();
		freeLanguages();
		Gdiplus::GdiplusShutdown(gdiPlusToken);
		PostQuitMessage(0);
	}
}

void exit_emu2() {
	save_config();

	if ((!cmdlineMode) || (cmdlineSave)) {
		ini_updateFile();
		// TODO: reimplement
		// if (!cmdlineNoGui) {
		// 	SaveRomBrowserCache();
		// }
	}
	ini_closeFile();
	// TODO: reimplement
	// freeRomDirList();
	// freeRomList();
	freeLanguages();
	PostQuitMessage(0);
}

BOOL IsMenuItemEnabled(HMENU hMenu, UINT uId) {
	return !(GetMenuState(hMenu, uId, MF_BYCOMMAND) & (MF_DISABLED | MF_GRAYED));
}

void ProcessToolTips(LPARAM lParam, HWND hWnd) {

	LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT)lParam;

	lpttt->hinst = app_hInstance;

	// Specify the resource identifier of the descriptive
	// text for the given button.
	HMENU hMenu = GetMenu(hWnd);

	switch (lpttt->hdr.idFrom) {
		case IDLOAD:
			TranslateDefault("Load ROM...", "Load ROM...", TempMessage);
			lpttt->lpszText = TempMessage;
			break;
		case EMU_PLAY:
			if (!emu_launched) {
				TranslateDefault("Start Emulation", "Start Emulation", TempMessage);
			} else if (emu_paused) {
				TranslateDefault("Resume Emulation", "Resume Emulation", TempMessage);
			} else {
				TranslateDefault("Emulating", "Emulating", TempMessage);
			}
			lpttt->lpszText = TempMessage;
			break;
		case EMU_PAUSE:
			TranslateDefault("Pause Emulation", "Pause Emulation", TempMessage);
			lpttt->lpszText = TempMessage;
			break;
		case EMU_STOP:
			TranslateDefault("Stop Emulation", "Stop Emulation", TempMessage);
			lpttt->lpszText = TempMessage;
			break;
		case FULL_SCREEN:
			TranslateDefault("Full Screen", "Full Screen", TempMessage);
			lpttt->lpszText = TempMessage;
			break;
		case IDGFXCONFIG:
			TranslateDefault("Video Settings...", "Video Settings...", TempMessage);
			lpttt->lpszText = TempMessage;
			break;
		case IDSOUNDCONFIG:
			TranslateDefault("Audio Settings...", "Audio Settings...", TempMessage);
			lpttt->lpszText = TempMessage;
			break;
		case IDINPUTCONFIG:
			TranslateDefault("Input Settings...", "Input Settings...", TempMessage);
			lpttt->lpszText = TempMessage;
			break;
		case IDRSPCONFIG:
			TranslateDefault("RSP Settings...", "RSP Settings...", TempMessage);
			lpttt->lpszText = TempMessage;
			break;
		case ID_LOAD_CONFIG:
			TranslateDefault("Settings...", "Settings...", TempMessage);
			lpttt->lpszText = TempMessage;
			break;
		default:
			break;
	}
}

void EnableStatusbar() {
	if (Config.is_statusbar_enabled) {
		if (!IsWindow(hStatus)) {
			CreateStatusBarWindow(mainHWND);
		}
	} else {
		DestroyWindow(hStatus);
		hStatus = NULL;
	}

	rombrowser_update_size();
}

void EnableToolbar() {
	if (Config.is_toolbar_enabled && !VCR_isCapturing()) {
		if (!hTool || !IsWindow(hTool)) {
			CreateToolBarWindow(mainHWND);
		}
	} else {
		if (hTool && IsWindow(hTool)) {
			DestroyWindow(hTool);
			hTool = NULL;
		}
	}

	rombrowser_update_size();
}


LRESULT CALLBACK NoGuiWndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	switch (Message) {
		case WM_KEYDOWN:
			switch (wParam) {
				case VK_TAB:
					manualFPSLimit = 0;
					break;
				default:
					break;
			}
			if (emu_launched) keyDown(wParam, lParam);
			break;
		case WM_KEYUP:
			switch (wParam) {
				case VK_TAB:
					manualFPSLimit = 1;
					break;
				case VK_ESCAPE:
					exit_emu(1);
					break;
				default:
					break;
			}
			if (emu_launched) keyUp(wParam, lParam);
			break;
		case WM_MOVE:
			if (emu_launched && !FullScreenMode) {
				moveScreen((int) wParam, lParam);
			}
		rombrowser_update_size();
			break;
		case WM_USER + 17:  SetFocus(mainHWND);
			break;
		case WM_CLOSE:
			exit_emu(1);
			break;

		default:
			return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return TRUE;
}


DWORD WINAPI UnpauseEmuAfterMenu(LPVOID lpParam) {
	Sleep(60); // Wait for another thread to clear MenuPaused

	if (emu_paused && !AutoPause && MenuPaused) {
		resumeEmu(FALSE);
	}
	MenuPaused = FALSE;
	return 0;
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	char path_buffer[_MAX_PATH];
	int ret;
	int i;
	static PAINTSTRUCT	ps;
	BOOL	minimize;
	HMENU hMenu = GetMenu(hwnd);

#ifdef LUA_WINDOWMESSAGE
	LuaWindowMessage(hwnd, Message, wParam, lParam);
#endif
	switch (Message) {

		case WM_DROPFILES:
		{
			HDROP h_file = (HDROP) wParam;
			char fname[MAX_PATH];
			LPSTR fext;
			DragQueryFile(h_file, 0, fname, sizeof(fname));
			LocalFree(h_file);
			fext = CharUpper(PathFindExtension(fname));
			if (lstrcmp(fext, ".N64") == 0 || lstrcmp(fext, ".V64") == 0 || lstrcmp(fext, ".Z64") == 0 || lstrcmp(fext, ".ROM") == 0) {
				StartRom(fname);
			} else if (lstrcmp(fext, ".M64") == 0) {
				if (rom) {
					if (!VCR_getReadOnly())	VCR_toggleReadOnly();

					if (VCR_startPlayback(fname, 0, 0) < 0) {
						printf("[VCR]: Drag drop Failed to start playback of %s", fname);
						break;
					} else
						SetStatusPlaybackStarted();
				}
			} else if (strcmp(fext, ".ST") == 0 || strcmp(fext, ".SAVESTATE") == 0) {
				if (rom) {
					savestates_select_filename(fname);
					savestates_job = LOADSTATE;
				}
			} else if (strcmp(fext, ".LUA") == 0) {
				if (rom) {
					for (; *fext; ++fext) *fext = (CHAR)tolower(*fext); // Deep in the code, lua will access file with that path (uppercase extension because stupid, useless programming at line 2677 converts it), see it doesnt exist and fail.
					// even this hack will fail under special circumstances
					LuaOpenAndRun(fname);
				}
			}

			break;
		}
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			BOOL hit = FALSE;
			if (manualFPSLimit) {
				if ((int)wParam == Config.fast_forward_hotkey.key) // fast-forward on
				{
					if (((GetKeyState(VK_SHIFT) & 0x8000) ? 1 : 0) == Config.fast_forward_hotkey.shift
						&& ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0) == Config.fast_forward_hotkey.ctrl
						&& ((GetKeyState(VK_MENU) & 0x8000) ? 1 : 0) == Config.fast_forward_hotkey.alt) {
						manualFPSLimit = 0;
						hit = TRUE;
					}
				}
			}
			for (i = 1; i < hotkeys.size(); i++) {
				if ((int)wParam == hotkeys[i]->key) {
					if (((GetKeyState(VK_SHIFT) & 0x8000) ? 1 : 0) == hotkeys[i]->shift
						&& ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0) == hotkeys[i]->ctrl
						&& ((GetKeyState(VK_MENU) & 0x8000) ? 1 : 0) == hotkeys[i]->alt) {
						SendMessage(hwnd, WM_COMMAND, hotkeys[i]->command, 0);
						hit = TRUE;
					}
				}
			}
			if (emu_launched)
				keyDown(wParam, lParam);
			if (!hit)
				return DefWindowProc(hwnd, Message, wParam, lParam);
		}	break;
		case WM_SYSKEYUP:
		case WM_KEYUP:
			if ((int)wParam == Config.fast_forward_hotkey.key) // fast-forward off
			{
				manualFPSLimit = 1;
				ffup = true; //fuck it, timers.c is too weird
			}
			if (emu_launched)
				keyUp(wParam, lParam);
			return DefWindowProc(hwnd, Message, wParam, lParam);
		case WM_NOTIFY:
		{
			LPNMHDR l_header = (LPNMHDR)lParam;

			if (wParam == IDC_ROMLIST) {
				rombrowser_notify(lParam);
			}
			switch ((l_header)->code) {
				case TTN_NEEDTEXT:
					ProcessToolTips(lParam, hwnd);
					break;
			}
			return 0;
		}
		case WM_MOVE:
		{
			if (emu_launched && !FullScreenMode) {
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
			if (!FullScreenMode) {
				SendMessage(hTool, TB_AUTOSIZE, 0, 0);
				SendMessage(hStatus, WM_SIZE, 0, 0);
				// TODO: reimplement
				// ResizeRomListControl();
			}
				rombrowser_update_size();
			break;
		}
		case WM_USER + 17:  SetFocus(mainHWND); break;
		case WM_CREATE:
			GetModuleFileName(NULL, path_buffer, sizeof(path_buffer));
			CreateToolBarWindow(hwnd);
			CreateStatusBarWindow(hwnd);
			SetupLanguages(hwnd);
			TranslateMenu(GetMenu(hwnd), hwnd);
			SetMenuAcceleratorsFromUser(hwnd);
			EnableToolbar();
			EnableStatusbar();
			return TRUE;
		case WM_CLOSE:
		{
			if (warn_recording())break;
			if (emu_launched) {
				shut_window = 1;
				exit_emu(0);
				return 0;
			} else {
				exit_emu(1);
				//DestroyWindow(hwnd);
			}
			break;
		}
		case WM_PAINT: //todo, work with updatescreen to use wmpaint
		{

			BeginPaint(hwnd, &ps);
			EndPaint(hwnd, &ps);

			return 0;
		}
	//		case WM_SETCURSOR:
	//			SetCursor(FALSE);
	//			return 0;
		case WM_WINDOWPOSCHANGING:  //allow gfx plugin to set arbitrary size
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
			if (!emu_paused) {
				MenuPaused = TRUE;
				pauseEmu(FALSE);
			}
			break;

		case WM_EXITMENULOOP:
			CreateThread(NULL, 0, UnpauseEmuAfterMenu, NULL, 0, NULL);
			break;
		case WM_ACTIVATE:
			UpdateWindow(hwnd);
			minimize = (BOOL)HIWORD(wParam);

			switch (LOWORD(wParam)) {
				case WA_ACTIVE:
				case WA_CLICKACTIVE:
					if (Config.is_unfocused_pause_enabled && emu_paused && !AutoPause) {
						resumeEmu(FALSE);
						AutoPause = emu_paused;
					}
					break;

				case WA_INACTIVE:
					AutoPause = emu_paused && !MenuPaused;
					if (Config.is_unfocused_pause_enabled && !emu_paused /*(&& minimize*/ && !FullScreenMode) {
						MenuPaused = FALSE;
						pauseEmu(FALSE);
					} else if (Config.is_unfocused_pause_enabled && MenuPaused) {
						MenuPaused = FALSE;
					}
					break;
				default:
					break;
			}
			break;
		case WM_COMMAND:
		{
			switch (LOWORD(wParam)) {
				case ID_MENU_LUASCRIPT_NEW:
				{
					::NewLuaScript((void(*)())lParam);
				} break;
				case ID_LUA_RECENT_FREEZE:
				{
					CheckMenuItem(hMenu, ID_LUA_RECENT_FREEZE, (Config.is_recent_scripts_frozen ^= 1) ? MF_CHECKED : MF_UNCHECKED);
					break;
				}
				case ID_LUA_RECENT_RESET:
				{
					lua_recent_scripts_reset();
					break;
				}
				case ID_LUA_LOAD_LATEST:
				{
					lua_recent_scripts_run(ID_LUA_RECENT);
					break;
				}
				case ID_MENU_LUASCRIPT_CLOSEALL:
				{
					::CloseAllLuaScript();
				} break;
				case ID_FORCESAVE:
					ini_updateFile();
					save_config();
					ini_closeFile();
					break;
				case ID_TRACELOG:
				#ifdef LUA_TRACELOG
				#ifdef LUA_TRACEINTERP
								// keep if check just in case user manages to screw with mupen config or something
					if (!dynacore) {
						::LuaTraceLogState();
					}
				#else
					if (!dynacore && interpcore == 1) {
						::LuaTraceLogState();
					}
				#endif
				#endif
					break;
				case IDGFXCONFIG:
				{
					BOOL wasPaused = emu_paused && !MenuPaused;
					MenuPaused = FALSE;
					if (emu_launched && !emu_paused)
						pauseEmu(FALSE);

					hwnd_plug = hwnd;
					plugin_config(get_plugin_by_name(Config.selected_video_plugin_name));
					if (emu_launched && emu_paused && !wasPaused)
						resumeEmu(FALSE);
				}	break;
				case IDINPUTCONFIG:
				{
					BOOL wasPaused = emu_paused && !MenuPaused;
					MenuPaused = FALSE;
					if (emu_launched && !emu_paused)
						pauseEmu(FALSE);

					hwnd_plug = hwnd;
					plugin_config(get_plugin_by_name(Config.selected_input_plugin_name));
					if (emu_launched && emu_paused && !wasPaused)
						resumeEmu(FALSE);
				}	break;
				case IDSOUNDCONFIG:
				{
					BOOL wasPaused = emu_paused && !MenuPaused;
					MenuPaused = FALSE;
					if (emu_launched && !emu_paused)
						pauseEmu(FALSE);

					hwnd_plug = hwnd;
					plugin_config(get_plugin_by_name(Config.selected_audio_plugin_name));
					if (emu_launched && emu_paused && !wasPaused)
						resumeEmu(FALSE);
				}	break;
				case IDRSPCONFIG:
				{
					BOOL wasPaused = emu_paused && !MenuPaused;
					MenuPaused = FALSE;
					if (emu_launched && !emu_paused)
						pauseEmu(FALSE);

					hwnd_plug = hwnd;
					plugin_config(get_plugin_by_name(Config.selected_rsp_plugin_name));
					if (emu_launched && emu_paused && !wasPaused)
						resumeEmu(FALSE);
				}	break;
				case EMU_STOP:
					MenuPaused = FALSE;
					if (warn_recording())break;
					if (emu_launched) {
						stop_it();
						//SleepEx(1000, TRUE); //todo check if can remove
						CreateThread(NULL, 0, closeRom, (LPVOID)1, 0, &Id);
						SendMessage(hTool, TB_CHECKBUTTON, EMU_PAUSE, 0);
						SendMessage(hTool, TB_CHECKBUTTON, EMU_PLAY, 0);
					}
					break;

				case EMU_PAUSE:
				{
					if (!emu_paused) {
						pauseEmu(VCR_isActive());
					} else if (MenuPaused) {
						MenuPaused = FALSE;
						CheckMenuItem(GetMenu(mainHWND), EMU_PAUSE, MF_BYCOMMAND | MFS_CHECKED);
						SendMessage(hTool, TB_CHECKBUTTON, EMU_PAUSE, TRUE);
					} else {
						resumeEmu(VCR_isActive());
					}
					break;
				}

				case EMU_FRAMEADVANCE:
				{
					MenuPaused = FALSE;
					if (!manualFPSLimit) break;
					extern int frame_advancing;
					frame_advancing = 1;
					VIs = 0; // prevent old VI value from showing error if running at super fast speeds
					resumeEmu(TRUE); // maybe multithreading unsafe
				}
				break;

				case EMU_VCRTOGGLEREADONLY:
					VCR_toggleReadOnly();
					break;

				case ID_LOOP_MOVIE:
					VCR_toggleLoopMovie();
					break;
				case ID_RESTART_MOVIE:
					if (VCR_isPlaying()) {
						VCR_setReadOnly(TRUE);
						bool err = VCR_startPlayback(Config.recent_movie_paths[0].c_str(), 0, 0);
						if (err == VCR_PLAYBACK_SUCCESS)
							SetStatusPlaybackStarted();
						else
							SetStatusTranslatedString(hStatus, 0, "Couldn't start latest movie");
					}
					break;
				case ID_REPLAY_LATEST:
					// Don't try to load a recent movie if not emulating!
					if (rom) {
						// Overwrite prevention? Path sanity check (Leave to internal handling)?
						VCR_setReadOnly(TRUE);
						bool err = VCR_startPlayback(Config.recent_movie_paths[0].c_str(), 0, 0);
						if (err == VCR_PLAYBACK_SUCCESS)
							SetStatusPlaybackStarted();
						else
							SetStatusTranslatedString(hStatus, 0, "Couldn't start latest movie");
					} else
						SetStatusTranslatedString(hStatus, 0, "Cannot load a movie while not emulating!");
					break;
				case ID_RECENTMOVIES_FREEZE:
					CheckMenuItem(hMenu, ID_RECENTMOVIES_FREEZE, (Config.is_recent_movie_paths_frozen ^= 1) ? MF_CHECKED : MF_UNCHECKED);
					break;
				case ID_RECENTMOVIES_RESET:
					vcr_recent_movies_reset();
					break;
				case EMU_PLAY:
					if (emu_launched) {
						if (emu_paused) {
							resumeEmu(FALSE);
						} else {
							SendMessage(hTool, TB_CHECKBUTTON, EMU_PLAY, 1);
							//The button is always checked when started and not on pause
						}
					} else {
						// TODO: reimplement
						// RomList_OpenRom();
					}
					break;

				case EMU_RESET:
					if (Config.is_reset_recording_disabled && warn_recording())break;
					extern int m_task;
					if (m_task == 3 && !Config.is_reset_recording_disabled) //recording
					{
						scheduled_restart = true;
						continue_vcr_on_restart_mode = true;
						display_status("Writing restart to m64...");
						break;
					}
					//resttart_mode = 1; //has issues
					resetEmu();
					break;

				case ID_LOAD_CONFIG:
				{
					BOOL wasPaused = emu_paused && !MenuPaused;
					MenuPaused = FALSE;
					if (emu_launched && !emu_paused) {
						pauseEmu(FALSE);
					}
					ChangeSettings(hwnd);
					ini_updateFile();
					if (emu_launched && emu_paused && !wasPaused) {
						resumeEmu(FALSE);
					}
				}
				break;
				case ID_AUDIT_ROMS:
					// TODO: reimplement
					// audit_roms();
					break;
				case ID_HELP_ABOUT:
				{
					BOOL wasMenuPaused = MenuPaused;
					MenuPaused = FALSE;
					ret = DialogBox(GetModuleHandle(NULL),
						MAKEINTRESOURCE(IDD_ABOUT), hwnd, AboutDlgProc);
					if (wasMenuPaused) {
						resumeEmu(TRUE);
					}
				}
				break;
				case ID_CRASHHANDLERDIALOGSHOW:
				{
					CrashHandlerDialog crashHandlerDialog(CrashHandlerDialog::Types::Ignorable, "This is a mock crash.");
					crashHandlerDialog.Show();
					break;
				}
				case ID_GAMEDEBUGGER:
					extern unsigned long op;

					GameDebuggerStart([]() {
						return Config.core_type == 2 ? op : -1;
						}, []() {
							return Config.core_type == 2 ? interp_addr : -1;
						});
					break;
				case ID_RAMSTART:
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
					sprintf(stroop_c, "<Emulator name=\"Mupen 5.0 RR\" processName=\"%s\" ramStart=\"%s\" endianness=\"little\"/>", proc_name, ram_start);

					auto stroop_str = std::string(stroop_c);
					if (MessageBoxA(mainHWND, "Do you want to copy the generated STROOP config line to your clipboard?", "STROOP", MB_ICONINFORMATION | MB_TASKMODAL | MB_YESNO) == IDYES) {
						copy_to_clipboard(mainHWND, stroop_str);
					}
					if (wasMenuPaused) {
						resumeEmu(TRUE);
					}
					break;
				}
				case IDLOAD:
				{
					BOOL wasMenuPaused = MenuPaused;
					MenuPaused = FALSE;
					// The default directory we open the file dialog window in is
					// the parent directory of the last rom that the user ran
					GetDefaultFileDialogPath(path_buffer, Config.recent_rom_paths[0].c_str());

					if (fdLoadRom.ShowFileDialog(path_buffer, L"*.n64;*.z64;*.v64;*.rom;*.bin;*.zip;*.usa;*.eur;*.jap", TRUE, FALSE, hwnd)) {
						char temp_buffer[_MAX_PATH];
						strcpy(temp_buffer, path_buffer);
						unsigned int i; for (i = 0; i < strlen(temp_buffer); i++)
							if (temp_buffer[i] == '/')
								temp_buffer[i] = '\\';
						char* slash = strrchr(temp_buffer, '\\');
						if (slash) {
							slash[1] = '\0';
						}
						StartRom(path_buffer);
					}
					if (wasMenuPaused) {
						resumeEmu(TRUE);
					}
				}
				break;
				case ID_EMULATOR_EXIT:
					if (warn_recording())break;
					shut_window = 1;
					if (emu_launched)
						exit_emu(0);
					else
						exit_emu(1);
					break;
				case FULL_SCREEN:
					if (emu_launched && (!recording || externalReadScreen)) {
						FullScreenMode = 1 - FullScreenMode;
						gui_ChangeWindow();
					}
					break;
				case REFRESH_ROM_BROWSER:
					if (!emu_launched) {
						rombrowser_build();
					}
					break;
				case ID_POPUP_ROM_SETTING:
					// TODO: reimplement
					// OpenRomProperties();
					break;
				case ID_START_ROM:
					// TODO: reimplement
					// RomList_OpenRom();
					break;
				case ID_START_ROM_ENTER:
					// TODO: reimplement
					// if (!emu_launched) RomList_OpenRom();
					break;
				case STATE_SAVE:
					if (!emu_paused || MenuPaused) {
						savestates_job = SAVESTATE;
					} else if (emu_launched) {
						savestates_save();
					}
					break;
				case STATE_SAVEAS:
				{
					BOOL wasMenuPaused = MenuPaused;
					MenuPaused = FALSE;

					GetDefaultFileDialogPath(path_buffer, Config.states_path.c_str());

					if (fdSaveStateAs.ShowFileDialog(path_buffer, L"*.st;*.savestate", FALSE, FALSE, hwnd)) {

						// HACK: allow .savestate and .st
						// by creating another buffer, copying original into it and stripping its' extension
						// and putting it back sanitized
						// if no extension inputted by user, fallback to .st
						strcpy(correctedPath, path_buffer);
						stripExt(correctedPath);
						if (!_stricmp(getExt(path_buffer), "savestate"))
							strcat(correctedPath, ".savestate");
						else
							strcat(correctedPath, ".st");

						savestates_select_filename(correctedPath);
						savestates_job = SAVESTATE;
					}
					if (wasMenuPaused) {
						resumeEmu(TRUE);
					}
				}
				break;
				case STATE_RESTORE:
					if (emu_launched) {
						savestates_job = LOADSTATE;
						// dont call savestatesload from ui thread right after setting flag for emu thread
						//savestates_load();
					}
					break;
				case STATE_LOAD:
				{
					BOOL wasMenuPaused = MenuPaused;
					MenuPaused = FALSE;
					GetDefaultFileDialogPath(path_buffer, Config.states_path.c_str());

					if (fdSaveLoadAs.ShowFileDialog(path_buffer, L"*.st;*.savestate;*.st0;*.st1;*.st2;*.st3;*.st4;*.st5;*.st6;*.st7;*.st8;*.st9,*.st10", TRUE, FALSE, hwnd)) {
						savestates_select_filename(path_buffer);
						savestates_job = LOADSTATE;
					}
					if (wasMenuPaused) {
						resumeEmu(TRUE);
					}
				}

				break;
				case ID_START_RECORD:
					if (emu_launched)
						OpenMovieRecordDialog();

					break;

				case ID_STOP_RECORD:
					if (VCR_isRecording()) {
						if (VCR_stopRecord(1) < 0) // seems ok (no)
							; // fail quietly
	//                        MessageBox(NULL, "Couldn't stop recording.", "VCR", MB_OK);
						else {
							ClearButtons();
							EnableMenuItem(hMenu, ID_STOP_RECORD, MF_GRAYED);
							EnableMenuItem(hMenu, ID_START_RECORD, MF_ENABLED);
							SetStatusTranslatedString(hStatus, 0, "Recording stopped");
						}
					}
					break;
				case ID_START_PLAYBACK:
					if (emu_launched)
						OpenMoviePlaybackDialog();

					break;
				case ID_STOP_PLAYBACK:
					if (VCR_isPlaying()) {
						if (VCR_stopPlayback() < 0)
							; // fail quietly
	//                        MessageBox(NULL, "Couldn't stop playback.", "VCR", MB_OK);
						else {
							ClearButtons();
							EnableMenuItem(hMenu, ID_STOP_PLAYBACK, MF_GRAYED);
							EnableMenuItem(hMenu, ID_START_PLAYBACK, MF_ENABLED);
							SetStatusTranslatedString(hStatus, 0, "Playback stopped");
						}
					}
					break;

				case ID_FFMPEG_START:
				{
					auto err = VCR_StartFFmpegCapture("ffmpeg_out.mp4", "-pixel_format yuv420p -loglevel debug -y");
					if (err == INIT_SUCCESS) {
						//SetWindowPos(mainHWND, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);  //Set on top avichg
						EnableMenuItem(hMenu, ID_START_CAPTURE, MF_GRAYED);
						EnableMenuItem(hMenu, ID_START_CAPTURE_PRESET, MF_GRAYED);
						EnableMenuItem(hMenu, ID_FFMPEG_START, MF_GRAYED);
						EnableMenuItem(hMenu, ID_END_CAPTURE, MF_ENABLED);
						if (!externalReadScreen) {
							EnableMenuItem(hMenu, FULL_SCREEN, MF_GRAYED);           //Disables fullscreen menu
							SendMessage(hTool, TB_ENABLEBUTTON, FULL_SCREEN, FALSE); //Disables fullscreen button
						}
						SetStatusTranslatedString(hStatus, 0, "Recording avi...");
						EnableEmulationMenuItems(TRUE);
					} else
						printf("Start capture error: %d\n", err);
					break;
				}

				case ID_START_CAPTURE_PRESET:
				case ID_START_CAPTURE:
					if (emu_launched) {
						BOOL wasPaused = emu_paused && !MenuPaused;
						MenuPaused = FALSE;
						if (emu_launched && !emu_paused)
							pauseEmu(FALSE);

						char rec_buffer[MAX_PATH];

						// The default directory we open the file dialog window in is
						// the parent directory of the last avi that the user captured
						GetDefaultFileDialogPath(path_buffer, Config.avi_capture_path.c_str());

						if (fdStartCapture.ShowFileDialog(path_buffer, L"*.avi", FALSE, FALSE, hwnd)) {



							int len = (int) strlen(path_buffer);
							if (len < 5 ||
								(path_buffer[len - 1] != 'i' && path_buffer[len - 1] != 'I') ||
								(path_buffer[len - 2] != 'v' && path_buffer[len - 2] != 'V') ||
								(path_buffer[len - 3] != 'a' && path_buffer[len - 3] != 'A') ||
								path_buffer[len - 4] != '.')
								strcat(path_buffer, ".avi");
							//Sleep(1000);
							// pass false to startCapture when "last preset" option was choosen
							if (VCR_startCapture(rec_buffer, path_buffer, LOWORD(wParam) == ID_START_CAPTURE) < 0) {
								MessageBox(NULL, "Couldn't start capturing.", "VCR", MB_OK);
								recording = FALSE;
							} else {
								//SetWindowPos(mainHWND, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);  //Set on top avichg
								EnableMenuItem(hMenu, ID_START_CAPTURE, MF_GRAYED);
								EnableMenuItem(hMenu, ID_START_CAPTURE_PRESET, MF_GRAYED);
								EnableMenuItem(hMenu, ID_FFMPEG_START, MF_GRAYED);
								EnableMenuItem(hMenu, ID_END_CAPTURE, MF_ENABLED);
								if (!externalReadScreen) {
									EnableMenuItem(hMenu, FULL_SCREEN, MF_GRAYED);           //Disables fullscreen menu
									SendMessage(hTool, TB_ENABLEBUTTON, FULL_SCREEN, FALSE); //Disables fullscreen button
								}
								SetStatusTranslatedString(hStatus, 0, "Recording avi...");
								EnableEmulationMenuItems(TRUE);
								recording = TRUE;
							}
						}

						if (emu_launched && emu_paused && !wasPaused)
							resumeEmu(FALSE);
					}

					break;
				case ID_END_CAPTURE:
					if (VCR_stopCapture() < 0)
						MessageBox(NULL, "Couldn't stop capturing.", "VCR", MB_OK);
					else {
						SetWindowPos(mainHWND, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
						EnableMenuItem(hMenu, ID_END_CAPTURE, MF_GRAYED);
						EnableMenuItem(hMenu, ID_START_CAPTURE, MF_ENABLED);
						EnableMenuItem(hMenu, ID_FFMPEG_START, MF_ENABLED);
						EnableMenuItem(hMenu, ID_START_CAPTURE_PRESET, MF_ENABLED);
						SetStatusTranslatedString(hStatus, 0, "Stopped video capture");
						recording = FALSE;
					}
					break;
				case ID_GENERATE_ROM_INFO:
					// TODO: reimplement
					// generateRomInfo();
					break;
				break;
				case GENERATE_BITMAP: // take/capture a screenshot
					if (Config.is_default_screenshots_directory_used) {
						sprintf(path_buffer, "%sScreenShots\\", AppPath);
						CaptureScreen(path_buffer);
					} else {
						sprintf(path_buffer, "%s", Config.screenshots_directory.c_str());
						CaptureScreen(path_buffer);
					}
					break;
				case ID_RECENTROMS_RESET:
					// TODO: reimplement
					// ClearRecentList(hwnd, TRUE);
					// SetRecentList(hwnd);
					break;
				case ID_RECENTROMS_FREEZE:
					// TODO: reimplement
					// FreezeRecentRoms(hwnd, TRUE);
					break;
				case ID_LOAD_LATEST:
					// TODO: reimplement
					// RunRecentRom(ID_RECENTROMS_FIRST);
				case IDC_GUI_TOOLBAR:
					Config.is_toolbar_enabled = 1 - Config.is_toolbar_enabled;
					EnableToolbar();
					if (Config.is_toolbar_enabled) CheckMenuItem(hMenu, IDC_GUI_TOOLBAR, MF_BYCOMMAND | MF_CHECKED);
					else CheckMenuItem(hMenu, IDC_GUI_TOOLBAR, MF_BYCOMMAND | MF_UNCHECKED);
					break;
				case IDC_GUI_STATUSBAR:
					Config.is_statusbar_enabled ^= true;
					EnableStatusbar();
					if (Config.is_statusbar_enabled) CheckMenuItem(hMenu, IDC_GUI_STATUSBAR, MF_BYCOMMAND | MF_CHECKED);
					else CheckMenuItem(hMenu, IDC_GUI_STATUSBAR, MF_BYCOMMAND | MF_UNCHECKED);
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
					InitTimer();
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
					InitTimer();
					break;
				case IDC_RESET_MODIFIER:
					Config.fps_modifier = 100;
					InitTimer();
					break;
				default:
					 //Language Support  from ID_LANG_ENGLISH to ID_LANG_ENGLISH+100
					if (LOWORD(wParam) >= ID_LANG_ENGLISH && LOWORD(wParam) <= (ID_LANG_ENGLISH + 100)) {
						SelectLang(hwnd, LOWORD(wParam));
						TranslateMenu(GetMenu(hwnd), hwnd);
						// TODO: reimplement
						// TranslateBrowserHeader(hRomList);
						// ShowTotalRoms();
					} else if (LOWORD(wParam) >= ID_CURRENTSAVE_1 && LOWORD(wParam) <= ID_CURRENTSAVE_10) {
						SelectState(hwnd, LOWORD(wParam));
					} else if (LOWORD(wParam) >= ID_SAVE_1 && LOWORD(wParam) <= ID_SAVE_10) {
						SaveTheState(hwnd, LOWORD(wParam));
					} else if (LOWORD(wParam) >= ID_LOAD_1 && LOWORD(wParam) <= ID_LOAD_10) {
						LoadTheState(hwnd, LOWORD(wParam));
					} else if (LOWORD(wParam) >= ID_RECENTROMS_FIRST && LOWORD(wParam) < (ID_RECENTROMS_FIRST + MAX_RECENT_ROMS)) {
						// TODO: reimplement
						// RunRecentRom(LOWORD(wParam));
					} else if (LOWORD(wParam) >= ID_RECENTMOVIES_FIRST && LOWORD(wParam) < (ID_RECENTMOVIES_FIRST + MAX_RECENT_MOVIE)) {
						if (vcr_recent_movies_play(LOWORD(wParam)) != SUCCESS) {
							SetStatusTranslatedString(hStatus, 0, "Could not load movie!");
							break;
						}
						// should probably make this code from the ID_REPLAY_LATEST case into a function on its own
						// because now it's used here too
						EnableMenuItem(hMenu, ID_STOP_RECORD, MF_GRAYED);
						EnableMenuItem(hMenu, ID_STOP_PLAYBACK, MF_ENABLED);

						if (!emu_paused || !emu_launched)
							SetStatusTranslatedString(hStatus, 0, "Playback started...");
						else
							SetStatusTranslatedString(hStatus, 0, "Playback started. (Paused)");
					} else if (LOWORD(wParam) >= ID_LUA_RECENT && LOWORD(wParam) < (ID_LUA_RECENT + LUA_MAX_RECENT)) {
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

//starts m64 and avi
//this is called from game thread because otherwise gfx plugin tries to resize window,
//but main thread waits for game thread to finish loading, and hangs.
void StartMovies() {
	//-m64, -g
	HMENU hMenu = GetMenu(mainHWND);
	printf("------thread done------\n");
	if (CmdLineParameterExist(CMDLINE_PLAY_M64) && CmdLineParameterExist(CMDLINE_GAME_FILENAME)) {
		char file[MAX_PATH];
		GetCmdLineParameter(CMDLINE_PLAY_M64, file);
		//not reading author nor description atm
		VCR_setReadOnly(TRUE);
		VCR_startPlayback(file, "", "");
		if (CmdLineParameterExist(CMDLINE_CAPTURE_AVI)) {
			GetCmdLineParameter(CMDLINE_CAPTURE_AVI, file);
			if (VCR_startCapture(0, file, false) < 0) {
				MessageBox(NULL, "Couldn't start capturing.", "VCR", MB_OK);
				recording = FALSE;
			} else {
				gStopAVI = true;
				SetWindowPos(mainHWND, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);  //Set on top
				EnableMenuItem(hMenu, ID_START_CAPTURE, MF_GRAYED);
				EnableMenuItem(hMenu, ID_START_CAPTURE_PRESET, MF_GRAYED);
				EnableMenuItem(hMenu, ID_END_CAPTURE, MF_ENABLED);
				if (!externalReadScreen) {
					EnableMenuItem(hMenu, FULL_SCREEN, MF_GRAYED);           //Disables fullscreen menu
					SendMessage(hTool, TB_ENABLEBUTTON, FULL_SCREEN, FALSE); //Disables fullscreen button
				}
				SetStatusTranslatedString(hStatus, 0, "Recording avi...");
				recording = TRUE;
			}
		}
		resumeEmu(FALSE);
	}
}

//-lua, -g
// runs multiple lua scripts with paths seperated by ;
// Ex: "path\script1.lua;path\script2.lua"
// From testing only works with 2 scripts ?
void StartLuaScripts() {
	HMENU hMenu = GetMenu(mainHWND);
	if (CmdLineParameterExist(CMDLINE_LUA) && CmdLineParameterExist(CMDLINE_GAME_FILENAME)) {
		char files[MAX_PATH];
		GetCmdLineParameter(CMDLINE_LUA, files);
		int len = (int) strlen(files);
		int numScripts = 1;
		int scriptStartPositions[MAX_LUA_OPEN_AND_RUN_INSTANCES] = {0};
		for (int i = 0; i < len; ++i) {
			if (files[i] == ';') {
				files[i] = 0; // turn ; into \0 so we can copy each part easily
				scriptStartPositions[numScripts] = i + 1;
				++numScripts;
				if (numScripts >= MAX_LUA_OPEN_AND_RUN_INSTANCES) {
					break;
				}
			}
		}
		char file[MAX_PATH];
		for (int i = 0; i < numScripts; ++i) {
			strcpy(file, &files[scriptStartPositions[i]]);
			LuaOpenAndRun(file);
		}
	}
}

//-st, -g
void StartSavestate() {
	HMENU hMenu = GetMenu(mainHWND);
	if (CmdLineParameterExist(CMDLINE_SAVESTATE) && CmdLineParameterExist(CMDLINE_GAME_FILENAME)
		&& !CmdLineParameterExist(CMDLINE_PLAY_M64)) {
		char file[MAX_PATH];
		GetCmdLineParameter(CMDLINE_SAVESTATE, file);
		savestates_select_filename(file);
		savestates_job = LOADSTATE;
	}
}

// Loads various variables from the current config state
void LoadConfigExternals() {
	if (VCR_isLooping() != (bool) Config.is_movie_loop_enabled) VCR_toggleLoopMovie();
}

// kaboom
LONG WINAPI ExceptionReleaseTarget(_EXCEPTION_POINTERS* ExceptionInfo) {
	// generate crash log

	char crashLog[1024 * 4] = {0};
	CrashHelper::GenerateLog(ExceptionInfo, crashLog);

	FILE* f = fopen("crash.log", "w+");
	fwrite(crashLog, sizeof(crashLog), 1, f);
	fclose(f);

	bool isIgnorable = !(ExceptionInfo->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE);

	printf("exception occured! creating crash dialog...\n");
	CrashHandlerDialog crashHandlerDialog(isIgnorable ? CrashHandlerDialog::Types::Ignorable : (CrashHandlerDialog::Types)0, "An exception has been thrown and a crash log has been automatically generated.\r\nPlease choose a way to proceed.");

	auto result = crashHandlerDialog.Show();

	switch (result) {
		case CrashHandlerDialog::Choices::Ignore:
			return EXCEPTION_CONTINUE_EXECUTION;
		case CrashHandlerDialog::Choices::Exit:
			return EXCEPTION_EXECUTE_HANDLER;
	}

	return EXCEPTION_EXECUTE_HANDLER;
}


int WINAPI WinMain(
	HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	//timeBeginPeriod(1);
#ifdef _DEBUG
	AllocConsole();
	FILE* f = 0;
	freopen_s(&f, "CONIN$", "r", stdin);
	freopen_s(&f, "CONOUT$", "w", stdout);
	freopen_s(&f, "CONOUT$", "w", stderr);
	printf("mupen64 debug console\n");
#endif
	/* Put absolute App path to AppPath variable */
	getAppFullPath(AppPath);
	app_hInstance = hInstance;
	InitCommonControls();
	SaveCmdLineParameter(lpCmdLine);
	printf("cmd: \"%s\"\n", lpCmdLine);
	ini_openFile();

	// ensure folders exist!
	{
		String path = AppPath;
		CreateDirectory((path + "save").c_str(), NULL);
		CreateDirectory((path + "Mempaks").c_str(), NULL);
		CreateDirectory((path + "Lang").c_str(), NULL);
		CreateDirectory((path + "ScreenShots").c_str(), NULL);
		CreateDirectory((path + "plugin").c_str(), NULL);
	}
	emu_launched = 0;
	emu_paused = 1;

	load_config();

	WNDCLASSEX wc;
	HWND hwnd;
	MSG Msg;
	HACCEL Accel;

	if (GuiDisabled()) {
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = 0;
		wc.lpfnWndProc = NoGuiWndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInstance;
		wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_M64ICONBIG));
		wc.hIconSm = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_M64ICONSMALL));
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = g_szClassName;

		if (!RegisterClassEx(&wc)) {
			MessageBox(NULL, "Window Registration Failed!", "Error!",
				MB_ICONEXCLAMATION | MB_OK);
			return 0;
		}

		hwnd = CreateWindowEx(
			0,
			g_szClassName,
			MUPEN_VERSION,
			WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
			Config.window_x, Config.window_y, Config.window_width, Config.window_height,
			NULL, NULL, hInstance, NULL);

		mainHWND = hwnd;
		ShowWindow(hwnd, nCmdShow);
		UpdateWindow(hwnd);

		StartGameByCommandLine();

		while (GetMessage(&Msg, NULL, 0, 0) > 0) {
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	} else {
	//window initialize

		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = 0;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInstance;
		wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_M64ICONBIG));
		wc.hIconSm = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_M64ICONSMALL));
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);//(HBRUSH)(COLOR_WINDOW+11);
		wc.lpszMenuName = MAKEINTRESOURCE(IDR_MYMENU);
		wc.lpszClassName = g_szClassName;

		if (!RegisterClassEx(&wc)) {
			MessageBox(NULL, "Window Registration Failed!", "Error!",
				MB_ICONEXCLAMATION | MB_OK);
			return 0;
		}

		Accel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCEL));

		hwnd = CreateWindowEx(
			0,
			g_szClassName,
			MUPEN_VERSION,
			WS_OVERLAPPEDWINDOW | WS_EX_COMPOSITED,
			Config.window_x, Config.window_y, Config.window_width, Config.window_height,
			NULL, NULL, hInstance, NULL);

		if (hwnd == NULL) {
			MessageBox(NULL, "Window Creation Failed!", "Error!",
				MB_ICONEXCLAMATION | MB_OK);
			return 0;
		}
		mainHWND = hwnd;
		ShowWindow(hwnd, nCmdShow);

		// This fixes offscreen recording issue
		SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_ACCEPTFILES | WS_EX_LAYERED); //this can't be applied before ShowWindow(), otherwise you must use some fancy function

		UpdateWindow(hwnd);

		setup_dummy_info();
		search_plugins();
		rombrowser_create();
		rombrowser_build();

		vcr_recent_movies_build();
		lua_recent_scripts_build();

		EnableEmulationMenuItems(0);
		if (!StartGameByCommandLine()) {
			cmdlineMode = 0;
		}

		LoadConfigExternals();

		//warning, this is ignored when debugger is attached (like visual studio)
		SetUnhandledExceptionFilter(ExceptionReleaseTarget);

		// raise noncontinuable exception (impossible to recover from it)
		//RaiseException(EXCEPTION_ACCESS_VIOLATION, EXCEPTION_NONCONTINUABLE, NULL, NULL);
		//
		// raise continuable exception
		//RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, NULL, NULL);


		while (GetMessage(&Msg, NULL, 0, 0) > 0) {
			if (!TranslateAccelerator(mainHWND, Accel, &Msg)) {
				TranslateMessage(&Msg);
				DispatchMessage(&Msg);
			}

			// modifier-only checks, cannot be obtained through windows messaging...
			for (int i = 0; i < hotkeys.size(); i++) {
				if (!hotkeys[i]->key && (hotkeys[i]->shift || hotkeys[i]->ctrl || hotkeys[i]->alt)) {
					if (i != 0) {
						if (((GetKeyState(VK_SHIFT) & 0x8000) ? 1 : 0) == hotkeys[i]->shift
							&& ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0) == hotkeys[i]->ctrl
							&& ((GetKeyState(VK_MENU) & 0x8000) ? 1 : 0) == hotkeys[i]->alt) {
							SendMessage(hwnd, WM_COMMAND, hotkeys[i]->command, 0);
						}
					} else // fast-forward
					{
						extern int frame_advancing;
						if (!frame_advancing) { // dont allow fastforward+frameadvance
							if (((GetKeyState(VK_SHIFT) & 0x8000) ? 1 : 0) == hotkeys[i]->shift
								&& ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0) == hotkeys[i]->ctrl
								&& ((GetKeyState(VK_MENU) & 0x8000) ? 1 : 0) == hotkeys[i]->alt) {
								manualFPSLimit = 0;
							} else {
								manualFPSLimit = 1;
							}
						}
					}
				}
			}
		}
	}
	return (int) Msg.wParam;
}
