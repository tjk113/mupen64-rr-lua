/**
 * Mupen64 - rom.c
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

/* This is the functions that load a rom into memory, it loads a roms
 * in multiple formats, gzipped or not. It searches the rom, in the roms
 * subdirectory or in the path specified in the path.cfg file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <string.h>
#include <ctype.h>

#include "rom.h"
#include "../memory/memory.h"
#include "unzip.h"
#include "guifuncs.h"
#include "md5.h"
#include "mupenIniApi.h"
#include "guifuncs.h"
#include "../main/win/Config.hpp"
#include <win\RomBrowser.hpp>
#include <win\RomSettings.h>
#include <win/main_win.h>

static FILE* rom_file;
static gzFile z_rom_file;
static unzFile zip;
static unz_file_info pfile_info;
static int i, tmp, z;

int romByteCount;
unsigned char* rom;
t_rom_header* ROM_HEADER;
rom_settings ROM_SETTINGS;

// updates global rom size variable and returns the size in megabits
static int findsize()
{
    if (!z)
    {
        fseek(rom_file, 0L, SEEK_END);
        romByteCount = ftell(rom_file);
        printf("rom size: %d bytes (or %d Mb or %d Megabits)\n",
               romByteCount, romByteCount / 1024 / 1024, romByteCount / 1024 / 1024 * 8);
        fseek(rom_file, 0L, SEEK_SET);
    }
    else if (z == 1)
    {
        romByteCount = 0;
        rom = (unsigned char*)malloc(100000);
        for (;;)
        {
            i = gzread(z_rom_file, rom, 100000);
            romByteCount += i;
            printf("rom size: %d bytes (or %d Mb or %d Megabits)\n",
                   romByteCount, romByteCount / 1024 / 1024, romByteCount / 1024 / 1024 * 8);
            if (!i) break;
        }
        free(rom);
        rom = NULL;
        gzseek(z_rom_file, 0L, SEEK_SET);
    }
    return romByteCount / 1024 / 1024 * 8;
    // divide through 131072 works too but im sure compiler is smart enough
}

const char* getExt(const char* filename)
{
    const char* dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

void stripExt(char* fname)
{
    char* end = fname + strlen(fname);

    while (end > fname && *end != '.' && *end != '\\' && *end != '/')
    {
        --end;
    }
    if ((end > fname && *end == '.') &&
        (*(end - 1) != '\\' && *(end - 1) != '/'))
    {
        *end = '\0';
    }
}

bool validRomExt(const char* filename)
{
    //#ifdef _DEBUG
    //	printf("%s\n", filename);
    //#endif
    const char* str = getExt(filename);
    if (str == "\0" || str == "0") return 0;
    // z64,n64,v64,rom
    return !_stricmp(str, "z64") ||
        !_stricmp(str, "n64") ||
        !_stricmp(str, "v64") ||
        !_stricmp(str, "rom");
}

std::string country_code_to_country_name(int country_code)
{
    switch (country_code & 0xFF)
    {
    case 0:
        return "Demo";
    case '7':
        return "Beta";
    case 0x41:
        return "USA/Japan";
    case 0x44:
        return "Germany";
    case 0x45:
        return "USA";
    case 0x46:
        return "France";
     case 'I':
        return "Italy";
    case 0x4A:
        return "Japan";
    case 'S':
        return "Spain";
    case 0x55:
    case 0x59:
        return "Australia";
    case 0x50:
    case 0x58:
    case 0x20:
    case 0x21:
    case 0x38:
    case 0x70:
        return "Europe";
    default:
        return "Unknown (" + std::to_string(country_code & 0xFF) + ")";
    }
}

static int find_file(char* argv)
{
    z = 0;
    i = (int)strlen(argv);
    {
        unsigned char buf[4];
        char szFileName[255], extraField[255], szComment[255];
        zip = unzOpen(argv);
        if (zip != NULL)
        {
            unzGoToFirstFile(zip);
            do
            {
                unzGetCurrentFileInfo(zip, &pfile_info, szFileName, 255,
                                      extraField, 255, szComment, 255);

                if (!validRomExt(szFileName))
                {
                    printf("skipping zipped invalid extension file %s\n", szFileName);
                    continue;
                } // is this how to do?

                unzOpenCurrentFile(zip);
                if (pfile_info.uncompressed_size >= 4)
                {
                    unzReadCurrentFile(zip, buf, 4);
                    if ((*reinterpret_cast<unsigned long*>(buf) != 0x40123780) &&
                        (*reinterpret_cast<unsigned long*>(buf) != 0x12408037) &&
                        (*reinterpret_cast<unsigned long*>(buf) != 0x80371240))
                    {
                        unzCloseCurrentFile(zip);
                    }
                    else
                    {
                        romByteCount = (int)pfile_info.uncompressed_size;
                        unzCloseCurrentFile(zip);
                        z = 2;
                        return 0;
                    }
                }
            }
            while (unzGoToNextFile(zip) != UNZ_END_OF_LIST_OF_FILE);
            unzClose(zip);
            return 1;
        }
    }
    if ((i > 3) && (argv[i - 3] == '.') &&
        (tolower(argv[i - 2]) == 'g') && (tolower(argv[i - 1]) == 'z'))
        argv[i - 3] = 0;
    rom_file = NULL;
    z_rom_file = NULL;
    rom_file = fopen(argv, "rb");
    if (rom_file == NULL)
    {
        z_rom_file = gzopen(strcat(argv, ".gz"), "rb");
        if (z_rom_file == NULL)
        {
            argv[i - 3] = 0;
            z_rom_file = gzopen(strcat(argv, ".GZ"), "rb");
            if (z_rom_file == NULL) return 1;
        }
        z = 1;
    }
    return 0;
}

int rom_read(const char* argv)
{
    md5_state_t state;
    md5_byte_t digest[16];
    mupenEntry* entry;
    bool invalidSize;
    char buf[1024], arg[1024], *s;

    strncpy(arg, argv, 1000);

    if (!Config.allow_suspicious_rom_loading && !validRomExt(argv))
        goto killRom;

    if (find_file(arg))
    {
        strncpy(arg, "roms/", 1000);
        if (find_file(strncat(arg, argv, 1000)))
        {
            rom_file = fopen("path.cfg", "rb");
            if (rom_file) fscanf(rom_file, "%1000s", buf);
            else buf[0] = 0;
            if (rom_file) fclose(rom_file);
            strncpy(arg, argv, 1000);
            if (find_file(strcat(buf, arg)))
            {
                printf("file not found or wrong path\n");
                return 1;
            }
        }
    }


    printf("file found\n");
    /*------------------------------------------------------------------------*/
    findsize(); // findsize() needs to be called first because it has side effects
    // rom sizes that make sense are 1kB (at least boot code) to 64Mb (extended roms)
    invalidSize = (romByteCount > 0x400'0000 || romByteCount < 0x1000);
    if (invalidSize && !Config.allow_suspicious_rom_loading) goto killRom;

    if (rom) free(rom);
    rom = (unsigned char*)malloc(romByteCount);

    tmp = 0;
    if (!z)
    {
        for (i = 0; i < romByteCount; i += (int)fread(rom + i, 1, 1000, rom_file))
        {
            if (tmp != (int)(((float)i / (float)romByteCount) * 100))
            {
                tmp = (int)((float)i / (float)(romByteCount) * 100);
            }
        }
    }
    else if (z == 1)
    {
        for (i = 0; i < romByteCount; i += gzread(z_rom_file, rom + i, 1000))
        {
            if (tmp != (int)(((float)i / (float)romByteCount) * 100))
            {
                tmp = (int)((float)i / (float)(romByteCount) * 100);
            }
        }
    }
    else
    {
        unzOpenCurrentFile(zip);
        for (i = 0; i < romByteCount; i += unzReadCurrentFile(zip, rom + i, 1000))
        {
            if (tmp != (int)(((float)i / (float)romByteCount) * 100))
            {
                tmp = (int)((float)i / (float)(romByteCount) * 100);
            }
        }
        unzCloseCurrentFile(zip);
    }
    if (!z) fclose(rom_file);
    else if (z == 1) gzclose(z_rom_file);
    else unzClose(zip);

    if (rom[0] == 0x37)
    {
        printf("byteswaping rom...\n");
        for (i = 0; i < (romByteCount / 2); i++)
        {
            tmp = rom[i * 2];
            rom[i * 2] = rom[i * 2 + 1];
            rom[i * 2 + 1] = (unsigned char)tmp;
        }
        printf("rom byteswaped\n");
    }
    if (rom[0] == 0x40)
    {
        for (i = 0; i < (romByteCount / 4); i++)
        {
            tmp = rom[i * 4];
            rom[i * 4] = rom[i * 4 + 3];
            rom[i * 4 + 3] = (unsigned char)tmp;
            tmp = rom[i * 4 + 1];
            rom[i * 4 + 1] = rom[i * 4 + 2];
            rom[i * 4 + 2] = (unsigned char)tmp;
        }
        printf("rom byteswaped\n");
    }
    else if ((rom[0] != 0x80)
        || (rom[1] != 0x37)
        || (rom[2] != 0x12)
        || (rom[3] != 0x40)

    )
    {
        printf("wrong file format !\n");
        free(rom);
        rom = NULL;
        return 1;
    }
    printf("rom loaded succesfully\n");

    if (!ROM_HEADER) ROM_HEADER = (t_rom_header*)malloc(sizeof(t_rom_header));
    memcpy(ROM_HEADER, rom, sizeof(t_rom_header));
    ROM_HEADER->unknown = 0;
    // Clean up ROMs that accidentally set the unused bytes (ensuring previous fields are null terminated)
    ROM_HEADER->Unknown[0] = 0;
    ROM_HEADER->Unknown[1] = 0;
    printf("%x %x %x %x\n", ROM_HEADER->init_PI_BSB_DOM1_LAT_REG,
           ROM_HEADER->init_PI_BSB_DOM1_PGS_REG,
           ROM_HEADER->init_PI_BSB_DOM1_PWD_REG,
           ROM_HEADER->init_PI_BSB_DOM1_PGS_REG2);
    printf("ClockRate=%x\n", sl((unsigned int)ROM_HEADER->ClockRate));
    printf("Version:%x\n", sl((unsigned int)ROM_HEADER->Release));
    printf("CRC: %x %x\n", sl((unsigned int)ROM_HEADER->CRC1), sl((unsigned int)ROM_HEADER->CRC2));
    //trim header
    memcpy(ROM_HEADER->nom, trim((char*)ROM_HEADER->nom), sizeof(ROM_HEADER->nom));
    printf("name: %s\n", (char*)ROM_HEADER->nom);


    if (sl(ROM_HEADER->Manufacturer_ID) == 'N') printf("Manufacturer: Nintendo\n");
    else printf("Manufacturer: %x\n", (unsigned int)(ROM_HEADER->Manufacturer_ID));
    printf("Cartridge_ID: %x\n", ROM_HEADER->Cartridge_ID);
    switch (ROM_HEADER->Country_code)
    {
    case 0x0044:
        printf("Country : Germany\n");
        break;
    case 0x0045:
        printf("Country : United States\n");
        break;
    case 0x004A:
        printf("Country : Japan\n");
        break;
    case 0x0050:
        printf("European cartridge\n");
        break;
    case 0x0055:
        printf("Country : Australia\n");
    default:
        printf("Country Code : %x\n", ROM_HEADER->Country_code);
    }
    printf("size: %d\n", (unsigned int)(sizeof(t_rom_header)));
    printf("PC= %x\n", sl((unsigned int)ROM_HEADER->PC));

    // loading rom settings and checking if it's a good dump
    md5_init(&state);
    md5_append(&state, (const md5_byte_t*)rom, romByteCount);
    md5_finish(&state, digest);
    printf("md5 code:");
    for (i = 0; i < 16; i++) printf("%02X", digest[i]);
    printf("\n");

    ini_openFile();

    for (i = 0; i < 16; i++) sprintf(arg + i * 2, "%02X", digest[i]);
    arg[32] = 0;
    strcpy(ROM_SETTINGS.MD5, arg);
    if ((entry = ini_search_by_md5(arg)) == NULL)
    {
        char mycrc[1024];
        printf("%x\n", (int)entry);
        sprintf(mycrc, "%08X-%08X-C%02X",
                (int)sl(ROM_HEADER->CRC1), (int)sl(ROM_HEADER->CRC2),
                ROM_HEADER->Country_code);
        if ((entry = ini_search_by_CRC(mycrc)) == NULL)
        {
            strcpy(ROM_SETTINGS.goodname, (const char*)ROM_HEADER->nom);
            strcat(ROM_SETTINGS.goodname, " (unknown rom)");
            printf("%s\n", ROM_SETTINGS.goodname);
            ROM_SETTINGS.eeprom_16kb = 0;
            return 0;
        }
        else
        {
            if (!Config.allow_suspicious_rom_loading)
            {
                free(rom);
                rom = NULL;
                free(ROM_HEADER);
                ROM_HEADER = NULL;
                return 1;
            }
            strcpy(ROM_SETTINGS.goodname, entry->goodname);
            strcat(ROM_SETTINGS.goodname, " (bad dump)");
            if (strcmp(entry->refMD5, "") != 0)
                entry = ini_search_by_md5(entry->refMD5);
            ROM_SETTINGS.eeprom_16kb = entry->eeprom16kb;
            return 0;
        }
    }
    s = entry->goodname;
    for (i = (int)strlen(s); i > 0 && s[i - 1] != '['; i--)
        if (i != 0)
        {
            if (s[i] == 'T' || s[i] == 't' || s[i] == 'h' || s[i] == 'f' || s[i] == 'o')
            {
                if (!Config.allow_suspicious_rom_loading)
                {
                killRom:
                    free(rom);
                    rom = NULL;
                    free(ROM_HEADER);
                    ROM_HEADER = NULL;
                    return 1;
                }
            }
            if (s[i] == 'b')
            {
                if (!Config.allow_suspicious_rom_loading)
                {
                    free(rom);
                    rom = NULL;
                    free(ROM_HEADER);
                    ROM_HEADER = NULL;
                    return 1;
                }
            }
        }
    strcpy(ROM_SETTINGS.goodname, entry->goodname);

    if (strcmp(entry->refMD5, "") != 0)
        entry = ini_search_by_md5(entry->refMD5);
    ROM_SETTINGS.eeprom_16kb = entry->eeprom16kb;
    printf("eeprom type:%d\n", ROM_SETTINGS.eeprom_16kb);
    return 0;
}

