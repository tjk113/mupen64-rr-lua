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

#define _WIN32_IE 0x0500
#include <LuaConsole.h>

#include <Windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "../guifuncs.h"
#include "main_win.h"
#include "Config.hpp"
#include "translation.h"
#include "features/RomBrowser.hpp"
#include "../../winproject/resource.h"
#include <vcr.h>

#include "features/Statusbar.hpp"

const char* get_savespath()
{
    static char defDir[MAX_PATH];
    if (Config.is_default_saves_directory_used)
    {
        sprintf(defDir, "%sSave\\", app_path.c_str());
        return defDir;
    }
    else return Config.saves_directory.c_str();
}

bool warn_recording()
{
    int res = 0, warnings = res;
    if (continue_vcr_on_restart_mode == 0)
    {
        std::string finalMessage = "";
        if (VCR_isRecording())
        {
            finalMessage.append("Movie recording ");
            warnings++;
        }
        if (VCR_isCapturing())
        {
            if (warnings > 0) { finalMessage.append(","); }
            finalMessage.append(" AVI capture ");
            warnings++;
        }
        if (enableTraceLog)
        {
            if (warnings > 0) { finalMessage.append(","); }
            finalMessage.append(" Trace logging ");
            warnings++;
        }
        finalMessage.append("is running. Are you sure you want to stop emulation?");
        if (warnings > 0) res = MessageBox(mainHWND, finalMessage.c_str(), "Stop emulation?",
                                           MB_YESNO | MB_ICONWARNING);
    }

    return res == 7;
}

void internal_warnsavestate(const char* messageCaption, const char* message, bool modal)
{
    if (!Config.is_savestate_warning_enabled) return;

    if (modal)
        MessageBox(mainHWND, message, messageCaption, MB_ICONERROR);
    else
        statusbar_post_text(std::string(messageCaption) + " - " + std::string(message));
}

void warn_savestate(const char* messageCaption, const char* message)
{
    internal_warnsavestate(messageCaption, message, false);
}

void warn_savestate(const char* messageCaption, const char* message, bool modal)
{
    internal_warnsavestate(messageCaption, message, modal);
}

