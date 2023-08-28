#include "LuaConsole.h"
#include "Recent.h"
#include <algorithm>
#include <windows.h>
#include "win/main_win.h"
#include "../winproject/resource.h"

void lua_recent_scripts_build(int32_t reset)
{
	HMENU h_menu = GetMenu(mainHWND);
	for (size_t i = 0; i < Config.recent_lua_script_paths.size(); i++)
	{
		if (Config.recent_lua_script_paths[i].empty())
		{
			continue;
		}
		DeleteMenu(h_menu, ID_LUA_RECENT + i, MF_BYCOMMAND);
	}

	if (reset)
	{
		Config.recent_lua_script_paths.clear();
	}

	HMENU h_sub_menu = GetSubMenu(h_menu, 6);
	h_sub_menu = GetSubMenu(h_sub_menu, 3);

	MENUITEMINFO menu_info = {0};
	menu_info.cbSize = sizeof(MENUITEMINFO);
	menu_info.fMask = MIIM_TYPE | MIIM_ID;
	menu_info.fType = MFT_STRING;
	menu_info.fState = MFS_ENABLED;

	for (size_t i = 0; i < Config.recent_lua_script_paths.size(); i++)
	{
		if (Config.recent_lua_script_paths[i].empty())
		{
			continue;
		}
		menu_info.dwTypeData = (LPSTR)Config.recent_lua_script_paths[i].c_str();
		menu_info.cch = strlen(menu_info.dwTypeData);
		menu_info.wID = ID_LUA_RECENT + i;
		InsertMenuItem(h_sub_menu, i + 3, TRUE, &menu_info);
	}
}

void lua_recent_scripts_add(const std::string& path)
{
    std::erase(Config.recent_lua_script_paths, path);
	Config.recent_lua_script_paths.insert(Config.recent_lua_script_paths.begin(), path);
	lua_recent_scripts_build();
}

int32_t lua_recent_scripts_run(uint16_t menu_item_id)
{
	const int index = menu_item_id - ID_LUA_RECENT;
	LuaOpenAndRun(Config.recent_lua_script_paths[index].c_str());
	return 1;
}
