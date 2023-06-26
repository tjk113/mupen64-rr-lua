/***************************************************************************
						  main_win.cpp  -  description
							 -------------------
	copyright C) 2003    : ShadowPrince (shadow@emulation64.com)
	modifications        : linker (linker@mail.bg)
	mupen64 author       : hacktarux (hacktarux@yahoo.fr)
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "LuaConsole.h"

#include "Recent.h"
#include "win/DebugInfo.hpp"
#include "ffmpeg_capture/ffmpeg_capture.hpp"

#if defined(__cplusplus) && !defined(_MSC_VER)
extern "C" {
#endif

#include <windows.h> // TODO: Include Windows.h not windows.h and see if it breaks
#include <Shlwapi.h>
#ifndef _WIN32_IE
#define _WIN32_IE 0x0500
#endif
#include <commctrl.h>
#include <stdlib.h>
#include <math.h>
#include <filesystem>
#ifndef _MSC_VER
#include <dirent.h>
#endif
#include "../../winproject/resource.h"
#include "../plugin.h"
#include "../rom.h"
#include "../../r4300/r4300.h"
#include "../../memory/memory.h"
#include "translation.h"
#include "rombrowser.h"
#include "main_win.h"
#include "configdialog.h"
#include "../guifuncs.h"
#include "../mupenIniApi.h"
#include "../savestates.h"
#include "dumplist.h"
#include "timers.h"
#include "config.h"
#include "RomSettings.h"
#include "GUI_logwindow.h"
#include "commandline.h"
#include "CrashHandlerDialog.h"
#include "CrashHelper.h"
#include "wrapper/ReassociatingFileDialog.h"
#include "../vcr.h"
#include "../../r4300/recomph.h"

#define EMULATOR_MAIN_CPP_DEF
#include "kaillera.h"
#include "../../memory/pif.h"
#undef EMULATOR_MAIN_CPP_DEF

#include <gdiplus.h>
#include "../main/win/GameDebugger.h"

#pragma comment (lib,"Gdiplus.lib")

	extern void CountryCodeToCountryName(int countrycode, char* countryname);

	void StartMovies();
	void StartLuaScripts();
	void StartSavestate();

	typedef std::string String;
	int shouldSave = FALSE;

	bool ffup = false;
	BOOL forceIgnoreRSP = false;

#if defined(__cplusplus) && !defined(_MSC_VER)
}
#endif



#ifdef _MSC_VER
#define snprintf	_snprintf
#define strcasecmp	stricmp
#define strncasecmp	strnicmp
#endif


static DWORD Id;
static DWORD SOUNDTHREADID;
static HANDLE SoundThreadHandle;
static BOOL FullScreenMode = 0;
DWORD WINAPI closeRom(LPVOID lpParam);

HANDLE EmuThreadHandle;
HWND hwnd_plug;
//int manualFPSLimit = 1 ;
static void gui_ChangeWindow();

static GFX_INFO dummy_gfx_info;
static GFX_INFO gfx_info;
static CONTROL_INFO dummy_control_info;
static CONTROL_INFO control_info;
static AUDIO_INFO dummy_audio_info;
static AUDIO_INFO audio_info;
static RSP_INFO dummy_rsp_info;
static RSP_INFO rsp_info;
static int currentSaveState = 1;
static unsigned char DummyHeader[0x40];


void(__cdecl* moveScreen)(int xpos, int ypos);
void(__cdecl* CaptureScreen) (char* Directory);
void(__cdecl* old_initiateControllers)(HWND hMainWindow, CONTROL Controls[4]);
void(__cdecl* aiUpdate)(BOOL Wait);


#ifndef _MSC_VER

CONTROL Controls[4];

void(__cdecl* getDllInfo)(PLUGIN_INFO* PluginInfo);
void(__cdecl* dllConfig)(HWND hParent);
void(__cdecl* dllTest)(HWND hParent);
void(__cdecl* dllAbout)(HWND hParent);

void(__cdecl* changeWindow)();
void(__cdecl* closeDLL_gfx)();
BOOL(__cdecl* initiateGFX)(GFX_INFO Gfx_Info);
void(__cdecl* processDList)();
void(__cdecl* processRDPList)();
void(__cdecl* romClosed_gfx)();
void(__cdecl* romOpen_gfx)();
void(__cdecl* showCFB)();
void(__cdecl* updateScreen)();
void(__cdecl* viStatusChanged)();
void(__cdecl* viWidthChanged)();


void(__cdecl* closeDLL_input)();
void(__cdecl* controllerCommand)(int Control, BYTE* Command);
void(__cdecl* getKeys)(int Control, BUTTONS* Keys);
void(__cdecl* setKeys)(int Control, BUTTONS  Keys);
void(__cdecl* initiateControllers)(CONTROL_INFO ControlInfo);
void(__cdecl* readController)(int Control, BYTE* Command);
void(__cdecl* romClosed_input)();
void(__cdecl* romOpen_input)();
void(__cdecl* keyDown)(WPARAM wParam, LPARAM lParam);
void(__cdecl* keyUp)(WPARAM wParam, LPARAM lParam);


void(__cdecl* aiDacrateChanged)(int SystemType);
void(__cdecl* aiLenChanged)();
DWORD(__cdecl* aiReadLength)();
void(__cdecl* closeDLL_audio)();
BOOL(__cdecl* initiateAudio)(AUDIO_INFO Audio_Info);
void(__cdecl* processAList)();
void(__cdecl* romClosed_audio)();
void(__cdecl* romOpen_audio)();

void(__cdecl* closeDLL_RSP)();
DWORD(__cdecl* doRspCycles)(DWORD Cycles);
void(__cdecl* initiateRSP)(RSP_INFO Rsp_Info, DWORD* CycleCount);
void(__cdecl* romClosed_RSP)();

void(__cdecl* fBRead)(DWORD addr);
void(__cdecl* fBWrite)(DWORD addr, DWORD size);
void(__cdecl* fBGetFrameBufferInfo)(void* p);
extern void(__cdecl* readScreen)(void** dest, long* width, long* height);

#endif // !_MSC_VER

/* dummy functions to prevent mupen from crashing if a plugin is missing */
static void __cdecl dummy_void() {}
static BOOL __cdecl dummy_initiateGFX(GFX_INFO Gfx_Info) { return TRUE; }
static BOOL __cdecl dummy_initiateAudio(AUDIO_INFO Audio_Info) { return TRUE; }
static void __cdecl dummy_initiateControllers(CONTROL_INFO Control_Info) {}
static void __cdecl dummy_aiDacrateChanged(int SystemType) {}
static DWORD __cdecl dummy_aiReadLength() { return 0; }
//static void dummy_aiUpdate(BOOL Wait) {}
static void __cdecl dummy_controllerCommand(int Control, BYTE* Command) {}
static void __cdecl dummy_getKeys(int Control, BUTTONS* Keys) {}
static void __cdecl dummy_setKeys(int Control, BUTTONS  Keys) {}
static void __cdecl dummy_readController(int Control, BYTE* Command) {}
static void __cdecl dummy_keyDown(WPARAM wParam, LPARAM lParam) {}
static void __cdecl dummy_keyUp(WPARAM wParam, LPARAM lParam) {}
static unsigned long dummy;
static DWORD __cdecl dummy_doRspCycles(DWORD Cycles) { return Cycles; };
static void __cdecl dummy_initiateRSP(RSP_INFO Rsp_Info, DWORD* CycleCount) {};

static DWORD WINAPI ThreadFunc(LPVOID lpParam);

const char g_szClassName[] = "myWindowClass";
char LastSelectedRom[_MAX_PATH];
bool scheduled_restart = false;
static BOOL restart_mode = 0;
BOOL really_restart_mode = 0;
BOOL clear_sram_on_restart_mode = 0;
BOOL continue_vcr_on_restart_mode = 0;
BOOL just_restarted_flag = 0;
static int InputPluginVersion;
static BOOL AutoPause = 0;
static BOOL MenuPaused = 0;
//extern int recording;
//static HICON hStatusIcon;                                  //Icon Handle for statusbar
static HWND hStaticHandle;                                 //Handle for static place
int externalReadScreen;

char TempMessage[200];
int emu_launched; // emu_emulating
int emu_paused;
int recording;
HWND hTool, mainHWND, hStatus, hRomList, hStatusProgress;
HINSTANCE app_hInstance;
BOOL manualFPSLimit = TRUE;
BOOL ignoreErrorEmulation = FALSE;
char statusmsg[800];

char gfx_name[255];
char input_name[255];
char sound_name[255];
char rsp_name[255];

char stroopConfigLine[150] = {0};
char correctedPath[260];
#define INCOMPATIBLE_PLUGINS_AMOUNT 1 // this is so bad
const char pluginBlacklist[INCOMPATIBLE_PLUGINS_AMOUNT][256] = {"Azimer\'s Audio v0.7"};

ReassociatingFileDialog fdBrowseMovie;
ReassociatingFileDialog fdBrowseMovie2;
ReassociatingFileDialog fdLoadRom;
ReassociatingFileDialog fdSaveLoadAs;
ReassociatingFileDialog fdSaveStateAs;
ReassociatingFileDialog fdStartCapture;

enum EThreadFuncState {
	TFS_INITMEM,
	TFS_LOADPLUGINS,
	TFS_LOADGFX,
	TFS_LOADINPUT,
	TFS_LOADAUDIO,
	TFS_LOADRSP,
	TFS_OPENGFX,
	TFS_OPENINPUT,
	TFS_OPENAUDIO,
	TFS_DISPLAYMODE,
	TFS_CREATESOUND,
	TFS_EMULATING,
	TFS_CLOSEINPUT,
	TFS_CLOSEAUDIO,
	TFS_CLOSERSP,
	TFS_UNLOADRSP,
	TFS_UNLOADINPUT,
	TFS_UNLOADAUDIO,
	TFS_CLOSEGFX,
	TFS_UNLOADGFX,
	TFS_NONE
};
const char* const ThreadFuncStateDescription[] = {
	"initializing memory",
	"loading plugins",
	"loading graphics plugin",
	"loading input plugin",
	"loading audio plugin",
	"loading RSP plugin",
	"opening graphics",
	"opening input",
	"opening audio",
	"setting display mode",
	"creating sound thread",
	"emulating",
	"closing the input plugin",
	"closing the audio plugin",
	"closing the RSP plugin",
	"unloading the RSP plugin DLL",
	"unloading the input plugin DLL",
	"unloading the audio plugin DLL",
	"closing the graphics plugin",
	"unloading the graphics plugin DLL",
	"(unknown)"
};
static int ThreadFuncState = TFS_NONE;


TCHAR CoreNames[3][30] = {TEXT("Interpreter"), TEXT("Dynamic Recompiler"), TEXT("Pure Interpreter")};

char AppPath[MAX_PATH];

void ClearButtons() {
	BUTTONS zero = {0};
	for (int i = 0; i < 4; i++) {
		setKeys(i, zero);
	}
}
void getAppFullPath(char* ret) {
	char drive[_MAX_DRIVE], dirn[_MAX_DIR];
	char fname[_MAX_FNAME], ext[_MAX_EXT];
	char path_buffer[_MAX_DIR];

	GetModuleFileName(NULL, path_buffer, sizeof(path_buffer));
	_splitpath(path_buffer, drive, dirn, fname, ext);
	strcpy(ret, drive);
	strcat(ret, dirn);
}

static void __cdecl sucre() {
   //printf("sucre\n");
}

static void gui_ChangeWindow() {
	if (FullScreenMode) {
		EnableStatusbar();
		EnableWindow(hTool, FALSE);
		ShowWindow(hTool, SW_HIDE);
		ShowWindow(hStatus, SW_HIDE);
		ShowCursor(FALSE);
		changeWindow();
	} else {
		changeWindow();
		ShowWindow(hTool, SW_SHOW);
		EnableWindow(hTool, TRUE);
		ShowWindow(hStatus, SW_SHOW);
		ShowCursor(TRUE);
	}

}

void SetupDummyInfo() {
	int i;

	/////// GFX ///////////////////////////
	//dummy_gfx_info.hWnd = mainHWND;
	dummy_gfx_info.hWnd = hStatus;
	dummy_gfx_info.hStatusBar = hStatus;
	dummy_gfx_info.MemoryBswaped = TRUE;
	dummy_gfx_info.HEADER = (BYTE*)DummyHeader;
	dummy_gfx_info.RDRAM = (BYTE*)rdram;
	dummy_gfx_info.DMEM = (BYTE*)SP_DMEM;
	dummy_gfx_info.IMEM = (BYTE*)SP_IMEM;
	dummy_gfx_info.MI_INTR_REG = &(MI_register.mi_intr_reg);
	dummy_gfx_info.DPC_START_REG = &(dpc_register.dpc_start);
	dummy_gfx_info.DPC_END_REG = &(dpc_register.dpc_end);
	dummy_gfx_info.DPC_CURRENT_REG = &(dpc_register.dpc_current);
	dummy_gfx_info.DPC_STATUS_REG = &(dpc_register.dpc_status);
	dummy_gfx_info.DPC_CLOCK_REG = &(dpc_register.dpc_clock);
	dummy_gfx_info.DPC_BUFBUSY_REG = &(dpc_register.dpc_bufbusy);
	dummy_gfx_info.DPC_PIPEBUSY_REG = &(dpc_register.dpc_pipebusy);
	dummy_gfx_info.DPC_TMEM_REG = &(dpc_register.dpc_tmem);
	dummy_gfx_info.VI_STATUS_REG = &(vi_register.vi_status);
	dummy_gfx_info.VI_ORIGIN_REG = &(vi_register.vi_origin);
	dummy_gfx_info.VI_WIDTH_REG = &(vi_register.vi_width);
	dummy_gfx_info.VI_INTR_REG = &(vi_register.vi_v_intr);
	dummy_gfx_info.VI_V_CURRENT_LINE_REG = &(vi_register.vi_current);
	dummy_gfx_info.VI_TIMING_REG = &(vi_register.vi_burst);
	dummy_gfx_info.VI_V_SYNC_REG = &(vi_register.vi_v_sync);
	dummy_gfx_info.VI_H_SYNC_REG = &(vi_register.vi_h_sync);
	dummy_gfx_info.VI_LEAP_REG = &(vi_register.vi_leap);
	dummy_gfx_info.VI_H_START_REG = &(vi_register.vi_h_start);
	dummy_gfx_info.VI_V_START_REG = &(vi_register.vi_v_start);
	dummy_gfx_info.VI_V_BURST_REG = &(vi_register.vi_v_burst);
	dummy_gfx_info.VI_X_SCALE_REG = &(vi_register.vi_x_scale);
	dummy_gfx_info.VI_Y_SCALE_REG = &(vi_register.vi_y_scale);
	dummy_gfx_info.CheckInterrupts = sucre;

	/////// AUDIO /////////////////////////
	dummy_audio_info.hwnd = mainHWND;
	dummy_audio_info.hinst = app_hInstance;
	dummy_audio_info.MemoryBswaped = TRUE;
	dummy_audio_info.HEADER = (BYTE*)DummyHeader;
	dummy_audio_info.RDRAM = (BYTE*)rdram;
	dummy_audio_info.DMEM = (BYTE*)SP_DMEM;
	dummy_audio_info.IMEM = (BYTE*)SP_IMEM;
	dummy_audio_info.MI_INTR_REG = &(MI_register.mi_intr_reg);
	dummy_audio_info.AI_DRAM_ADDR_REG = &(ai_register.ai_dram_addr);
	dummy_audio_info.AI_LEN_REG = &(ai_register.ai_len);
	dummy_audio_info.AI_CONTROL_REG = &(ai_register.ai_control);
	dummy_audio_info.AI_STATUS_REG = &(ai_register.ai_status);
	dummy_audio_info.AI_DACRATE_REG = &(ai_register.ai_dacrate);
	dummy_audio_info.AI_BITRATE_REG = &(ai_register.ai_bitrate);
	dummy_audio_info.CheckInterrupts = sucre;

	///// CONTROLS ///////////////////////////
	dummy_control_info.hMainWindow = mainHWND;
	dummy_control_info.hinst = app_hInstance;
	dummy_control_info.MemoryBswaped = TRUE;
	dummy_control_info.HEADER = (BYTE*)DummyHeader;
	dummy_control_info.Controls = Controls;
	for (i = 0; i < 4; i++) {
		Controls[i].Present = FALSE;
		Controls[i].RawData = FALSE;
		Controls[i].Plugin = PLUGIN_NONE;
	}

	//////// RSP /////////////////////////////
	dummy_rsp_info.MemoryBswaped = TRUE;
	dummy_rsp_info.RDRAM = (BYTE*)rdram;
	dummy_rsp_info.DMEM = (BYTE*)SP_DMEM;
	dummy_rsp_info.IMEM = (BYTE*)SP_IMEM;
	dummy_rsp_info.MI_INTR_REG = &MI_register.mi_intr_reg;
	dummy_rsp_info.SP_MEM_ADDR_REG = &sp_register.sp_mem_addr_reg;
	dummy_rsp_info.SP_DRAM_ADDR_REG = &sp_register.sp_dram_addr_reg;
	dummy_rsp_info.SP_RD_LEN_REG = &sp_register.sp_rd_len_reg;
	dummy_rsp_info.SP_WR_LEN_REG = &sp_register.sp_wr_len_reg;
	dummy_rsp_info.SP_STATUS_REG = &sp_register.sp_status_reg;
	dummy_rsp_info.SP_DMA_FULL_REG = &sp_register.sp_dma_full_reg;
	dummy_rsp_info.SP_DMA_BUSY_REG = &sp_register.sp_dma_busy_reg;
	dummy_rsp_info.SP_PC_REG = &rsp_register.rsp_pc;
	dummy_rsp_info.SP_SEMAPHORE_REG = &sp_register.sp_semaphore_reg;
	dummy_rsp_info.DPC_START_REG = &dpc_register.dpc_start;
	dummy_rsp_info.DPC_END_REG = &dpc_register.dpc_end;
	dummy_rsp_info.DPC_CURRENT_REG = &dpc_register.dpc_current;
	dummy_rsp_info.DPC_STATUS_REG = &dpc_register.dpc_status;
	dummy_rsp_info.DPC_CLOCK_REG = &dpc_register.dpc_clock;
	dummy_rsp_info.DPC_BUFBUSY_REG = &dpc_register.dpc_bufbusy;
	dummy_rsp_info.DPC_PIPEBUSY_REG = &dpc_register.dpc_pipebusy;
	dummy_rsp_info.DPC_TMEM_REG = &dpc_register.dpc_tmem;
	dummy_rsp_info.CheckInterrupts = sucre;
	dummy_rsp_info.ProcessDlistList = processDList;
	dummy_rsp_info.ProcessAlistList = processAList;
	dummy_rsp_info.ProcessRdpList = processRDPList;
	dummy_rsp_info.ShowCFB = showCFB;

}

void SaveGlobalPlugins(BOOL method) {
	static char gfx_temp[100], input_temp[100], sound_temp[100], rsp_temp[100];

	if (method)  //Saving
	{
		sprintf(gfx_temp, gfx_name);
		sprintf(input_temp, input_name);
		sprintf(sound_temp, sound_name);
		sprintf(rsp_temp, rsp_name);
	} else         //Loading
	{
		sprintf(gfx_name, gfx_temp);
		sprintf(input_name, input_temp);
		sprintf(sound_name, sound_temp);
		sprintf(rsp_name, rsp_temp);
	}
}

static int LastState = ID_CURRENTSAVE_1;
void SelectState(HWND hWnd, int StateID) {
	HMENU hMenu = GetMenu(hWnd);
	CheckMenuItem(hMenu, LastState, MF_BYCOMMAND | MFS_UNCHECKED);
	CheckMenuItem(hMenu, StateID, MF_BYCOMMAND | MFS_CHECKED);
	currentSaveState = StateID - (ID_CURRENTSAVE_1 - 1);
	LastState = StateID;
	savestates_select_slot(currentSaveState);
}

void SaveTheState(HWND hWnd, int StateID) {
	SelectState(hWnd, (StateID - ID_SAVE_1) + ID_CURRENTSAVE_1);

//	SendMessage(hWnd, WM_COMMAND, STATE_SAVE, 0);
	if (!emu_paused || MenuPaused)
		savestates_job = SAVESTATE;
	else if (emu_launched)
		savestates_save();
}

