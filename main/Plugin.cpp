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


#include "Plugin.hpp"

#include <assert.h>

#include "rom.h"
#include "win/Config.hpp"
#include "../memory/memory.h"
#include <win/main_win.h>

extern HWND mainHWND;
extern HWND hStatus;
extern HINSTANCE app_hInstance;

CONTROL Controls[4];

GFX_INFO dummy_gfx_info;
AUDIO_INFO dummy_audio_info;
CONTROL_INFO dummy_control_info;
RSP_INFO dummy_rsp_info;
unsigned char dummy_header[0x40];

GFX_INFO gfx_info;
AUDIO_INFO audio_info;
CONTROL_INFO control_info;
RSP_INFO rsp_info;

std::vector<t_plugin*> plugins;

t_plugin* video_plugin = nullptr;
t_plugin* audio_plugin = nullptr;
t_plugin* input_plugin = nullptr;
t_plugin* rsp_plugin = nullptr;

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

void(__cdecl* moveScreen)(int xpos, int ypos);
void(__cdecl* CaptureScreen) (char* Directory);
void(__cdecl* old_initiateControllers)(HWND hMainWindow, CONTROL Controls[4]);
void(__cdecl* aiUpdate)(BOOL Wait);

int load_gfx(HMODULE handle_gfx)
{
	if (handle_gfx)
	{
		changeWindow = (void(__cdecl*)())GetProcAddress(
			handle_gfx, "ChangeWindow");
		closeDLL_gfx = (void(__cdecl*)())GetProcAddress(handle_gfx, "CloseDLL");
		dllAbout = (void(__cdecl*)(HWND hParent))GetProcAddress(
			handle_gfx, "DllAbout");
		dllConfig = (void(__cdecl*)(HWND hParent))GetProcAddress(
			handle_gfx, "DllConfig");
		dllTest = (void(__cdecl*)(HWND hParent))GetProcAddress(
			handle_gfx, "DllTest");
		initiateGFX = (BOOL(__cdecl*)(GFX_INFO Gfx_Info))GetProcAddress(
			handle_gfx, "InitiateGFX");
		processDList = (void(__cdecl*)())GetProcAddress(
			handle_gfx, "ProcessDList");
		processRDPList = (void(__cdecl*)())GetProcAddress(
			handle_gfx, "ProcessRDPList");
		romClosed_gfx = (void(__cdecl*)())GetProcAddress(
			handle_gfx, "RomClosed");
		romOpen_gfx = (void(__cdecl*)())GetProcAddress(handle_gfx, "RomOpen");
		showCFB = (void(__cdecl*)())GetProcAddress(handle_gfx, "ShowCFB");
		updateScreen = (void(__cdecl*)())GetProcAddress(
			handle_gfx, "UpdateScreen");
		viStatusChanged = (void(__cdecl*)())GetProcAddress(
			handle_gfx, "ViStatusChanged");
		viWidthChanged = (void(__cdecl*)())GetProcAddress(
			handle_gfx, "ViWidthChanged");
		moveScreen = (void(__cdecl*)(int, int))GetProcAddress(
			handle_gfx, "MoveScreen");
		CaptureScreen = (void(__cdecl*)(char* Directory))GetProcAddress(
			handle_gfx, "CaptureScreen");
		if (Config.is_internal_capture_forced)
		{
			readScreen = NULL;
			externalReadScreen = 0;
			DllCrtFree = free;
		} else
		{
			readScreen = (void(__cdecl
				*)(void** dest, long* width, long* height))GetProcAddress(
				handle_gfx, "ReadScreen");
			if (readScreen == NULL)
			{
				//try to find readscreen2 instead (gln64)
				readScreen = (void(__cdecl*)(void** dest, long* width,
				                             long* height))GetProcAddress(
					handle_gfx, "ReadScreen2");
				if (readScreen == NULL)
				{
					externalReadScreen = 0;
					DllCrtFree = free;
				}
			}
			if (readScreen)
			{
				externalReadScreen = 1;
				DllCrtFree = (void(__cdecl*)(void*))GetProcAddress(
					handle_gfx, "DllCrtFree");
				if (DllCrtFree == NULL) DllCrtFree = free;
				//attempt to match the crt, avi capture will probably crash without this
			}
		}

		fBRead = (void(__cdecl*)(DWORD))GetProcAddress(handle_gfx, "FBRead");
		fBWrite = (void(__cdecl*)(DWORD, DWORD))GetProcAddress(
			handle_gfx, "FBWrite");
		fBGetFrameBufferInfo = (void(__cdecl*)(void*))GetProcAddress(
			handle_gfx, "FBGetFrameBufferInfo");

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
		if (CaptureScreen == NULL)
			CaptureScreen = (void(__cdecl*)(char*))
				dummy_void;
		if (moveScreen == NULL)
			moveScreen = (void(__cdecl*)(int, int))
				dummy_void;

		gfx_info.hWnd = mainHWND;
		if (Config.is_statusbar_enabled)
		{
			gfx_info.hStatusBar = hStatus;
		} else
		{
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
		gfx_info.CheckInterrupts = dummy_void;
		initiateGFX(gfx_info);
	} else
	{
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

int load_input(HMODULE handle_input)
{
	int i;
	PLUGIN_INFO PluginInfo;
	if (handle_input)
	{
		getDllInfo = (void(__cdecl*)(PLUGIN_INFO* PluginInfo))GetProcAddress(
			handle_input, "GetDllInfo");
		getDllInfo(&PluginInfo);

		closeDLL_input = (void(__cdecl*)())GetProcAddress(
			handle_input, "CloseDLL");
		controllerCommand = (void(__cdecl*)(int Control, BYTE* Command))
			GetProcAddress(handle_input, "ControllerCommand");
		getKeys = (void(__cdecl*)(int Control, BUTTONS* Keys))GetProcAddress(
			handle_input, "GetKeys");
		setKeys = (void(__cdecl*)(int Control, BUTTONS Keys))GetProcAddress(
			handle_input, "SetKeys");
		if (PluginInfo.Version == 0x0101)
			initiateControllers = (void(__cdecl*)(CONTROL_INFO ControlInfo))
				GetProcAddress(handle_input, "InitiateControllers");
		else
			old_initiateControllers = (void(__cdecl*)(
				HWND hMainWindow, CONTROL Controls[4]))GetProcAddress(
				handle_input, "InitiateControllers");
		readController = (void(__cdecl*)(int Control, BYTE* Command))
			GetProcAddress(handle_input, "ReadController");
		romClosed_input = (void(__cdecl*)())GetProcAddress(
			handle_input, "RomClosed");
		romOpen_input = (void(__cdecl*)())GetProcAddress(
			handle_input, "RomOpen");
		keyDown = (void(__cdecl*)(WPARAM wParam, LPARAM lParam))GetProcAddress(
			handle_input, "WM_KeyDown");
		keyUp = (void(__cdecl*)(WPARAM wParam, LPARAM lParam))GetProcAddress(
			handle_input, "WM_KeyUp");

		if (closeDLL_input == NULL) closeDLL_input = dummy_void;
		if (controllerCommand == NULL)
			controllerCommand =
				dummy_controllerCommand;
		if (getKeys == NULL) getKeys = dummy_getKeys;
		if (setKeys == NULL) setKeys = dummy_setKeys;
		if (initiateControllers == NULL)
			initiateControllers =
				dummy_initiateControllers;
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
		for (i = 0; i < 4; i++)
		{
			Controls[i].Present = FALSE;
			Controls[i].RawData = FALSE;
			Controls[i].Plugin = controller_extension::none;
		}
		if (PluginInfo.Version == 0x0101)
		{
			initiateControllers(control_info);
		} else
		{
			old_initiateControllers(mainHWND, Controls);
		}
	} else
	{
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


int load_sound(HMODULE handle_sound)
{
	if (handle_sound)
	{
		closeDLL_audio = (void(__cdecl*)(void))GetProcAddress(
			handle_sound, "CloseDLL");
		aiDacrateChanged = (void(__cdecl*)(int))GetProcAddress(
			handle_sound, "AiDacrateChanged");
		aiLenChanged = (void(__cdecl*)(void))GetProcAddress(
			handle_sound, "AiLenChanged");
		aiReadLength = (DWORD(__cdecl*)(void))GetProcAddress(
			handle_sound, "AiReadLength");
		initiateAudio = (BOOL(__cdecl*)(AUDIO_INFO))GetProcAddress(
			handle_sound, "InitiateAudio");
		romClosed_audio = (void(__cdecl*)(void))GetProcAddress(
			handle_sound, "RomClosed");
		romOpen_audio = (void(__cdecl*)(void))GetProcAddress(
			handle_sound, "RomOpen");
		processAList = (void(__cdecl*)(void))GetProcAddress(
			handle_sound, "ProcessAList");
		aiUpdate = (void(__cdecl*)(BOOL))GetProcAddress(handle_sound, "AiUpdate");
		auto a = GetLastError();

		if (aiDacrateChanged == NULL) aiDacrateChanged = dummy_aiDacrateChanged;
		if (aiLenChanged == NULL) aiLenChanged = dummy_void;
		if (aiReadLength == NULL) aiReadLength = dummy_aiReadLength;
		if (aiUpdate == NULL) aiUpdate = dummy_aiUpdate;
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

		audio_info.MI_INTR_REG = &dummy; //&(MI_register.mi_intr_reg);

		audio_info.AI_DRAM_ADDR_REG = &(ai_register.ai_dram_addr);
		audio_info.AI_LEN_REG = &(ai_register.ai_len);
		audio_info.AI_CONTROL_REG = &(ai_register.ai_control);
		audio_info.AI_STATUS_REG = &dummy; //&(ai_register.ai_status);
		audio_info.AI_DACRATE_REG = &(ai_register.ai_dacrate);
		audio_info.AI_BITRATE_REG = &(ai_register.ai_bitrate);

		audio_info.CheckInterrupts = dummy_void;
		initiateAudio(audio_info);
	} else
	{
		aiDacrateChanged = dummy_aiDacrateChanged;
		aiLenChanged = dummy_void;
		aiReadLength = dummy_aiReadLength;
		closeDLL_audio = dummy_void;
		initiateAudio = dummy_initiateAudio;
		processAList = dummy_void;
		romClosed_audio = dummy_void;
		romOpen_audio = dummy_void;
	}
	return 0;
}

int load_rsp(HMODULE handle_RSP)
{
	int i = 4;
	if (handle_RSP)
	{
		closeDLL_RSP = (void(__cdecl*)(void))GetProcAddress(
			handle_RSP, "CloseDLL");
		doRspCycles = (DWORD(__cdecl*)(DWORD))GetProcAddress(
			handle_RSP, "DoRspCycles");
		initiateRSP = (void(__cdecl*)(RSP_INFO, DWORD*))GetProcAddress(
			handle_RSP, "InitiateRSP");
		romClosed_RSP = (void(__cdecl*)(void))GetProcAddress(
			handle_RSP, "RomClosed");

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
		rsp_info.CheckInterrupts = dummy_void;
		rsp_info.ProcessDlistList = processDList;
		rsp_info.ProcessAlistList = processAList;
		rsp_info.ProcessRdpList = processRDPList;
		rsp_info.ShowCFB = showCFB;
		initiateRSP(rsp_info, (DWORD*)&i);
	} else
	{
		closeDLL_RSP = dummy_void;
		doRspCycles = dummy_doRspCycles;
		initiateRSP = dummy_initiateRSP;
		romClosed_RSP = dummy_void;
	}
	return 0;
}

t_plugin* plugin_create(std::string path)
{
	t_plugin* plugin = new t_plugin();

	HMODULE h_module = LoadLibrary(path.c_str());

	if (h_module == nullptr)
	{
		return 0;
	}

	const auto get_dll_info = ProcAddress(h_module, "GetDllInfo");

	if (!get_dll_info._fp)
	{
		FreeLibrary(h_module);
		return 0;
	}

	getDllInfo = get_dll_info;

	PLUGIN_INFO plugin_info;
	getDllInfo(&plugin_info);

	while (plugin_info.Name[strlen(plugin_info.Name) - 1] == ' ')
	{
		plugin_info.Name[strlen(plugin_info.Name) - 1] = '\0';
	}

	plugin->handle = h_module;
	plugin->name = std::string(plugin_info.Name);
	plugin->type = static_cast<plugin_type>(plugin_info.Type);
	plugin->version = plugin_info.Version;
	plugin->path = path;

	return plugin;
}

void plugin_destroy(t_plugin* plugin)
{
	if (plugin == nullptr || plugin->handle == nullptr) {
		return;
	}
	printf("Plugin %s destroyed\n", plugin->path.c_str());
	FreeLibrary(plugin->handle);
	delete plugin;
	plugin = nullptr;
}

void plugin_config(t_plugin* plugin)
{
	switch (plugin->type)
	{
	case plugin_type::video:
		if (!emu_launched)
		{
			initiateGFX = (BOOL(__cdecl*)(GFX_INFO Gfx_Info))GetProcAddress(
				plugin->handle, "InitiateGFX");
			if (!initiateGFX(dummy_gfx_info))
			{
				MessageBox(mainHWND, "Failed to initiate gfx plugin.", NULL,
				           MB_ICONERROR);
			}
		}

		dllConfig = (void(__cdecl*)(HWND hParent))GetProcAddress(
			plugin->handle, "DllConfig");
		if (dllConfig) dllConfig(hwnd_plug);

		if (!emu_launched)
		{
			closeDLL_gfx = (void(__cdecl*)())GetProcAddress(
				plugin->handle, "CloseDLL");
			if (closeDLL_gfx) closeDLL_gfx();
		}
		break;
	case plugin_type::audio:
		if (!emu_launched)
		{
			initiateAudio = (BOOL(__cdecl*)(AUDIO_INFO))GetProcAddress(
				plugin->handle, "InitiateAudio");
			if (!initiateAudio(dummy_audio_info))
			{
				ShowMessage("Failed to initialize audio plugin.");
			}
		}

		dllConfig = (void(__cdecl*)(HWND hParent))GetProcAddress(
			plugin->handle, "DllConfig");
		if (dllConfig) dllConfig(hwnd_plug);
		if (!emu_launched)
		{
			closeDLL_audio = (void(__cdecl*)())GetProcAddress(
				plugin->handle, "CloseDLL");
			if (closeDLL_audio) closeDLL_audio();
		}
		break;
	case plugin_type::input:
		if (!emu_launched)
		{
			if (plugin->version == 0x0101)
			{
				initiateControllers = (void(__cdecl*)(CONTROL_INFO ControlInfo))
					GetProcAddress(plugin->handle, "InitiateControllers");
				initiateControllers(dummy_control_info);
			} else
			{
				old_initiateControllers = (void(__cdecl*)(
					HWND hMainWindow, CONTROL Controls[4]))GetProcAddress(
					plugin->handle, "InitiateControllers");
				old_initiateControllers(mainHWND, Controls);
			}
		}

		dllConfig = (void(__cdecl*)(HWND hParent))GetProcAddress(
			plugin->handle, "DllConfig");
		if (dllConfig) dllConfig(hwnd_plug);

		if (!emu_launched)
		{
			closeDLL_input = (void(__cdecl*)())GetProcAddress(
				plugin->handle, "CloseDLL");
			if (closeDLL_input) closeDLL_input();
		}

		break;
	case plugin_type::rsp:
		if (!emu_launched)
		{
			initiateRSP = (void(__cdecl*)(RSP_INFO, DWORD*))GetProcAddress(
				plugin->handle, "InitiateRSP");
			DWORD i = 0;
			initiateRSP(dummy_rsp_info, (DWORD*)&i);
		}

		dllConfig = (void(__cdecl*)(HWND hParent))GetProcAddress(
			plugin->handle, "DllConfig");
		if (dllConfig) dllConfig(hwnd_plug);
		if (!emu_launched)
		{
			closeDLL_RSP = (void(__cdecl*)())GetProcAddress(
				plugin->handle, "CloseDLL");
			if (closeDLL_RSP) closeDLL_RSP();
		}
		break;
	default:
		assert(false);
		break;
	}
}

void plugin_test(t_plugin* plugin)
{
}

void plugin_about(t_plugin* plugin)
{
}

std::string get_plugins_directory()
{
	return Config.is_default_plugins_directory_used
		       ? std::string("plugins\\")
		       : Config.plugins_directory;
}


void destroy_plugins() {
	for (auto plugin : plugins) {
		plugin_destroy(plugin);
	}
	plugins.clear();
}

void search_plugins()
{
	if (!plugins.empty())
	{
		destroy_plugins();
	}

	std::vector<std::string> files = get_files_with_extension_in_directory(
		get_plugins_directory(), "dll");

	for (const auto& file : files)
	{
		t_plugin* plugin = plugin_create(file);
		if (plugin == nullptr) {
			printf("Plugin %s is busted\n", file.c_str());
			continue;
		}
		plugins.push_back(plugin);
		printf("Plugin %s loaded\n", file.c_str());
	}
}

void load_plugins()
{
	search_plugins();

	video_plugin = get_plugin_by_name(Config.selected_video_plugin_name);
	audio_plugin = get_plugin_by_name(Config.selected_audio_plugin_name);
	input_plugin = get_plugin_by_name(Config.selected_input_plugin_name);
	rsp_plugin = get_plugin_by_name(Config.selected_rsp_plugin_name);

	load_gfx(video_plugin->handle);
	load_sound(audio_plugin->handle);
	load_input(input_plugin->handle);
	load_rsp(rsp_plugin->handle);
}


void setup_dummy_info()
{
	int i;

	/////// GFX ///////////////////////////
	dummy_gfx_info.hWnd = hStatus;
	dummy_gfx_info.hStatusBar = hStatus;
	dummy_gfx_info.MemoryBswaped = TRUE;
	dummy_gfx_info.HEADER = (BYTE*)dummy_header;
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
	dummy_gfx_info.CheckInterrupts = dummy_void;

	/////// AUDIO /////////////////////////
	dummy_audio_info.hwnd = mainHWND;
	dummy_audio_info.hinst = app_hInstance;
	dummy_audio_info.MemoryBswaped = TRUE;
	dummy_audio_info.HEADER = (BYTE*)dummy_header;
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
	dummy_audio_info.CheckInterrupts = dummy_void;

	///// CONTROLS ///////////////////////////
	dummy_control_info.hMainWindow = mainHWND;
	dummy_control_info.hinst = app_hInstance;
	dummy_control_info.MemoryBswaped = TRUE;
	dummy_control_info.HEADER = (BYTE*)dummy_header;
	dummy_control_info.Controls = Controls;
	for (i = 0; i < 4; i++)
	{
		Controls[i].Present = FALSE;
		Controls[i].RawData = FALSE;
		Controls[i].Plugin = controller_extension::none;
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
	dummy_rsp_info.CheckInterrupts = dummy_void;
	dummy_rsp_info.ProcessDlistList = processDList;
	dummy_rsp_info.ProcessAlistList = processAList;
	dummy_rsp_info.ProcessRdpList = processRDPList;
	dummy_rsp_info.ShowCFB = showCFB;
}
