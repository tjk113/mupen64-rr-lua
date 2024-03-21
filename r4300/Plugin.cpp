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


// ReSharper disable CppCStyleCast
#include "Plugin.hpp"

#include <assert.h>

#include "../r4300/rom.h"
#include <shared/Config.hpp>
#include "../memory/memory.h"
#include "../r4300/r4300.h"
#include <win/main_win.h>
#include <dbghelp.h>

#include "guifuncs.h"
#include "win/features/Statusbar.hpp"

extern HWND mainHWND;
extern HINSTANCE app_instance;

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

std::string get_plugins_directory()
{
	if (Config.is_default_plugins_directory_used)
	{
		return app_path + "plugin\\";
	}
	return Config.plugins_directory;
}

/* dummy functions to prevent mupen from crashing if a plugin is missing */
static void __cdecl dummy_void()
{
}

static int __cdecl dummy_initiateGFX(GFX_INFO Gfx_Info) { return TRUE; }

static int __cdecl dummy_initiateAudio(AUDIO_INFO Audio_Info)
{
	return TRUE;
}

static void __cdecl dummy_initiateControllers(CONTROL_INFO Control_Info)
{
}

static void __cdecl dummy_aiDacrateChanged(int SystemType)
{
}

static DWORD __cdecl dummy_aiReadLength() { return 0; }

static void __cdecl dummy_aiUpdate(BOOL)
{
}

static void __cdecl dummy_controllerCommand(int Control, BYTE* Command)
{
}

static void __cdecl dummy_getKeys(int Control, BUTTONS* Keys)
{
}

static void __cdecl dummy_setKeys(int Control, BUTTONS Keys)
{
}

static void __cdecl dummy_readController(int Control, BYTE* Command)
{
}

static void __cdecl dummy_keyDown(WPARAM wParam, LPARAM lParam)
{
}

static void __cdecl dummy_keyUp(WPARAM wParam, LPARAM lParam)
{
}

static unsigned long dummy;

static void __cdecl dummy_initiateRSP(RSP_INFO Rsp_Info,
                                      unsigned long* CycleCount)
{
};

static void __cdecl dummy_fBRead(DWORD addr)
{
};

static void __cdecl dummy_fBWrite(DWORD addr, DWORD size)
{
};

static void __cdecl dummy_fBGetFrameBufferInfo(void* p)
{
};

GETDLLINFO getDllInfo;
DLLCONFIG dllConfig;
DLLTEST dllTest;
DLLABOUT dllAbout;

CHANGEWINDOW changeWindow = dummy_void;
CLOSEDLL_GFX closeDLL_gfx = dummy_void;
INITIATEGFX initiateGFX = dummy_initiateGFX;
PROCESSDLIST processDList = dummy_void;
PROCESSRDPLIST processRDPList = dummy_void;
ROMCLOSED_GFX romClosed_gfx = dummy_void;
ROMOPEN_GFX romOpen_gfx = dummy_void;
SHOWCFB showCFB = dummy_void;
UPDATESCREEN updateScreen = dummy_void;
VISTATUSCHANGED viStatusChanged = dummy_void;
VIWIDTHCHANGED viWidthChanged = dummy_void;
READSCREEN readScreen;
DLLCRTFREE DllCrtFree;
MOVESCREEN moveScreen;
CAPTURESCREEN CaptureScreen;
GETVIDEOSIZE get_video_size;
READVIDEO read_video;
FBREAD fBRead = dummy_fBRead;
FBWRITE fBWrite = dummy_fBWrite;
FBGETFRAMEBUFFERINFO fBGetFrameBufferInfo = dummy_fBGetFrameBufferInfo;

AIDACRATECHANGED aiDacrateChanged = dummy_aiDacrateChanged;
AILENCHANGED aiLenChanged = dummy_void;
AIREADLENGTH aiReadLength = dummy_aiReadLength;
CLOSEDLL_AUDIO closeDLL_audio = dummy_void;
INITIATEAUDIO initiateAudio = dummy_initiateAudio;
PROCESSALIST processAList = dummy_void;
ROMCLOSED_AUDIO romClosed_audio = dummy_void;
ROMOPEN_AUDIO romOpen_audio = dummy_void;
AIUPDATE aiUpdate = dummy_aiUpdate;