void LoadTheState(HWND hWnd, int StateID) {
	SelectState(hWnd, (StateID - ID_LOAD_1) + ID_CURRENTSAVE_1);

//	SendMessage(hWnd, WM_COMMAND, STATE_RESTORE, 0);
	if (emu_launched) {
		savestates_job = LOADSTATE;
	}
	//don't
	//if(emu_paused){
		//update_pif_read(FALSE); // pass in true and it will stuck
	//}
}



//--------------------- plugin storage type ----------------
typedef struct _plugins plugins;
struct _plugins {
	char* file_name;
	char* plugin_name;
	HMODULE handle;
	int type;
	plugins* next;
};
static plugins* liste_plugins = NULL, * current;

void insert_plugin(plugins* p, char* file_name,
	char* plugin_name, void* handle, int type, int num) {




	if (p->next)
		insert_plugin(p->next, file_name, plugin_name, handle, type,
			(p->type == type) ? num + 1 : num);
	else {
		for (int i = 0; i < INCOMPATIBLE_PLUGINS_AMOUNT; i++) {
			if (strstr(plugin_name, pluginBlacklist[i])) {
				char* msg = (char*)malloc(sizeof(pluginBlacklist[i]));

				sprintf(msg, "An incompatible plugin with the name \"%s\" was detected.\
                \nIt is highly recommended to skip loading this plugin as not doing so might cause instability.\
                \nAre you sure you want to load this plugin?", plugin_name);

				int res = MessageBox(0, msg, "Incompatible plugin", MB_YESNO | MB_TOPMOST | MB_ICONWARNING);

				free(msg);

				if (res == IDNO)
					return;
				else
					; // todo: punch user in the face

			}
		}
		p->next = (plugins*)malloc(sizeof(plugins));
		p->next->type = type;
		p->next->handle = (HMODULE)handle;
		p->next->file_name = (char*)malloc(strlen(file_name) + 1);
		strcpy(p->next->file_name, file_name);
		p->next->plugin_name = (char*)malloc(strlen(plugin_name) + 7);
		sprintf(p->next->plugin_name, "%s", plugin_name);
		p->next->next = NULL;
	}
}

void rewind_plugin() {
	current = liste_plugins;
}

char* next_plugin() {
	if (!current->next) return NULL;
	current = current->next;
	return current->plugin_name;
}

int get_plugin_type() {
	if (!current->next) return -1;
	return current->next->type;
}

char* getPluginNameInner(plugins* p, char* pluginpath, int plugintype) {
	if (!p->next) return NULL;
	if ((plugintype == p->next->type) && (_stricmp(p->next->file_name, pluginpath) == 0))
		return p->next->plugin_name;
	else
		return getPluginNameInner(p->next, pluginpath, plugintype);
}

char* getPluginName(char* pluginpath, int plugintype) {
	return getPluginNameInner(liste_plugins, pluginpath, plugintype);

}

HMODULE get_handle(plugins* p, char* name) {
	if (!p->next) return NULL;

	while (p->next->plugin_name[strlen(p->next->plugin_name) - 1] == ' ')
		p->next->plugin_name[strlen(p->next->plugin_name) - 1] = '\0';

	if (!strcmp(p->next->plugin_name, name))
		return p->next->handle;
	else
		return (HMODULE)get_handle(p->next, name);
}

char* getExtension(char* str) {
	if (strlen(str) > 3) return str + strlen(str) - 3;
	else return NULL;
}



struct ProcAddress {
	FARPROC _fp;
	ProcAddress(HMODULE module, LPCSTR name) : _fp(NULL) {
		_fp = ::GetProcAddress(module, name);
	}
	template<class T>
	operator T() const {
		return reinterpret_cast<T>(_fp);
	}
};

void search_plugins() {
	// TODO: ���̕ӂ�Ȃɂ��Ă邩�悭���ĂȂ�
	liste_plugins = (plugins*)malloc(sizeof(plugins));
	liste_plugins->type = -1;
	liste_plugins->next = NULL;
	current = liste_plugins;

	String pluginDir;
	if (Config.is_default_plugins_directory_used) {
		pluginDir.assign(AppPath).append("\\plugin");
	} else {
		pluginDir.assign(Config.plugins_directory);
	}

	WIN32_FIND_DATA entry;
	HANDLE dirHandle = ::FindFirstFile((pluginDir + "\\*.dll").c_str(), &entry);
	if (dirHandle == INVALID_HANDLE_VALUE) {
		return;
	}

	do { //plugin�f�B���N�g����ǂ�ŁA�v���O�C�����X�g�𐶐�
		if (String(::getExtension(entry.cFileName)) == "dll") {
			String pluginPath;
			pluginPath.assign(pluginDir)
				.append("\\").append(entry.cFileName);
			MUPEN64RR_DEBUGINFO(pluginPath);

			HMODULE pluginHandle = LoadLibrary(pluginPath.c_str());
			if (pluginHandle != NULL
				&& (getDllInfo = ProcAddress(pluginHandle, "GetDllInfo")) != NULL) {
				PLUGIN_INFO pluginInfo;
				getDllInfo(&pluginInfo);
				while (pluginInfo.Name[strlen(pluginInfo.Name) - 1] == ' ') {
					pluginInfo.Name[strlen(pluginInfo.Name) - 1] = '\0';
				}
				insert_plugin(liste_plugins, entry.cFileName, pluginInfo.Name,
					pluginHandle, pluginInfo.Type, 0);
			}
		}
	} while (!!FindNextFile(dirHandle, &entry));

}

void exec_config(char* name) {
	HMODULE handle;
	PLUGIN_INFO PluginInfo;
	handle = (HMODULE)get_handle(liste_plugins, name);
	int i;
	BOOL wasPaused = emu_paused && !MenuPaused;
	MenuPaused = FALSE;
	if (emu_launched && !emu_paused) {
		pauseEmu(FALSE);
	};

	if (handle) {

		getDllInfo = (void(__cdecl*)(PLUGIN_INFO * PluginInfo))GetProcAddress(handle, "GetDllInfo");

		getDllInfo(&PluginInfo);
		switch (PluginInfo.Type) {
			case PLUGIN_TYPE_AUDIO:
				if (!emu_launched) {
					initiateAudio = (BOOL(__cdecl*)(AUDIO_INFO))GetProcAddress(handle, "InitiateAudio");
					if (!initiateAudio(dummy_audio_info)) {
						ShowMessage("Failed to initialize audio plugin.");
					}
				}

				dllConfig = (void(__cdecl*)(HWND hParent))GetProcAddress(handle, "DllConfig");
				if (dllConfig) dllConfig(hwnd_plug);
				if (!emu_launched) {
					closeDLL_audio = (void(__cdecl*)())GetProcAddress(handle, "CloseDLL");
					if (closeDLL_audio) closeDLL_audio();
				}
				break;

			case PLUGIN_TYPE_GFX:
				if (!emu_launched) {
					initiateGFX = (BOOL(__cdecl*)(GFX_INFO Gfx_Info))GetProcAddress(handle, "InitiateGFX");
					if (!initiateGFX(dummy_gfx_info)) {
						ShowMessage("Failed to initiate gfx plugin.");
					}
				}

				dllConfig = (void(__cdecl*)(HWND hParent))GetProcAddress(handle, "DllConfig");
				if (dllConfig) dllConfig(hwnd_plug);
				if (!emu_launched) {
					closeDLL_gfx = (void(__cdecl*)())GetProcAddress(handle, "CloseDLL");
					if (closeDLL_gfx) closeDLL_gfx();
				}
				break;

			case PLUGIN_TYPE_CONTROLLER:
				if (!emu_launched) {
					if (PluginInfo.Version == 0x0101) {
						initiateControllers = (void(__cdecl*)(CONTROL_INFO ControlInfo))GetProcAddress(handle, "InitiateControllers");
						initiateControllers(dummy_control_info);
					} else {
						old_initiateControllers = (void(__cdecl*)(HWND hMainWindow, CONTROL Controls[4]))GetProcAddress(handle, "InitiateControllers");
						old_initiateControllers(mainHWND, Controls);
					}
				}

				dllConfig = (void(__cdecl*)(HWND hParent))GetProcAddress(handle, "DllConfig");
				if (dllConfig) dllConfig(hwnd_plug);

				if (!emu_launched) {
					closeDLL_input = (void(__cdecl*)())GetProcAddress(handle, "CloseDLL");
					if (closeDLL_input) closeDLL_input();
				}
				break;
			case PLUGIN_TYPE_RSP:
				if (!emu_launched) {
					initiateRSP = (void(__cdecl*)(RSP_INFO, DWORD*))GetProcAddress(handle, "InitiateRSP");
					initiateRSP(dummy_rsp_info, (DWORD*)&i);
				}

				dllConfig = (void(__cdecl*)(HWND hParent))GetProcAddress(handle, "DllConfig");
				if (dllConfig) dllConfig(hwnd_plug);
				if (!emu_launched) {
					closeDLL_RSP = (void(__cdecl*)())GetProcAddress(handle, "CloseDLL");
					if (closeDLL_RSP) closeDLL_RSP();
				}
				break;
			default:
				dllConfig = (void(__cdecl*)(HWND hParent))GetProcAddress(handle, "DllConfig");
				if (dllConfig) dllConfig(hwnd_plug);
				break;
		}


	}

	if (emu_launched && emu_paused && !wasPaused) {
		resumeEmu(FALSE);
	}
}

void exec_test(char* name) {
	HMODULE handle;

	handle = (HMODULE)get_handle(liste_plugins, name);
	if (handle) {
		dllTest = (void(__cdecl*)(HWND hParent))GetProcAddress(handle, "DllTest");
		if (dllTest) dllTest(hwnd_plug);
	}
}

void exec_about(char* name) {
	HMODULE handle;

	handle = (HMODULE)get_handle(liste_plugins, name);
	if (handle) {
		dllAbout = (void(__cdecl*)(HWND hParent))GetProcAddress(handle, "DllAbout");
		if (dllAbout) dllAbout(hwnd_plug);
	}
}
int check_plugins() {
	// Bad implementation... i forgot this is C++ and not C so
	// i went with a low-level implementation at first...
	// But hey this works 
	void* handle_gfx, * handle_input, * handle_sound, * handle_rsp;
	handle_gfx = get_handle(liste_plugins, gfx_name);
	handle_input = get_handle(liste_plugins, input_name);
	handle_sound = get_handle(liste_plugins, sound_name);
	handle_rsp = get_handle(liste_plugins, rsp_name);

	void* pluginHandles[4] = {handle_gfx, handle_input, handle_sound, handle_rsp}; // can probably be done in one line
	char pluginsMissing = 0;
	std::string pluginNames[] = {"Video", "Input", "Sound", "RSP"};
	std::string finalMessage = "Plugin(s) missing: ";
	for (char i = 0; i <= 3; i++) {
		if (!pluginHandles[i]) {
			printf("Plugin missing: %s\n", pluginNames[i].c_str());
			if (i != 3)
				pluginNames[i].append(", ");

			finalMessage.append(pluginNames[i]);
			pluginsMissing++;
			//strcat(finalMessage, pluginNames[i]);
		}
	}

	if (pluginsMissing != 0) {
		// strdup seems like a bad idea... this too :)
		if (pluginsMissing == 1) {
			/*finalMessage.pop_back();*/
			finalMessage.resize(finalMessage.size() - 2);
			/* HACK: instead of doing better programming, just trim last letter (whitespace too)*/
		}
		if (MessageBox(mainHWND, (finalMessage + "\nDo you want to select plugins?").c_str(), "Error", MB_TASKMODAL | MB_ICONERROR | MB_YESNO) == IDYES) {
			ChangeSettings(mainHWND);
			ini_updateFile(Config.is_ini_compressed);
		}

		return FALSE;
	}

	return TRUE;
}

int load_gfx(HMODULE handle_gfx) {
	if (handle_gfx) {
		changeWindow = (void(__cdecl*)())GetProcAddress(handle_gfx, "ChangeWindow");
		closeDLL_gfx = (void(__cdecl*)())GetProcAddress(handle_gfx, "CloseDLL");
		dllAbout = (void(__cdecl*)(HWND hParent))GetProcAddress(handle_gfx, "DllAbout");
		dllConfig = (void(__cdecl*)(HWND hParent))GetProcAddress(handle_gfx, "DllConfig");
		dllTest = (void(__cdecl*)(HWND hParent))GetProcAddress(handle_gfx, "DllTest");
		initiateGFX = (BOOL(__cdecl*)(GFX_INFO Gfx_Info))GetProcAddress(handle_gfx, "InitiateGFX");
		processDList = (void(__cdecl*)())GetProcAddress(handle_gfx, "ProcessDList");
		processRDPList = (void(__cdecl*)())GetProcAddress(handle_gfx, "ProcessRDPList");
		romClosed_gfx = (void(__cdecl*)())GetProcAddress(handle_gfx, "RomClosed");
		romOpen_gfx = (void(__cdecl*)())GetProcAddress(handle_gfx, "RomOpen");
		showCFB = (void(__cdecl*)())GetProcAddress(handle_gfx, "ShowCFB");
		updateScreen = (void(__cdecl*)())GetProcAddress(handle_gfx, "UpdateScreen");
		viStatusChanged = (void(__cdecl*)())GetProcAddress(handle_gfx, "ViStatusChanged");
		viWidthChanged = (void(__cdecl*)())GetProcAddress(handle_gfx, "ViWidthChanged");
		moveScreen = (void(__cdecl*)(int, int))GetProcAddress(handle_gfx, "MoveScreen");
		CaptureScreen = (void(__cdecl*)(char* Directory))GetProcAddress(handle_gfx, "CaptureScreen");
		if (Config.is_internal_capture_forced) {
			readScreen = NULL;
			externalReadScreen = 0;
			DllCrtFree = free;
		} else {
			readScreen = (void(__cdecl*)(void** dest, long* width, long* height))GetProcAddress(handle_gfx, "ReadScreen");
			if (readScreen == NULL) {
				//try to find readscreen2 instead (gln64)
				readScreen = (void(__cdecl*)(void** dest, long* width, long* height))GetProcAddress(handle_gfx, "ReadScreen2");
				if (readScreen == NULL) {
					externalReadScreen = 0;
					DllCrtFree = free;
				}

			}
			if (readScreen) {
				externalReadScreen = 1;
				DllCrtFree = (void(__cdecl*)(void*))GetProcAddress(handle_gfx, "DllCrtFree");
				if (DllCrtFree == NULL) DllCrtFree = free; //attempt to match the crt, avi capture will probably crash without this
			}

		}

		fBRead = (void(__cdecl*)(DWORD))GetProcAddress(handle_gfx, "FBRead");
		fBWrite = (void(__cdecl*)(DWORD, DWORD))GetProcAddress(handle_gfx, "FBWrite");
		fBGetFrameBufferInfo = (void(__cdecl*)(void*))GetProcAddress(handle_gfx, "FBGetFrameBufferInfo");

		if (changeWindow == NULL) changeWindow = dummy_void;
		if (closeDLL_gfx == NULL) closeDLL_gfx = dummy_void;
		if (initiateGFX == NULL) initiateGFX = dummy_initiateGFX;
		if (processDList == NULL) processDList = dummy_void;
		if (processRDPList == NULL) processRDPList = dummy_void;
		if (romClosed_gfx == NULL) romClosed_gfx = dummy_void;
		if (romOpen_gfx == NULL) romOpen_gfx = dummy_void;
		if (showCFB == NULL) showCFB = dummy_void;
		if (updateScreen == NULL) updateScreen = dummy_void;
		if (viStatusChanged == NULL) viStatusChanged = dummy_void;
		if (viWidthChanged == NULL) viWidthChanged = dummy_void;
		if (CaptureScreen == NULL) CaptureScreen = (void(__cdecl*)(char*))dummy_void;
		if (moveScreen == NULL) moveScreen = (void(__cdecl*)(int, int))dummy_void;

		gfx_info.hWnd = mainHWND;
		if (Config.is_statusbar_enabled) {
			gfx_info.hStatusBar = hStatus;
		} else {
			gfx_info.hStatusBar = NULL;
		}
		gfx_info.MemoryBswaped = TRUE;
		gfx_info.HEADER = rom;
		gfx_info.RDRAM = (BYTE*)rdram;
		gfx_info.DMEM = (BYTE*)SP_DMEM;
		gfx_info.IMEM = (BYTE*)SP_IMEM;
		gfx_info.MI_INTR_REG = &(MI_register.mi_intr_reg);
		gfx_info.DPC_START_REG = &(dpc_register.dpc_start);
		gfx_info.DPC_END_REG = &(dpc_register.dpc_end);
		gfx_info.DPC_CURRENT_REG = &(dpc_register.dpc_current);
		gfx_info.DPC_STATUS_REG = &(dpc_register.dpc_status);
		gfx_info.DPC_CLOCK_REG = &(dpc_register.dpc_clock);
		gfx_info.DPC_BUFBUSY_REG = &(dpc_register.dpc_bufbusy);
		gfx_info.DPC_PIPEBUSY_REG = &(dpc_register.dpc_pipebusy);
		gfx_info.DPC_TMEM_REG = &(dpc_register.dpc_tmem);
		gfx_info.VI_STATUS_REG = &(vi_register.vi_status);
		gfx_info.VI_ORIGIN_REG = &(vi_register.vi_origin);
		gfx_info.VI_WIDTH_REG = &(vi_register.vi_width);
		gfx_info.VI_INTR_REG = &(vi_register.vi_v_intr);
		gfx_info.VI_V_CURRENT_LINE_REG = &(vi_register.vi_current);
		gfx_info.VI_TIMING_REG = &(vi_register.vi_burst);
		gfx_info.VI_V_SYNC_REG = &(vi_register.vi_v_sync);
		gfx_info.VI_H_SYNC_REG = &(vi_register.vi_h_sync);
		gfx_info.VI_LEAP_REG = &(vi_register.vi_leap);
		gfx_info.VI_H_START_REG = &(vi_register.vi_h_start);
		gfx_info.VI_V_START_REG = &(vi_register.vi_v_start);
		gfx_info.VI_V_BURST_REG = &(vi_register.vi_v_burst);
		gfx_info.VI_X_SCALE_REG = &(vi_register.vi_x_scale);
		gfx_info.VI_Y_SCALE_REG = &(vi_register.vi_y_scale);
		gfx_info.CheckInterrupts = sucre;
		initiateGFX(gfx_info);
	} else {
		changeWindow = dummy_void;
		closeDLL_gfx = dummy_void;
		initiateGFX = dummy_initiateGFX;
		processDList = dummy_void;
		processRDPList = dummy_void;
		romClosed_gfx = dummy_void;
		romOpen_gfx = dummy_void;
		showCFB = dummy_void;
		updateScreen = dummy_void;
		viStatusChanged = dummy_void;
		viWidthChanged = dummy_void;
	}
	return 0;
}
int load_input(HMODULE handle_input) {
	int i;
	PLUGIN_INFO PluginInfo;
	if (handle_input) {
		getDllInfo = (void(__cdecl*)(PLUGIN_INFO * PluginInfo))GetProcAddress(handle_input, "GetDllInfo");
		getDllInfo(&PluginInfo);

		closeDLL_input = (void(__cdecl*)())GetProcAddress(handle_input, "CloseDLL");
		controllerCommand = (void(__cdecl*)(int Control, BYTE * Command))GetProcAddress(handle_input, "ControllerCommand");
		getKeys = (void(__cdecl*)(int Control, BUTTONS * Keys))GetProcAddress(handle_input, "GetKeys");
		setKeys = (void(__cdecl*)(int Control, BUTTONS  Keys))GetProcAddress(handle_input, "SetKeys");
		if (PluginInfo.Version == 0x0101)
			initiateControllers = (void(__cdecl*)(CONTROL_INFO ControlInfo))GetProcAddress(handle_input, "InitiateControllers");
		else
			old_initiateControllers = (void(__cdecl*)(HWND hMainWindow, CONTROL Controls[4]))GetProcAddress(handle_input, "InitiateControllers");
		readController = (void(__cdecl*)(int Control, BYTE * Command))GetProcAddress(handle_input, "ReadController");
		romClosed_input = (void(__cdecl*)())GetProcAddress(handle_input, "RomClosed");
		romOpen_input = (void(__cdecl*)())GetProcAddress(handle_input, "RomOpen");
		keyDown = (void(__cdecl*)(WPARAM wParam, LPARAM lParam))GetProcAddress(handle_input, "WM_KeyDown");
		keyUp = (void(__cdecl*)(WPARAM wParam, LPARAM lParam))GetProcAddress(handle_input, "WM_KeyUp");

		if (closeDLL_input == NULL) closeDLL_input = dummy_void;
		if (controllerCommand == NULL) controllerCommand = dummy_controllerCommand;
		if (getKeys == NULL) getKeys = dummy_getKeys;
		if (setKeys == NULL) setKeys = dummy_setKeys;
		if (initiateControllers == NULL) initiateControllers = dummy_initiateControllers;
		if (readController == NULL) readController = dummy_readController;
		if (romClosed_input == NULL) romClosed_input = dummy_void;
		if (romOpen_input == NULL) romOpen_input = dummy_void;
		if (keyDown == NULL) keyDown = dummy_keyDown;
		if (keyUp == NULL) keyUp = dummy_keyUp;

		control_info.hMainWindow = mainHWND;
		control_info.hinst = app_hInstance;
		control_info.MemoryBswaped = TRUE;
		control_info.HEADER = rom;
		control_info.Controls = Controls;
		for (i = 0; i < 4; i++) {
			Controls[i].Present = FALSE;
			Controls[i].RawData = FALSE;
			Controls[i].Plugin = PLUGIN_NONE;
		}
		if (PluginInfo.Version == 0x0101) {
			initiateControllers(control_info);
		} else {
			old_initiateControllers(mainHWND, Controls);
		}
		InputPluginVersion = PluginInfo.Version;
	} else {
		closeDLL_input = dummy_void;
		controllerCommand = dummy_controllerCommand;
		getKeys = dummy_getKeys;
		setKeys = dummy_setKeys;
		initiateControllers = dummy_initiateControllers;
		readController = dummy_readController;
		romClosed_input = dummy_void;
		romOpen_input = dummy_void;
		keyDown = dummy_keyDown;
		keyUp = dummy_keyUp;
	}
	return 0;
}


