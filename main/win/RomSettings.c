/***************************************************************************
						  RomSettings.c  -  description
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

#include <windows.h>
#include <ctype.h>
#include "main_win.h"
#include "RomSettings.h"

char* trim(char* str)
{
    unsigned char *ibuf, *obuf;

    if (str)
    {
        for (ibuf = obuf = (unsigned char*)str; *ibuf;)
        {
            while (*ibuf && (isspace(*ibuf)))
                ibuf++;
            if (*ibuf && (obuf != (unsigned char*)str))
                *(obuf++) = ' ';
            while (*ibuf && (!isspace(*ibuf)))
                *(obuf++) = *(ibuf++);
        }
        *obuf = 0;
    }
    return (char*)(str);
}