CLOSEDLL_INPUT closeDLL_input = dummy_void;
CONTROLLERCOMMAND controllerCommand = dummy_controllerCommand;
GETKEYS getKeys = dummy_getKeys;
SETKEYS setKeys = dummy_setKeys;
OLD_INITIATECONTROLLERS old_initiateControllers;
INITIATECONTROLLERS initiateControllers = dummy_initiateControllers;
READCONTROLLER readController = dummy_readController;
ROMCLOSED_INPUT romClosed_input = dummy_void;
ROMOPEN_INPUT romOpen_input = dummy_void;
KEYDOWN keyDown = dummy_keyDown;
KEYUP keyUp = dummy_keyUp;

CLOSEDLL_RSP closeDLL_RSP = dummy_void;
DORSPCYCLES doRspCycles = dummy_doRspCycles;
INITIATERSP initiateRSP = dummy_initiateRSP;
ROMCLOSED_RSP romClosed_RSP = dummy_void;

// Gets module's runtime free function
auto get_dll_crt_free(HMODULE handle)
{
	auto dll_crt_free = (DLLCRTFREE)GetProcAddress(
		handle, "DllCrtFree");
	if (dll_crt_free != nullptr) return dll_crt_free;

	ULONG size;
	auto import_descriptor = (PIMAGE_IMPORT_DESCRIPTOR)
		ImageDirectoryEntryToDataEx(handle, true, IMAGE_DIRECTORY_ENTRY_IMPORT,
		                            &size, nullptr);
	if (import_descriptor != nullptr)
	{
		while (import_descriptor->Characteristics && import_descriptor->Name)
		{
			auto importDllName = (LPCSTR)((PBYTE)handle + import_descriptor->
				Name);
			auto importDllHandle = GetModuleHandleA(importDllName);
			if (importDllHandle != nullptr)
			{
				dll_crt_free = (DLLCRTFREE)GetProcAddress(
					importDllHandle, "free");
				if (dll_crt_free != nullptr) return dll_crt_free;
			}

			import_descriptor++;
		}
	}


	// this is probably always wrong
	return free;
}

void load_gfx(HMODULE handle)
{
	changeWindow = (CHANGEWINDOW)GetProcAddress(handle, "ChangeWindow");
	closeDLL_gfx = (CLOSEDLL_GFX)GetProcAddress(handle, "CloseDLL");
	dllAbout = (DLLABOUT)GetProcAddress(handle, "DllAbout");
	dllConfig = (DLLCONFIG)GetProcAddress(handle, "DllConfig");
	dllTest = (DLLTEST)GetProcAddress(handle, "DllTest");
	initiateGFX = (INITIATEGFX)GetProcAddress(handle, "InitiateGFX");
	processDList = (PROCESSDLIST)GetProcAddress(handle, "ProcessDList");
	processRDPList = (PROCESSRDPLIST)GetProcAddress(handle, "ProcessRDPList");
	romClosed_gfx = (ROMCLOSED_GFX)GetProcAddress(handle, "RomClosed");
	romOpen_gfx = (ROMOPEN_GFX)GetProcAddress(handle, "RomOpen");
	showCFB = (SHOWCFB)GetProcAddress(handle, "ShowCFB");
	updateScreen = (UPDATESCREEN)GetProcAddress(handle, "UpdateScreen");
	viStatusChanged = (VISTATUSCHANGED)GetProcAddress(handle, "ViStatusChanged");
	viWidthChanged = (VIWIDTHCHANGED)GetProcAddress(handle, "ViWidthChanged");
	moveScreen = (MOVESCREEN)GetProcAddress(handle, "MoveScreen");
	CaptureScreen = (CAPTURESCREEN)GetProcAddress(handle, "CaptureScreen");
	readScreen = (READSCREEN)GetProcAddress(handle, "ReadScreen");

	if (readScreen == nullptr)
	{
		// we dont have the primary ReadScreen, so plugin is probably gln which only exports ReadScreen2
		readScreen = (READSCREEN)GetProcAddress(handle, "ReadScreen2");
	}

	// ReadScreen returns a plugin-allocated buffer which must be freed by the same CRT
	DllCrtFree = get_dll_crt_free(handle);

	get_video_size = (GETVIDEOSIZE)GetProcAddress(handle, "mge_get_video_size");
	read_video = (READVIDEO)GetProcAddress(handle, "mge_read_video");
	fBRead = (FBREAD)GetProcAddress(handle, "FBRead");
	fBWrite = (FBWRITE)GetProcAddress(handle, "FBWrite");
	fBGetFrameBufferInfo = (FBGETFRAMEBUFFERINFO)GetProcAddress(handle, "FBGetFrameBufferInfo");

	if (changeWindow == nullptr) changeWindow = dummy_void;
	if (closeDLL_gfx == nullptr) closeDLL_gfx = dummy_void;
	if (initiateGFX == nullptr) initiateGFX = dummy_initiateGFX;
	if (processDList == nullptr) processDList = dummy_void;
	if (processRDPList == nullptr) processRDPList = dummy_void;
	if (romClosed_gfx == nullptr) romClosed_gfx = dummy_void;
	if (romOpen_gfx == nullptr) romOpen_gfx = dummy_void;
	if (showCFB == nullptr) showCFB = dummy_void;
	if (updateScreen == nullptr) updateScreen = dummy_void;
	if (viStatusChanged == nullptr) viStatusChanged = dummy_void;
	if (viWidthChanged == nullptr) viWidthChanged = dummy_void;
	if (CaptureScreen == nullptr) CaptureScreen = (CAPTURESCREEN)dummy_void;
	if (moveScreen == nullptr) moveScreen = (MOVESCREEN)dummy_void;

	gfx_info.hWnd = mainHWND;
	gfx_info.hStatusBar = Config.is_statusbar_enabled ? Statusbar::hwnd() : nullptr;
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
}

