//Based on recent roms but better
#include "LuaConsole.h"
#include "Recent.h"
#include <windows.h>
#include "win/main_win.h"
#include "../winproject/resource.h" //for menu id

BOOL freezeRecentScript;
bool emptyRecentScripts = false;

//takes config and puts the entries to menu
void BuildRecentScriptsMenu(HWND hwnd) {
	int i;
	MENUITEMINFO menuinfo = {0};
	HMENU hMenu = GetMenu(hwnd);
	HMENU hSubMenu = GetSubMenu(hMenu, 6);
	hSubMenu = GetSubMenu(hSubMenu, 3);
	//DeleteMenu(hSubMenu, ID_LUA_RECENT, MF_BYCOMMAND); //remove the "no recent scripts" entry, add later if in fact no recent

	menuinfo.cbSize = sizeof(MENUITEMINFO);
	menuinfo.fMask = MIIM_TYPE | MIIM_ID;
	menuinfo.fType = MFT_STRING;
	menuinfo.fState = MFS_ENABLED;

	for (i = 0; i < Config.recent_lua_script_paths.size(); i++) {
		auto& str = Config.recent_lua_script_paths[i];
		if (str == "") {
			if (i == 0) {
				menuinfo.dwTypeData = (LPTSTR)"No Recent Scripts";
			} else break;
		} else {
			menuinfo.dwTypeData = (LPSTR)str.c_str();
		}

		menuinfo.cch = strlen(menuinfo.dwTypeData);
		menuinfo.wID = ID_LUA_RECENT + i;
		InsertMenuItem(hSubMenu, i + 3, TRUE, &menuinfo);
	}
}

void EnableRecentScriptsMenu(HMENU hMenu, BOOL flag) {
	if (!emptyRecentScripts) {
		for (int i = 0; i < LUA_MAX_RECENT; i++) {
			EnableMenuItem(hMenu, ID_LUA_RECENT + i, flag ? MF_ENABLED : MF_DISABLED);
		}
		EnableMenuItem(hMenu, ID_LUA_LOAD_LATEST, flag ? MF_ENABLED : MF_DISABLED);
	}
	EnableMenuItem(hMenu, ID_MENU_LUASCRIPT_NEW, !IsMenuItemEnabled(hMenu, REFRESH_ROM_BROWSER) ? MF_ENABLED : MF_DISABLED);
	EnableMenuItem(hMenu, ID_MENU_LUASCRIPT_CLOSEALL, !IsMenuItemEnabled(hMenu, REFRESH_ROM_BROWSER) ? MF_ENABLED : MF_DISABLED);
}

void ClearRecent(BOOL clear_array) {
	int i;
	HMENU hMenu;

	hMenu = GetMenu(mainHWND);
	for (i = 0; i < LUA_MAX_RECENT; i++) {
		DeleteMenu(hMenu, ID_LUA_RECENT + i, MF_BYCOMMAND);
	}
	if (clear_array) {
		Config.recent_lua_script_paths.clear();
	}
	emptyRecentScripts = false;
}

void RefreshRecent() {
	//nuke the menu
	ClearRecent(FALSE);
	//rebuild
	BuildRecentScriptsMenu(mainHWND);


}


//Adds path to recent paths, if already in list then just moves things around
//newer scripts are earlier in array
void AddToRecentScripts(char* path) {
	if (Config.is_recent_scripts_frozen) return; // fuck off?

	int i = 0;
	//Either finds index of path in recent list, or stops at last one
	//notice how it doesn't matter if last==path or not, we do same swapping later
	for (; i < Config.recent_lua_script_paths.size() - 1; ++i) {
		//if matches or empty (list is not full), break
		if (Config.recent_lua_script_paths[i][0] == 0 || Config.recent_lua_script_paths[i] == path) break;
	}
	//now swap all elements backwards starting from `i`
	for (int j = i; j > 0; --j) {
		Config.recent_lua_script_paths[j] = Config.recent_lua_script_paths[j - 1];
	}
	//now write to top
	Config.recent_lua_script_paths[0] = path;
	//rebuild menu
	RefreshRecent();
}

void RunRecentScript(WORD menuItem) {
	int index = menuItem - ID_LUA_RECENT;
	LuaOpenAndRun(Config.recent_lua_script_paths[index].c_str());
}