int load_sound(HMODULE handle_sound) {
	if (handle_sound) {
		closeDLL_audio = (void(__cdecl*)(void))GetProcAddress(handle_sound, "CloseDLL");
		aiDacrateChanged = (void(__cdecl*)(int))GetProcAddress(handle_sound, "AiDacrateChanged");
		aiLenChanged = (void(__cdecl*)(void))GetProcAddress(handle_sound, "AiLenChanged");
		aiReadLength = (DWORD(__cdecl*)(void))GetProcAddress(handle_sound, "AiReadLength");
		initiateAudio = (BOOL(__cdecl*)(AUDIO_INFO))GetProcAddress(handle_sound, "InitiateAudio");
		romClosed_audio = (void(__cdecl*)(void))GetProcAddress(handle_sound, "RomClosed");
		romOpen_audio = (void(__cdecl*)(void))GetProcAddress(handle_sound, "RomOpen");
		processAList = (void(__cdecl*)(void))GetProcAddress(handle_sound, "ProcessAList");
		aiUpdate = (void(__cdecl*)(BOOL))GetProcAddress(handle_sound, "AiUpdate");

		if (aiDacrateChanged == NULL) aiDacrateChanged = dummy_aiDacrateChanged;
		if (aiLenChanged == NULL) aiLenChanged = dummy_void;
		if (aiReadLength == NULL) aiReadLength = dummy_aiReadLength;
		//if (aiUpdate == NULL) aiUpdate = dummy_aiUpdate;
		if (closeDLL_audio == NULL) closeDLL_audio = dummy_void;
		if (initiateAudio == NULL) initiateAudio = dummy_initiateAudio;
		if (processAList == NULL) processAList = dummy_void;
		if (romClosed_audio == NULL) romClosed_audio = dummy_void;
		if (romOpen_audio == NULL) romOpen_audio = dummy_void;

		audio_info.hwnd = mainHWND;
		audio_info.hinst = app_hInstance;
		audio_info.MemoryBswaped = TRUE;
		audio_info.HEADER = rom;

		audio_info.RDRAM = (BYTE*)rdram;
		audio_info.DMEM = (BYTE*)SP_DMEM;
		audio_info.IMEM = (BYTE*)SP_IMEM;

		audio_info.MI_INTR_REG = &dummy;//&(MI_register.mi_intr_reg);

		audio_info.AI_DRAM_ADDR_REG = &(ai_register.ai_dram_addr);
		audio_info.AI_LEN_REG = &(ai_register.ai_len);
		audio_info.AI_CONTROL_REG = &(ai_register.ai_control);
		audio_info.AI_STATUS_REG = &dummy;//&(ai_register.ai_status);
		audio_info.AI_DACRATE_REG = &(ai_register.ai_dacrate);
		audio_info.AI_BITRATE_REG = &(ai_register.ai_bitrate);

		audio_info.CheckInterrupts = sucre;
		initiateAudio(audio_info);
	} else {
		aiDacrateChanged = dummy_aiDacrateChanged;
		aiLenChanged = dummy_void;
		aiReadLength = dummy_aiReadLength;
		//aiUpdate = dummy_aiUpdate;
		closeDLL_audio = dummy_void;
		initiateAudio = dummy_initiateAudio;
		processAList = dummy_void;
		romClosed_audio = dummy_void;
		romOpen_audio = dummy_void;
	}
	return 0;
}

int load_rsp(HMODULE handle_RSP) {
	int i = 4;
	if (handle_RSP) {
		closeDLL_RSP = (void(__cdecl*)(void))GetProcAddress(handle_RSP, "CloseDLL");
		doRspCycles = (DWORD(__cdecl*)(DWORD))GetProcAddress(handle_RSP, "DoRspCycles");
		initiateRSP = (void(__cdecl*)(RSP_INFO, DWORD*))GetProcAddress(handle_RSP, "InitiateRSP");
		romClosed_RSP = (void(__cdecl*)(void))GetProcAddress(handle_RSP, "RomClosed");

		if (closeDLL_RSP == NULL) closeDLL_RSP = dummy_void;
		if (doRspCycles == NULL) doRspCycles = dummy_doRspCycles;
		if (initiateRSP == NULL) initiateRSP = dummy_initiateRSP;
		if (romClosed_RSP == NULL) romClosed_RSP = dummy_void;

		rsp_info.MemoryBswaped = TRUE;
		rsp_info.RDRAM = (BYTE*)rdram;
		rsp_info.DMEM = (BYTE*)SP_DMEM;
		rsp_info.IMEM = (BYTE*)SP_IMEM;
		rsp_info.MI_INTR_REG = &MI_register.mi_intr_reg;
		rsp_info.SP_MEM_ADDR_REG = &sp_register.sp_mem_addr_reg;
		rsp_info.SP_DRAM_ADDR_REG = &sp_register.sp_dram_addr_reg;
		rsp_info.SP_RD_LEN_REG = &sp_register.sp_rd_len_reg;
		rsp_info.SP_WR_LEN_REG = &sp_register.sp_wr_len_reg;
		rsp_info.SP_STATUS_REG = &sp_register.sp_status_reg;
		rsp_info.SP_DMA_FULL_REG = &sp_register.sp_dma_full_reg;
		rsp_info.SP_DMA_BUSY_REG = &sp_register.sp_dma_busy_reg;
		rsp_info.SP_PC_REG = &rsp_register.rsp_pc;
		rsp_info.SP_SEMAPHORE_REG = &sp_register.sp_semaphore_reg;
		rsp_info.DPC_START_REG = &dpc_register.dpc_start;
		rsp_info.DPC_END_REG = &dpc_register.dpc_end;
		rsp_info.DPC_CURRENT_REG = &dpc_register.dpc_current;
		rsp_info.DPC_STATUS_REG = &dpc_register.dpc_status;
		rsp_info.DPC_CLOCK_REG = &dpc_register.dpc_clock;
		rsp_info.DPC_BUFBUSY_REG = &dpc_register.dpc_bufbusy;
		rsp_info.DPC_PIPEBUSY_REG = &dpc_register.dpc_pipebusy;
		rsp_info.DPC_TMEM_REG = &dpc_register.dpc_tmem;
		rsp_info.CheckInterrupts = sucre;
		rsp_info.ProcessDlistList = processDList;
		rsp_info.ProcessAlistList = processAList;
		rsp_info.ProcessRdpList = processRDPList;
		rsp_info.ShowCFB = showCFB;
		initiateRSP(rsp_info, (DWORD*)&i);
	} else {
		closeDLL_RSP = dummy_void;
		doRspCycles = dummy_doRspCycles;
		initiateRSP = dummy_initiateRSP;
		romClosed_RSP = dummy_void;
	}
	return 0;
}

int load_plugins() {
	HMODULE handle_gfx, handle_input, handle_sound, handle_rsp;

	DEFAULT_ROM_SETTINGS TempRomSettings;

	TempRomSettings = GetDefaultRomSettings((char*)ROM_HEADER->nom);
	if (!Config.use_global_plugins) {
		handle_gfx = get_handle(liste_plugins, TempRomSettings.GfxPluginName);
		if (handle_gfx == NULL) { handle_gfx = get_handle(liste_plugins, gfx_name); } else { sprintf(gfx_name, TempRomSettings.GfxPluginName); }

		handle_input = get_handle(liste_plugins, TempRomSettings.InputPluginName);
		if (handle_input == NULL) handle_input = get_handle(liste_plugins, input_name);
		else { sprintf(input_name, TempRomSettings.InputPluginName); }

		handle_sound = get_handle(liste_plugins, TempRomSettings.SoundPluginName);
		if (handle_sound == NULL) handle_sound = get_handle(liste_plugins, sound_name);
		else { sprintf(sound_name, TempRomSettings.SoundPluginName); }

		handle_rsp = get_handle(liste_plugins, TempRomSettings.RspPluginName);
		if (handle_rsp == NULL) handle_rsp = get_handle(liste_plugins, rsp_name);
		else { sprintf(rsp_name, TempRomSettings.RspPluginName); }
	} else {
		handle_gfx = get_handle(liste_plugins, gfx_name);
		handle_input = get_handle(liste_plugins, input_name);
		handle_sound = get_handle(liste_plugins, sound_name);
		handle_rsp = get_handle(liste_plugins, rsp_name);
	}
	ThreadFuncState = TFS_LOADGFX;
	ShowInfo("Loading gfx -  %s", gfx_name);
	load_gfx(handle_gfx);
	ThreadFuncState = TFS_LOADINPUT;
	ShowInfo("Loading input -  %s", input_name);
	load_input(handle_input);
	ThreadFuncState = TFS_LOADAUDIO;
	ShowInfo("Loading sound - %s", sound_name);
	load_sound(handle_sound);
	ThreadFuncState = TFS_LOADRSP;
	ShowInfo("Loading RSP - %s", rsp_name);
	load_rsp(handle_rsp);

	return (1);
}


void WaitEmuThread() {
	DWORD ExitCode;
	int count;

	for (count = 0; count < 400; count++) {
		SleepEx(50, TRUE);
		GetExitCodeThread(EmuThreadHandle, &ExitCode);
		if (ExitCode != STILL_ACTIVE) {
			EmuThreadHandle = NULL;
//			count = 100;
			break;
		}
	}
	if (EmuThreadHandle != NULL || ExitCode != 0) {

		if (EmuThreadHandle != NULL) {
			ShowError("Abnormal emu thread termination!");
			TerminateThread(EmuThreadHandle, 0);
			EmuThreadHandle = NULL;
		}

		char str[256];
		sprintf(str, "There was a problem with %s.", ThreadFuncStateDescription[ThreadFuncState]);
		ShowError(str);
		sprintf(str, "%s\nYou should quit and re-open the emulator before doing anything else,\nor you may encounter serious errors.", str);
		MessageBox(NULL, str, "Warning", MB_OK);
	}
	emu_launched = 0;
	emu_paused = 1;
}


void resumeEmu(BOOL quiet) {
	BOOL wasPaused = emu_paused;
	if (emu_launched) {
		if (!quiet)
			ShowInfo("Resume emulation");
		emu_paused = 0;
		//		ResumeThread(EmuThreadHandle);
		ResumeThread(SoundThreadHandle);
		if (!quiet)
			SetStatusTranslatedString(hStatus, 0, "Emulation started");
		SendMessage(hTool, TB_CHECKBUTTON, EMU_PAUSE, 0);
		SendMessage(hTool, TB_CHECKBUTTON, EMU_PLAY, 1);
	} else {
		SendMessage(hTool, TB_CHECKBUTTON, EMU_PLAY, 0);
	}

	if (emu_paused != wasPaused && !quiet)
		CheckMenuItem(GetMenu(mainHWND), EMU_PAUSE, MF_BYCOMMAND | (emu_paused ? MFS_CHECKED : MFS_UNCHECKED));
}

//void autoPauseEmu(flag)
//{
//    if (flag) {  //Auto Pause emulator
//        AutoPause = 1;
//        if (!emu_paused) {
//                pauseEmu(FALSE);
//        }
//    }
//    else {
//        if (AutoPause&&emu_paused) {
//                resumeEmu(FALSE);
//        }
//        AutoPause = 0;
//    }    
//}


void pauseEmu(BOOL quiet) {
	BOOL wasPaused = emu_paused;
	if (emu_launched) {
		VCR_updateFrameCounter();
		if (!quiet)
			ShowInfo("Pause emulation");
		emu_paused = 1;
//		SuspendThread(EmuThreadHandle);
		if (!quiet) // HACK (not a typo) seems to help avoid a race condition that permanently disables sound when doing frame advance
			SuspendThread(SoundThreadHandle);
		if (!quiet)
			SetStatusTranslatedString(hStatus, 0, "Emulation paused");

		SendMessage(hTool, TB_CHECKBUTTON, EMU_PAUSE, 1);
		SendMessage(hTool, TB_CHECKBUTTON, EMU_PLAY, 0);
		SendMessage(hTool, TB_CHECKBUTTON, EMU_STOP, 0);
	} else {
		SendMessage(hTool, TB_CHECKBUTTON, EMU_PAUSE, 0);
		CheckMenuItem(GetMenu(mainHWND), EMU_PAUSE, MF_BYCOMMAND | MFS_UNCHECKED);
	}

	if (emu_paused != wasPaused && !MenuPaused)
		CheckMenuItem(GetMenu(mainHWND), EMU_PAUSE, MF_BYCOMMAND | (emu_paused ? MFS_CHECKED : MFS_UNCHECKED));
}

BOOL StartRom(char* fullRomPath) {
	if (romBrowserBusy) {
		display_status("Rom browser busy!");
		return TRUE;
	}
	LONG winstyle;
	if (emu_launched) {
		   /*closeRom();*/
		really_restart_mode = TRUE;
		strcpy(LastSelectedRom, fullRomPath);
		CreateThread(NULL, 0, closeRom, NULL, 0, &Id);
	}
	//if (emu_paused) resumeEmu(1);
//     while (emu_launched) {
//		SleepEx(10, TRUE);
//	 }
	else {
						//Makes window not resizable                         
						//ShowWindow(mainHWND, FALSE);
		winstyle = GetWindowLong(mainHWND, GWL_STYLE);
		winstyle = winstyle & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
		SetWindowLong(mainHWND, GWL_STYLE, winstyle);
		SetWindowPos(mainHWND, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);  //Set on top
		//ShowWindow(mainHWND, TRUE);

		if (!restart_mode) {
			if (!check_plugins()) {
				return TRUE;
			}

			SetStatusMode(1);

			SetStatusTranslatedString(hStatus, 0, "Loading ROM...");
			SendMessage(hStatus, SB_SETTEXT, 2, (LPARAM)fullRomPath);

			if (rom_read(fullRomPath)) {
				emu_launched = 0;
				saveMD5toCache(ROM_SETTINGS.MD5);
				SetStatusMode(0);
				SetStatusTranslatedString(hStatus, 0, "Failed to open rom");
				return TRUE;
			}

			sprintf(LastSelectedRom, fullRomPath);
			saveMD5toCache(ROM_SETTINGS.MD5);
			AddToRecentList(mainHWND, fullRomPath);

			InitTimer();

			EnableEmulationMenuItems(TRUE);
			ShowRomBrowser(FALSE, FALSE);
			SaveGlobalPlugins(TRUE);
		}
		ShowInfo("");
		ShowWarning("Starting ROM: %s ", ROM_SETTINGS.goodname);

		SetStatusMode(2);

		ShowInfo("Creating emulation thread...");
		EmuThreadHandle = CreateThread(NULL, 0, ThreadFunc, NULL, 0, &Id);

		extern int m_task;
		if (m_task == 0) {
			sprintf(TempMessage, MUPEN_VERSION " - %s", ROM_HEADER->nom);
			SetWindowText(mainHWND, TempMessage);
		}
		SendMessage(hTool, TB_CHECKBUTTON, EMU_PLAY, 1);
		SetStatusTranslatedString(hStatus, 0, "Emulation started");
		printf("Thread created\n");
		EnableEmulationMenuItems(TRUE);
		return FALSE;
	}
	return 0;
}

void exit_emu2();
static int shut_window = 0;

//void closeRom()
DWORD WINAPI closeRom(LPVOID lpParam) //lpParam - treated as bool, show romlist? 
{
	LONG winstyle;                                //Used for setting new style to the window
 //   int browserwidths[] = {400, -1};              //Rombrowser statusbar

	if (emu_launched) {
		if (emu_paused) {
			MenuPaused = FALSE;
			resumeEmu(FALSE);
		}

		if (recording && !continue_vcr_on_restart_mode) {
		   //Sleep(1000); // HACK - seems to help crash on closing ROM during capture...?
			if (VCR_stopCapture() != 0) // but it never returns non-zero ???
				MessageBox(NULL, "Couldn't stop capturing", "VCR", MB_OK);
			else {
				SetWindowPos(mainHWND, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				SetStatusTranslatedString(hStatus, 0, "Stopped AVI capture");
				recording = FALSE;
			}
		}


		ShowInfo("Closing emulation thread...");
		stop_it();

		WaitEmuThread();

		EndGameKaillera();

	  /*romClosed_input();
	  ShowInfo("Emu thread: romClosed (input plugin)");
	  romClosed_gfx();
	  ShowInfo("Emu thread: romClosed (gfx plugin)");
	  romClosed_audio();
	  ShowInfo("Emu thread: romClosed (audio plugin)");
	  romClosed_RSP();
	  ShowInfo("Emu thread: romClosed (RSP plugin)");
	  closeDLL_RSP();
	  ShowInfo("Emu thread: RSP plugin closed");
	  closeDLL_input();
	  ShowInfo("Emu thread: input plugin closed");
	  closeDLL_gfx();
	  ShowInfo("Emu thread: gfx plugin closed");
	  closeDLL_audio();
	  ShowInfo("Emu thread: audio plugin closed");*/

		if (!restart_mode) {
			ShowInfo("Free rom and memory....");
			free(rom);
			rom = NULL;
			free(ROM_HEADER);
			ROM_HEADER = NULL;
			free_memory();

			ShowInfo("Init emulation menu items....");
			EnableEmulationMenuItems(FALSE);
			SaveGlobalPlugins(FALSE);
			ShowRomBrowser(!really_restart_mode, !!lpParam);

			extern int m_task;
			if (m_task == 0) {
				SetWindowText(mainHWND, MUPEN_VERSION);
				SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)" ");
			}
		}
		ShowInfo("Rom closed.");

		if (shut_window) {
		  //exit_emu2();
		  //SleepEx(100,TRUE);
			SendMessage(mainHWND, WM_CLOSE, 0, 0);
			return 0;
		}
	}

	SetStatusMode(0);
	SetStatusTranslatedString(hStatus, 0, "Rom Closed");

	//Makes window resizable                         
	winstyle = GetWindowLong(mainHWND, GWL_STYLE);
	winstyle |= WS_THICKFRAME;
	SetWindowLong(mainHWND, GWL_STYLE, winstyle);
	SetWindowPos(mainHWND, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);  //Set on top

	{
		if (really_restart_mode) {


			if (clear_sram_on_restart_mode) {
				VCR_clearAllSaveData();
				clear_sram_on_restart_mode = FALSE;
			}

			really_restart_mode = FALSE;
			extern int m_task;
			if (m_task != 0)
				just_restarted_flag = TRUE;
			if (StartRom(LastSelectedRom)) { // If rom loading fails
				closeRom(lpParam);
				ShowRomBrowser(TRUE, TRUE);
				SetStatusTranslatedString(hStatus, 0, "Failed to open rom");
			}
		}
	}

	continue_vcr_on_restart_mode = FALSE;

	return 0;
}