void load_input(uint16_t version, HMODULE handle)
{
	int i;

	closeDLL_input = (CLOSEDLL_INPUT)GetProcAddress(handle, "CloseDLL");
	controllerCommand = (CONTROLLERCOMMAND)GetProcAddress(handle, "ControllerCommand");
	getKeys = (GETKEYS)GetProcAddress(
		handle, "GetKeys");
	setKeys = (SETKEYS)GetProcAddress(
		handle, "SetKeys");
	if (version == 0x0101)
		initiateControllers = (INITIATECONTROLLERS)GetProcAddress(handle, "InitiateControllers");
	else
		old_initiateControllers = (OLD_INITIATECONTROLLERS)GetProcAddress(handle, "InitiateControllers");

	readController = (READCONTROLLER)GetProcAddress(handle, "ReadController");
	romClosed_input = (ROMCLOSED_INPUT)GetProcAddress(handle, "RomClosed");
	romOpen_input = (ROMOPEN_INPUT)GetProcAddress(handle, "RomOpen");
	keyDown = (KEYDOWN)GetProcAddress(handle, "WM_KeyDown");
	keyUp = (KEYUP)GetProcAddress(handle, "WM_KeyUp");

	if (closeDLL_input == nullptr) closeDLL_input = dummy_void;
	if (controllerCommand == nullptr)
		controllerCommand =
			dummy_controllerCommand;
	if (getKeys == nullptr) getKeys = dummy_getKeys;
	if (setKeys == nullptr) setKeys = dummy_setKeys;
	if (initiateControllers == nullptr)
		initiateControllers =
			dummy_initiateControllers;
	if (readController == nullptr) readController = dummy_readController;
	if (romClosed_input == nullptr) romClosed_input = dummy_void;
	if (romOpen_input == nullptr) romOpen_input = dummy_void;
	if (keyDown == nullptr) keyDown = dummy_keyDown;
	if (keyUp == nullptr) keyUp = dummy_keyUp;

	control_info.hMainWindow = mainHWND;
	control_info.hinst = app_instance;
	control_info.MemoryBswaped = TRUE;
	control_info.HEADER = rom;
	control_info.Controls = Controls;
	for (i = 0; i < 4; i++)
	{
		Controls[i].Present = FALSE;
		Controls[i].RawData = FALSE;
		Controls[i].Plugin = controller_extension::none;
	}
	if (version == 0x0101)
	{
		initiateControllers(control_info);
	} else
	{
		old_initiateControllers(mainHWND, Controls);
	}
}


