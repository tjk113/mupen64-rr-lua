/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// ReSharper disable CppCStyleCast

#include "stdafx.h"
#include <gui/features/Statusbar.h>
#include <Config.h>
#include <FrontendService.h>
#include <PlatformService.h>
#include <Plugin.h>

#include <gui/Loggers.h>
#include <gui/Main.h>

core_gfx_info dummy_gfx_info;
core_audio_info dummy_audio_info;
core_input_info dummy_control_info;
core_rsp_info dummy_rsp_info;
unsigned char dummy_header[0x40];

core_gfx_info gfx_info;
core_audio_info audio_info;
core_input_info control_info;
core_rsp_info rsp_info;

DLLABOUT dll_about;
DLLCONFIG dll_config;
DLLTEST dll_test;

static uint32_t __cdecl dummy_doRspCycles(uint32_t Cycles)
{
    return Cycles;
}

static void __cdecl dummy_void()
{
}

static int32_t __cdecl dummy_initiateGFX(core_gfx_info Gfx_Info)
{
    return 1;
}

static int32_t __cdecl dummy_initiateAudio(core_audio_info Audio_Info)
{
    return 1;
}

static void __cdecl dummy_initiateControllers(core_input_info Control_Info)
{
}

static void __cdecl dummy_aiDacrateChanged(int32_t SystemType)
{
}

static uint32_t __cdecl dummy_aiReadLength()
{
    return 0;
}

static void __cdecl dummy_aiUpdate(int32_t)
{
}

static void __cdecl dummy_controllerCommand(int32_t Control, uint8_t* Command)
{
}

static void __cdecl dummy_getKeys(int32_t Control, core_buttons* Keys)
{
}

static void __cdecl dummy_setKeys(int32_t Control, core_buttons Keys)
{
}

static void __cdecl dummy_readController(int32_t Control, uint8_t* Command)
{
}

static void __cdecl dummy_keyDown(uint32_t wParam, int32_t lParam)
{
}

static void __cdecl dummy_keyUp(uint32_t wParam, int32_t lParam)
{
}

static uint32_t dummy;

static void __cdecl dummy_initiateRSP(core_rsp_info Rsp_Info,
                                      uint32_t* CycleCount){};

static void __cdecl dummy_fBRead(uint32_t addr){};

static void __cdecl dummy_fBWrite(uint32_t addr, uint32_t size){};

static void __cdecl dummy_fBGetFrameBufferInfo(void* p){};

static void __cdecl dummy_readScreen(void**, int32_t*, int32_t*)
{
}

static void __cdecl dummy_moveScreen(int32_t, int32_t)
{
}

#define FUNC(target, type, fallback, name)                \
    target = (type)GetProcAddress((HMODULE)handle, name); \
    if (!target)                                          \
    target = fallback