void resetEmu() {
	 // why is it so damned difficult to reset the game?
	 // right now it's hacked to exit to the GUI then re-load the ROM,
	 // but it should be possible to reset the game while it's still running
	 // simply by clearing out some memory and maybe notifying the plugins...
	if (emu_launched) {
		extern int frame_advancing;
		frame_advancing = false;
		ShowInfo("Restart Rom");
		restart_mode = 0;
		really_restart_mode = TRUE;
		MenuPaused = FALSE;
		CreateThread(NULL, 0, closeRom, NULL, 0, &Id);
	}

}

void setDefaultPlugins() {
	ReadCfgString("Plugins", "Graphics", "", gfx_name);
	ReadCfgString("Plugins", "Sound", "", sound_name);
	ReadCfgString("Plugins", "Input", "", input_name);
	ReadCfgString("Plugins", "RSP", "", rsp_name);
	rewind_plugin();
}

void ShowMessage(const char* lpszMessage) {
	MessageBox(NULL, lpszMessage, "Info", MB_OK);
}

void CreateToolBarWindow(HWND hwnd) {
	TBBUTTON tbButtons[] =
	{
		{0, IDLOAD, TBSTATE_ENABLED, TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
		{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0, 0}, 0, 0},
		{1, EMU_PLAY, TBSTATE_ENABLED, TBSTYLE_CHECK | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
		{2, EMU_PAUSE, TBSTATE_ENABLED, TBSTYLE_CHECK | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
		{3, EMU_STOP, TBSTATE_ENABLED, TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
		{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0, 0}, 0, 0},
		{4, FULL_SCREEN, TBSTATE_ENABLED, TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
		{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0, 0}, 0, 0},
		{5, IDGFXCONFIG, TBSTATE_ENABLED, TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
		{6, IDSOUNDCONFIG, TBSTATE_ENABLED, TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
		{7, IDINPUTCONFIG, TBSTATE_ENABLED, TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
		{8, IDRSPCONFIG, TBSTATE_ENABLED, TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
		{9, ID_LOAD_CONFIG, TBSTATE_ENABLED, TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
	};
	auto tbButtonsCount = sizeof(tbButtons) / sizeof(TBBUTTON);
	hTool = CreateToolbarEx(hwnd,
		WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS /* | */
		/*CS_ADJUSTABLE    no this causes a bug...*/,
		IDC_TOOLBAR, 10, app_hInstance, IDB_TOOLBAR,
		tbButtons, tbButtonsCount, 16, 16, tbButtonsCount * 16, 16, sizeof(TBBUTTON));

	if (hTool == NULL)
		MessageBox(hwnd, "Could not create tool bar.", "Error", MB_OK | MB_ICONERROR);


	if (emu_launched) {
		if (emu_paused) {
			SendMessage(hTool, TB_CHECKBUTTON, EMU_PAUSE, 1);
		} else {
			SendMessage(hTool, TB_CHECKBUTTON, EMU_PLAY, 1);
		}
	} else {
		getSelectedRom();     //Used for enabling/disabling the play button
		//SendMessage( hTool, TB_ENABLEBUTTON, EMU_PLAY, FALSE );
		SendMessage(hTool, TB_ENABLEBUTTON, EMU_STOP, FALSE);
		SendMessage(hTool, TB_ENABLEBUTTON, EMU_PAUSE, FALSE);
		SendMessage(hTool, TB_ENABLEBUTTON, FULL_SCREEN, FALSE);
	}

}

void CreateStatusBarWindow(HWND hwnd) {
   //Create Status bar
	hStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL,
		WS_CHILD | WS_VISIBLE /*| SBARS_SIZEGRIP*/, 0, 0, 0, 0,
		hwnd, (HMENU)IDC_MAIN_STATUS, GetModuleHandle(NULL), NULL);

	if (emu_launched) SetStatusMode(2);
	else SetStatusMode(0);
}

int pauseAtFrame = -1;

/// <summary>
/// Helper function because this is repeated multiple times
/// </summary>
void SetStatusPlaybackStarted() {
	HMENU hMenu = GetMenu(mainHWND);
	EnableMenuItem(hMenu, ID_STOP_RECORD, MF_GRAYED);
	EnableMenuItem(hMenu, ID_STOP_PLAYBACK, MF_ENABLED);

	if (!emu_paused || !emu_launched)
		SetStatusTranslatedString(hStatus, 0, "Playback started...");
	else
		SetStatusTranslatedString(hStatus, 0, "Playback started. (Paused)");
}

LRESULT CALLBACK PlayMovieProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	char tempbuf[MAX_PATH];
	char tempbuf2[MAX_PATH];

	HWND descriptionDialog;
	HWND authorDialog;

//    md5_byte_t digest[16];
//    ROM_INFO *pRomInfo;
//    char tempname[100];
//    HWND hwndPB;    
//    pRomInfo = getSelectedRom();
//    DEFAULT_ROM_SETTINGS TempRomSettings;

	static char path_buffer[_MAX_PATH];

//    if (pRomInfo==NULL) { 
//       EndDialog(hwnd, IDOK);
//       return FALSE; 
//      }
	switch (Message) {
		case WM_INITDIALOG:
			descriptionDialog = GetDlgItem(hwnd, IDC_INI_DESCRIPTION);
			authorDialog = GetDlgItem(hwnd, IDC_INI_AUTHOR);

			SendMessage(descriptionDialog, EM_SETLIMITTEXT, MOVIE_DESCRIPTION_DATA_SIZE, 0);
			SendMessage(authorDialog, EM_SETLIMITTEXT, MOVIE_AUTHOR_DATA_SIZE, 0);

			CountryCodeToCountryName(ROM_HEADER->Country_code, tempbuf2);
			sprintf(tempbuf, "%s (%s)", ROM_HEADER->nom, tempbuf2);
			strcat(tempbuf, ".m64");
			SetDlgItemText(hwnd, IDC_INI_MOVIEFILE, tempbuf);

			SetDlgItemText(hwnd, IDC_ROM_INTERNAL_NAME2, (CHAR*)ROM_HEADER->nom);

			CountryCodeToCountryName(ROM_HEADER->Country_code, tempbuf);
			SetDlgItemText(hwnd, IDC_ROM_COUNTRY2, tempbuf);

			sprintf(tempbuf, "%X", (unsigned int)ROM_HEADER->CRC1);
			SetDlgItemText(hwnd, IDC_ROM_CRC3, tempbuf);

			SetDlgItemText(hwnd, IDC_MOVIE_VIDEO_TEXT2, gfx_name);
			SetDlgItemText(hwnd, IDC_MOVIE_INPUT_TEXT2, input_name);
			SetDlgItemText(hwnd, IDC_MOVIE_SOUND_TEXT2, sound_name);
			SetDlgItemText(hwnd, IDC_MOVIE_RSP_TEXT2, rsp_name);

			strcpy(tempbuf, Controls[0].Present ? "Present" : "Disconnected");
			if (Controls[0].Present && Controls[0].Plugin == PLUGIN_MEMPAK)
				strcat(tempbuf, " with mempak");
			if (Controls[0].Present && Controls[0].Plugin == PLUGIN_RUMBLE_PAK)
				strcat(tempbuf, " with rumble");
			SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER1_TEXT2, tempbuf);

			strcpy(tempbuf, Controls[1].Present ? "Present" : "Disconnected");
			if (Controls[1].Present && Controls[1].Plugin == PLUGIN_MEMPAK)
				strcat(tempbuf, " with mempak");
			if (Controls[1].Present && Controls[1].Plugin == PLUGIN_RUMBLE_PAK)
				strcat(tempbuf, " with rumble pak");
			SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER2_TEXT2, tempbuf);

			strcpy(tempbuf, Controls[2].Present ? "Present" : "Disconnected");
			if (Controls[2].Present && Controls[2].Plugin == PLUGIN_MEMPAK)
				strcat(tempbuf, " with mempak");
			if (Controls[2].Present && Controls[2].Plugin == PLUGIN_RUMBLE_PAK)
				strcat(tempbuf, " with rumble pak");
			SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER3_TEXT2, tempbuf);

			strcpy(tempbuf, Controls[3].Present ? "Present" : "Disconnected");
			if (Controls[3].Present && Controls[3].Plugin == PLUGIN_MEMPAK)
				strcat(tempbuf, " with mempak");
			if (Controls[3].Present && Controls[3].Plugin == PLUGIN_RUMBLE_PAK)
				strcat(tempbuf, " with rumble pak");
			SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER4_TEXT2, tempbuf);

			CheckDlgButton(hwnd, IDC_MOVIE_READONLY, VCR_getReadOnly());


			SetFocus(GetDlgItem(hwnd, IDC_INI_MOVIEFILE));


			goto refresh; // better than making it a macro or zillion-argument function

			return FALSE;
		case WM_CLOSE:
			if (!emu_launched) {
				ShowWindow(hRomList, FALSE);
				ShowWindow(hRomList, TRUE);
			}
			EndDialog(hwnd, IDOK);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_OK:
				case IDOK:
				{
					VCR_coreStopped();
					{
						BOOL success;
						unsigned int num = GetDlgItemInt(hwnd, IDC_PAUSEAT_FIELD, &success, TRUE);
						if (((signed int)num) >= 0 && success)
							pauseAtFrame = num;
						else
							pauseAtFrame = -1;
					}
					GetDlgItemText(hwnd, IDC_INI_MOVIEFILE, tempbuf, MAX_PATH);

					// turn WCHAR into UTF8
					WCHAR authorWC[MOVIE_AUTHOR_DATA_SIZE];
					char authorUTF8[MOVIE_AUTHOR_DATA_SIZE * 4];
					if (GetDlgItemTextW(hwnd, IDC_INI_AUTHOR, authorWC, MOVIE_AUTHOR_DATA_SIZE))
						WideCharToMultiByte(CP_UTF8, 0, authorWC, -1, authorUTF8, sizeof(authorUTF8), NULL, NULL);
					else
						GetDlgItemTextA(hwnd, IDC_INI_AUTHOR, authorUTF8, MOVIE_AUTHOR_DATA_SIZE);

					WCHAR descriptionWC[MOVIE_DESCRIPTION_DATA_SIZE];
					char descriptionUTF8[MOVIE_DESCRIPTION_DATA_SIZE * 4];
					if (GetDlgItemTextW(hwnd, IDC_INI_DESCRIPTION, descriptionWC, MOVIE_DESCRIPTION_DATA_SIZE))
						WideCharToMultiByte(CP_UTF8, 0, descriptionWC, -1, descriptionUTF8, sizeof(descriptionUTF8), NULL, NULL);
					else
						GetDlgItemTextA(hwnd, IDC_INI_DESCRIPTION, descriptionUTF8, MOVIE_DESCRIPTION_DATA_SIZE);

					VCR_setReadOnly(IsDlgButtonChecked(hwnd, IDC_MOVIE_READONLY));

					auto playbackResult = VCR_startPlayback(tempbuf, authorUTF8, descriptionUTF8);

					if (strlen(tempbuf) == 0 || playbackResult != VCR_PLAYBACK_SUCCESS) {
						char errorString[MAX_PATH];

						sprintf(errorString, "Failed to start movie \"%s\" ", tempbuf);

						switch (playbackResult) {
							case VCR_PLAYBACK_ERROR:
								strcat(errorString, " - unknown error");
								break;
							case VCR_PLAYBACK_SAVESTATE_MISSING:
								strcat(errorString, " - savestate is missing");
								break;
							case VCR_PLAYBACK_FILE_BUSY:
								strcat(errorString, " - file is locked");
								break;
							case VCR_PLAYBACK_INCOMPATIBLE:
								strcat(errorString, " - configuration incompatibility");
								break;
						}

						MessageBox(hwnd, errorString, "VCR", MB_OK);
						break;
					} else {
						SetStatusPlaybackStarted();
						resumeEmu(TRUE); // Unpause emu if it was paused before
					}
					//                    GetDlgItemText(hwnd, IDC_INI_COMMENTS, (LPSTR) TempMessage, 128 );
					//                    setIniComments(pRomInfo,TempMessage);
					//                    strncpy(pRomInfo->UserNotes,TempMessage,sizeof(pRomInfo->UserNotes));
					if (!emu_launched) {                    //Refreshes the ROM Browser
						ShowWindow(hRomList, FALSE);
						ShowWindow(hRomList, TRUE);
					}
					EndDialog(hwnd, IDOK);
					//                    romInfoHWND = NULL; 
				}
				break;
				case IDC_CANCEL:
				case IDCANCEL:
					if (!emu_launched) {
						ShowWindow(hRomList, FALSE);
						ShowWindow(hRomList, TRUE);
					}
					EndDialog(hwnd, IDOK);
//                    romInfoHWND = NULL;
					break;
				case IDC_MOVIE_BROWSE:
				{
				 // The default directory we open the file dialog window in is
				 // the parent directory of the last movie that the user ran
					GetDefaultFileDialogPath(path_buffer, Config.recent_movie_paths[0]);

					if (fdBrowseMovie.ShowFileDialog(path_buffer, L"*.m64;*.rec", TRUE, FALSE, hwnd))
						SetDlgItemText(hwnd, IDC_INI_MOVIEFILE, path_buffer);
				}
				goto refresh;
				break;
				case IDC_MOVIE_REFRESH:
					goto refresh;
					break;
			}
			break;
	}
	return FALSE;

refresh:


	ShowInfo("[VCR]:refreshing movie info...");


	GetDlgItemText(hwnd, IDC_INI_MOVIEFILE, tempbuf, MAX_PATH);
	SMovieHeader m_header = VCR_getHeaderInfo(tempbuf);

	SetDlgItemText(hwnd, IDC_ROM_INTERNAL_NAME, m_header.romNom);

	CountryCodeToCountryName(m_header.romCountry, tempbuf);
	SetDlgItemText(hwnd, IDC_ROM_COUNTRY, tempbuf);

	sprintf(tempbuf, "%X", (unsigned int)m_header.romCRC);
	SetDlgItemText(hwnd, IDC_ROM_CRC, tempbuf);

//ShowInfo("refreshing movie plugins...\n");

	SetDlgItemText(hwnd, IDC_MOVIE_VIDEO_TEXT, m_header.videoPluginName);
	SetDlgItemText(hwnd, IDC_MOVIE_INPUT_TEXT, m_header.inputPluginName);
	SetDlgItemText(hwnd, IDC_MOVIE_SOUND_TEXT, m_header.soundPluginName);
	SetDlgItemText(hwnd, IDC_MOVIE_RSP_TEXT, m_header.rspPluginName);

//ShowInfo("refreshing movie controllers...\n");

	strcpy(tempbuf, (m_header.controllerFlags & CONTROLLER_1_PRESENT) ? "Present" : "Disconnected");
	if (m_header.controllerFlags & CONTROLLER_1_MEMPAK)
		strcat(tempbuf, " with mempak");
	if (m_header.controllerFlags & CONTROLLER_1_RUMBLE)
		strcat(tempbuf, " with rumble");
	SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER1_TEXT, tempbuf);

	strcpy(tempbuf, (m_header.controllerFlags & CONTROLLER_2_PRESENT) ? "Present" : "Disconnected");
	if (m_header.controllerFlags & CONTROLLER_2_MEMPAK)
		strcat(tempbuf, " with mempak");
	if (m_header.controllerFlags & CONTROLLER_2_RUMBLE)
		strcat(tempbuf, " with rumble");
	SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER2_TEXT, tempbuf);

	strcpy(tempbuf, (m_header.controllerFlags & CONTROLLER_3_PRESENT) ? "Present" : "Disconnected");
	if (m_header.controllerFlags & CONTROLLER_3_MEMPAK)
		strcat(tempbuf, " with mempak");
	if (m_header.controllerFlags & CONTROLLER_3_RUMBLE)
		strcat(tempbuf, " with rumble");
	SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER3_TEXT, tempbuf);

	strcpy(tempbuf, (m_header.controllerFlags & CONTROLLER_4_PRESENT) ? "Present" : "Disconnected");
	if (m_header.controllerFlags & CONTROLLER_4_MEMPAK)
		strcat(tempbuf, " with mempak");
	if (m_header.controllerFlags & CONTROLLER_4_RUMBLE)
		strcat(tempbuf, " with rumble");
	SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER4_TEXT, tempbuf);

//ShowInfo("refreshing movie start/frames...\n");

	SetDlgItemText(hwnd, IDC_FROMSNAPSHOT_TEXT, (m_header.startFlags & MOVIE_START_FROM_SNAPSHOT) ? "Savestate" : "Start");
	if (m_header.startFlags & MOVIE_START_FROM_EEPROM) {
		SetDlgItemTextA(hwnd, IDC_FROMSNAPSHOT_TEXT, "EEPROM");
	}

	sprintf(tempbuf, "%u  (%u input)", (int)m_header.length_vis, (int)m_header.length_samples);
	SetDlgItemText(hwnd, IDC_MOVIE_FRAMES, tempbuf);

//ShowInfo("calculating movie length...\n");

	if (m_header.vis_per_second == 0)
		m_header.vis_per_second = 60;

	double seconds = (double)m_header.length_vis / (double)m_header.vis_per_second;
	double minutes = seconds / 60.0;
	if (seconds)
		seconds = fmod(seconds, 60.0);
	double hours = minutes / 60.0;
	if (minutes)
		minutes = fmod(minutes, 60.0);

//ShowInfo("refreshing movie length...\n");

	if (hours >= 1.0)
		sprintf(tempbuf, "%d hours and %.1f minutes", (unsigned int)hours, (float)minutes);
	else if (minutes >= 1.0)
		sprintf(tempbuf, "%d minutes and %.0f seconds", (unsigned int)minutes, (float)seconds);
	else if (m_header.length_vis != 0)
		sprintf(tempbuf, "%.1f seconds", (float)seconds);
	else
		strcpy(tempbuf, "0 seconds");
	SetDlgItemText(hwnd, IDC_MOVIE_LENGTH, tempbuf);

//ShowInfo("refreshing movie rerecords...\n");

	sprintf(tempbuf, "%u", m_header.rerecord_count);
	SetDlgItemText(hwnd, IDC_MOVIE_RERECORDS, tempbuf);

	ShowInfo("[VCR]:refreshing movie author and description...");

	//	SetDlgItemText(hwnd,IDC_INI_AUTHOR,m_header.authorInfo);
	//	SetDlgItemText(hwnd,IDC_INI_DESCRIPTION,m_header.description);

	{

		// convert utf8 metadata to windows widechar
		WCHAR wszMeta[MOVIE_MAX_METADATA_SIZE];
		if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, m_header.authorInfo, -1, wszMeta, MOVIE_AUTHOR_DATA_SIZE)) {
			SetLastError(0);
			SetWindowTextW(GetDlgItem(hwnd, IDC_INI_AUTHOR), wszMeta);
			if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {
				// not implemented on this system - convert as best we can to 1-byte characters and set with that
				// TODO: load unicows.dll instead so SetWindowTextW won't fail even on Win98/ME
				char ansiStr[MOVIE_AUTHOR_DATA_SIZE];
				WideCharToMultiByte(CP_ACP, 0, wszMeta, -1, ansiStr, MOVIE_AUTHOR_DATA_SIZE, NULL, NULL);
				SetWindowTextA(GetDlgItem(hwnd, IDC_INI_AUTHOR), ansiStr);

				if (strlen(ansiStr) == 0)
					SetWindowTextA(GetDlgItem(hwnd, IDC_INI_AUTHOR), "(too lazy to type name)");

				SetLastError(0);
			} else {
				if (wcslen(wszMeta) == 0)
					SetWindowTextW(GetDlgItem(hwnd, IDC_INI_AUTHOR), L"(too lazy to type name)");
			}
		}
		if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, m_header.description, -1, wszMeta, MOVIE_DESCRIPTION_DATA_SIZE)) {
			SetWindowTextW(GetDlgItem(hwnd, IDC_INI_DESCRIPTION), wszMeta);
			if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {
				char ansiStr[MOVIE_DESCRIPTION_DATA_SIZE];
				WideCharToMultiByte(CP_ACP, 0, wszMeta, -1, ansiStr, MOVIE_DESCRIPTION_DATA_SIZE, NULL, NULL);
				SetWindowTextA(GetDlgItem(hwnd, IDC_INI_DESCRIPTION), ansiStr);

				if (strlen(ansiStr) == 0)
					SetWindowTextA(GetDlgItem(hwnd, IDC_INI_DESCRIPTION), "(no description entered)");

				SetLastError(0);
			} else {
				if (wcslen(wszMeta) == 0)
					SetWindowTextW(GetDlgItem(hwnd, IDC_INI_DESCRIPTION), L"(no description entered)");
			}
		}
	}

	ShowInfo("[VCR]:done refreshing");

	return FALSE;
}

