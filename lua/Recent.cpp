//Based on recent roms but better
#include "Recent.h"
#include "LuaConsole.h"

#ifdef _WIN32
#include <windows.h>
#include "win/main_win.h"
#include "../winproject/resource.h" //for menu id
#endif

BOOL freezeRecentScript;

//takes config and puts the entries to menu
void BuildRecentScriptsMenu(HWND hwnd) {
	int i;
	bool empty = false;
	MENUITEMINFO menuinfo = { 0 };
	HMENU hMenu = GetMenu(hwnd);
	HMENU hSubMenu = GetSubMenu(hMenu, 5);
	hSubMenu = GetSubMenu(hSubMenu, 2);
	//DeleteMenu(hSubMenu, ID_LUA_RECENT, MF_BYCOMMAND); //remove the "no recent scripts" entry, add later if in fact no recent
	
	menuinfo.cbSize = sizeof(MENUITEMINFO);
	menuinfo.fMask = MIIM_TYPE | MIIM_ID;
	menuinfo.fType = MFT_STRING;
	menuinfo.fState = MFS_ENABLED;
	for (i = 0; i < LUA_MAX_RECENT; i++) {
		if (strcmp(Config.RecentScripts[i], "") == 0)
		{
			if (i == 0)
			{
				menuinfo.dwTypeData = "No recent scripts";
				empty = true;
			}
			else break;
		}
		else
			menuinfo.dwTypeData = Config.RecentScripts[i];

		
		menuinfo.cch = strlen(menuinfo.dwTypeData);
		menuinfo.wID = ID_LUA_RECENT + i;
		InsertMenuItem(hSubMenu, i+3, TRUE, &menuinfo);
		if (empty)  EnableMenuItem(hSubMenu, ID_LUA_RECENT, MF_DISABLED);
	}
}

void ClearRecent(BOOL clear_array) {
	int i;
	HMENU hMenu;

	hMenu = GetMenu(mainHWND);
	for (i = 0; i < LUA_MAX_RECENT; i++) {
		DeleteMenu(hMenu, ID_LUA_RECENT + i, MF_BYCOMMAND);
	}
	if (clear_array) {
		memset(Config.RecentScripts, 0, LUA_MAX_RECENT * sizeof(Config.RecentScripts[0]));
	}

}

void RefreshRecent()
{
	//nuke the menu 
	ClearRecent(FALSE);
	//rebuild
	BuildRecentScriptsMenu(mainHWND);


}


//Adds path to recent paths, if already in list then just moves things around
//newer scripts are earlier in array
void AddToRecentScripts(char* path)
{
	if (Config.RecentScriptsFreeze) return; // fuck off?

	int i = 0;
	//Either finds index of path in recent list, or stops at last one
	//notice how it doesn't matter if last==path or not, we do same swapping later
	for (; i<LUA_MAX_RECENT-1; ++i)
	{
		//if matches or empty (list is not full), break
		if (Config.RecentScripts[i][0]==0 || !strcmp(Config.RecentScripts[i], path)) break;
	}
	//now swap all elements backwards starting from `i`
	for (int j = i; j>0; --j)
	{
		strcpy(Config.RecentScripts[j], Config.RecentScripts[j-1]);
	}
	//now write to top
	strcpy(Config.RecentScripts[0], path);
	//rebuild menu
	RefreshRecent();
}

void RunRecentScript(WORD menuItem)
{
		char path[MAX_PATH];
		int index = menuItem - ID_LUA_RECENT;
		sprintf(path, Config.RecentScripts[index]);
		LuaOpenAndRun(path);
}