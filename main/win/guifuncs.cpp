/***************************************************************************
						  guifuncs.c  -  description
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

#include <LuaConsole.h>
#include <Windows.h>
#include <commctrl.h>
#include <cstdio>
#include "../guifuncs.h"
#include "main_win.h"
#include "Config.hpp"
#include <vcr.h>

#include "../../r4300/tracelog.h"
#include "capture/EncodingManager.h"
#include "features/Statusbar.hpp"


bool confirm_user_exit()
{
    int res = 0;
	int warnings = 0;

    std::string final_message;
    if (vcr_is_recording())
    {
	    final_message.append("Movie recording ");
	    warnings++;
    }
    if (EncodingManager::is_capturing())
    {
	    if (warnings > 0) { final_message.append(","); }
	    final_message.append(" AVI capture ");
	    warnings++;
    }
    if (tracelog::active())
    {
	    if (warnings > 0) { final_message.append(","); }
	    final_message.append(" Trace logging ");
	    warnings++;
    }
    final_message.
	    append("is running. Are you sure you want to stop emulation?");
    if (warnings > 0)
	    res = MessageBox(mainHWND, final_message.c_str(), "Stop emulation?",
	                     MB_YESNO | MB_ICONWARNING);

    return res == IDYES || warnings == 0;
}

void show_modal_info(const char* str, const char* title)
{
	MessageBox(mainHWND, str, title, MB_OK | MB_ICONINFORMATION);
}
