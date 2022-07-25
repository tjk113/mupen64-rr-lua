/***************************************************************************
                          config.h  -  description
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

void WriteCfgString   (const char *Section, const char *Key,char *Value) ;
void WriteCfgInt      (const char *Section, const char *Key,int Value) ;
void ReadCfgString    (const char *Section, const char *Key, const char *DefaultValue,char *retValue) ;
int ReadCfgInt        (const char *Section, const char *Key,int DefaultValue) ;

extern int round_to_zero;
extern int emulate_float_crashes;
extern int input_delay;
extern int LUA_double_buffered; 

void LoadConfig()  ;
void SaveConfig()  ; 
    