LRESULT CALLBACK RecordMovieProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	char tempbuf[MAX_PATH];
	char tempbuf2[MAX_PATH];
	int checked_movie_type;
	HWND descriptionDialog;
	HWND authorDialog;

	switch (Message) {
		case WM_INITDIALOG:

			checked_movie_type = Config.last_movie_type;
			descriptionDialog = GetDlgItem(hwnd, IDC_INI_DESCRIPTION);
			authorDialog = GetDlgItem(hwnd, IDC_INI_AUTHOR);

			SendMessage(descriptionDialog, EM_SETLIMITTEXT, MOVIE_DESCRIPTION_DATA_SIZE, 0);
			SendMessage(authorDialog, EM_SETLIMITTEXT, MOVIE_AUTHOR_DATA_SIZE, 0);

			SetDlgItemText(hwnd, IDC_INI_AUTHOR, Config.last_movie_author);
			SetDlgItemText(hwnd, IDC_INI_DESCRIPTION, "");

			CheckRadioButton(hwnd, IDC_FROMSNAPSHOT_RADIO, IDC_FROMSTART_RADIO, checked_movie_type);

			CountryCodeToCountryName(ROM_HEADER->Country_code, tempbuf2);
			sprintf(tempbuf, "%s (%s)", ROM_HEADER->nom, tempbuf2);
			strcat(tempbuf, ".m64");
			SetDlgItemText(hwnd, IDC_INI_MOVIEFILE, tempbuf);

			SetDlgItemText(hwnd, IDC_ROM_INTERNAL_NAME2, (CHAR*)ROM_HEADER->nom);

			CountryCodeToCountryName(ROM_HEADER->Country_code, tempbuf);
			SetDlgItemText(hwnd, IDC_ROM_COUNTRY2, tempbuf);

			sprintf(tempbuf, "%X", (unsigned int)ROM_HEADER->CRC1);
			SetDlgItemText(hwnd, IDC_ROM_CRC3, tempbuf);

			SetDlgItemText(hwnd, IDC_MOVIE_VIDEO_TEXT2, gfx_name);
			SetDlgItemText(hwnd, IDC_MOVIE_INPUT_TEXT2, input_name);
			SetDlgItemText(hwnd, IDC_MOVIE_SOUND_TEXT2, sound_name);
			SetDlgItemText(hwnd, IDC_MOVIE_RSP_TEXT2, rsp_name);

			strcpy(tempbuf, Controls[0].Present ? "Present" : "Disconnected");
			if (Controls[0].Present && Controls[0].Plugin == PLUGIN_MEMPAK)
				strcat(tempbuf, " with mempak");
			if (Controls[0].Present && Controls[0].Plugin == PLUGIN_RUMBLE_PAK)
				strcat(tempbuf, " with rumble");
			SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER1_TEXT2, tempbuf);

			strcpy(tempbuf, Controls[1].Present ? "Present" : "Disconnected");
			if (Controls[1].Present && Controls[1].Plugin == PLUGIN_MEMPAK)
				strcat(tempbuf, " with mempak");
			if (Controls[1].Present && Controls[1].Plugin == PLUGIN_RUMBLE_PAK)
				strcat(tempbuf, " with rumble pak");
			SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER2_TEXT2, tempbuf);

			strcpy(tempbuf, Controls[2].Present ? "Present" : "Disconnected");
			if (Controls[2].Present && Controls[2].Plugin == PLUGIN_MEMPAK)
				strcat(tempbuf, " with mempak");
			if (Controls[2].Present && Controls[2].Plugin == PLUGIN_RUMBLE_PAK)
				strcat(tempbuf, " with rumble pak");
			SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER3_TEXT2, tempbuf);

			strcpy(tempbuf, Controls[3].Present ? "Present" : "Disconnected");
			if (Controls[3].Present && Controls[3].Plugin == PLUGIN_MEMPAK)
				strcat(tempbuf, " with mempak");
			if (Controls[3].Present && Controls[3].Plugin == PLUGIN_RUMBLE_PAK)
				strcat(tempbuf, " with rumble pak");
			SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER4_TEXT2, tempbuf);

			EnableWindow(GetDlgItem(hwnd, IDC_EXTSAVESTATE), 0); // workaround because initial selected button is "Start"

			SetFocus(GetDlgItem(hwnd, IDC_INI_AUTHOR));

			return FALSE;
		case WM_CLOSE:
			if (!emu_launched) {
				ShowWindow(hRomList, FALSE);
				ShowWindow(hRomList, TRUE);
			}
			EndDialog(hwnd, IDOK);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_OK:
				case IDOK:
				{

					// turn WCHAR into UTF8
					WCHAR authorWC[MOVIE_AUTHOR_DATA_SIZE];
					char authorUTF8[MOVIE_AUTHOR_DATA_SIZE * 4];
					if (GetDlgItemTextW(hwnd, IDC_INI_AUTHOR, authorWC, MOVIE_AUTHOR_DATA_SIZE))
						WideCharToMultiByte(CP_UTF8, 0, authorWC, -1, authorUTF8, sizeof(authorUTF8), NULL, NULL);
					else
						GetDlgItemTextA(hwnd, IDC_INI_AUTHOR, authorUTF8, MOVIE_AUTHOR_DATA_SIZE);

					strncpy_s(Config.last_movie_author, authorUTF8, strlen(authorUTF8));

					WCHAR descriptionWC[MOVIE_DESCRIPTION_DATA_SIZE];
					char descriptionUTF8[MOVIE_DESCRIPTION_DATA_SIZE * 4];
					if (GetDlgItemTextW(hwnd, IDC_INI_DESCRIPTION, descriptionWC, MOVIE_DESCRIPTION_DATA_SIZE))
						WideCharToMultiByte(CP_UTF8, 0, descriptionWC, -1, descriptionUTF8, sizeof(descriptionUTF8), NULL, NULL);
					else
						GetDlgItemTextA(hwnd, IDC_INI_DESCRIPTION, descriptionUTF8, MOVIE_DESCRIPTION_DATA_SIZE);


					GetDlgItemText(hwnd, IDC_INI_MOVIEFILE, tempbuf, MAX_PATH);

					// big
					checked_movie_type = IsDlgButtonChecked(hwnd, IDC_FROMSNAPSHOT_RADIO) ? IDC_FROMSNAPSHOT_RADIO : IsDlgButtonChecked(hwnd, IDC_FROMSTART_RADIO)
						? IDC_FROMSTART_RADIO : IsDlgButtonChecked(hwnd, IDC_FROMEEPROM_RADIO) ? IDC_FROMEEPROM_RADIO : IDC_FROMEXISTINGSNAPSHOT_RADIO;
					unsigned short flag = checked_movie_type == IDC_FROMSNAPSHOT_RADIO ? MOVIE_START_FROM_SNAPSHOT : checked_movie_type == IDC_FROMSTART_RADIO
						? MOVIE_START_FROM_NOTHING : checked_movie_type == IDC_FROMEEPROM_RADIO ? MOVIE_START_FROM_EEPROM : MOVIE_START_FROM_EXISTING_SNAPSHOT;
					Config.last_movie_type = checked_movie_type;

					bool allowClosing = true;
					if (flag == MOVIE_START_FROM_EXISTING_SNAPSHOT) {
						// The default directory we open the file dialog window in is the 
						// parent directory of the last savestate that the user saved or loaded
						std::filesystem::path path = Config.states_path;
						GetDefaultFileDialogPath(tempbuf, Config.states_path);

						if (fdBrowseMovie2.ShowFileDialog(tempbuf, L"*.st;*.savestate", TRUE, FALSE, hwnd)) {
							savestates_select_filename(tempbuf);
							path = tempbuf;
							path.replace_extension("m64");
							strncpy(tempbuf, path.string().c_str(), MAX_PATH);

							if (std::filesystem::exists(path)) {
								sprintf(tempbuf2, "\"%s\" already exists. Are you sure want to overwrite this movie?", tempbuf);
								if (MessageBox(hwnd, tempbuf2, "VCR", MB_YESNO) == IDNO)
									break;
							}
						} else
							allowClosing = false;
					}

					if (allowClosing) {
						if (strlen(tempbuf) == 0 || VCR_startRecord(tempbuf, flag, authorUTF8, descriptionUTF8, !IsDlgButtonChecked(hwnd, IDC_EXTSAVESTATE)) < 0) {
							sprintf(tempbuf2, "Couldn't start recording\nof \"%s\".", tempbuf);
							MessageBox(hwnd, tempbuf2, "VCR", MB_OK);
							break;
						} else {
							HMENU hMenu = GetMenu(mainHWND);
							EnableMenuItem(hMenu, ID_STOP_RECORD, MF_ENABLED);
							EnableMenuItem(hMenu, ID_STOP_PLAYBACK, MF_GRAYED);
							SetStatusTranslatedString(hStatus, 0, "Recording replay...");
						}

						if (!emu_launched) {                    //Refreshes the ROM Browser
							ShowWindow(hRomList, FALSE);
							ShowWindow(hRomList, TRUE);
						}
						EndDialog(hwnd, IDOK);
					}
				} break;
				case IDC_CANCEL:
				case IDCANCEL:
					if (!emu_launched) {
						ShowWindow(hRomList, FALSE);
						ShowWindow(hRomList, TRUE);
					}
					EndDialog(hwnd, IDOK);
					break;
				case IDC_MOVIE_BROWSE:
				{
					// The default directory we open the file dialog window in is
					// the parent directory of the last movie that the user ran
					GetDefaultFileDialogPath(tempbuf, Config.recent_movie_paths[0]); // if the first movie ends up being deleted this is fucked

					if (fdBrowseMovie2.ShowFileDialog(tempbuf, L"*.m64;*.rec", TRUE, FALSE, hwnd)) {
						if (strlen(tempbuf) > 0 && (strlen(tempbuf) < 4 || _stricmp(tempbuf + strlen(tempbuf) - 4, ".m64") != 0))
							strcat(tempbuf, ".m64");
						SetDlgItemText(hwnd, IDC_INI_MOVIEFILE, tempbuf);
					}
				}
				break;
				//case IDC_EXISTINGSAVESTATE_BROWSE:
				//{
				//    char path_buffer[MAX_PATH];
				//    if (defExt)
				//    if (fdBrowseMovie2.ShowFileDialog(path_buffer, L"*.st;*.savestate", TRUE, FALSE, hwnd)) {
				//        if (strlen(path_buffer) > 0 && (strlen(path_buffer) < 4 || _stricmp(path_buffer + strlen(path_buffer) - 4, ".m64") != 0))
				//            strcat(path_buffer, ".m64");
				//        SetDlgItemText(hwnd, IDC_INI_MOVIEFILE, path_buffer);
				//    }
				//}

				case IDC_FROMEEPROM_RADIO:
					EnableWindow(GetDlgItem(hwnd, IDC_EXTSAVESTATE), 0);
					EnableWindow(GetDlgItem(hwnd, IDC_MOVIE_BROWSE), 1);
					EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE), 1);
					EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE_TEXT), 1);
					break;
				case IDC_FROMSNAPSHOT_RADIO:
					EnableWindow(GetDlgItem(hwnd, IDC_EXTSAVESTATE), 1);
					EnableWindow(GetDlgItem(hwnd, IDC_MOVIE_BROWSE), 1);
					EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE), 1);
					EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE_TEXT), 1);
					break;
				case IDC_FROMEXISTINGSNAPSHOT_RADIO:
					EnableWindow(GetDlgItem(hwnd, IDC_EXTSAVESTATE), 1);
					EnableWindow(GetDlgItem(hwnd, IDC_MOVIE_BROWSE), 0);
					EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE), 0);
					EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE_TEXT), 0);
					break;
				case IDC_FROMSTART_RADIO:
					EnableWindow(GetDlgItem(hwnd, IDC_EXTSAVESTATE), 0);
					EnableWindow(GetDlgItem(hwnd, IDC_MOVIE_BROWSE), 1);
					EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE), 1);
					EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE_TEXT), 1);
					break;
			}
			break;
	}
	return FALSE;
}

void OpenMoviePlaybackDialog() {
	BOOL wasPaused = emu_paused && !MenuPaused;
	MenuPaused = FALSE;
	if (emu_launched && !emu_paused)
		pauseEmu(FALSE);

	DialogBox(GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDD_MOVIE_PLAYBACK_DIALOG), mainHWND, (DLGPROC)PlayMovieProc);

	if (emu_launched && emu_paused && !wasPaused)
		resumeEmu(FALSE);
}
void OpenMovieRecordDialog() {
	BOOL wasPaused = emu_paused && !MenuPaused;
	MenuPaused = FALSE;
	if (emu_launched && !emu_paused)
		pauseEmu(FALSE);

	DialogBox(GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDD_MOVIE_RECORD_DIALOG), mainHWND, (DLGPROC)RecordMovieProc);

	if (emu_launched && emu_paused && !wasPaused)
		resumeEmu(FALSE);
}


void SetStatusMode(int mode) {
	RECT rcClient;                                    //Client area of parent window 
	const int loadingwidths[] = {200, 300, -1};       //Initial statusbar
	const int emulatewidthsFPSVIS[] = {230, 300, 370, 440, -1};//Emulating statusbar with FPS and VIS
	const int emulatewidthsFPS[] = {230, 300, 370, -1};    //Emulating statusbar with FPS
	const int emulatewidths[] = {230, 300, -1};            //Emulating statusbar
	const int browserwidths[] = {400, -1};	          //Initial statusbar
	int parts;


	if (hStatusProgress) DestroyWindow(hStatusProgress);
	//if (hStatusIcon)     DeleteObject( (HGDIOBJ) hStatusIcon );
	if (hStaticHandle)   DestroyWindow(hStaticHandle);

	//Setting status widths
	if (Config.is_statusbar_enabled) {
		switch (mode) {
			case 0:                 //Rombrowser Statusbar
	/*             //Adds sizing grid
				 statusstyle = GetWindowLong( hStatus, GWL_STYLE );
				 statusstyle = statusstyle | SBARS_SIZEGRIP;
				 SetWindowLong( hStatus, GWL_STYLE, statusstyle );
				 SetWindowPos( hStatus, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED );  //Set on top
	*/
				SendMessage(hStatus, SB_SETPARTS, sizeof(browserwidths) / sizeof(int), (LPARAM)browserwidths);
				SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)statusmsg);
				SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)"");
				//ShowTotalRoms();
				break;

			case 1:                 //Loading Statusbar
	/*             //Removes the sizing grid
				 statusstyle = GetWindowLong( hStatus, GWL_STYLE );
				 statusstyle = statusstyle & ~SBARS_SIZEGRIP;
				 SetWindowLong( hStatus, GWL_STYLE, statusstyle );
				 SetWindowPos( hStatus, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED );  //Set on top
	*/
				SendMessage(hStatus, SB_SETPARTS, sizeof(loadingwidths) / sizeof(int), (LPARAM)loadingwidths);
				SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)statusmsg);
				SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)"");
				SendMessage(hStatus, SB_SETTEXT, 2, (LPARAM)"");

				GetClientRect(hStatus, &rcClient);

				hStatusProgress = CreateWindowEx(0, PROGRESS_CLASS,
					(LPSTR)NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
					204, 4, 89, rcClient.bottom - 6,
					hStatus, (HMENU)0, app_hInstance, NULL);
					/*hStatusProgress = CreateWindowEx(0, PROGRESS_CLASS,
							  (LPSTR) NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
							  201, 2, 99, rcClient.bottom-2,
							  hStatus, (HMENU) 0, app_hInstance, NULL);
					*/
				SendMessage(hStatusProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
				SendMessage(hStatusProgress, PBM_SETBARCOLOR, 0, RGB(90, 0, 100));
				break;

			case 2:                    //Emulating Statusbar
				if (Config.show_fps && Config.show_vis_per_second) {
					SendMessage(hStatus, SB_SETPARTS, sizeof(emulatewidthsFPSVIS) / sizeof(int), (LPARAM)emulatewidthsFPSVIS);
					SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)""); //rr
					SendMessage(hStatus, SB_SETTEXT, 2, (LPARAM)""); //FPS
					SendMessage(hStatus, SB_SETTEXT, 3, (LPARAM)""); //VIS
					parts = 5;
				} else if (Config.show_fps) {
					SendMessage(hStatus, SB_SETPARTS, sizeof(emulatewidthsFPS) / sizeof(int), (LPARAM)emulatewidthsFPS);
					SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)"");
					SendMessage(hStatus, SB_SETTEXT, 2, (LPARAM)"");
					parts = 4;
				} else if (Config.show_vis_per_second) {
					SendMessage(hStatus, SB_SETPARTS, sizeof(emulatewidthsFPS) / sizeof(int), (LPARAM)emulatewidthsFPS);
					SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)"");
					SendMessage(hStatus, SB_SETTEXT, 2, (LPARAM)"");
					parts = 4;
				} else {
					SendMessage(hStatus, SB_SETPARTS, sizeof(emulatewidths) / sizeof(int), (LPARAM)emulatewidths);
					SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)"");
					parts = 3;
				}

				SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)statusmsg);
				sprintf(TempMessage, "%s", ROM_SETTINGS.goodname);
				SendMessage(hStatus, SB_SETTEXT, parts - 1, (LPARAM)TempMessage);
   /*
				switch( ROM_HEADER->Country_code&0xFF )             //Choosing icon
				{
				   case 0:
					  hStatusIcon = LoadImage( app_hInstance, MAKEINTRESOURCE(IDI_DEMO),
										 IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED );
				   break;                         //IDI_DEMO
				   case '7':
					  hStatusIcon = LoadImage( app_hInstance, MAKEINTRESOURCE(IDI_BETA),
										 IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED );
				   break;                         //IDI_BETA
				   case 0x44:
					  hStatusIcon = LoadImage( app_hInstance, MAKEINTRESOURCE(IDI_GERMANY),
										 IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED );
				   break;                         //IDI_GERMANY
				   case 0x45:
					  hStatusIcon = LoadImage( app_hInstance, MAKEINTRESOURCE(IDI_USA),
										 IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED );
				   break;                         //IDI_USA
				   case 0x4A:
					  hStatusIcon = LoadImage( app_hInstance, MAKEINTRESOURCE(IDI_JAPAN),
										 IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED );
				   break;                         //IDI_JAPAN
				   case 0x20:
				   case 0x21:
				   case 0x38:
				   case 0x70:
				   case 0x50:
				   case 0x58:
					  hStatusIcon = LoadImage( app_hInstance, MAKEINTRESOURCE(IDI_EUROPE),
										 IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED );
				   break;                         //IDI_EUROPE
				   case 0x55:
					  hStatusIcon = LoadImage( app_hInstance, MAKEINTRESOURCE(IDI_AUSTRALIA),
										 IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED );
				   break;                        //IDI_AUSTRALIA
				   case 'I':
					  hStatusIcon = LoadImage( app_hInstance, MAKEINTRESOURCE(IDI_ITALIA),
										 IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED );
				  break;                         //IDI_ITALIA
				   case 0x46:
					  hStatusIcon = LoadImage( app_hInstance, MAKEINTRESOURCE(IDI_FRANCE),
										 IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED );
				   break;                        //IDI_FRANCE
				   case 'S':
					  hStatusIcon = LoadImage( app_hInstance, MAKEINTRESOURCE(IDI_SPAIN),
										 IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED );
				   break;                         //IDI_SPAIN
				   default:
					  hStatusIcon = LoadImage( app_hInstance, MAKEINTRESOURCE(IDI_USA),
										 IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED );
				   break;
				}

				GetClientRect(hStatus, &rcClient);
				hStaticHandle = CreateWindowEx(0, "Static", (LPCTSTR) "ROMIcon",
								   WS_CHILD | WS_VISIBLE | SS_ICON,
								   347, ((rcClient.bottom - rcClient.top) - 16)/2 + 1,
								   0, 0, hStatus, (HMENU) 0, app_hInstance, NULL);
				SendMessage( hStaticHandle, STM_SETICON, (WPARAM) hStatusIcon, 0 );
   */
				break;
		}        //Switch
	}        //If
}

