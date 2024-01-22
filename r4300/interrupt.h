/**
 * Mupen64 - interrupt.h
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
#ifndef INTERRUPT_H
#define INTERRUPT_H

void compare_interrupt();
void gen_dp();
void init_interrupt();

extern int vi_field;
extern unsigned long next_vi;

void gen_interrupt();
void check_interrupt();

void translate_event_queue(unsigned long base);
void remove_event(int type);
void add_interrupt_event_count(int type, unsigned long count);
void add_interrupt_event(int type, unsigned long delay);
unsigned long get_event(int type);

int save_eventqueue_infos(char* buf);
void load_eventqueue_infos(char* buf);

enum {
	VI_INT = 0x001,
	COMPARE_INT = 0x002,
	CHECK_INT = 0x004,
	SI_INT = 0x008,
	PI_INT = 0x010,
	SPECIAL_INT = 0x020,
	AI_INT = 0x040,
	SP_INT = 0x080,
	DP_INT = 0x100
};

#endif // INTERRUPT_H