int fill_header(const char* argv)
{
    char arg[1024];
    strncpy(arg, argv, 1000);
    if (find_file(arg)/* || !validRomExt(argv) && Config.alertBAD */)
    {
        printf("file not found or wrong path\n");
        return 0;
    }
    /*------------------------------------------------------------------------*/
    findsize();
    if (rom) free(rom);
    rom = (unsigned char*)malloc(0x40);

    tmp = 0;

    if (!z)
        fread(rom, 1, 0x40, rom_file);
    else if (z == 1)
        gzread(z_rom_file, rom, 0x40);
    else
    {
        unzOpenCurrentFile(zip);
        unzReadCurrentFile(zip, rom, 0x40);
        unzCloseCurrentFile(zip);
    }
    if (!z) fclose(rom_file);
    else if (z == 1) gzclose(z_rom_file);
    else unzClose(zip);

    if ((rom[0] != 0x80) || (rom[1] != 0x37) || (rom[2] != 0x12) || (rom[3] != 0x40))
    {
        free(rom);
        rom = NULL;
        return 0;
    }
    rom_byteswap(rom);

    if (ROM_HEADER == NULL) ROM_HEADER = (t_rom_header*)malloc(sizeof(t_rom_header));
    memcpy(ROM_HEADER, rom, 0x40);
    free(rom);
    rom = NULL;
    return romByteCount;
}