void EnableEmulationMenuItems(BOOL emulationRunning) {
	extern BOOL continue_vcr_on_restart_mode;

	HMENU hMenu = GetMenu(mainHWND);

#ifdef _DEBUG
	EnableMenuItem(hMenu, ID_CRASHHANDLERDIALOGSHOW, MF_ENABLED);
#endif

	if (emulationRunning) {
		EnableMenuItem(hMenu, EMU_STOP, MF_ENABLED);
		//EnableMenuItem(hMenu,IDLOAD,MF_GRAYED);
		EnableMenuItem(hMenu, EMU_PAUSE, MF_ENABLED);
		EnableMenuItem(hMenu, EMU_FRAMEADVANCE, MF_ENABLED);
		EnableMenuItem(hMenu, EMU_PLAY, MF_ENABLED);
		EnableMenuItem(hMenu, FULL_SCREEN, MF_ENABLED);
		EnableMenuItem(hMenu, STATE_SAVE, MF_ENABLED);
		EnableMenuItem(hMenu, STATE_SAVEAS, MF_ENABLED);
		EnableMenuItem(hMenu, STATE_RESTORE, MF_ENABLED);
		EnableMenuItem(hMenu, STATE_LOAD, MF_ENABLED);
		EnableMenuItem(hMenu, GENERATE_BITMAP, MF_ENABLED);
		EnableRecentROMsMenu(hMenu, TRUE);
		EnableMenuItem(hMenu, EMU_RESET, MF_ENABLED);
		EnableMenuItem(hMenu, REFRESH_ROM_BROWSER, MF_GRAYED);
		EnableMenuItem(hMenu, ID_RESTART_MOVIE, MF_ENABLED);
		EnableMenuItem(hMenu, ID_AUDIT_ROMS, MF_GRAYED);
		EnableMenuItem(hMenu, ID_FFMPEG_START, MF_DISABLED);

	#ifdef GAME_DEBUGGER
		EnableMenuItem(hMenu, ID_GAMEDEBUGGER, MF_ENABLED);
	#endif
		if (dynacore)
			EnableMenuItem(hMenu, ID_TRACELOG, MF_DISABLED);
		else
			EnableMenuItem(hMenu, ID_TRACELOG, MF_ENABLED);


		if (!continue_vcr_on_restart_mode) {
			EnableMenuItem(hMenu, ID_START_RECORD, MF_ENABLED);
			EnableMenuItem(hMenu, ID_STOP_RECORD, VCR_isRecording() ? MF_ENABLED : MF_GRAYED);
			EnableMenuItem(hMenu, ID_START_PLAYBACK, MF_ENABLED);
			EnableMenuItem(hMenu, ID_STOP_PLAYBACK, (VCR_isRestarting() || VCR_isPlaying()) ? MF_ENABLED : MF_GRAYED);
			EnableMenuItem(hMenu, ID_START_CAPTURE, MF_ENABLED);
			EnableMenuItem(hMenu, ID_START_CAPTURE_PRESET, MF_ENABLED);
			EnableMenuItem(hMenu, ID_END_CAPTURE, VCR_isCapturing() ? MF_ENABLED : MF_GRAYED);
			EnableRecentROMsMenu(hMenu, TRUE);
			EnableRecentMoviesMenu(hMenu, TRUE);
			EnableRecentScriptsMenu(hMenu, TRUE);
		}

		if (Config.is_toolbar_enabled) {
			SendMessage(hTool, TB_ENABLEBUTTON, EMU_PLAY, TRUE);
			SendMessage(hTool, TB_ENABLEBUTTON, EMU_STOP, TRUE);
			SendMessage(hTool, TB_ENABLEBUTTON, EMU_PAUSE, TRUE);
			if (recording && !externalReadScreen)
				SendMessage(hTool, TB_ENABLEBUTTON, FULL_SCREEN, FALSE);
			else
				SendMessage(hTool, TB_ENABLEBUTTON, FULL_SCREEN, TRUE);
		}
	} else {
		EnableMenuItem(hMenu, EMU_STOP, MF_GRAYED);
		EnableMenuItem(hMenu, IDLOAD, MF_ENABLED);
		EnableMenuItem(hMenu, EMU_PAUSE, MF_GRAYED);
		EnableMenuItem(hMenu, EMU_FRAMEADVANCE, MF_GRAYED);
		EnableMenuItem(hMenu, EMU_PLAY, MF_GRAYED);
		EnableMenuItem(hMenu, FULL_SCREEN, MF_GRAYED);
		EnableMenuItem(hMenu, STATE_SAVE, MF_GRAYED);
		EnableMenuItem(hMenu, STATE_SAVEAS, MF_GRAYED);
		EnableMenuItem(hMenu, STATE_RESTORE, MF_GRAYED);
		EnableMenuItem(hMenu, STATE_LOAD, MF_GRAYED);
		EnableMenuItem(hMenu, GENERATE_BITMAP, MF_GRAYED);
		EnableRecentROMsMenu(hMenu, TRUE);
		EnableMenuItem(hMenu, EMU_RESET, MF_GRAYED);
		EnableMenuItem(hMenu, REFRESH_ROM_BROWSER, MF_ENABLED);
		EnableMenuItem(hMenu, ID_RESTART_MOVIE, MF_GRAYED);
		EnableMenuItem(hMenu, ID_GAMEDEBUGGER, MF_GRAYED);
		EnableMenuItem(hMenu, ID_TRACELOG, MF_DISABLED);
		EnableMenuItem(hMenu, ID_AUDIT_ROMS, MF_ENABLED);
		EnableMenuItem(hMenu, ID_FFMPEG_START, MF_GRAYED);

		if (!continue_vcr_on_restart_mode) {
			EnableMenuItem(hMenu, ID_START_RECORD, MF_GRAYED);
			EnableMenuItem(hMenu, ID_STOP_RECORD, MF_GRAYED);
			EnableMenuItem(hMenu, ID_START_PLAYBACK, MF_GRAYED);
			EnableMenuItem(hMenu, ID_STOP_PLAYBACK, MF_GRAYED);
			EnableMenuItem(hMenu, ID_START_CAPTURE, MF_GRAYED);
			EnableMenuItem(hMenu, ID_START_CAPTURE_PRESET, MF_GRAYED);
			EnableMenuItem(hMenu, ID_END_CAPTURE, MF_GRAYED);
			EnableRecentROMsMenu(hMenu, TRUE);
			EnableRecentMoviesMenu(hMenu, FALSE);
			EnableRecentScriptsMenu(hMenu, FALSE);
			LONG winstyle;
			winstyle = GetWindowLong(mainHWND, GWL_STYLE);
			winstyle |= WS_MAXIMIZEBOX;
			SetWindowLong(mainHWND, GWL_STYLE, winstyle);
			SetWindowPos(mainHWND, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);  //Set on top
		}
		if (Config.is_toolbar_enabled) {
			getSelectedRom(); //Used to check if the play button should be enabled or not
			//SendMessage( hTool, TB_ENABLEBUTTON, EMU_PLAY, FALSE );
			SendMessage(hTool, TB_ENABLEBUTTON, EMU_STOP, FALSE);
			SendMessage(hTool, TB_ENABLEBUTTON, EMU_PAUSE, FALSE);
			SendMessage(hTool, TB_ENABLEBUTTON, FULL_SCREEN, FALSE);
		}
	}

	if (Config.is_toolbar_enabled) CheckMenuItem(hMenu, IDC_GUI_TOOLBAR, MF_BYCOMMAND | MF_CHECKED);
	else CheckMenuItem(hMenu, IDC_GUI_TOOLBAR, MF_BYCOMMAND | MF_UNCHECKED);
	if (Config.is_statusbar_enabled) CheckMenuItem(hMenu, IDC_GUI_STATUSBAR, MF_BYCOMMAND | MF_CHECKED);
	else CheckMenuItem(hMenu, IDC_GUI_STATUSBAR, MF_BYCOMMAND | MF_UNCHECKED);
	if (Config.is_movie_loop_enabled) CheckMenuItem(hMenu, ID_LOOP_MOVIE, MF_BYCOMMAND | MF_CHECKED);
	else CheckMenuItem(hMenu, ID_LOOP_MOVIE, MF_BYCOMMAND | MF_UNCHECKED);
	if (Config.is_recent_movie_paths_frozen) CheckMenuItem(hMenu, ID_RECENTMOVIES_FREEZE, MF_BYCOMMAND | MF_CHECKED);
	if (Config.is_recent_scripts_frozen) CheckMenuItem(hMenu, ID_LUA_RECENT_FREEZE, MF_BYCOMMAND | MF_CHECKED);
}

static DWORD WINAPI SoundThread(LPVOID lpParam) {
	while (emu_launched)
		aiUpdate(1);
	ExitThread(0);
}

static DWORD WINAPI StartMoviesThread(LPVOID lpParam) {
	Sleep(2000);
	StartMovies();
	ExitThread(0);
}

static DWORD WINAPI ThreadFunc(LPVOID lpParam) {
	ThreadFuncState = TFS_NONE;
	ShowInfo("Emu thread: Start");

	ThreadFuncState = TFS_INITMEM;
	ShowInfo("Init memory....");
	init_memory();
	ThreadFuncState = TFS_LOADPLUGINS;
	ShowInfo("Loading plugins....");
	load_plugins();
	ThreadFuncState = TFS_OPENGFX;
	ShowInfo("Rom open gfx....");
	romOpen_gfx();
	ThreadFuncState = TFS_OPENINPUT;
	ShowInfo("Rom open input....");
	romOpen_input();
	ThreadFuncState = TFS_OPENAUDIO;
	ShowInfo("Rom open audio....");
	romOpen_audio();

	ThreadFuncState = TFS_DISPLAYMODE;
	dynacore = Config.core_type;
	ShowInfo("Core = %s", CoreNames[dynacore]);

	emu_paused = 0;
	emu_launched = 1;
	restart_mode = 0;

	ThreadFuncState = TFS_CREATESOUND;
	ShowInfo("Emu thread: Creating sound thread...");
	SoundThreadHandle = CreateThread(NULL, 0, SoundThread, NULL, 0, &SOUNDTHREADID);
	ThreadFuncState = TFS_EMULATING;
	ShowInfo("Emu thread: Emulation started....");
	CreateThread(NULL, 0, StartMoviesThread, NULL, 0, NULL);
	//StartMovies(); // check commandline args
	StartLuaScripts();
	StartSavestate();
	AtResetCallback();
	if (pauseAtFrame == 0 && VCR_isStartingAndJustRestarted()) {
		while (emu_paused) {
			Sleep(10);
		}
		pauseEmu(FALSE);
		pauseAtFrame = -1;
	}
	go();
	ShowInfo("Emu thread: Core stopped...");
	ThreadFuncState = TFS_CLOSEINPUT;
	romClosed_input();
	ShowInfo("Emu thread: romClosed (input plugin)");
	ThreadFuncState = TFS_CLOSEAUDIO;
	romClosed_audio();
	ShowInfo("Emu thread: romClosed (audio plugin)");
	ThreadFuncState = TFS_CLOSERSP;
	romClosed_RSP();
	ShowInfo("Emu thread: romClosed (RSP plugin)");
	ThreadFuncState = TFS_UNLOADRSP;
	closeDLL_RSP();
	ShowInfo("Emu thread: RSP plugin closed");
	ThreadFuncState = TFS_UNLOADINPUT;
	closeDLL_input();
	ShowInfo("Emu thread: input plugin closed");
	ThreadFuncState = TFS_UNLOADAUDIO;
	closeDLL_audio();
	ShowInfo("Emu thread: audio plugin closed");
	ThreadFuncState = TFS_CLOSEGFX;
	romClosed_gfx();
	ShowInfo("Emu thread: romClosed (gfx plugin)");
	ThreadFuncState = TFS_UNLOADGFX;
	closeDLL_gfx();
	ShowInfo("Emu thread: gfx plugin closed");
	ThreadFuncState = TFS_NONE;
	ExitThread(0);
}

void exit_emu(int postquit) {

	if (postquit) {
		if (!cmdlineMode || cmdlineSave) {
			ini_updateFile(Config.is_ini_compressed);
			if (!cmdlineNoGui)
				SaveRomBrowserCache();
		}
		//if (shouldSave)
		SaveConfig();
		ini_closeFile();
	} else {
		CreateThread(NULL, 0, closeRom, NULL, 0, &Id);
	}

	if (postquit) {
		freeRomDirList();
		freeRomList();
		freeLanguages();
		Gdiplus::GdiplusShutdown(gdiPlusToken);
		//printf("free gdiplus\n");
		PostQuitMessage(0);
	}
}

void exit_emu2() {
	if (warn_recording())
		return;

	if ((!cmdlineMode) || (cmdlineSave)) {
		ini_updateFile(Config.is_ini_compressed);
		SaveConfig();
		if (!cmdlineNoGui) {
			SaveRomBrowserCache();
		}
	}
	ini_closeFile();
	freeRomDirList();
	freeRomList();
	freeLanguages();
	PostQuitMessage(0);
}

BOOL IsMenuItemEnabled(HMENU hMenu, UINT uId) {
	return !(GetMenuState(hMenu, uId, MF_BYCOMMAND) & (MF_DISABLED | MF_GRAYED));
}

void ProcessToolTips(LPARAM lParam, HWND hWnd) {
	LPTOOLTIPTEXT lpttt;

	lpttt = (LPTOOLTIPTEXT)lParam;
	lpttt->hinst = app_hInstance;

	// Specify the resource identifier of the descriptive 
	// text for the given button. 
	HMENU hMenu = GetMenu(hWnd);

	switch (lpttt->hdr.idFrom) {
		case IDLOAD:
			TranslateDefault("Load ROM...", "Load ROM...", TempMessage);
			lpttt->lpszText = TempMessage;
			break;
		case EMU_PLAY:
			if (!emu_launched) {
				TranslateDefault("Start Emulation", "Start Emulation", TempMessage);
			} else if (emu_paused) {
				TranslateDefault("Resume Emulation", "Resume Emulation", TempMessage);
			} else {
				TranslateDefault("Emulating", "Emulating", TempMessage);
			}
			lpttt->lpszText = TempMessage;
			break;
		case EMU_PAUSE:
			TranslateDefault("Pause Emulation", "Pause Emulation", TempMessage);
			lpttt->lpszText = TempMessage;
			break;
		case EMU_STOP:
			TranslateDefault("Stop Emulation", "Stop Emulation", TempMessage);
			lpttt->lpszText = TempMessage;
			break;
		case FULL_SCREEN:
			TranslateDefault("Full Screen", "Full Screen", TempMessage);
			lpttt->lpszText = TempMessage;
			break;
		case IDGFXCONFIG:
			TranslateDefault("Video Settings...", "Video Settings...", TempMessage);
			lpttt->lpszText = TempMessage;
			break;
		case IDSOUNDCONFIG:
			TranslateDefault("Audio Settings...", "Audio Settings...", TempMessage);
			lpttt->lpszText = TempMessage;
			break;
		case IDINPUTCONFIG:
			TranslateDefault("Input Settings...", "Input Settings...", TempMessage);
			lpttt->lpszText = TempMessage;
			break;
		case IDRSPCONFIG:
			TranslateDefault("RSP Settings...", "RSP Settings...", TempMessage);
			lpttt->lpszText = TempMessage;
			break;
		case ID_LOAD_CONFIG:
			TranslateDefault("Settings...", "Settings...", TempMessage);
			lpttt->lpszText = TempMessage;
			break;
	}
}

void EnableStatusbar() {
	shouldSave = TRUE;

	if (Config.is_statusbar_enabled) {
		if (!IsWindow(hStatus)) {
			CreateStatusBarWindow(mainHWND);
			ResizeRomListControl();
		}
	} else {
		DestroyWindow(hStatus);
		hStatus = NULL;
		ResizeRomListControl();
	}
}

void EnableToolbar() {
	shouldSave = TRUE;

	if (Config.is_toolbar_enabled && !VCR_isCapturing()) {
		if (!hTool || !IsWindow(hTool)) {
			CreateToolBarWindow(mainHWND);
			ResizeRomListControl();
		}
	} else {
		if (hTool && IsWindow(hTool)) {
			DestroyWindow(hTool);
			hTool = NULL;
			ResizeRomListControl();
		}
	}
}


LRESULT CALLBACK NoGuiWndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	switch (Message) {
		case WM_KEYDOWN:
			switch (wParam) {
				case VK_TAB:
					manualFPSLimit = 0;
					break;
			}
			if (emu_launched) keyDown(wParam, lParam);
			break;
		case WM_KEYUP:
			switch (wParam) {
				case VK_TAB:
					manualFPSLimit = 1;
					break;
				case VK_ESCAPE:
					exit_emu(1);
					break;
			}
			if (emu_launched) keyUp(wParam, lParam);
			break;
		case WM_MOVE:
			if (emu_launched && !FullScreenMode) {
				moveScreen(wParam, lParam);
			}
			break;
		case WM_USER + 17:  SetFocus(mainHWND);
			break;
		case WM_CREATE:
			// searching the plugins...........
			search_plugins();
			setDefaultPlugins();
			////////////////////////////
			return TRUE;
		case WM_CLOSE:
			exit_emu(1);
			break;

		default:
			return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return TRUE;
}