void load_audio(HMODULE handle)
{
	closeDLL_audio = (CLOSEDLL_AUDIO)GetProcAddress(handle, "CloseDLL");
	aiDacrateChanged = (AIDACRATECHANGED)GetProcAddress(handle, "AiDacrateChanged");
	aiLenChanged = (AILENCHANGED)GetProcAddress(handle, "AiLenChanged");
	aiReadLength = (AIREADLENGTH)GetProcAddress(handle, "AiReadLength");
	initiateAudio = (INITIATEAUDIO)GetProcAddress(handle, "InitiateAudio");
	romClosed_audio = (ROMCLOSED_AUDIO)GetProcAddress(handle, "RomClosed");
	romOpen_audio = (ROMOPEN_AUDIO)GetProcAddress(handle, "RomOpen");
	processAList = (PROCESSALIST)GetProcAddress(handle, "ProcessAList");
	aiUpdate = (AIUPDATE)GetProcAddress(handle, "AiUpdate");

	if (aiDacrateChanged == nullptr)
		aiDacrateChanged =
			dummy_aiDacrateChanged;
	if (aiLenChanged == nullptr) aiLenChanged = dummy_void;
	if (aiReadLength == nullptr) aiReadLength = dummy_aiReadLength;
	if (aiUpdate == nullptr) aiUpdate = dummy_aiUpdate;
	if (closeDLL_audio == nullptr) closeDLL_audio = dummy_void;
	if (initiateAudio == nullptr) initiateAudio = dummy_initiateAudio;
	if (processAList == nullptr) processAList = dummy_void;
	if (romClosed_audio == nullptr) romClosed_audio = dummy_void;
	if (romOpen_audio == nullptr) romOpen_audio = dummy_void;

	audio_info.hwnd = mainHWND;
	audio_info.hinst = app_instance;
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
}

void load_rsp(HMODULE handle)
{
	int i = 4;

	closeDLL_RSP = (CLOSEDLL_RSP)GetProcAddress(handle, "CloseDLL");
	doRspCycles = (DORSPCYCLES)GetProcAddress(handle, "DoRspCycles");
	initiateRSP = (INITIATERSP)GetProcAddress(handle, "InitiateRSP");
	romClosed_RSP = (ROMCLOSED_RSP)GetProcAddress(handle, "RomClosed");

	if (closeDLL_RSP == nullptr) closeDLL_RSP = dummy_void;
	if (doRspCycles == nullptr) doRspCycles = dummy_doRspCycles;
	if (initiateRSP == nullptr) initiateRSP = dummy_initiateRSP;
	if (romClosed_RSP == nullptr) romClosed_RSP = dummy_void;

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
}

std::optional<std::unique_ptr<Plugin>> Plugin::create(
	std::filesystem::path path)
{
	HMODULE module = LoadLibrary(path.string().c_str());

	if (module == nullptr)
	{
		return std::nullopt;
	}

	const auto get_dll_info = (GETDLLINFO)GetProcAddress(
		module, "GetDllInfo");

	if (!get_dll_info)
	{
		FreeLibrary(module);
		return std::nullopt;
	}

	PLUGIN_INFO plugin_info;
	get_dll_info(&plugin_info);

	const size_t plugin_name_len = strlen(plugin_info.Name);
	while (plugin_info.Name[plugin_name_len - 1] == ' ')
	{
		plugin_info.Name[plugin_name_len - 1] = '\0';
	}

	auto plugin = std::make_unique<Plugin>();

	plugin->m_path = path;
	plugin->m_name = std::string(plugin_info.Name);
	plugin->m_type = static_cast<plugin_type>(plugin_info.Type);
	plugin->m_version = plugin_info.Version;
	plugin->m_module = module;

	printf("[Plugin] Created plugin %s\n", plugin->m_name.c_str());
	return plugin;
}

Plugin::~Plugin()
{
	FreeLibrary((HMODULE)m_module);
	printf("[Plugin] Destroyed plugin %s\n", m_name.c_str());
}

