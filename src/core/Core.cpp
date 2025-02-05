/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "Core.h"

#include "memory/memory.h"

core_params* g_core{};

core_result core_init(core_params* params)
{
    g_core = params;
    
    g_core->rdram = rdram;
    g_core->rdram_register = &rdram_register;
    g_core->pi_register = &pi_register;
    g_core->MI_register = &MI_register;
    g_core->sp_register = &sp_register;
    g_core->si_register = &si_register;
    g_core->vi_register = &vi_register;
    g_core->rsp_register = &rsp_register;
    g_core->ri_register = &ri_register;
    g_core->ai_register = &ai_register;
    g_core->dpc_register = &dpc_register;
    g_core->dps_register = &dps_register;
    g_core->SP_DMEM = SP_DMEM;
    g_core->SP_IMEM = SP_IMEM;
    g_core->PIF_RAM = PIF_RAM;
    
    return Res_Ok;
}

bool core_vr_get_mge_available()
{
    return g_core->plugin_funcs.read_video && g_core->plugin_funcs.get_video_size;
}