DWORD WINAPI UnpauseEmuAfterMenu(LPVOID lpParam) {
	Sleep(60); // Wait for another thread to clear MenuPaused

	if (emu_paused && !AutoPause && MenuPaused) {
		resumeEmu(FALSE);
	}
	MenuPaused = FALSE;
	return 0;
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	char path_buffer[_MAX_PATH];
	int ret;
	int i;
	static PAINTSTRUCT	ps;
	BOOL	minimize;
	HMENU hMenu = GetMenu(hwnd);

#ifdef LUA_WINDOWMESSAGE
	LuaWindowMessage(hwnd, Message, wParam, lParam);
#endif
	switch (Message) {
		case WM_DROPFILES:
			//HDROP hFile = (HDROP) wParam;
			char fname[MAX_PATH];
			LPSTR fext;
			DragQueryFile((HDROP)wParam, 0, fname, sizeof(fname));
			fext = CharUpper(PathFindExtension(fname));
			if (lstrcmp(fext, ".N64") == 0 || lstrcmp(fext, ".V64") == 0 || lstrcmp(fext, ".Z64") == 0 || lstrcmp(fext, ".ROM") == 0) {
				StartRom(fname);
			} else if (lstrcmp(fext, ".M64") == 0) {
				if (rom) {
					if (!VCR_getReadOnly())	VCR_toggleReadOnly();

					if (VCR_startPlayback(fname, 0, 0) < 0) {
						printf("[VCR]: Drag drop Failed to start playback of %s", fname);
						break;
					} else
						SetStatusPlaybackStarted();
				}
			} else if (strcmp(fext, ".ST") == 0 || strcmp(fext, ".SAVESTATE") == 0) {
				if (rom) {
					savestates_select_filename(fname);
					savestates_job = LOADSTATE;
				}
			} else if (strcmp(fext, ".LUA") == 0) {
				if (rom) {
					for (; *fext; ++fext) *fext = tolower(*fext); // Deep in the code, lua will access file with that path (uppercase extension because stupid, useless programming at line 2677 converts it), see it doesnt exist and fail.
					// even this hack will fail under special circumstances
					LuaOpenAndRun(fname);
				}
			}

			break;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			BOOL hit = FALSE;
			if (manualFPSLimit) {
				if ((int)wParam == Config.hotkeys[0].key) // fast-forward on
				{
					if (((GetKeyState(VK_SHIFT) & 0x8000) ? 1 : 0) == Config.hotkeys[0].shift
						&& ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0) == Config.hotkeys[0].ctrl
						&& ((GetKeyState(VK_MENU) & 0x8000) ? 1 : 0) == Config.hotkeys[0].alt) {
						manualFPSLimit = 0;
						hit = TRUE;
					}
				}
			}
			for (i = 1; i < NUM_HOTKEYS; i++) {
				if ((int)wParam == Config.hotkeys[i].key) {
					if (((GetKeyState(VK_SHIFT) & 0x8000) ? 1 : 0) == Config.hotkeys[i].shift
						&& ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0) == Config.hotkeys[i].ctrl
						&& ((GetKeyState(VK_MENU) & 0x8000) ? 1 : 0) == Config.hotkeys[i].alt) {
						SendMessage(hwnd, WM_COMMAND, Config.hotkeys[i].command, 0);
						hit = TRUE;
					}
				}
			}
			if (emu_launched)
				keyDown(wParam, lParam);
			if (!hit)
				return DefWindowProc(hwnd, Message, wParam, lParam);
		}	break;
		case WM_SYSKEYUP:
		case WM_KEYUP:
			if ((int)wParam == Config.hotkeys[0].key) // fast-forward off
			{
				manualFPSLimit = 1;
				ffup = true; //fuck it, timers.c is too weird
			}
			if (emu_launched)
				keyUp(wParam, lParam);
			return DefWindowProc(hwnd, Message, wParam, lParam);
			break;
		case WM_NOTIFY:
			if (wParam == IDC_ROMLIST) {
				RomListNotify((LPNMHDR)lParam);
			}
			switch (((LPNMHDR)lParam)->code) {
				case TTN_NEEDTEXT:
					ProcessToolTips(lParam, hwnd);
					break;
			}
			return 0;
		case WM_MOVE:
			if (emu_launched && !FullScreenMode) {
				moveScreen(wParam, lParam);
			}
			break;
		case WM_SIZE:
			if (!FullScreenMode) {
				SendMessage(hTool, TB_AUTOSIZE, 0, 0);
				SendMessage(hStatus, WM_SIZE, 0, 0);
				ResizeRomListControl();
			}
			break;
		case WM_USER + 17:  SetFocus(mainHWND); break;
		case WM_CREATE:
				// searching the plugins...........
			GetModuleFileName(NULL, path_buffer, sizeof(path_buffer));
			search_plugins();
			setDefaultPlugins();
			CreateToolBarWindow(hwnd);
			CreateStatusBarWindow(hwnd);
			SetupLanguages(hwnd);
			TranslateMenu(GetMenu(hwnd), hwnd);
			SetMenuAcceleratorsFromUser(hwnd);
			CreateRomListControl(hwnd);
			SetRecentList(hwnd);
			BuildRecentMoviesMenu(hwnd);
			BuildRecentScriptsMenu(hwnd);
			EnableToolbar();
			EnableStatusbar();
			////////////////////////////
			return TRUE;
		case WM_CLOSE:
			if (warn_recording())break;
			if (emu_launched) {
				shut_window = 1;
				exit_emu(0);
				return 0;
			} else {
				exit_emu(1);
				//DestroyWindow(hwnd);
			}
			break;

		case WM_PAINT: //todo, work with updatescreen to use wmpaint
		{

			BeginPaint(hwnd, &ps);
			EndPaint(hwnd, &ps);

			return 0;
		}
	//		case WM_SETCURSOR:
	//			SetCursor(FALSE);
	//			return 0;
		case WM_WINDOWPOSCHANGING:  //allow gfx plugin to set arbitrary size 
			return 0;
		case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
			lpMMI->ptMinTrackSize.x = MIN_WINDOW_W;
			lpMMI->ptMinTrackSize.y = MIN_WINDOW_H;
			// this might break small res with gfx plugin!!!
		}
		break;
		case WM_ENTERMENULOOP:
			AutoPause = emu_paused;
			if (!emu_paused) {
				MenuPaused = TRUE;
				pauseEmu(FALSE);
			}
			break;

		case WM_EXITMENULOOP:
			CreateThread(NULL, 0, UnpauseEmuAfterMenu, NULL, 0, NULL);
			break;
		case WM_ACTIVATE:
			UpdateWindow(hwnd);
			minimize = (BOOL)HIWORD(wParam);

			switch (LOWORD(wParam)) {
				case WA_ACTIVE:
				case WA_CLICKACTIVE:
					if (Config.is_unfocused_pause_enabled && emu_paused && !AutoPause) {
						resumeEmu(FALSE);
						AutoPause = emu_paused;
					}
					break;

				case WA_INACTIVE:
					AutoPause = emu_paused && !MenuPaused;
					if (Config.is_unfocused_pause_enabled && !emu_paused /*(&& minimize*/ && !FullScreenMode) {
						MenuPaused = FALSE;
						pauseEmu(FALSE);
					} else if (Config.is_unfocused_pause_enabled && MenuPaused) {
						MenuPaused = FALSE;
					}
					break;
			}
			break;
		case WM_COMMAND:
		{
			switch (LOWORD(wParam)) {
				case ID_MENU_LUASCRIPT_NEW:
				{
					EnableRecentScriptsMenu(hMenu, TRUE);
					::NewLuaScript((void(*)())lParam);
				} break;
				case ID_LUA_RECENT_FREEZE:
				{
					CheckMenuItem(hMenu, ID_LUA_RECENT_FREEZE, (Config.is_recent_scripts_frozen ^= 1) ? MF_CHECKED : MF_UNCHECKED);
					shouldSave = TRUE;
					break;
				}
				case ID_LUA_RECENT_RESET:
				{
					ClearRecent(TRUE);
					BuildRecentScriptsMenu(hwnd);
					break;
				}
				case ID_LUA_LOAD_LATEST:
				{
					RunRecentScript(ID_LUA_RECENT);
					break;
				}
				case ID_MENU_LUASCRIPT_CLOSEALL:
				{
					::CloseAllLuaScript();
				} break;
				case ID_FORCESAVE:
					shouldSave = TRUE;
					ini_updateFile(Config.is_ini_compressed);
					SaveRomBrowserCache();
					SaveConfig();
					ini_closeFile();
					break;
				case ID_TRACELOG:
				#ifdef LUA_TRACELOG
				#ifdef LUA_TRACEINTERP
								// keep if check just in case user manages to screw with mupen config or something
					if (!dynacore) {
						::LuaTraceLogState();
					}
				#else
					if (!dynacore && interpcore == 1) {
						::LuaTraceLogState();
					}
				#endif
				#endif
					break;
				case IDGFXCONFIG:
				{
					BOOL wasPaused = emu_paused && !MenuPaused;
					MenuPaused = FALSE;
					if (emu_launched && !emu_paused)
						pauseEmu(FALSE);

					hwnd_plug = hwnd;
					exec_config(gfx_name);

					if (emu_launched && emu_paused && !wasPaused)
						resumeEmu(FALSE);
				}	break;
				case IDINPUTCONFIG:
				{
					BOOL wasPaused = emu_paused && !MenuPaused;
					MenuPaused = FALSE;
					if (emu_launched && !emu_paused)
						pauseEmu(FALSE);

					hwnd_plug = hwnd;
					exec_config(input_name);

					if (emu_launched && emu_paused && !wasPaused)
						resumeEmu(FALSE);
				}	break;
				case IDSOUNDCONFIG:
				{
					BOOL wasPaused = emu_paused && !MenuPaused;
					MenuPaused = FALSE;
					if (emu_launched && !emu_paused)
						pauseEmu(FALSE);

					hwnd_plug = hwnd;
					exec_config(sound_name);

					if (emu_launched && emu_paused && !wasPaused)
						resumeEmu(FALSE);
				}	break;
				case IDRSPCONFIG:
				{
					BOOL wasPaused = emu_paused && !MenuPaused;
					MenuPaused = FALSE;
					if (emu_launched && !emu_paused)
						pauseEmu(FALSE);

					hwnd_plug = hwnd;
					exec_config(rsp_name);

					if (emu_launched && emu_paused && !wasPaused)
						resumeEmu(FALSE);
				}	break;
				case EMU_STOP:
					MenuPaused = FALSE;
					if (warn_recording())break;
					if (emu_launched) {
						  //if (warn_recording())break;
						  //closeRom();
						stop_it();
						//SleepEx(1000, TRUE); //todo check if can remove
						CreateThread(NULL, 0, closeRom, (LPVOID)1, 0, &Id);
						SendMessage(hTool, TB_CHECKBUTTON, EMU_PAUSE, 0);
						SendMessage(hTool, TB_CHECKBUTTON, EMU_PLAY, 0);
					}
					break;

				case EMU_PAUSE:
				{
					if (!emu_paused) {
						pauseEmu(VCR_isActive());
					} else if (MenuPaused) {
						MenuPaused = FALSE;
						CheckMenuItem(GetMenu(mainHWND), EMU_PAUSE, MF_BYCOMMAND | MFS_CHECKED);
						SendMessage(hTool, TB_CHECKBUTTON, EMU_PAUSE, TRUE);
					} else {
						resumeEmu(VCR_isActive());
					}
					break;
				}

				case EMU_FRAMEADVANCE:
				{
					MenuPaused = FALSE;
					if (!manualFPSLimit) break;
					extern int frame_advancing;
					frame_advancing = 1;
					VIs = 0; // prevent old VI value from showing error if running at super fast speeds
					resumeEmu(TRUE); // maybe multithreading unsafe
				}
				break;

				case EMU_VCRTOGGLEREADONLY:
					VCR_toggleReadOnly();
					break;

				case ID_LOOP_MOVIE:
					VCR_toggleLoopMovie();
					shouldSave = TRUE;
					break;
				case ID_RESTART_MOVIE:
					if (VCR_isPlaying()) {
						VCR_setReadOnly(TRUE);
						bool err = VCR_startPlayback(Config.recent_movie_paths[0], 0, 0);
						if (err == VCR_PLAYBACK_SUCCESS)
							SetStatusPlaybackStarted();
						else
							SetStatusTranslatedString(hStatus, 0, "Couldn't start latest movie");
					}
					break;
				case ID_REPLAY_LATEST:
					// Don't try to load a recent movie if not emulating!
					if (rom) {
						// Overwrite prevention? Path sanity check (Leave to internal handling)?
						VCR_setReadOnly(TRUE);
						bool err = VCR_startPlayback(Config.recent_movie_paths[0], 0, 0);
						if (err == VCR_PLAYBACK_SUCCESS)
							SetStatusPlaybackStarted();
						else
							SetStatusTranslatedString(hStatus, 0, "Couldn't start latest movie");
					} else
						SetStatusTranslatedString(hStatus, 0, "Cannot load a movie while not emulating!");
					break;
				case ID_RECENTMOVIES_FREEZE:
					CheckMenuItem(hMenu, ID_RECENTMOVIES_FREEZE, (Config.is_recent_movie_paths_frozen ^= 1) ? MF_CHECKED : MF_UNCHECKED);
					shouldSave = TRUE;
					break;
					//FreezeRecentMovies(mainHWND, TRUE);
					break;
				case ID_RECENTMOVIES_RESET:
					ClearRecentMovies(TRUE);
					BuildRecentMoviesMenu(mainHWND);
					break;
				case EMU_PLAY:
					if (emu_launched) {
						if (emu_paused) {
							resumeEmu(FALSE);
						} else {
							SendMessage(hTool, TB_CHECKBUTTON, EMU_PLAY, 1);
							//The button is always checked when started and not on pause
						}
					} else {
						RomList_OpenRom();
					}
					break;

				case EMU_RESET:
					if (Config.is_reset_recording_disabled && warn_recording())break;
					extern int m_task;
					if (m_task == 3 && !Config.is_reset_recording_disabled) //recording
					{
						scheduled_restart = true;
						continue_vcr_on_restart_mode = true;
						display_status("Writing restart to m64...");
						break;
					}
					//resttart_mode = 1; //has issues
					resetEmu();
					break;

				case ID_LOAD_CONFIG:
				{
					BOOL wasPaused = emu_paused && !MenuPaused;
					MenuPaused = FALSE;
					if (emu_launched && !emu_paused) {
						pauseEmu(FALSE);
					}
					shouldSave = TRUE;
					ChangeSettings(hwnd);
					ini_updateFile(Config.is_ini_compressed);
					if (emu_launched && emu_paused && !wasPaused) {
						resumeEmu(FALSE);
					}
				}
				break;
				case ID_AUDIT_ROMS:
					ret = DialogBox(GetModuleHandle(NULL),
						MAKEINTRESOURCE(IDD_AUDIT_ROMS_DIALOG), hwnd, AuditDlgProc);
					break;
				case ID_HELP_ABOUT:
				{
					BOOL wasMenuPaused = MenuPaused;
					MenuPaused = FALSE;
					ret = DialogBox(GetModuleHandle(NULL),
						MAKEINTRESOURCE(IDD_ABOUT), hwnd, AboutDlgProc);
					if (wasMenuPaused) {
						resumeEmu(TRUE);
					}
				}
				break;
				case ID_CRASHHANDLERDIALOGSHOW:
				{
					CrashHandlerDialog crashHandlerDialog(CrashHandlerDialog::Types::Ignorable, "This is a mock crash.");
					crashHandlerDialog.Show();
					break;
				}
				case ID_GAMEDEBUGGER:
					extern unsigned long op;

					GameDebuggerStart([]() {
						return Config.core_type == 2 ? op : -1;
						}, []() {
							return Config.core_type == 2 ? interp_addr : -1;
						});
					break;
				case ID_RAMSTART:
				{

					BOOL wasMenuPaused = MenuPaused;
					MenuPaused = FALSE;

					pauseEmu(TRUE);

					// todo: simplify

					// maybe pack formatted and buffer and result into one long char array where you only read necessary part

					char buf[12]; // ram start
					sprintf(buf, "0x%#08p", rdram);

					if (!stroopConfigLine[0]) {
						TCHAR procName[MAX_PATH];
						GetModuleFileName(NULL, procName, MAX_PATH);
						_splitpath(procName, 0, 0, procName, 0);

						sprintf(stroopConfigLine, "<Emulator name=\"Mupen 5.0 RR\" processName=\"%s\" ramStart=\"%s\" endianness=\"little\"/>", procName, buf);

					}
					std::string stdstr_buf = stroopConfigLine;
				#ifdef _WIN32
					if (MessageBoxA(0, buf, "RAM Start (Click Yes to Copy STROOP config line)", MB_ICONINFORMATION | MB_TASKMODAL | MB_YESNO) == IDYES) {
						OpenClipboard(mainHWND);
						EmptyClipboard();
						HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, stdstr_buf.size() + 1);
						if (hg) {
							memcpy(GlobalLock(hg), stdstr_buf.c_str(), stdstr_buf.size() + 1);
							GlobalUnlock(hg);
							SetClipboardData(CF_TEXT, hg);
							CloseClipboard();
							GlobalFree(hg);
						} else { printf("Failed to copy"); CloseClipboard(); }
					}


				#endif


					if (wasMenuPaused) {
						resumeEmu(TRUE);
					}
					break;
				}
				case IDLOAD:
				{
					BOOL wasMenuPaused = MenuPaused;
					MenuPaused = FALSE;
					// The default directory we open the file dialog window in is
					// the parent directory of the last rom that the user ran
					GetDefaultFileDialogPath(path_buffer, Config.recent_rom_paths[0]);

					if (fdLoadRom.ShowFileDialog(path_buffer, L"*.n64;*.z64;*.v64;*.rom;*.bin;*.zip;*.usa;*.eur;*.jap", TRUE, FALSE, hwnd)) {
						char temp_buffer[_MAX_PATH];
						strcpy(temp_buffer, path_buffer);
						unsigned int i; for (i = 0; i < strlen(temp_buffer); i++)
							if (temp_buffer[i] == '/')
								temp_buffer[i] = '\\';
						char* slash = strrchr(temp_buffer, '\\');
						if (slash) {
							slash[1] = '\0';
							if (addDirectoryToLinkedList(temp_buffer)) {
								SendDlgItemMessage(hwnd, IDC_ROMBROWSER_DIR_LIST, LB_ADDSTRING, 0, (LPARAM)temp_buffer);
								AddDirToList(temp_buffer, TRUE);
								SaveRomBrowserDirs();
								SaveConfig();
							}
						}
						StartRom(path_buffer);
					}
					if (wasMenuPaused) {
						resumeEmu(TRUE);
					}
				}
				break;
				case ID_EMULATOR_EXIT:
					if (warn_recording())break;
					shut_window = 1;
					if (emu_launched)
						exit_emu(0);
					else
						exit_emu(1);
					break;
				case FULL_SCREEN:
					if (emu_launched && (!recording || externalReadScreen)) {
						FullScreenMode = 1 - FullScreenMode;
						gui_ChangeWindow();
					}
					break;
				case REFRESH_ROM_BROWSER:
					if (!emu_launched) {
						RefreshRomBrowser();
					}
					break;
				case ID_POPUP_ROM_SETTING:
					OpenRomProperties();
					break;
				case ID_START_ROM:
					RomList_OpenRom();
					break;
				case ID_START_ROM_ENTER:
					if (!emu_launched) RomList_OpenRom();
					break;
				case STATE_SAVE:
					if (!emu_paused || MenuPaused) {
						savestates_job = SAVESTATE;
					} else if (emu_launched) {
						savestates_save();
					}
					break;
				case STATE_SAVEAS:
				{
					BOOL wasMenuPaused = MenuPaused;
					MenuPaused = FALSE;

					GetDefaultFileDialogPath(path_buffer, Config.states_path);

					if (fdSaveStateAs.ShowFileDialog(path_buffer, L"*.st;*.savestate", FALSE, FALSE, hwnd)) {

						// HACK: allow .savestate and .st
						// by creating another buffer, copying original into it and stripping its' extension
						// and putting it back sanitized
						// if no extension inputted by user, fallback to .st
						strcpy(correctedPath, path_buffer);
						stripExt(correctedPath);
						if (!stricmp(getExt(path_buffer), "savestate"))
							strcat(correctedPath, ".savestate");
						else
							strcat(correctedPath, ".st");

						savestates_select_filename(correctedPath);
						savestates_job = SAVESTATE;
					}
					if (wasMenuPaused) {
						resumeEmu(TRUE);
					}
				}
				break;
				case STATE_RESTORE:
					if (emu_launched) {
						savestates_job = LOADSTATE;
						// dont call savestatesload from ui thread right after setting flag for emu thread
						//savestates_load();
					}
					break;
				case STATE_LOAD:
				{
					BOOL wasMenuPaused = MenuPaused;
					MenuPaused = FALSE;
					GetDefaultFileDialogPath(path_buffer, Config.states_path);

					if (fdSaveLoadAs.ShowFileDialog(path_buffer, L"*.st;*.savestate;*.st0;*.st1;*.st2;*.st3;*.st4;*.st5;*.st6;*.st7;*.st8;*.st9,*.st10", TRUE, FALSE, hwnd)) {
						savestates_select_filename(path_buffer);
						savestates_job = LOADSTATE;
					}
					if (wasMenuPaused) {
						resumeEmu(TRUE);
					}
				}

				break;
				case ID_START_RECORD:
					if (emu_launched)
						OpenMovieRecordDialog();

					break;

				case ID_STOP_RECORD:
					if (VCR_isRecording()) {
						if (VCR_stopRecord(1) < 0) // seems ok (no)
							; // fail quietly
	//                        MessageBox(NULL, "Couldn't stop recording.", "VCR", MB_OK);
						else {
							ClearButtons();
							EnableMenuItem(hMenu, ID_STOP_RECORD, MF_GRAYED);
							EnableMenuItem(hMenu, ID_START_RECORD, MF_ENABLED);
							SetStatusTranslatedString(hStatus, 0, "Recording stopped");
						}
					}
					break;
				case ID_START_PLAYBACK:
					if (emu_launched)
						OpenMoviePlaybackDialog();

					break;
				case ID_STOP_PLAYBACK:
					if (VCR_isPlaying()) {
						if (VCR_stopPlayback() < 0)
							; // fail quietly
	//                        MessageBox(NULL, "Couldn't stop playback.", "VCR", MB_OK);
						else {
							ClearButtons();
							EnableMenuItem(hMenu, ID_STOP_PLAYBACK, MF_GRAYED);
							EnableMenuItem(hMenu, ID_START_PLAYBACK, MF_ENABLED);
							SetStatusTranslatedString(hStatus, 0, "Playback stopped");
						}
					}
					break;

				case ID_FFMPEG_START:
				{
					auto err = VCR_StartFFmpegCapture("ffmpeg_out.mp4", "-pixel_format yuv420p -loglevel debug -y");
					if (err == INIT_SUCCESS) {
						//SetWindowPos(mainHWND, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);  //Set on top avichg
						EnableMenuItem(hMenu, ID_START_CAPTURE, MF_GRAYED);
						EnableMenuItem(hMenu, ID_START_CAPTURE_PRESET, MF_GRAYED);
						EnableMenuItem(hMenu, ID_FFMPEG_START, MF_GRAYED);
						EnableMenuItem(hMenu, ID_END_CAPTURE, MF_ENABLED);
						if (!externalReadScreen) {
							EnableMenuItem(hMenu, FULL_SCREEN, MF_GRAYED);           //Disables fullscreen menu
							SendMessage(hTool, TB_ENABLEBUTTON, FULL_SCREEN, FALSE); //Disables fullscreen button
						}
						SetStatusTranslatedString(hStatus, 0, "Recording avi...");
						EnableEmulationMenuItems(TRUE);
					} else
						ShowInfo("Start capture error: %d", err);
					break;
				}

				case ID_START_CAPTURE_PRESET:
				case ID_START_CAPTURE:
					if (emu_launched) {
						BOOL wasPaused = emu_paused && !MenuPaused;
						MenuPaused = FALSE;
						if (emu_launched && !emu_paused)
							pauseEmu(FALSE);

						char rec_buffer[MAX_PATH];

						// The default directory we open the file dialog window in is
						// the parent directory of the last avi that the user captured
						GetDefaultFileDialogPath(path_buffer, Config.avi_capture_path);

						if (fdStartCapture.ShowFileDialog(path_buffer, L"*.avi", FALSE, FALSE, hwnd)) {



							int len = strlen(path_buffer);
							if (len < 5 ||
								(path_buffer[len - 1] != 'i' && path_buffer[len - 1] != 'I') ||
								(path_buffer[len - 2] != 'v' && path_buffer[len - 2] != 'V') ||
								(path_buffer[len - 3] != 'a' && path_buffer[len - 3] != 'A') ||
								path_buffer[len - 4] != '.')
								strcat(path_buffer, ".avi");
							//Sleep(1000);
							// pass false to startCapture when "last preset" option was choosen
							if (VCR_startCapture(rec_buffer, path_buffer, LOWORD(wParam) == ID_START_CAPTURE) < 0) {
								MessageBox(NULL, "Couldn't start capturing.", "VCR", MB_OK);
								recording = FALSE;
							} else {
								//SetWindowPos(mainHWND, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);  //Set on top avichg
								EnableMenuItem(hMenu, ID_START_CAPTURE, MF_GRAYED);
								EnableMenuItem(hMenu, ID_START_CAPTURE_PRESET, MF_GRAYED);
								EnableMenuItem(hMenu, ID_FFMPEG_START, MF_GRAYED);
								EnableMenuItem(hMenu, ID_END_CAPTURE, MF_ENABLED);
								if (!externalReadScreen) {
									EnableMenuItem(hMenu, FULL_SCREEN, MF_GRAYED);           //Disables fullscreen menu
									SendMessage(hTool, TB_ENABLEBUTTON, FULL_SCREEN, FALSE); //Disables fullscreen button
								}
								SetStatusTranslatedString(hStatus, 0, "Recording avi...");
								EnableEmulationMenuItems(TRUE);
								recording = TRUE;
							}
						}

						if (emu_launched && emu_paused && !wasPaused)
							resumeEmu(FALSE);
					}

					break;
				case ID_END_CAPTURE:
					if (VCR_stopCapture() < 0)
						MessageBox(NULL, "Couldn't stop capturing.", "VCR", MB_OK);
					else {
						SetWindowPos(mainHWND, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
						EnableMenuItem(hMenu, ID_END_CAPTURE, MF_GRAYED);
						EnableMenuItem(hMenu, ID_START_CAPTURE, MF_ENABLED);
						EnableMenuItem(hMenu, ID_FFMPEG_START, MF_ENABLED);
						EnableMenuItem(hMenu, ID_START_CAPTURE_PRESET, MF_ENABLED);
						SetStatusTranslatedString(hStatus, 0, "Stopped video capture");
						recording = FALSE;
					}
					break;
				case ID_GENERATE_ROM_INFO:
					generateRomInfo();
					break;
				case ID_LANG_INFO_MENU:
				{
					BOOL wasMenuPaused = MenuPaused;
					MenuPaused = FALSE;
					ret = DialogBox(GetModuleHandle(NULL),
						MAKEINTRESOURCE(IDD_LANG_INFO), hwnd, LangInfoProc);
					if (wasMenuPaused) {
						resumeEmu(TRUE);
					}
				}

				break;
				case GENERATE_BITMAP: // take/capture a screenshot

					if (Config.is_default_screenshots_directory_used) {
						sprintf(path_buffer, "%sScreenShots\\", AppPath);
						CaptureScreen(path_buffer);
					} else {
						CaptureScreen(Config.screenshots_directory);
					}
					break;
				case ID_RECENTROMS_RESET:
					ClearRecentList(hwnd, TRUE);
					SetRecentList(hwnd);
					break;
				case ID_RECENTROMS_FREEZE:
					FreezeRecentRoms(hwnd, TRUE);
					break;
				case ID_LOAD_LATEST:
					RunRecentRom(ID_RECENTROMS_FIRST);
				case ID_LOG_WINDOW:
					ShowHideLogWindow();
					break;
				case ID_KAILLERA:
					CreateThread(NULL, 0, KailleraThread, NULL, 0, &SOUNDTHREADID);
					break;
				case IDC_GUI_TOOLBAR:
					Config.is_toolbar_enabled = 1 - Config.is_toolbar_enabled;
					EnableToolbar();
					if (Config.is_toolbar_enabled) CheckMenuItem(hMenu, IDC_GUI_TOOLBAR, MF_BYCOMMAND | MF_CHECKED);
					else CheckMenuItem(hMenu, IDC_GUI_TOOLBAR, MF_BYCOMMAND | MF_UNCHECKED);
					break;
				case IDC_GUI_STATUSBAR:
					Config.is_statusbar_enabled = 1 - Config.is_statusbar_enabled;
					EnableStatusbar();
					if (Config.is_statusbar_enabled) CheckMenuItem(hMenu, IDC_GUI_STATUSBAR, MF_BYCOMMAND | MF_CHECKED);
					else CheckMenuItem(hMenu, IDC_GUI_STATUSBAR, MF_BYCOMMAND | MF_UNCHECKED);
					break;
				case IDC_INCREASE_MODIFIER:
					if (Config.fps_modifier < 50)
						Config.fps_modifier = Config.fps_modifier + 5;
					else if (Config.fps_modifier < 100)
						Config.fps_modifier = Config.fps_modifier + 10;
					else if (Config.fps_modifier < 200)
						Config.fps_modifier = Config.fps_modifier + 25;
					else if (Config.fps_modifier < 1000)
						Config.fps_modifier = Config.fps_modifier + 50;
					if (Config.fps_modifier > 1000)
						Config.fps_modifier = 1000;
					InitTimer();
					break;
				case IDC_DECREASE_MODIFIER:
					if (Config.fps_modifier > 200)
						Config.fps_modifier = Config.fps_modifier - 50;
					else if (Config.fps_modifier > 100)
						Config.fps_modifier = Config.fps_modifier - 25;
					else if (Config.fps_modifier > 50)
						Config.fps_modifier = Config.fps_modifier - 10;
					else if (Config.fps_modifier > 5)
						Config.fps_modifier = Config.fps_modifier - 5;
					if (Config.fps_modifier < 5)
						Config.fps_modifier = 5;
					InitTimer();
					break;
				case IDC_RESET_MODIFIER:
					Config.fps_modifier = 100;
					InitTimer();
					break;
				default:
					 //Language Support  from ID_LANG_ENGLISH to ID_LANG_ENGLISH+100
					if (LOWORD(wParam) >= ID_LANG_ENGLISH && LOWORD(wParam) <= (ID_LANG_ENGLISH + 100)) {
						SelectLang(hwnd, LOWORD(wParam));
						TranslateMenu(GetMenu(hwnd), hwnd);
						TranslateBrowserHeader(hRomList);
						ShowTotalRoms();
					} else if (LOWORD(wParam) >= ID_CURRENTSAVE_1 && LOWORD(wParam) <= ID_CURRENTSAVE_10) {
						SelectState(hwnd, LOWORD(wParam));
					} else if (LOWORD(wParam) >= ID_SAVE_1 && LOWORD(wParam) <= ID_SAVE_10) {
						SaveTheState(hwnd, LOWORD(wParam));
					} else if (LOWORD(wParam) >= ID_LOAD_1 && LOWORD(wParam) <= ID_LOAD_10) {
						LoadTheState(hwnd, LOWORD(wParam));
					} else if (LOWORD(wParam) >= ID_RECENTROMS_FIRST && LOWORD(wParam) < (ID_RECENTROMS_FIRST + MAX_RECENT_ROMS)) {
						RunRecentRom(LOWORD(wParam));
					} else if (LOWORD(wParam) >= ID_RECENTMOVIES_FIRST && LOWORD(wParam) < (ID_RECENTMOVIES_FIRST + MAX_RECENT_MOVIE)) {
						if (RunRecentMovie(LOWORD(wParam)) != SUCCESS) {
							SetStatusTranslatedString(hStatus, 0, "Could not load movie!");
							break;
						}
						// should probably make this code from the ID_REPLAY_LATEST case into a function on its own
						// because now it's used here too
						EnableMenuItem(hMenu, ID_STOP_RECORD, MF_GRAYED);
						EnableMenuItem(hMenu, ID_STOP_PLAYBACK, MF_ENABLED);

						if (!emu_paused || !emu_launched)
							SetStatusTranslatedString(hStatus, 0, "Playback started...");
						else
							SetStatusTranslatedString(hStatus, 0, "Playback started. (Paused)");
					} else if (LOWORD(wParam) >= ID_LUA_RECENT && LOWORD(wParam) < (ID_LUA_RECENT + LUA_MAX_RECENT)) {
						printf("run recent script\n");
						RunRecentScript(LOWORD(wParam));
					}
					break;
			}
		}
		break;
		default:
			return DefWindowProc(hwnd, Message, wParam, lParam);
	}

	return TRUE;
}

