/***************************************************************************
						  commandline.c  -  description
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

// Based on code from 1964 by Schibo and Rice
// Slightly improved command line params parsing function to work with spaced arguments

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "commandline.h"

#include "Config.hpp"
#include "main_win.h"
#include "../plugin.hpp"

static DWORD Id;
BOOL cmdlineMode = 0;
BOOL cmdlineSave = 0;
BOOL cmdlineNoGui = 0;
char cmdLineParameterBuf[512] = {0};

void SaveCmdLineParameter(char* cmdline)
{
    strcpy(cmdLineParameterBuf, cmdline);
    if (cmdLineParameterBuf[0] != '\0')
    {
        int i;
        int len = (int)strlen(cmdLineParameterBuf);
        for (i = 0; i < len; i++)
        {
            if (isupper(cmdLineParameterBuf[i]))
            {
                cmdLineParameterBuf[i] = (char)tolower(cmdLineParameterBuf[i]);
            }
        }
    }
}

//To get a command line parameter if available, please pass a flag
// Flags:
//	"-v"	-> return video plugin name
//	"-a"	-> return audio plugin name
//  "-c"	-> return controller plugin name
//  "-g"	-> return game name to run
//	"-f"	-> return play-in-full-screen flag
//	"-r"	-> return rom path
//  "-nogui"-> nogui mode
//  "-save" -> save options on exit 

//  "-m64"  -> play m64 from path, requires -g
//  "-avi"  -> capture m64 to avi, requires -m64
//  "-lua"  -> play a lua script from path, requires -g
//  "-st"   -> load a savestate from path, requires -g, cant have -m64
const char* CmdLineArgFlags[] =
{
    "-a",
    "-v",
    "-c",
    "-rsp",
    "-r",
    "-g",
    "-f",
    "-nogui",
    "-save",
    //new parameters
    "-m64",
    "-avi",
    "-lua",
    "-st"
};

void GetCmdLineParameter(CmdLineParameterType arg, char* buf)
{
    char* ptr1;
    char* ptr2 = buf;

    if ((arg >= CMDLINE_MAX_NUMBER) || (ptr1 = strstr(cmdLineParameterBuf, CmdLineArgFlags[arg])) == NULL)
    {
        buf[0] = 0;
        return;
    }

    if (arg == CMDLINE_FULL_SCREEN_FLAG)
    {
        strcpy(buf, "1");
        return;
    }

    if (arg == CMDLINE_NO_GUI)
    {
        strcpy(buf, "1");
        return;
    }

    if (arg == CMDLINE_SAVE_OPTIONS)
    {
        strcpy(buf, "1");
        return;
    }

    ptr1 = strstr(cmdLineParameterBuf, CmdLineArgFlags[arg]);
    ptr1 += strlen(CmdLineArgFlags[arg]); //Skip the flag

    while (*ptr1 != 0 && isspace(*ptr1))
    {
        ptr1++; //skip all spaces
    }

    if (strncmp(ptr1, "\"", 1) == 0)
    {
        ptr1++; //skipping first "
        while (!(strncmp(ptr1, "\"", 1) == 0) && (*ptr1 != 0))
        {
            *ptr2++ = *ptr1++;
        }
    }
    else
    {
        while (!isspace(*ptr1) && *ptr1 != 0)
        {
            *ptr2++ = *ptr1++;
        }
    }
    *ptr2 = 0;
}

std::string setPluginFromCmndLine(CmdLineParameterType plugintype, int spec_type)
{
    char tempstr[100];
    char* tempPluginStr;
    GetCmdLineParameter(plugintype, tempstr);
    if (tempstr[0] != '\0')
    {
        printf("Command Line: Checking plugin name: %s\n", tempstr);
        // TODO: reimplement
        // tempPluginStr = getPluginName(tempstr, spec_type);
        if (tempPluginStr)
        {
            return tempPluginStr;
        }
        
    }
    // Where is return statement?
}

BOOL StartGameByCommandLine()
{
    char szFileName[MAX_PATH];

    if (cmdLineParameterBuf[0] == '\0')
    {
        printf("No command line params specified\n");
        return FALSE;
    }

    cmdlineMode = 1;

    if (CmdLineParameterExist(CMDLINE_SAVE_OPTIONS))
    {
        printf("Command Line: Save mode on\n");
        cmdlineSave = 1;
    }

    //Plugins
    Config.selected_video_plugin = setPluginFromCmndLine(CMDLINE_VIDEO_PLUGIN, plugin_type::video);
    Config.selected_audio_plugin = setPluginFromCmndLine(CMDLINE_AUDIO_PLUGIN, plugin_type::audio);
    Config.selected_input_plugin = setPluginFromCmndLine(CMDLINE_CONTROLLER_PLUGIN, plugin_type::input);
    Config.selected_rsp_plugin = setPluginFromCmndLine(CMDLINE_RSP_PLUGIN, plugin_type::rsp);

    if (!CmdLineParameterExist(CMDLINE_GAME_FILENAME))
    {
        printf("Command Line: Rom name not specified\n");
        return FALSE;
    }
    else
    {
        GetCmdLineParameter(CMDLINE_GAME_FILENAME, szFileName);
        printf("Command Line: Rom Name :%s\n", szFileName);
    }

    strcpy(rom_path, szFileName);
    if (start_rom(nullptr))
    {
        return TRUE;
    }
    else
    {
        printf("Command Line: Rom not found\n");
        return FALSE;
    }
}

BOOL GuiDisabled()
{
    cmdlineNoGui = CmdLineParameterExist(CMDLINE_NO_GUI);
    return cmdlineNoGui;
}

BOOL CmdLineParameterExist(CmdLineParameterType param)
{
    char tempStr[MAX_PATH];
    GetCmdLineParameter(param, tempStr);
    if (tempStr[0] == '\0')
    {
        return FALSE;
    }
    return TRUE;
}