void load_gfx(void* handle)
{
    FUNC(g_core.plugin_funcs.change_window, CHANGEWINDOW, dummy_void, "ChangeWindow");
    FUNC(g_core.plugin_funcs.close_dll_gfx, CLOSEDLL_GFX, dummy_void, "CloseDLL");
    FUNC(g_core.plugin_funcs.initiate_gfx, INITIATEGFX, dummy_initiateGFX, "InitiateGFX");
    FUNC(g_core.plugin_funcs.process_d_list, PROCESSDLIST, dummy_void, "ProcessDList");
    FUNC(g_core.plugin_funcs.process_rdp_list, PROCESSRDPLIST, dummy_void, "ProcessRDPList");
    FUNC(g_core.plugin_funcs.rom_closed_gfx, ROMCLOSED_GFX, dummy_void, "RomClosed");
    FUNC(g_core.plugin_funcs.rom_open_gfx, ROMOPEN_GFX, dummy_void, "RomOpen");
    FUNC(g_core.plugin_funcs.show_cfb, SHOWCFB, dummy_void, "ShowCFB");
    FUNC(g_core.plugin_funcs.update_screen, UPDATESCREEN, dummy_void, "UpdateScreen");
    FUNC(g_core.plugin_funcs.vi_status_changed, VISTATUSCHANGED, dummy_void, "ViStatusChanged");
    FUNC(g_core.plugin_funcs.vi_width_changed, VIWIDTHCHANGED, dummy_void, "ViWidthChanged");
    FUNC(g_core.plugin_funcs.move_screen, MOVESCREEN, dummy_moveScreen, "MoveScreen");
    FUNC(g_core.plugin_funcs.capture_screen, CAPTURESCREEN, nullptr, "CaptureScreen");
    FUNC(g_core.plugin_funcs.read_screen, READSCREEN, (READSCREEN)PlatformService::get_function_in_module(handle, "ReadScreen2"), "ReadScreen");
    FUNC(g_core.plugin_funcs.get_video_size, GETVIDEOSIZE, nullptr, "mge_get_video_size");
    FUNC(g_core.plugin_funcs.read_video, READVIDEO, nullptr, "mge_read_video");
    FUNC(g_core.plugin_funcs.fb_read, FBREAD, dummy_fBRead, "FBRead");
    FUNC(g_core.plugin_funcs.fb_write, FBWRITE, dummy_fBWrite, "FBWrite");
    FUNC(g_core.plugin_funcs.fb_get_frame_buffer_info, FBGETFRAMEBUFFERINFO, dummy_fBGetFrameBufferInfo, "FBGetFrameBufferInfo");

    // ReadScreen returns a plugin-allocated buffer which must be freed by the same CRT
    g_core.plugin_funcs.dll_crt_free = PlatformService::get_free_function_in_module(handle);

    gfx_info.hWnd = g_main_hwnd;
    gfx_info.hStatusBar = g_config.is_statusbar_enabled ? Statusbar::hwnd() : nullptr;
    gfx_info.MemoryBswaped = 1;
    gfx_info.HEADER = g_core.rom;
    gfx_info.RDRAM = (uint8_t*)g_core.rdram;
    gfx_info.DMEM = (uint8_t*)g_core.SP_DMEM;
    gfx_info.IMEM = (uint8_t*)g_core.SP_IMEM;
    gfx_info.MI_INTR_REG = &(g_core.MI_register->mi_intr_reg);
    gfx_info.DPC_START_REG = &(g_core.dpc_register->dpc_start);
    gfx_info.DPC_END_REG = &(g_core.dpc_register->dpc_end);
    gfx_info.DPC_CURRENT_REG = &(g_core.dpc_register->dpc_current);
    gfx_info.DPC_STATUS_REG = &(g_core.dpc_register->dpc_status);
    gfx_info.DPC_CLOCK_REG = &(g_core.dpc_register->dpc_clock);
    gfx_info.DPC_BUFBUSY_REG = &(g_core.dpc_register->dpc_bufbusy);
    gfx_info.DPC_PIPEBUSY_REG = &(g_core.dpc_register->dpc_pipebusy);
    gfx_info.DPC_TMEM_REG = &(g_core.dpc_register->dpc_tmem);
    gfx_info.VI_STATUS_REG = &(g_core.vi_register->vi_status);
    gfx_info.VI_ORIGIN_REG = &(g_core.vi_register->vi_origin);
    gfx_info.VI_WIDTH_REG = &(g_core.vi_register->vi_width);
    gfx_info.VI_INTR_REG = &(g_core.vi_register->vi_v_intr);
    gfx_info.VI_V_CURRENT_LINE_REG = &(g_core.vi_register->vi_current);
    gfx_info.VI_TIMING_REG = &(g_core.vi_register->vi_burst);
    gfx_info.VI_V_SYNC_REG = &(g_core.vi_register->vi_v_sync);
    gfx_info.VI_H_SYNC_REG = &(g_core.vi_register->vi_h_sync);
    gfx_info.VI_LEAP_REG = &(g_core.vi_register->vi_leap);
    gfx_info.VI_H_START_REG = &(g_core.vi_register->vi_h_start);
    gfx_info.VI_V_START_REG = &(g_core.vi_register->vi_v_start);
    gfx_info.VI_V_BURST_REG = &(g_core.vi_register->vi_v_burst);
    gfx_info.VI_X_SCALE_REG = &(g_core.vi_register->vi_x_scale);
    gfx_info.VI_Y_SCALE_REG = &(g_core.vi_register->vi_y_scale);
    gfx_info.CheckInterrupts = dummy_void;
    g_core.plugin_funcs.initiate_gfx(gfx_info);
}

