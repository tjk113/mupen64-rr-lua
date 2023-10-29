/**
 * Mupen64 - rom.h
 * Copyright (C) 2002 Hacktarux
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 *
 * If you want to contribute to the project please contact
 * me first (maybe someone is already making what you are
 * planning to do).
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
 * You should have received a copy of the GNU General Public
 * Licence along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
**/
#include <string>

#ifndef ROM_H
#define ROM_H

extern uint8_t* rom;
extern size_t rom_size;
extern uint16_t rom_md5;

int rom_read(const char* argv);
bool is_case_insensitive_equal(const std::string& a, const std::string& b);
void strip_extension(char* fname);

typedef struct s_rom_header
{
	unsigned char init_PI_BSB_DOM1_LAT_REG;
	unsigned char init_PI_BSB_DOM1_PGS_REG;
	unsigned char init_PI_BSB_DOM1_PWD_REG;
	unsigned char init_PI_BSB_DOM1_PGS_REG2;
	unsigned long ClockRate;
	unsigned long PC;
	unsigned long Release;
	unsigned long CRC1;
	unsigned long CRC2;
	unsigned long Unknown[2];
	unsigned char nom[20];
	unsigned long unknown;
	unsigned long Manufacturer_ID;
	unsigned short Cartridge_ID;
	unsigned short Country_code;
	unsigned long Boot_Code[1008];
} t_rom_header;

extern t_rom_header ROM_HEADER;

std::string country_code_to_country_name(int country_code);

inline static void rom_byteswap(uint8_t* rom)
{
	uint8_t tmp = 0;

	if (rom[0] == 0x37)
	{
		for (size_t i = 0; i < (0x40 / 2); i++)
		{
			tmp = rom[i * 2];
			rom[i * 2] = rom[i * 2 + 1];
			rom[i * 2 + 1] = tmp;
		}
	}
	if (rom[0] == 0x40)
	{
		for (size_t i = 0; i < (0x40 / 4); i++)
		{
			tmp = rom[i * 4];
			rom[i * 4] = rom[i * 4 + 3];
			rom[i * 4 + 3] = tmp;
			tmp = rom[i * 4 + 1];
			rom[i * 4 + 1] = rom[i * 4 + 2];
			rom[i * 4 + 2] = tmp;
		}
	}
}

inline static char* trim(char* str)
{
	char *ibuf, *obuf;

	if (str)
	{
		for (ibuf = obuf = str; *ibuf; )
		{
			while (*ibuf && (isspace (*ibuf)))
				ibuf++;
			if (*ibuf && (obuf != str))
				*(obuf++) = ' ';
			while (*ibuf && (!isspace (*ibuf)))
				*(obuf++) = *(ibuf++);
		}
		*obuf = 0;
	}
	return str;
}

#endif
