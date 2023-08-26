/***************************************************************************
						  rombrowser.h  -  description
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

#ifndef ROMBROWSER_H
#define ROMBROWSER_H

#include <windows.h>
#include <commctrl.h>
#include "../md5.h"

#define MAX_RECENT_ROMS 10

typedef struct s_rom_header
{
	char sz_full_file_name[MAX_PATH];
	char status[60];
	char file_name[200];
	char internal_name[22];
	char good_name[200];
	char cart_id[3];
	char plugin_notes[250];
	char core_notes[250];
	char user_notes[250];
	char developer[30];
	char release_date[30];
	char genre[15];
	int rom_size;
	BYTE manufacturer;
	BYTE country;
	DWORD crc1;
	DWORD crc2;
	char md5[33];
} t_rom_header;


#endif