void calculateMD5(const char* argv, unsigned char digest[16])
{
    md5_state_t state;
    char arg[1024];

    strncpy(arg, argv, 1000);
    if (find_file(arg))
    {
        printf("file not found or wrong path\n");
        return;
    }
    /*------------------------------------------------------------------------*/
    findsize();
    if (rom) free(rom);
    rom = (unsigned char*)malloc(romByteCount);

    tmp = 0;
    if (!z)
    {
        for (i = 0; i < romByteCount; i += (int)fread(rom + i, 1, 1000, rom_file))
        {
            if (tmp != (int)(((float)i / (float)romByteCount) * 100))
            {
                tmp = (int)((float)i / (float)(romByteCount) * 100);
            }
        }
    }
    else if (z == 1)
    {
        for (i = 0; i < romByteCount; i += gzread(z_rom_file, rom + i, 1000))
        {
            if (tmp != (int)((i / (float)romByteCount) * 100))
            {
                tmp = (int)(i / (float)(romByteCount) * 100);
            }
        }
    }
    else
    {
        unzOpenCurrentFile(zip);
        for (i = 0; i < romByteCount; i += unzReadCurrentFile(zip, rom + i, 1000))
        {
            if (tmp != (int)((i / (float)romByteCount) * 100))
            {
                tmp = (int)(i / (float)(romByteCount) * 100);
            }
        }
        unzCloseCurrentFile(zip);
    }
    if (!z) fclose(rom_file);
    else if (z == 1) gzclose(z_rom_file);
    else unzClose(zip);

    if (rom[0] == 0x37)
    {
        printf("byteswaping rom...\n");
        for (i = 0; i < (romByteCount / 2); i++)
        {
            tmp = rom[i * 2];
            rom[i * 2] = rom[i * 2 + 1];
            rom[i * 2 + 1] = (unsigned char)tmp;
        }
        printf("rom byteswaped\n");
    }
    if (rom[0] == 0x40)
    {
        for (i = 0; i < (romByteCount / 4); i++)
        {
            tmp = rom[i * 4];
            rom[i * 4] = rom[i * 4 + 3];
            rom[i * 4 + 3] = (unsigned char)tmp;
            tmp = rom[i * 4 + 1];
            rom[i * 4 + 1] = rom[i * 4 + 2];
            rom[i * 4 + 2] = (unsigned char)tmp;
        }
        printf("rom byteswaped\n");
    }
    else if ((rom[0] != 0x80) || (rom[1] != 0x37) || (rom[2] != 0x12) || (rom[3] != 0x40))
    {
        printf("wrong file format !\n");
        free(rom);
        rom = NULL;
        return;
    }
    md5_init(&state);
    md5_append(&state, (const md5_byte_t*)rom, romByteCount);
    md5_finish(&state, digest);
    free(rom);
    rom = NULL;
}