void load_input(uint16_t version, void* handle)
{
    FUNC(g_core.plugin_funcs.close_dll_input, CLOSEDLL_INPUT, dummy_void, "CloseDLL");
    FUNC(g_core.plugin_funcs.controller_command, CONTROLLERCOMMAND, dummy_controllerCommand, "ControllerCommand");
    FUNC(g_core.plugin_funcs.get_keys, GETKEYS, dummy_getKeys, "GetKeys");
    FUNC(g_core.plugin_funcs.set_keys, SETKEYS, dummy_setKeys, "SetKeys");
    if (version == 0x0101)
    {
        FUNC(g_core.plugin_funcs.initiate_controllers, INITIATECONTROLLERS, dummy_initiateControllers, "InitiateControllers");
    }
    else
    {
        FUNC(g_core.plugin_funcs.old_initiate_controllers, OLD_INITIATECONTROLLERS, nullptr, "InitiateControllers");
    }
    FUNC(g_core.plugin_funcs.read_controller, READCONTROLLER, dummy_readController, "ReadController");
    FUNC(g_core.plugin_funcs.rom_closed_input, ROMCLOSED_INPUT, dummy_void, "RomClosed");
    FUNC(g_core.plugin_funcs.rom_open_input, ROMOPEN_INPUT, dummy_void, "RomOpen");
    FUNC(g_core.plugin_funcs.key_down, KEYDOWN, dummy_keyDown, "WM_KeyDown");
    FUNC(g_core.plugin_funcs.key_up, KEYUP, dummy_keyUp, "WM_KeyUp");

    control_info.hMainWindow = g_main_hwnd;
    control_info.hinst = g_app_instance;
    control_info.MemoryBswaped = 1;
    control_info.HEADER = g_core.rom;
    control_info.Controls = g_core.controls;
    for (auto& controller : g_core.controls)
    {
        controller.Present = 0;
        controller.RawData = 0;
        controller.Plugin = (int32_t)ce_none;
    }
    if (version == 0x0101)
    {
        g_core.plugin_funcs.initiate_controllers(control_info);
    }
    else
    {
        g_core.plugin_funcs.old_initiate_controllers(g_main_hwnd, g_core.controls);
    }
}


void load_audio(void* handle)
{
    FUNC(g_core.plugin_funcs.close_dll_audio, CLOSEDLL_AUDIO, dummy_void, "CloseDLL");
    FUNC(g_core.plugin_funcs.ai_dacrate_changed, AIDACRATECHANGED, dummy_aiDacrateChanged, "AiDacrateChanged");
    FUNC(g_core.plugin_funcs.ai_len_changed, AILENCHANGED, dummy_void, "AiLenChanged");
    FUNC(g_core.plugin_funcs.ai_read_length, AIREADLENGTH, dummy_aiReadLength, "AiReadLength");
    FUNC(g_core.plugin_funcs.initiate_audio, INITIATEAUDIO, dummy_initiateAudio, "InitiateAudio");
    FUNC(g_core.plugin_funcs.rom_closed_audio, ROMCLOSED_AUDIO, dummy_void, "RomClosed");
    FUNC(g_core.plugin_funcs.rom_open_audio, ROMOPEN_AUDIO, dummy_void, "RomOpen");
    FUNC(g_core.plugin_funcs.process_a_list, PROCESSALIST, dummy_void, "ProcessAList");
    FUNC(g_core.plugin_funcs.ai_update, AIUPDATE, dummy_aiUpdate, "AiUpdate");

    audio_info.hwnd = g_main_hwnd;
    audio_info.hinst = g_app_instance;
    audio_info.MemoryBswaped = 1;
    audio_info.HEADER = g_core.rom;
    audio_info.RDRAM = (uint8_t*)g_core.rdram;
    audio_info.DMEM = (uint8_t*)g_core.SP_DMEM;
    audio_info.IMEM = (uint8_t*)g_core.SP_IMEM;
    audio_info.MI_INTR_REG = &dummy; //&(g_core.MI_register->mi_intr_reg);
    audio_info.AI_DRAM_ADDR_REG = &(g_core.ai_register->ai_dram_addr);
    audio_info.AI_LEN_REG = &(g_core.ai_register->ai_len);
    audio_info.AI_CONTROL_REG = &(g_core.ai_register->ai_control);
    audio_info.AI_STATUS_REG = &dummy; //&(g_core.ai_register->ai_status);
    audio_info.AI_DACRATE_REG = &(g_core.ai_register->ai_dacrate);
    audio_info.AI_BITRATE_REG = &(g_core.ai_register->ai_bitrate);

    audio_info.CheckInterrupts = dummy_void;
    g_core.plugin_funcs.initiate_audio(audio_info);
}

