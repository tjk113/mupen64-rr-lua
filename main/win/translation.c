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

#include "translation.h"

#include <Windows.h>
#include <stdio.h>
#include <commctrl.h>
#include "Config.hpp"
#include "../../winproject/resource.h"
#include "main_win.h"

void SetMenuAccelerator(HMENU hMenu, int elementID, const char* Acc)
{
    char String[800];
    MENUITEMINFO menuinfo;

    GetMenuString(hMenu, elementID, String, 800, MF_BYPOSITION);
    char* tab = strrchr(String, '\t');
    if (tab)
        *tab = '\0';
    if (strcmp(Acc, ""))
        sprintf(String, "%s\t%s", String, Acc); // String into itself

    memset(&menuinfo, 0, sizeof(MENUITEMINFO));
    menuinfo.cbSize = sizeof(MENUITEMINFO);
    menuinfo.fMask = MIIM_TYPE;
    menuinfo.fType = MFT_STRING;
    menuinfo.dwTypeData = String;
    menuinfo.cch = sizeof(String);
    SetMenuItemInfo(hMenu, elementID, TRUE, &menuinfo);
}

void SetHotkeyMenuAccelerators(t_hotkey* hotkey, HMENU hmenu, int menuItemID)
{
    if (hmenu && menuItemID >= 0)
    {
        const std::string hotkey_str = hotkey_to_string(hotkey);
        SetMenuAccelerator(hmenu, menuItemID, hotkey_str == "(nothing)" ? "" : hotkey_str.c_str());
    }
}
