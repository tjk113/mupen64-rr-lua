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
#include <Windows.h>
#include "commandline.h"

#include "main_win.h"
#include "helpers/string_helpers.h"
#include "lib/argh.h"

std::string commandline_rom;

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

void commandline_set()
{
	argh::parser cmdl(__argv);

	// first arg is always executable
	// if only that is present, we dont have any options
	if (cmdl.size() == 1)
	{
		return;
	}


	// handle "Open With...":
	if (cmdl.size() == 2)
	{
		commandline_rom = cmdl[1];
		return;
	}

	// TODO: add back old ones
}

void commandline_load_rom()
{
	if (commandline_rom.empty())
	{
		return;
	}

	strcpy(rom_path, commandline_rom.c_str());
	CreateThread(NULL, 0, start_rom, nullptr, 0, &start_rom_id);
}
