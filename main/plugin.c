/**
 * Mupen64 - plugin.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

inline char* realpath(const char* pathname, char* resolvedname) {
	return _fullpath(resolvedname, pathname, _MAX_PATH);
}

#include "plugin.h"
#include "rom.h"
#include "../memory/memory.h"
#include "../r4300/interupt.h"
#include "../r4300/r4300.h"


#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif

CONTROL Controls[4];

static GFX_INFO gfx_info;
AUDIO_INFO audio_info;
static CONTROL_INFO control_info;
static RSP_INFO rsp_info;


void(__cdecl* getDllInfo)(PLUGIN_INFO* PluginInfo);
void(__cdecl* dllConfig)(HWND hParent);
void(__cdecl* dllTest)(HWND hParent);
void(__cdecl* dllAbout)(HWND hParent);

void(__cdecl* changeWindow)() = dummy_void;
void(__cdecl* closeDLL_gfx)() = dummy_void;
BOOL(__cdecl* initiateGFX)(GFX_INFO Gfx_Info) = dummy_initiateGFX;
void(__cdecl* processDList)() = dummy_void;
void(__cdecl* processRDPList)() = dummy_void;
void(__cdecl* romClosed_gfx)() = dummy_void;
void(__cdecl* romOpen_gfx)() = dummy_void;
void(__cdecl* showCFB)() = dummy_void;
void(__cdecl* updateScreen)() = dummy_void;
void(__cdecl* viStatusChanged)() = dummy_void;
void(__cdecl* viWidthChanged)() = dummy_void;
void(__cdecl* readScreen)(void** dest, long* width, long* height) = 0;
void(__cdecl* DllCrtFree)(void* block);

void(__cdecl* aiDacrateChanged)(int SystemType) = dummy_aiDacrateChanged;
void(__cdecl* aiLenChanged)() = dummy_void;
DWORD(__cdecl* aiReadLength)() = dummy_aiReadLength;
//void (__cdecl*aiUpdate)(BOOL Wait) = dummy_aiUpdate;
void(__cdecl* closeDLL_audio)() = dummy_void;
BOOL(__cdecl* initiateAudio)(AUDIO_INFO Audio_Info) = dummy_initiateAudio;
void(__cdecl* processAList)() = dummy_void;
void(__cdecl* romClosed_audio)() = dummy_void;
void(__cdecl* romOpen_audio)() = dummy_void;

void(__cdecl* closeDLL_input)() = dummy_void;
void(__cdecl* controllerCommand)(int Control, BYTE* Command) = dummy_controllerCommand;
void(__cdecl* getKeys)(int Control, BUTTONS* Keys) = dummy_getKeys;
void(__cdecl* setKeys)(int Control, BUTTONS Keys) = dummy_setKeys;
void(__cdecl* initiateControllers)(CONTROL_INFO ControlInfo) = dummy_initiateControllers;
void(__cdecl* readController)(int Control, BYTE* Command) = dummy_readController;
void(__cdecl* romClosed_input)() = dummy_void;
void(__cdecl* romOpen_input)() = dummy_void;
void(__cdecl* keyDown)(WPARAM wParam, LPARAM lParam) = dummy_keyDown;
void(__cdecl* keyUp)(WPARAM wParam, LPARAM lParam) = dummy_keyUp;

void(__cdecl* closeDLL_RSP)() = dummy_void;
DWORD(__cdecl* doRspCycles)(DWORD Cycles) = dummy_doRspCycles;
void(__cdecl* initiateRSP)(RSP_INFO Rsp_Info, DWORD* CycleCount) = dummy_initiateRSP;
void(__cdecl* romClosed_RSP)() = dummy_void;

void(__cdecl* fBRead)(DWORD addr) = dummy_fBRead;
void(__cdecl* fBWrite)(DWORD addr, DWORD size) = dummy_fBWrite;
void(__cdecl* fBGetFrameBufferInfo)(void* p) = dummy_fBGetFrameBufferInfo;

//--------------------- plugin storage type ----------------
typedef struct _plugins plugins;
struct _plugins {
	char* file_name;
	char* plugin_name;
	void* handle;
	int type;
	plugins* next;
};
static plugins* liste_plugins = NULL, * current;

static void insert_plugin(plugins* p, const char* file_name,
	const char* plugin_name, void* handle, int type, int num) {
	if (p->next)
		insert_plugin(p->next, file_name, plugin_name, handle, type,
			(p->type == type) ? num + 1 : num);
	else {
		p->next = (plugins*)malloc(sizeof(plugins));
		p->next->type = type;
		p->next->handle = handle;
		p->next->file_name = (char*)malloc(strlen(file_name) + 1);
		strcpy(p->next->file_name, file_name);
		p->next->plugin_name = (char*)malloc(strlen(plugin_name) + 7);
		sprintf(p->next->plugin_name, "%d - %s",
			num + ((p->type == type) ? 2 : 1), plugin_name);
		p->next->next = NULL;
	}
}

void plugin_rewind() {
	current = liste_plugins;
}

char* plugin_next() {
	if (!current->next) return NULL;
	current = current->next;
	return current->plugin_name;
}

int plugin_type() {
	if (!current->next) return -1;
	return current->next->type;
}

static void* get_handle(plugins* p, const char* name) {
	if (!p) return NULL;
	if (!p->next) return NULL;
	if (!strcmp(p->next->plugin_name, name))
		return p->next->handle;
	return get_handle(p->next, name);
}

char* plugin_filename_by_name(const char* name) {
	plugins* p;

	if (!liste_plugins)
		return NULL;

	p = liste_plugins->next;

	while (p) {
		if (!strcmp(name, p->plugin_name))
			return p->file_name;
		p = p->next;
	}
	return NULL;
}

char* plugin_name_by_filename(const char* filename) {
	plugins* p;
	char real_filename1[PATH_MAX], real_filename2[PATH_MAX];

	if (!liste_plugins)
		return NULL;

	p = liste_plugins->next;

	while (p) {
		if (!realpath(filename, real_filename1))
			strcpy(real_filename1, filename);

		if (!realpath(p->file_name, real_filename2))
			strcpy(real_filename2, p->file_name);

		if (!strcmp(real_filename1, real_filename2))
			return p->plugin_name;

		p = p->next;
	}
	return NULL;
}