void load_rsp(void* handle)
{
    FUNC(g_core.plugin_funcs.close_dll_rsp, CLOSEDLL_RSP, dummy_void, "CloseDLL");
    FUNC(g_core.plugin_funcs.do_rsp_cycles, DORSPCYCLES, dummy_doRspCycles, "DoRspCycles");
    FUNC(g_core.plugin_funcs.initiate_rsp, INITIATERSP, dummy_initiateRSP, "InitiateRSP");
    FUNC(g_core.plugin_funcs.rom_closed_rsp, ROMCLOSED_RSP, dummy_void, "RomClosed");

    rsp_info.MemoryBswaped = 1;
    rsp_info.RDRAM = (uint8_t*)g_core.rdram;
    rsp_info.DMEM = (uint8_t*)g_core.SP_DMEM;
    rsp_info.IMEM = (uint8_t*)g_core.SP_IMEM;
    rsp_info.MI_INTR_REG = &g_core.MI_register->mi_intr_reg;
    rsp_info.SP_MEM_ADDR_REG = &g_core.sp_register->sp_mem_addr_reg;
    rsp_info.SP_DRAM_ADDR_REG = &g_core.sp_register->sp_dram_addr_reg;
    rsp_info.SP_RD_LEN_REG = &g_core.sp_register->sp_rd_len_reg;
    rsp_info.SP_WR_LEN_REG = &g_core.sp_register->sp_wr_len_reg;
    rsp_info.SP_STATUS_REG = &g_core.sp_register->sp_status_reg;
    rsp_info.SP_DMA_FULL_REG = &g_core.sp_register->sp_dma_full_reg;
    rsp_info.SP_DMA_BUSY_REG = &g_core.sp_register->sp_dma_busy_reg;
    rsp_info.SP_PC_REG = &g_core.rsp_register->rsp_pc;
    rsp_info.SP_SEMAPHORE_REG = &g_core.sp_register->sp_semaphore_reg;
    rsp_info.DPC_START_REG = &g_core.dpc_register->dpc_start;
    rsp_info.DPC_END_REG = &g_core.dpc_register->dpc_end;
    rsp_info.DPC_CURRENT_REG = &g_core.dpc_register->dpc_current;
    rsp_info.DPC_STATUS_REG = &g_core.dpc_register->dpc_status;
    rsp_info.DPC_CLOCK_REG = &g_core.dpc_register->dpc_clock;
    rsp_info.DPC_BUFBUSY_REG = &g_core.dpc_register->dpc_bufbusy;
    rsp_info.DPC_PIPEBUSY_REG = &g_core.dpc_register->dpc_pipebusy;
    rsp_info.DPC_TMEM_REG = &g_core.dpc_register->dpc_tmem;
    rsp_info.CheckInterrupts = dummy_void;
    rsp_info.ProcessDlistList = g_core.plugin_funcs.process_d_list;
    rsp_info.ProcessAlistList = g_core.plugin_funcs.process_a_list;
    rsp_info.ProcessRdpList = g_core.plugin_funcs.process_rdp_list;
    rsp_info.ShowCFB = g_core.plugin_funcs.show_cfb;

    int32_t i = 4;
    g_core.plugin_funcs.initiate_rsp(rsp_info, (uint32_t*)&i);
}

std::pair<std::wstring, std::unique_ptr<Plugin>> Plugin::create(std::filesystem::path path)
{
    uint64_t error = 0;
    void* module = PlatformService::load_library(path.wstring().c_str(), &error);

    if (module == nullptr)
    {
        return std::make_pair(std::format(L"LoadLibrary (code {})", error), nullptr);
    }

    const auto get_dll_info = (GETDLLINFO)PlatformService::get_function_in_module(
    module, "GetDllInfo");

    if (!get_dll_info)
    {
        PlatformService::free_library(module);
        return std::make_pair(L"GetDllInfo missing", nullptr);
    }

    core_plugin_info plugin_info;
    get_dll_info(&plugin_info);

    const size_t plugin_name_len = strlen(plugin_info.Name);
    while (plugin_info.Name[plugin_name_len - 1] == ' ')
    {
        plugin_info.Name[plugin_name_len - 1] = '\0';
    }

    auto plugin = std::make_unique<Plugin>();

    plugin->m_path = path;
    plugin->m_name = std::string(plugin_info.Name);
    plugin->m_type = static_cast<core_plugin_type>(plugin_info.Type);
    plugin->m_version = plugin_info.Version;
    plugin->m_module = module;

    g_view_logger->info("[Plugin] Created plugin {}", plugin->m_name);
    return std::make_pair(L"", std::move(plugin));
}

