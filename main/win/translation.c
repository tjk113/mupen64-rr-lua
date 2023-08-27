/***************************************************************************
						  translation.c  -  description
							 -------------------
	copyright            : (C) 2003 by ShadowPrince (shadow@emulation64.com)
	modifications        : linker (linker@mail.bg)
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef _MSC_VER
#include <dirent.h>
#endif
#ifndef _WIN32_IE
#define _WIN32_IE 0x0500
#endif
#include <commctrl.h>
#include "translation.h"
#include "../../winproject/resource.h"
#include "main_win.h"

static int LastLang = -1;

typedef struct _languages  languages;
struct _languages {
	char* file_name;
	char* language_name;
	languages* next;
};
static languages* lang_list = NULL, * current_lang = NULL;



void insert_lang(languages* p, char* file_name, char* language_name) {
	while (p->next) {
		p = p->next;
	}
	p->next = (languages*)malloc(sizeof(languages));
	p->next->file_name = (char*)malloc(strlen(file_name) + 1);
	strcpy(p->next->file_name, file_name);
	p->next->language_name = (char*)malloc(strlen(language_name) + 7);
	strcpy(p->next->language_name, language_name);
	p->next->next = NULL;
	return;
}

void rewind_lang() {
	current_lang = lang_list;
}

void search_languages() {
	HANDLE hFind;
	WIN32_FIND_DATA FindFileData;
	char cwd[MAX_PATH];
	char name[MAX_PATH];
	char String[800];

	lang_list = (languages*)malloc(sizeof(languages));
	lang_list->next = NULL;

	sprintf(cwd, "%slang", AppPath);

	sprintf(String, "%s\\*.lng", cwd);
	hFind = FindFirstFile(String, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		return;
	}
	do {
		if (strcmp(FindFileData.cFileName, ".") != 0 &&
			strcmp(FindFileData.cFileName, "..") != 0 &&
			!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			sprintf(name, "%slang\\%s", AppPath, FindFileData.cFileName);
			memset(String, 0, sizeof(String));
			GetPrivateProfileSectionNames(String, sizeof(String), name);
			if (String) {
				insert_lang(lang_list, name, String);
			}
		}
	} while (FindNextFile(hFind, &FindFileData));
	FindClose(hFind);
}

void SetCurrentLangByName(const char* language_name) {
	languages* p;
	p = lang_list->next;
	while (p) {
		if (strcmp(p->language_name, language_name) == 0) {
			current_lang = p;
			return;
		}
		p = p->next;
	}
}

void SelectLang(HWND hWnd, int LangMenuID) {
	char String[800];
	HMENU hMenu = GetMenu(hWnd);
	MENUITEMINFO menuinfo;
	menuinfo.cbSize = sizeof(MENUITEMINFO);
	menuinfo.fMask = MIIM_TYPE;
	menuinfo.fType = MFT_STRING;
	menuinfo.dwTypeData = String;
	menuinfo.cch = sizeof(String);
	GetMenuItemInfo(hMenu, LangMenuID, FALSE, &menuinfo);

	Config.language = std::string(String);

	if (LastLang != -1) {
		CheckMenuItem(hMenu, LastLang, MF_BYCOMMAND | MFS_UNCHECKED);
	}
	LastLang = LangMenuID;
	CheckMenuItem(hMenu, LastLang, MF_BYCOMMAND | MFS_CHECKED);
	SetCurrentLangByName(Config.language.c_str());
}

void SetupLanguages(HWND hWnd) {
	char String[800];
	HMENU hMenu = GetMenu(hWnd), hSubMenu;
	MENUITEMINFO menuinfo;
	int count;
	count = 1;
	languages* p;
	search_languages();
	p = lang_list->next;
	hSubMenu = GetSubMenu(hMenu, 0);
	hSubMenu = GetSubMenu(hSubMenu, 8);
	menuinfo.cbSize = sizeof(MENUITEMINFO);
	menuinfo.fMask = MIIM_TYPE | MIIM_ID;
	menuinfo.fType = MFT_STRING;
	menuinfo.fState = MFS_ENABLED;
	menuinfo.dwTypeData = String;
	menuinfo.cch = sizeof(hSubMenu);
	while (p) {
		if (strcmp(p->language_name, "English") == 0) {
			if (Config.language == "English") {
				CheckMenuItem(hSubMenu, ID_LANG_ENGLISH, MF_BYCOMMAND | MFS_CHECKED);
				LastLang = ID_LANG_ENGLISH;
				current_lang = p;
			}
		} else {
			menuinfo.dwTypeData = p->language_name;
			menuinfo.cch = strlen(p->language_name) + 1;
			menuinfo.wID = ID_LANG_ENGLISH + count;
			InsertMenuItem(hSubMenu, count++, TRUE, &menuinfo);
			if (Config.language == p->language_name) {
				CheckMenuItem(hSubMenu, menuinfo.wID, MF_BYCOMMAND | MFS_CHECKED);
				LastLang = menuinfo.wID;
				current_lang = p;
			}
		}
		p = p->next;
	}
	SetCurrentLangByName(Config.language.c_str());
}

void freeLanguages() {
	languages* p, * temp;
	p = lang_list;
	while (p) {
		temp = p->next;
		free(p);
		p = temp;
	}
}

void Translate(const char* GuiWord, char* Ret) {
	TranslateDefault(GuiWord, GuiWord, Ret);
}

void TranslateDefault(const char* GuiWord, const char* Default, char* Ret) {
	if (current_lang) {
		GetPrivateProfileString(current_lang->language_name, GuiWord, Default, Ret, 200, current_lang->file_name);
	} else {
		snprintf(Ret, 200, GuiWord);
	}
}

void SetItemTranslatedString(HWND hwnd, int ElementID, const char* Str) {
	char String[800];
	Translate(Str, String);
	SetDlgItemText(hwnd, ElementID, String);
}

void SetItemTranslatedStringDefault(HWND hwnd, int ElementID, const char* Str, const char* Def) {
	char String[800];
	TranslateDefault(Str, Def, String);
	SetDlgItemText(hwnd, ElementID, String);
}


void SetStatusTranslatedString(HWND hStatus, int section, const char* Str) {
	char String[800];
	Translate(Str, String);
	if (section == 0) strcpy(statusmsg, String);
	SendMessage(hStatus, SB_SETTEXT, section, (LPARAM)String);
}

void SetMenuTranslatedString(HMENU hMenu, int elementID, const char* Str, const char* Acc) {
	char String[800];
	MENUITEMINFO menuinfo;
	Translate(Str, String);
	if (strcmp(Acc, "")) sprintf(String, "%s\t%s", String, Acc);

	memset(&menuinfo, 0, sizeof(MENUITEMINFO));
	menuinfo.cbSize = sizeof(MENUITEMINFO);
	menuinfo.fMask = MIIM_TYPE;
	menuinfo.fType = MFT_STRING;
	menuinfo.dwTypeData = String;
	menuinfo.cch = sizeof(String);
	SetMenuItemInfo(hMenu, elementID, TRUE, &menuinfo);
}


void SetMenuAccelerator(HMENU hMenu, int elementID, const char* Acc) {
	char String[800];
	MENUITEMINFO menuinfo;

	GetMenuString(hMenu, elementID, String, 800, MF_BYPOSITION);
	char* tab = strrchr(String, '\t');
	if (tab)
		*tab = '\0';
	if (strcmp(Acc, ""))
		sprintf(String, "%s\t%s", String, Acc);

	memset(&menuinfo, 0, sizeof(MENUITEMINFO));
	menuinfo.cbSize = sizeof(MENUITEMINFO);
	menuinfo.fMask = MIIM_TYPE;
	menuinfo.fType = MFT_STRING;
	menuinfo.dwTypeData = String;
	menuinfo.cch = sizeof(String);
	SetMenuItemInfo(hMenu, elementID, TRUE, &menuinfo);
}

static void SetHotkeyMenuAccelerators(t_hotkey* hotkey, HMENU hmenu, int menuItemID) {
	char buf[64] = { 0 };
	hotkey_to_string(hotkey, buf);

	if (hmenu && menuItemID >= 0) {
		if (strcmp(buf, "(nothing)"))
			SetMenuAccelerator(hmenu, menuItemID, buf);
		else
			SetMenuAccelerator(hmenu, menuItemID, "");
	}
}

void TranslateMenu(HMENU hMenu, HWND mainHWND) {
	MENUITEMINFO mii{};
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_SUBMENU | MIIM_ID | MIIM_STRING;
	char buf[256];
	int nCount = GetMenuItemCount(hMenu);

	for (int i = 0; i < nCount; i++) {
		mii.dwTypeData = buf;
		mii.cch = sizeof(buf);
		if (!GetMenuItemInfo(hMenu, i, TRUE, &mii))
			continue;
		if (mii.cch > 0) {
			SetMenuTranslatedString(hMenu, i, buf, "");
		}
		if (mii.hSubMenu != NULL)
			TranslateMenu(mii.hSubMenu, mainHWND); // ** recursive **
	}
}

void SetMenuAcceleratorsFromUser(HWND mainHWND) {
	SetHotkeyMenuAccelerators(&Config.pause_hotkey, GetSubMenu(GetMenu(mainHWND), 1), 0);
	SetHotkeyMenuAccelerators(&Config.frame_advance_hotkey, GetSubMenu(GetMenu(mainHWND), 1), 1);
	SetHotkeyMenuAccelerators(&Config.load_from_current_slot_hotkey, GetSubMenu(GetMenu(mainHWND), 1), 4);
	SetHotkeyMenuAccelerators(&Config.save_to_current_slot_hotkey, GetSubMenu(GetMenu(mainHWND), 1), 6);

	SetHotkeyMenuAccelerators(&Config.toggle_read_only_hotkey, GetSubMenu(GetMenu(mainHWND), 3), 13);
	SetHotkeyMenuAccelerators(&Config.start_movie_playback_hotkey, GetSubMenu(GetMenu(mainHWND), 3), 3);
	SetHotkeyMenuAccelerators(&Config.stop_movie_playback_hotkey, GetSubMenu(GetMenu(mainHWND), 3), 4);
	SetHotkeyMenuAccelerators(&Config.start_movie_recording_hotkey, GetSubMenu(GetMenu(mainHWND), 3), 0);
	SetHotkeyMenuAccelerators(&Config.stop_movie_recording_hotkey, GetSubMenu(GetMenu(mainHWND), 3), 1);
	SetHotkeyMenuAccelerators(&Config.take_screenshot_hotkey, GetSubMenu(GetMenu(mainHWND), 1), 2);
	SetHotkeyMenuAccelerators(&Config.save_to_current_slot_hotkey, GetSubMenu(GetMenu(mainHWND), 1), 4);
	SetHotkeyMenuAccelerators(&Config.load_from_current_slot_hotkey, GetSubMenu(GetMenu(mainHWND), 1), 6);
	SetHotkeyMenuAccelerators(&Config.select_slot_1_hotkey, GetSubMenu(GetSubMenu(GetMenu(mainHWND), 1), 9), 0);
	SetHotkeyMenuAccelerators(&Config.select_slot_2_hotkey, GetSubMenu(GetSubMenu(GetMenu(mainHWND), 1), 9), 1);
	SetHotkeyMenuAccelerators(&Config.select_slot_3_hotkey, GetSubMenu(GetSubMenu(GetMenu(mainHWND), 1), 9), 2);
	SetHotkeyMenuAccelerators(&Config.select_slot_4_hotkey, GetSubMenu(GetSubMenu(GetMenu(mainHWND), 1), 9), 3);
	SetHotkeyMenuAccelerators(&Config.select_slot_5_hotkey, GetSubMenu(GetSubMenu(GetMenu(mainHWND), 1), 9), 4);
	SetHotkeyMenuAccelerators(&Config.select_slot_6_hotkey, GetSubMenu(GetSubMenu(GetMenu(mainHWND), 1), 9), 5);
	SetHotkeyMenuAccelerators(&Config.select_slot_7_hotkey, GetSubMenu(GetSubMenu(GetMenu(mainHWND), 1), 9), 6);
	SetHotkeyMenuAccelerators(&Config.select_slot_8_hotkey, GetSubMenu(GetSubMenu(GetMenu(mainHWND), 1), 9), 7);
	SetHotkeyMenuAccelerators(&Config.select_slot_9_hotkey, GetSubMenu(GetSubMenu(GetMenu(mainHWND), 1), 9), 8);

	SetHotkeyMenuAccelerators(&Config.restart_movie_hotkey, GetSubMenu(GetMenu(mainHWND), 3), 12);
	SetHotkeyMenuAccelerators(&Config.play_latest_movie_hotkey, GetSubMenu(GetMenu(mainHWND), 3), 7);
}


void TranslateGeneralDialog(HWND hwnd) {
	SetItemTranslatedString(hwnd, IDC_MESSAGES, "Safety");
	SetItemTranslatedString(hwnd, IDC_MANAGEBADROM, "Manage Bad ROMs");
	SetItemTranslatedString(hwnd, IDC_ALERTSAVESTATEWARNINGS, "Alert Save Error");
	SetItemTranslatedString(hwnd, IDC_ALLOW_ARBITRARY_SAVESTATE_LOADING, "State-independent savestate loading");

	SetItemTranslatedString(hwnd, IDC_FPSTITLE, "FPS and VIs");

	SetItemTranslatedString(hwnd, IDC_LIMITFPS, "Limit FPS (auto)");
	SetItemTranslatedString(hwnd, IDC_SHOWFPS, "Show FPS");
	SetItemTranslatedString(hwnd, IDC_SHOWVIS, "Show VIs");
	SetItemTranslatedString(hwnd, IDC_FASTFORWARDSKIPFREQ, "Fast forward skip frequency:");

	SetItemTranslatedString(hwnd, IDC_INTERP, "Interpreter");
	SetItemTranslatedString(hwnd, IDC_RECOMP, "Dynamic Recompiler");
	SetItemTranslatedString(hwnd, IDC_PURE_INTERP, "Pure Interpreter");
	SetItemTranslatedString(hwnd, IDC_CPUCORE, "Core");

}

void TranslateRomInfoDialog(HWND hwnd) {
	char tmp[200];
	SetItemTranslatedStringDefault(hwnd, IDC_ROM_HEADER_INFO_TEXT, "ROM Information", "RP ROM Information");
	SetItemTranslatedStringDefault(hwnd, IDC_ROM_FULLPATH_TEXT, "File Location: ", "RP File Location");
	SetItemTranslatedStringDefault(hwnd, IDC_ROM_GOODNAME_TEXT, "Good Name: ", "RP Good Name");
	SetItemTranslatedStringDefault(hwnd, IDC_ROM_INTERNAL_NAME_TEXT, "Internal Name: ", "RP Internal Name");
	SetItemTranslatedStringDefault(hwnd, IDC_ROM_SIZE_TEXT, "Size: ", "RP Size");
	SetItemTranslatedStringDefault(hwnd, IDC_ROM_COUNTRY_TEXT, "Country: ", "RP Country:");
	SetItemTranslatedStringDefault(hwnd, IDC_ROM_INICODE_TEXT, "Ini Code: ", "RP Ini Code");
	SetItemTranslatedStringDefault(hwnd, IDC_ROM_MD5_TEXT, "MD5 Checksum: ", "RP MD5 Checksum");
	SetItemTranslatedStringDefault(hwnd, IDC_MD5_CALCULATE, "Calculate", "RP Calculate");

	SetItemTranslatedStringDefault(hwnd, IDC_ROM_PLUGINS, "Plugins", "RP Plugins");
	SetItemTranslatedStringDefault(hwnd, IDC_GFXPLUGIN, "Video Plugin", "RP Video Plugin");
	SetItemTranslatedStringDefault(hwnd, IDC_SOUNDPLUGIN, "Sound Plugin", "RP Sound Plugin");
	SetItemTranslatedStringDefault(hwnd, IDC_INPUTPLUGIN, "Input Plugin", "RP Input Plugin");
	SetItemTranslatedStringDefault(hwnd, IDC_RSPPLUGIN, "RSP Plugin", "RP RSP Plugin");
	SetItemTranslatedStringDefault(hwnd, IDC_SAVE_PROFILE, "Save Plugins", "RP Save Plugins");

	SetItemTranslatedStringDefault(hwnd, IDC_INI_COMMENTS_TEXT, "Comments", "RP Comments");

	SetItemTranslatedString(hwnd, IDC_OK, "Ok");
	SetItemTranslatedString(hwnd, IDC_CANCEL, "Cancel");
	TranslateDefault("ROM Properties", "ROM Properties", tmp);
	SetWindowText(hwnd, tmp);
}

void TranslateRomBrowserMenu(HMENU hMenu) {
	SetMenuTranslatedString(hMenu, 0, "Play Game", "Enter");
	SetMenuTranslatedString(hMenu, 1, "ROM Properties...", "Ctrl P");
	SetMenuTranslatedString(hMenu, 3, "Refresh ROM List", "Ctrl Alt R");
}

void TranslateAuditDialog(HWND hwnd) {
	char tmp[200];
	SetItemTranslatedString(hwnd, IDC_TOTAL_ROMS_TEXT, "Audit Total ROMs");
	SetItemTranslatedString(hwnd, IDC_CURRENT_ROM_TEXT, "Audit Current ROM");
	SetItemTranslatedString(hwnd, IDC_ROM_FULLPATH_TEXT, "Audit File Location");
	SetItemTranslatedString(hwnd, IDC_AUDIT_START, "Start");
	SetItemTranslatedString(hwnd, IDC_AUDIT_STOP, "Stop");
	SetItemTranslatedString(hwnd, IDC_AUDIT_CLOSE, "Close");
	TranslateDefault("Audit ROMs", "Audit ROMs", tmp);
	SetWindowText(hwnd, tmp);
}

void TranslateLangInfoDialog(HWND hwnd) {
	char tmp[200];
	TranslateDefault("Language Information Dialog", "Language Information", tmp);
	SetItemTranslatedString(hwnd, IDOK, "Ok");
	if (!current_lang)         // no language (default english)
							   // dont show placeholder text
		return;

	SetItemTranslatedString(hwnd, IDC_LANG_AUTHOR_TEXT, "Translation Author TEXT");
	SetItemTranslatedString(hwnd, IDC_LANG_VERSION_TEXT, "Version TEXT");
	SetItemTranslatedString(hwnd, IDC_LANG_DATE_TEXT, "Creation Date TEXT");
	SetItemTranslatedString(hwnd, IDC_LANG_AUTHOR, "Translation Author");
	SetItemTranslatedString(hwnd, IDC_LANG_VERSION, "Version");
	SetItemTranslatedString(hwnd, IDC_LANG_DATE, "Creation Date");
	SetWindowText(hwnd, tmp);
}

void TranslateAdvancedDialog(HWND hwnd) {
	SetItemTranslatedString(hwnd, IDC_COMMON, "Common Options");
	SetItemTranslatedString(hwnd, IDC_PAUSENOTACTIVE, "Pause when unfocused");
	SetItemTranslatedString(hwnd, IDC_GUI_TOOLBAR, "Show toolbar");
	SetItemTranslatedString(hwnd, IDC_GUI_STATUSBAR, "Show statusbar");

	SetItemTranslatedString(hwnd, IDC_COMPATIBILITY, "Compatibility Options");
	SetItemTranslatedString(hwnd, IDC_NO_AUDIO_DELAY, "No audio delay");
	SetItemTranslatedString(hwnd, IDC_NO_COMPILED_JUMP, "No compiled jump");
	SetItemTranslatedString(hwnd, IDC_EMULATEFLOATCRASHES, "Emulate float crashes");
	SetItemTranslatedString(hwnd, IDC_ROUNDTOZERO, "WiiVC Rounding");

	SetItemTranslatedString(hwnd, IDC_ROMBROWSERCOLUMNS, "Rombrowser Columns");
	SetItemTranslatedString(hwnd, IDC_COLUMN_GOODNAME, "GoodName");
	SetItemTranslatedString(hwnd, IDC_COLUMN_INTERNALNAME, "Internal Name");
	SetItemTranslatedString(hwnd, IDC_COLUMN_COUNTRY, "Country");
	SetItemTranslatedString(hwnd, IDC_COLUMN_SIZE, "ROM size");
	SetItemTranslatedString(hwnd, IDC_COLUMN_COMMENTS, "Comments");
	SetItemTranslatedString(hwnd, IDC_COLUMN_FILENAME, "File Name");
	SetItemTranslatedString(hwnd, IDC_COLUMN_MD5, "MD5");

	SetItemTranslatedString(hwnd, IDC_WARNING_OPTIONS, "Warning Options");

	SetItemTranslatedString(hwnd, IDC_RECORDINGOPT, "Recording Options");
	SetItemTranslatedString(hwnd, IDC_NORESET, "Record resets");
	SetItemTranslatedString(hwnd, IDC_FORCEINTERNAL, "Force internal avi capture");
}

void TranslateHotkeyDialog(HWND hwnd) {
	SetItemTranslatedString(hwnd, IDC_HOTKEYS_FLOWGROUP, "Speed / Flow / Movie / Other");
	SetItemTranslatedString(hwnd, IDC_FASTFORWARD, "Fast-Forward:");
	SetItemTranslatedString(hwnd, IDC_FRAMEADVANCE, "Frame Advance:");
	SetItemTranslatedString(hwnd, IDC_SPDUP, "Speed Up:");
	SetItemTranslatedString(hwnd, IDC_SPDDOWN, "Speed Down:");
	SetItemTranslatedString(hwnd, IDC_PAUSE, "Pause / Resume:");
	SetItemTranslatedString(hwnd, IDC_READONLY, "Toggle Read-Only:");
	SetItemTranslatedString(hwnd, IDC_SCREENSHOT, "Take Screenshot:");

	SetItemTranslatedString(hwnd, IDC_PLAYMOVIE, "Play Movie:");
	SetItemTranslatedString(hwnd, IDC_STOPPLAY, "Stop Playing:");
	SetItemTranslatedString(hwnd, IDC_RECORDMOVIE, "Record Movie:");
	SetItemTranslatedString(hwnd, IDC_STOPRECORD, "Stop Recording:");

	SetItemTranslatedString(hwnd, IDC_HOTKEYS_SAVEGROUP2, "Save");
	SetItemTranslatedString(hwnd, IDC_SAVE, "Save");
	SetItemTranslatedString(hwnd, IDC_LOAD, "Load");
	SetItemTranslatedString(hwnd, IDC_SELECT, "Select (1-9)");
	SetItemTranslatedString(hwnd, IDC_CURRENT, "Current:");
}