void Plugin::config()
{
	switch (m_type)
	{
	case plugin_type::video:
		{
			if (!emu_launched)
			{
				auto initiateGFX = (INITIATEGFX)GetProcAddress((HMODULE)m_module, "InitiateGFX");
				if (initiateGFX && !initiateGFX(dummy_gfx_info))
				{
					show_modal_info("Couldn't initialize video plugin.", nullptr);
				}
			}

			auto dllConfig = (DLLCONFIG)GetProcAddress((HMODULE)m_module, "DllConfig");
			if (dllConfig) dllConfig(hwnd_plug);

			if (!emu_launched)
			{
				auto closeDLL_gfx = (CLOSEDLL_GFX)GetProcAddress((HMODULE)m_module, "CloseDLL");
				if (closeDLL_gfx) closeDLL_gfx();
			}
			break;
		}
	case plugin_type::audio:
		{
			if (!emu_launched)
			{
				auto initiateAudio = (INITIATEAUDIO)GetProcAddress((HMODULE)m_module, "InitiateAudio");
				if (initiateAudio && !initiateAudio(dummy_audio_info))
				{
					show_modal_info("Couldn't initialize audio plugin.", nullptr);
				}
			}

			auto dllConfig = (DLLCONFIG)GetProcAddress((HMODULE)m_module, "DllConfig");
			if (dllConfig) dllConfig(hwnd_plug);

			if (!emu_launched)
			{
				auto closeDLL_audio = (CLOSEDLL_AUDIO)GetProcAddress((HMODULE)m_module, "CloseDLL");
				if (closeDLL_audio) closeDLL_audio();
			}
			break;
		}
	case plugin_type::input:
		{
			if (!emu_launched)
			{
				if (m_version == 0x0101)
				{
					auto initiateControllers = (INITIATECONTROLLERS)GetProcAddress((HMODULE)m_module, "InitiateControllers");
					if(initiateControllers) initiateControllers(dummy_control_info);
				} else
				{
					auto old_initiateControllers = (OLD_INITIATECONTROLLERS)GetProcAddress((HMODULE)m_module, "InitiateControllers");
					if(old_initiateControllers) old_initiateControllers(mainHWND, Controls);
				}
			}

			auto dllConfig = (DLLCONFIG)GetProcAddress((HMODULE)m_module, "DllConfig");
			if (dllConfig) dllConfig(hwnd_plug);

			if (!emu_launched)
			{
				auto closeDLL_input = (CLOSEDLL_INPUT)GetProcAddress((HMODULE)m_module, "CloseDLL");
				if (closeDLL_input) closeDLL_input();
			}
			break;
		}
	case plugin_type::rsp:
		{
			if (!emu_launched)
			{
				auto initiateRSP = (INITIATERSP)GetProcAddress((HMODULE)m_module, "InitiateRSP");
				unsigned long i = 0;
				if(initiateRSP) initiateRSP(dummy_rsp_info, &i);
			}

			auto dllConfig = (DLLCONFIG)GetProcAddress((HMODULE)m_module, "DllConfig");
			if (dllConfig) dllConfig(hwnd_plug);

			if (!emu_launched)
			{
				auto closeDLL_RSP = (CLOSEDLL_RSP)GetProcAddress((HMODULE)m_module, "CloseDLL");
				if (closeDLL_RSP) closeDLL_RSP();
			}
			break;
		}
	default:
		assert(false);
		break;
	}
}

void Plugin::test()
{
	dllTest = (DLLTEST)GetProcAddress((HMODULE)m_module, "DllTest");
	if (dllTest) dllTest(hwnd_plug);
}

void Plugin::about()
{
	dllAbout = (DLLABOUT)GetProcAddress((HMODULE)m_module, "DllAbout");
	if (dllAbout) dllAbout(hwnd_plug);
}

void Plugin::load_into_globals()
{
	switch (m_type)
	{
	case video:
		load_gfx((HMODULE)m_module);
		break;
	case audio:
		load_audio((HMODULE)m_module);
		break;
	case input:
		load_input(m_version, (HMODULE)m_module);
		break;
	case rsp:
		load_rsp((HMODULE)m_module);
		break;
	}
}

std::vector<std::unique_ptr<Plugin>> get_available_plugins()
{
	std::vector<std::unique_ptr<Plugin>> plugins;
	std::vector<std::string> files = get_files_with_extension_in_directory(
		get_plugins_directory(), "dll");

	for (const auto& file : files)
	{
		auto plugin = Plugin::create(file);
		if (plugin.has_value())
		{
			plugins.push_back(std::move(plugin.value()));
		}
	}
	return plugins;
}

void setup_dummy_info()
{
	int i;

	/////// GFX ///////////////////////////
	dummy_gfx_info.hWnd = Statusbar::hwnd();
	dummy_gfx_info.hStatusBar = Statusbar::hwnd();
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
	dummy_audio_info.hinst = app_instance;
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
	dummy_control_info.hinst = app_instance;
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