Plugin::~Plugin()
{
    PlatformService::free_library((void*)m_module);
}

void Plugin::config()
{
    switch (m_type)
    {
    case plugin_video:
        {
            if (!core_vr_get_launched())
            {
                // NOTE: Since olden days, dummy render target hwnd was the statusbar.
                dummy_gfx_info.hWnd = Statusbar::hwnd();
                dummy_gfx_info.hStatusBar = Statusbar::hwnd();

                auto initiateGFX = (INITIATEGFX)PlatformService::get_function_in_module((void*)m_module, "InitiateGFX");
                if (initiateGFX && !initiateGFX(dummy_gfx_info))
                {
                    FrontendService::show_dialog(L"Couldn't initialize video plugin.", L"Core", fsvc_information);
                }
            }

            auto dllConfig = (DLLCONFIG)PlatformService::get_function_in_module((void*)m_module, "DllConfig");
            if (dllConfig)
                dllConfig(g_hwnd_plug);

            if (!core_vr_get_launched())
            {
                auto closeDLL_gfx = (CLOSEDLL_GFX)PlatformService::get_function_in_module((void*)m_module, "CloseDLL");
                if (closeDLL_gfx)
                    closeDLL_gfx();
            }
            break;
        }
    case plugin_audio:
        {
            if (!core_vr_get_launched())
            {
                auto initiateAudio = (INITIATEAUDIO)PlatformService::get_function_in_module((void*)m_module, "InitiateAudio");
                if (initiateAudio && !initiateAudio(dummy_audio_info))
                {
                    FrontendService::show_dialog(L"Couldn't initialize audio plugin.", L"Core", fsvc_information);
                }
            }

            auto dllConfig = (DLLCONFIG)PlatformService::get_function_in_module((void*)m_module, "DllConfig");
            if (dllConfig)
                dllConfig(g_hwnd_plug);

            if (!core_vr_get_launched())
            {
                auto closeDLL_audio = (CLOSEDLL_AUDIO)PlatformService::get_function_in_module((void*)m_module, "CloseDLL");
                if (closeDLL_audio)
                    closeDLL_audio();
            }
            break;
        }
    case plugin_input:
        {
            if (!core_vr_get_launched())
            {
                if (m_version == 0x0101)
                {
                    auto initiateControllers = (INITIATECONTROLLERS)PlatformService::get_function_in_module((void*)m_module, "InitiateControllers");
                    if (initiateControllers)
                        initiateControllers(dummy_control_info);
                }
                else
                {
                    auto old_initiateControllers = (OLD_INITIATECONTROLLERS)PlatformService::get_function_in_module((void*)m_module, "InitiateControllers");
                    if (old_initiateControllers)
                        old_initiateControllers(g_main_hwnd, g_core.controls);
                }
            }

            auto dllConfig = (DLLCONFIG)PlatformService::get_function_in_module((void*)m_module, "DllConfig");
            if (dllConfig)
                dllConfig(g_hwnd_plug);

            if (!core_vr_get_launched())
            {
                auto closeDLL_input = (CLOSEDLL_INPUT)PlatformService::get_function_in_module((void*)m_module, "CloseDLL");
                if (closeDLL_input)
                    closeDLL_input();
            }
            break;
        }
    case plugin_rsp:
        {
            if (!core_vr_get_launched())
            {
                auto initiateRSP = (INITIATERSP)PlatformService::get_function_in_module((void*)m_module, "InitiateRSP");
                uint32_t i = 0;
                if (initiateRSP)
                    initiateRSP(dummy_rsp_info, &i);
            }

            auto dllConfig = (DLLCONFIG)PlatformService::get_function_in_module((void*)m_module, "DllConfig");
            if (dllConfig)
                dllConfig(g_hwnd_plug);

            if (!core_vr_get_launched())
            {
                auto closeDLL_RSP = (CLOSEDLL_RSP)PlatformService::get_function_in_module((void*)m_module, "CloseDLL");
                if (closeDLL_RSP)
                    closeDLL_RSP();
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
    dll_test = (DLLTEST)PlatformService::get_function_in_module((void*)m_module, "DllTest");
    if (dll_test)
        dll_test(g_hwnd_plug);
}

void Plugin::about()
{
    dll_about = (DLLABOUT)PlatformService::get_function_in_module((void*)m_module, "DllAbout");
    if (dll_about)
        dll_about(g_hwnd_plug);
}

void Plugin::initiate()
{
    switch (m_type)
    {
    case plugin_video:
        load_gfx(m_module);
        break;
    case plugin_audio:
        load_audio(m_module);
        break;
    case plugin_input:
        load_input(m_version, m_module);
        break;
    case plugin_rsp:
        load_rsp(m_module);
        break;
    }
}

void setup_dummy_info()
{
    int32_t i;

    /////// GFX ///////////////////////////

    dummy_gfx_info.MemoryBswaped = 1;
    dummy_gfx_info.HEADER = (uint8_t*)dummy_header;
    dummy_gfx_info.RDRAM = (uint8_t*)g_core.rdram;
    dummy_gfx_info.DMEM = (uint8_t*)g_core.SP_DMEM;
    dummy_gfx_info.IMEM = (uint8_t*)g_core.SP_IMEM;
    dummy_gfx_info.MI_INTR_REG = &(g_core.MI_register->mi_intr_reg);
    dummy_gfx_info.DPC_START_REG = &(g_core.dpc_register->dpc_start);
    dummy_gfx_info.DPC_END_REG = &(g_core.dpc_register->dpc_end);
    dummy_gfx_info.DPC_CURRENT_REG = &(g_core.dpc_register->dpc_current);
    dummy_gfx_info.DPC_STATUS_REG = &(g_core.dpc_register->dpc_status);
    dummy_gfx_info.DPC_CLOCK_REG = &(g_core.dpc_register->dpc_clock);
    dummy_gfx_info.DPC_BUFBUSY_REG = &(g_core.dpc_register->dpc_bufbusy);
    dummy_gfx_info.DPC_PIPEBUSY_REG = &(g_core.dpc_register->dpc_pipebusy);
    dummy_gfx_info.DPC_TMEM_REG = &(g_core.dpc_register->dpc_tmem);
    dummy_gfx_info.VI_STATUS_REG = &(g_core.vi_register->vi_status);
    dummy_gfx_info.VI_ORIGIN_REG = &(g_core.vi_register->vi_origin);
    dummy_gfx_info.VI_WIDTH_REG = &(g_core.vi_register->vi_width);
    dummy_gfx_info.VI_INTR_REG = &(g_core.vi_register->vi_v_intr);
    dummy_gfx_info.VI_V_CURRENT_LINE_REG = &(g_core.vi_register->vi_current);
    dummy_gfx_info.VI_TIMING_REG = &(g_core.vi_register->vi_burst);
    dummy_gfx_info.VI_V_SYNC_REG = &(g_core.vi_register->vi_v_sync);
    dummy_gfx_info.VI_H_SYNC_REG = &(g_core.vi_register->vi_h_sync);
    dummy_gfx_info.VI_LEAP_REG = &(g_core.vi_register->vi_leap);
    dummy_gfx_info.VI_H_START_REG = &(g_core.vi_register->vi_h_start);
    dummy_gfx_info.VI_V_START_REG = &(g_core.vi_register->vi_v_start);
    dummy_gfx_info.VI_V_BURST_REG = &(g_core.vi_register->vi_v_burst);
    dummy_gfx_info.VI_X_SCALE_REG = &(g_core.vi_register->vi_x_scale);
    dummy_gfx_info.VI_Y_SCALE_REG = &(g_core.vi_register->vi_y_scale);
    dummy_gfx_info.CheckInterrupts = dummy_void;

    /////// AUDIO /////////////////////////
    dummy_audio_info.hwnd = g_main_hwnd;
    dummy_audio_info.hinst = g_app_instance;
    dummy_audio_info.MemoryBswaped = 1;
    dummy_audio_info.HEADER = (uint8_t*)dummy_header;
    dummy_audio_info.RDRAM = (uint8_t*)g_core.rdram;
    dummy_audio_info.DMEM = (uint8_t*)g_core.SP_DMEM;
    dummy_audio_info.IMEM = (uint8_t*)g_core.SP_IMEM;
    dummy_audio_info.MI_INTR_REG = &(g_core.MI_register->mi_intr_reg);
    dummy_audio_info.AI_DRAM_ADDR_REG = &(g_core.ai_register->ai_dram_addr);
    dummy_audio_info.AI_LEN_REG = &(g_core.ai_register->ai_len);
    dummy_audio_info.AI_CONTROL_REG = &(g_core.ai_register->ai_control);
    dummy_audio_info.AI_STATUS_REG = &(g_core.ai_register->ai_status);
    dummy_audio_info.AI_DACRATE_REG = &(g_core.ai_register->ai_dacrate);
    dummy_audio_info.AI_BITRATE_REG = &(g_core.ai_register->ai_bitrate);
    dummy_audio_info.CheckInterrupts = dummy_void;

    ///// CONTROLS ///////////////////////////
    dummy_control_info.hMainWindow = g_main_hwnd;
    dummy_control_info.hinst = g_app_instance;
    dummy_control_info.MemoryBswaped = 1;
    dummy_control_info.HEADER = (uint8_t*)dummy_header;
    dummy_control_info.Controls = g_core.controls;
    for (i = 0; i < 4; i++)
    {
        g_core.controls[i].Present = 0;
        g_core.controls[i].RawData = 0;
        g_core.controls[i].Plugin = (int32_t)ce_none;
    }

    //////// RSP /////////////////////////////
    dummy_rsp_info.MemoryBswaped = 1;
    dummy_rsp_info.RDRAM = (uint8_t*)g_core.rdram;
    dummy_rsp_info.DMEM = (uint8_t*)g_core.SP_DMEM;
    dummy_rsp_info.IMEM = (uint8_t*)g_core.SP_IMEM;
    dummy_rsp_info.MI_INTR_REG = &g_core.MI_register->mi_intr_reg;
    dummy_rsp_info.SP_MEM_ADDR_REG = &g_core.sp_register->sp_mem_addr_reg;
    dummy_rsp_info.SP_DRAM_ADDR_REG = &g_core.sp_register->sp_dram_addr_reg;
    dummy_rsp_info.SP_RD_LEN_REG = &g_core.sp_register->sp_rd_len_reg;
    dummy_rsp_info.SP_WR_LEN_REG = &g_core.sp_register->sp_wr_len_reg;
    dummy_rsp_info.SP_STATUS_REG = &g_core.sp_register->sp_status_reg;
    dummy_rsp_info.SP_DMA_FULL_REG = &g_core.sp_register->sp_dma_full_reg;
    dummy_rsp_info.SP_DMA_BUSY_REG = &g_core.sp_register->sp_dma_busy_reg;
    dummy_rsp_info.SP_PC_REG = &g_core.rsp_register->rsp_pc;
    dummy_rsp_info.SP_SEMAPHORE_REG = &g_core.sp_register->sp_semaphore_reg;
    dummy_rsp_info.DPC_START_REG = &g_core.dpc_register->dpc_start;
    dummy_rsp_info.DPC_END_REG = &g_core.dpc_register->dpc_end;
    dummy_rsp_info.DPC_CURRENT_REG = &g_core.dpc_register->dpc_current;
    dummy_rsp_info.DPC_STATUS_REG = &g_core.dpc_register->dpc_status;
    dummy_rsp_info.DPC_CLOCK_REG = &g_core.dpc_register->dpc_clock;
    dummy_rsp_info.DPC_BUFBUSY_REG = &g_core.dpc_register->dpc_bufbusy;
    dummy_rsp_info.DPC_PIPEBUSY_REG = &g_core.dpc_register->dpc_pipebusy;
    dummy_rsp_info.DPC_TMEM_REG = &g_core.dpc_register->dpc_tmem;
    dummy_rsp_info.CheckInterrupts = dummy_void;
    dummy_rsp_info.ProcessDlistList = g_core.plugin_funcs.process_d_list;
    dummy_rsp_info.ProcessAlistList = g_core.plugin_funcs.process_a_list;
    dummy_rsp_info.ProcessRdpList = g_core.plugin_funcs.process_rdp_list;
    dummy_rsp_info.ShowCFB = g_core.plugin_funcs.show_cfb;
}

std::wstring hotkey_to_string(const cfg_hotkey* hotkey)
{
    char buf[260]{};
    const int k = hotkey->key;

    if (!hotkey->ctrl && !hotkey->shift && !hotkey->alt && !hotkey->key)
    {
        return L"(nothing)";
    }

    if (hotkey->ctrl)
        strcat(buf, "Ctrl ");
    if (hotkey->shift)
        strcat(buf, "Shift ");
    if (hotkey->alt)
        strcat(buf, "Alt ");
    if (k)
    {
        char buf2[64] = {0};
        if ((k >= 0x30 && k <= 0x39) || (k >= 0x41 && k <= 0x5A))
            sprintf(buf2, "%c", static_cast<char>(k));
        else if ((k >= VK_F1 && k <= VK_F24))
            sprintf(buf2, "F%d", k - (VK_F1 - 1));
        else if ((k >= VK_NUMPAD0 && k <= VK_NUMPAD9))
            sprintf(buf2, "Num%d", k - VK_NUMPAD0);
        else
            switch (k)
            {
            case VK_LBUTTON:
                strcpy(buf2, "LMB");
                break;
            case VK_RBUTTON:
                strcpy(buf2, "RMB");
                break;
            case VK_MBUTTON:
                strcpy(buf2, "MMB");
                break;
            case VK_XBUTTON1:
                strcpy(buf2, "XMB1");
                break;
            case VK_XBUTTON2:
                strcpy(buf2, "XMB2");
                break;
            case VK_SPACE:
                strcpy(buf2, "Space");
                break;
            case VK_BACK:
                strcpy(buf2, "Backspace");
                break;
            case VK_TAB:
                strcpy(buf2, "Tab");
                break;
            case VK_CLEAR:
                strcpy(buf2, "Clear");
                break;
            case VK_RETURN:
                strcpy(buf2, "Enter");
                break;
            case VK_PAUSE:
                strcpy(buf2, "Pause");
                break;
            case VK_CAPITAL:
                strcpy(buf2, "Caps");
                break;
            case VK_PRIOR:
                strcpy(buf2, "PageUp");
                break;
            case VK_NEXT:
                strcpy(buf2, "PageDn");
                break;
            case VK_END:
                strcpy(buf2, "End");
                break;
            case VK_HOME:
                strcpy(buf2, "Home");
                break;
            case VK_LEFT:
                strcpy(buf2, "Left");
                break;
            case VK_UP:
                strcpy(buf2, "Up");
                break;
            case VK_RIGHT:
                strcpy(buf2, "Right");
                break;
            case VK_DOWN:
                strcpy(buf2, "Down");
                break;
            case VK_SELECT:
                strcpy(buf2, "Select");
                break;
            case VK_PRINT:
                strcpy(buf2, "Print");
                break;
            case VK_SNAPSHOT:
                strcpy(buf2, "PrintScrn");
                break;
            case VK_INSERT:
                strcpy(buf2, "Insert");
                break;
            case VK_DELETE:
                strcpy(buf2, "Delete");
                break;
            case VK_HELP:
                strcpy(buf2, "Help");
                break;
            case VK_MULTIPLY:
                strcpy(buf2, "Num*");
                break;
            case VK_ADD:
                strcpy(buf2, "Num+");
                break;
            case VK_SUBTRACT:
                strcpy(buf2, "Num-");
                break;
            case VK_DECIMAL:
                strcpy(buf2, "Num.");
                break;
            case VK_DIVIDE:
                strcpy(buf2, "Num/");
                break;
            case VK_NUMLOCK:
                strcpy(buf2, "NumLock");
                break;
            case VK_SCROLL:
                strcpy(buf2, "ScrollLock");
                break;
            case /*VK_OEM_PLUS*/ 0xBB:
                strcpy(buf2, "=+");
                break;
            case /*VK_OEM_MINUS*/ 0xBD:
                strcpy(buf2, "-_");
                break;
            case /*VK_OEM_COMMA*/ 0xBC:
                strcpy(buf2, ",");
                break;
            case /*VK_OEM_PERIOD*/ 0xBE:
                strcpy(buf2, ".");
                break;
            case VK_OEM_7:
                strcpy(buf2, "'\"");
                break;
            case VK_OEM_6:
                strcpy(buf2, "]}");
                break;
            case VK_OEM_5:
                strcpy(buf2, "\\|");
                break;
            case VK_OEM_4:
                strcpy(buf2, "[{");
                break;
            case VK_OEM_3:
                strcpy(buf2, "`~");
                break;
            case VK_OEM_2:
                strcpy(buf2, "/?");
                break;
            case VK_OEM_1:
                strcpy(buf2, ";:");
                break;
            default:
                sprintf(buf2, "(%d)", k);
                break;
            }
        strcat(buf, buf2);
    }
    // TODO: Port to Unicode properly
    return string_to_wstring(buf);
}