//starts m64 and avi
//this is called from game thread because otherwise gfx plugin tries to resize window,
//but main thread waits for game thread to finish loading, and hangs.
void StartMovies() {
	//-m64, -g
	HMENU hMenu = GetMenu(mainHWND);
	printf("------thread done------\n");
	if (CmdLineParameterExist(CMDLINE_PLAY_M64) && CmdLineParameterExist(CMDLINE_GAME_FILENAME)) {
		char file[MAX_PATH];
		GetCmdLineParameter(CMDLINE_PLAY_M64, file);
		//not reading author nor description atm
		VCR_setReadOnly(TRUE);
		VCR_startPlayback(file, "", "");
		if (CmdLineParameterExist(CMDLINE_CAPTURE_AVI)) {
			GetCmdLineParameter(CMDLINE_CAPTURE_AVI, file);
			if (VCR_startCapture(0, file, false) < 0) {
				MessageBox(NULL, "Couldn't start capturing.", "VCR", MB_OK);
				recording = FALSE;
			} else {
				gStopAVI = true;
				SetWindowPos(mainHWND, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);  //Set on top
				EnableMenuItem(hMenu, ID_START_CAPTURE, MF_GRAYED);
				EnableMenuItem(hMenu, ID_START_CAPTURE_PRESET, MF_GRAYED);
				EnableMenuItem(hMenu, ID_END_CAPTURE, MF_ENABLED);
				if (!externalReadScreen) {
					EnableMenuItem(hMenu, FULL_SCREEN, MF_GRAYED);           //Disables fullscreen menu
					SendMessage(hTool, TB_ENABLEBUTTON, FULL_SCREEN, FALSE); //Disables fullscreen button
				}
				SetStatusTranslatedString(hStatus, 0, "Recording avi...");
				recording = TRUE;
			}
		}
		resumeEmu(FALSE);
	}
}

//-lua, -g
// runs multiple lua scripts with paths seperated by ;
// Ex: "path\script1.lua;path\script2.lua"
// From testing only works with 2 scripts ?
void StartLuaScripts() {
	HMENU hMenu = GetMenu(mainHWND);
	if (CmdLineParameterExist(CMDLINE_LUA) && CmdLineParameterExist(CMDLINE_GAME_FILENAME)) {
		char files[MAX_PATH];
		GetCmdLineParameter(CMDLINE_LUA, files);
		int len = strlen(files);
		int numScripts = 1;
		int scriptStartPositions[MAX_LUA_OPEN_AND_RUN_INSTANCES] = {0};
		for (int i = 0; i < len; ++i) {
			if (files[i] == ';') {
				files[i] = 0; // turn ; into \0 so we can copy each part easily
				scriptStartPositions[numScripts] = i + 1;
				++numScripts;
				if (numScripts >= MAX_LUA_OPEN_AND_RUN_INSTANCES) {
					break;
				}
			}
		}
		char file[MAX_PATH];
		for (int i = 0; i < numScripts; ++i) {
			strcpy(file, &files[scriptStartPositions[i]]);
			LuaOpenAndRun(file);
		}
	}
}

//-st, -g
void StartSavestate() {
	HMENU hMenu = GetMenu(mainHWND);
	if (CmdLineParameterExist(CMDLINE_SAVESTATE) && CmdLineParameterExist(CMDLINE_GAME_FILENAME)
		&& !CmdLineParameterExist(CMDLINE_PLAY_M64)) {
		char file[MAX_PATH];
		GetCmdLineParameter(CMDLINE_SAVESTATE, file);
		savestates_select_filename(file);
		savestates_job = LOADSTATE;
	}
}

// Loads various variables from the current config state
void LoadConfigExternals() {
	if (VCR_isLooping() != Config.is_movie_loop_enabled) VCR_toggleLoopMovie();
}

// kaboom
LONG WINAPI ExceptionReleaseTarget(_EXCEPTION_POINTERS* ExceptionInfo) {
	// generate crash log

	char crashLog[1024 * 4] = {0};
	CrashHelper::GenerateLog(ExceptionInfo, crashLog);

	FILE* f = fopen("crash.log", "w+");
	fwrite(crashLog, sizeof(crashLog), 1, f);
	fclose(f);

	bool isIgnorable = !(ExceptionInfo->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE);

	printf("exception occured! creating crash dialog...\n");
	CrashHandlerDialog crashHandlerDialog(isIgnorable ? CrashHandlerDialog::Types::Ignorable : (CrashHandlerDialog::Types)0, "An exception has been thrown and a crash log has been automatically generated.\r\nPlease choose a way to proceed.");

	auto result = crashHandlerDialog.Show();

	switch (result) {
		case CrashHandlerDialog::Choices::Ignore:
			return EXCEPTION_CONTINUE_EXECUTION;
		case CrashHandlerDialog::Choices::Exit:
			return EXCEPTION_EXECUTE_HANDLER;
	}

	return EXCEPTION_EXECUTE_HANDLER;
}


int WINAPI WinMain(
	HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	//timeBeginPeriod(1);
#ifdef _DEBUG
	AllocConsole();
	FILE* f = 0;
	freopen_s(&f, "CONIN$", "r", stdin);
	freopen_s(&f, "CONOUT$", "w", stdout);
	freopen_s(&f, "CONOUT$", "w", stderr);
	printf("mupen64 debug console\n");
#endif
	/* Put absolute App path to AppPath variable */
	getAppFullPath(AppPath);
	app_hInstance = hInstance;
	InitCommonControls();
	SaveCmdLineParameter(lpCmdLine);
	printf("cmd: \"%s\"\n", lpCmdLine);
	ini_openFile();

	// ensure folders exist!
	{
		String path = AppPath;
		CreateDirectory((path + "save").c_str(), NULL);
		CreateDirectory((path + "Mempaks").c_str(), NULL);
		CreateDirectory((path + "Lang").c_str(), NULL);
		CreateDirectory((path + "ScreenShots").c_str(), NULL);
		CreateDirectory((path + "plugin").c_str(), NULL);
	}
	emu_launched = 0;
	emu_paused = 1;
	/************    Loading Config  *******/
	LoadConfig();
  /************************************************************************/

	WNDCLASSEX wc;
	HWND hwnd;
	MSG Msg;
	HACCEL Accel;

	if (GuiDisabled()) {
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = 0;
		wc.lpfnWndProc = NoGuiWndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInstance;
		wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_M64ICONBIG));
		wc.hIconSm = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_M64ICONSMALL));
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = g_szClassName;

		if (!RegisterClassEx(&wc)) {
			MessageBox(NULL, "Window Registration Failed!", "Error!",
				MB_ICONEXCLAMATION | MB_OK);
			return 0;
		}

		hwnd = CreateWindowEx(
			0,
			g_szClassName,
			MUPEN_VERSION,
			WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
			Config.window_x, Config.window_y, Config.window_width, Config.window_height,
			NULL, NULL, hInstance, NULL);

	#ifdef _DEBUG
		GUI_CreateLogWindow(hwnd);
	#endif
		mainHWND = hwnd;
		ShowWindow(hwnd, nCmdShow);
		UpdateWindow(hwnd);

		StartGameByCommandLine();

		ShowInfo(MUPEN_VERSION " - Nintendo 64 emulator - Guiless mode");

		while (GetMessage(&Msg, NULL, 0, 0) > 0) {
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	} else {
	//window initialize
	#if 1
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = 0;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInstance;
		wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_M64ICONBIG));
		wc.hIconSm = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_M64ICONSMALL));
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);//(HBRUSH)(COLOR_WINDOW+11);
		wc.lpszMenuName = MAKEINTRESOURCE(IDR_MYMENU);
		wc.lpszClassName = g_szClassName;

		if (!RegisterClassEx(&wc)) {
			MessageBox(NULL, "Window Registration Failed!", "Error!",
				MB_ICONEXCLAMATION | MB_OK);
			return 0;
		}

		Accel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCEL));

		hwnd = CreateWindowEx(
			0,
			g_szClassName,
			MUPEN_VERSION,
			WS_OVERLAPPEDWINDOW | WS_EX_COMPOSITED,
			Config.window_x, Config.window_y, Config.window_width, Config.window_height,
			NULL, NULL, hInstance, NULL);

		if (hwnd == NULL) {
			MessageBox(NULL, "Window Creation Failed!", "Error!",
				MB_ICONEXCLAMATION | MB_OK);
			return 0;
		}
		mainHWND = hwnd;
		ShowWindow(hwnd, nCmdShow);

		// init FFmpeg
		//auto err = InitFFMPEGTest();

		// This fixes offscreen recording issue
		SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_ACCEPTFILES | WS_EX_LAYERED); //this can't be applied before ShowWindow(), otherwise you must use some fancy function
		BringWindowToTop(hRomList);
		ListView_SetExtendedListViewStyleEx(hRomList, LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER);

		UpdateWindow(hwnd);
	#endif
		EnableMenuItem(GetMenu(hwnd), ID_LOG_WINDOW, MF_DISABLED);
	#ifdef _DEBUG
		if (GUI_CreateLogWindow(mainHWND)) EnableMenuItem(GetMenu(hwnd), ID_LOG_WINDOW, MF_ENABLED);
	#endif




		if (!isKailleraExist()) {
			DeleteMenu(GetMenu(hwnd), ID_KAILLERA, MF_BYCOMMAND);
		}

		SetupDummyInfo();

		EnableEmulationMenuItems(0);
		if (!StartGameByCommandLine()) {
			cmdlineMode = 0;
		}

		ShowInfo(MUPEN_VERSION " - Mupen64 - Nintendo 64 emulator - GUI mode");

		LoadConfigExternals();

		//warning, this is ignored when debugger is attached (like visual studio)
		SetUnhandledExceptionFilter(ExceptionReleaseTarget);

		// raise noncontinuable exception (impossible to recover from it)
		//RaiseException(EXCEPTION_ACCESS_VIOLATION, EXCEPTION_NONCONTINUABLE, NULL, NULL);
		// 
		// raise continuable exception
		//RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, NULL, NULL);


		while (GetMessage(&Msg, NULL, 0, 0) > 0) {
			if (!TranslateAccelerator(mainHWND, Accel, &Msg)) {
				TranslateMessage(&Msg);
				DispatchMessage(&Msg);
			}

			// modifier-only checks, cannot be obtained through windows messaging...
			int i;
			for (i = 0; i < NUM_HOTKEYS; i++) {
				if (!Config.hotkeys[i].key && (Config.hotkeys[i].shift || Config.hotkeys[i].ctrl || Config.hotkeys[i].alt)) {
					if (i != 0) {
						if (((GetKeyState(VK_SHIFT) & 0x8000) ? 1 : 0) == Config.hotkeys[i].shift
							&& ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0) == Config.hotkeys[i].ctrl
							&& ((GetKeyState(VK_MENU) & 0x8000) ? 1 : 0) == Config.hotkeys[i].alt) {
							SendMessage(hwnd, WM_COMMAND, Config.hotkeys[i].command, 0);
						}
					} else // fast-forward 
					{
						extern int frame_advancing;
						if (!frame_advancing) { // dont allow fastforward+frameadvance
							if (((GetKeyState(VK_SHIFT) & 0x8000) ? 1 : 0) == Config.hotkeys[i].shift
								&& ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0) == Config.hotkeys[i].ctrl
								&& ((GetKeyState(VK_MENU) & 0x8000) ? 1 : 0) == Config.hotkeys[i].alt) {
								manualFPSLimit = 0;
							} else {
								manualFPSLimit = 1;
							}
						}
					}
				}
			}
		}
	}

	CloseLogWindow();
	CloseKaillera();
	return Msg.wParam;
}
