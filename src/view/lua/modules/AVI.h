/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <capture/EncodingManager.h>

namespace LuaCore::Avi
{
    static int StartCapture(lua_State* L)
    {
        const char* fname = lua_tostring(L, 1);

        if (!EncodingManager::is_capturing())
            EncodingManager::start_capture(fname, static_cast<core_encoder_type>(g_config.encoder_type), false);
        else
            luaL_error(
                L,
                "Tried to start AVI capture when one was already in progress");
        return 0;
    }

    static int StopCapture(lua_State* L)
    {
        if (EncodingManager::is_capturing())
            EncodingManager::stop_capture();
        else
            luaL_error(L, "Tried to end AVI capture when none was in progress");
        return 0;
    }
}
