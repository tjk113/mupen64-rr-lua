/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <gui/Main.h>
#include <AsyncExecutor.h>

namespace LuaCore::Movie
{
    // Movie
    static int PlayMovie(lua_State* L)
    {
        const char* fname = lua_tostring(L, 1);
        g_config.vcr_readonly = true;
        Messenger::broadcast(Messenger::Message::ReadonlyChanged, (bool)g_config.vcr_readonly);
        AsyncExecutor::invoke_async([=] { core_vcr_start_playback(fname); });
        return 0;
    }

    static int StopMovie(lua_State* L)
    {
        core_vcr_stop_all();
        return 0;
    }

    static int GetMovieFilename(lua_State* L)
    {
        if (core_vcr_get_task() == task_idle)
        {
            luaL_error(L, "No movie is currently playing");
            lua_pushstring(L, "");
        }
        else
        {
            lua_pushstring(L, core_vcr_get_path().string().c_str());
        }
        return 1;
    }

    static int GetVCRReadOnly(lua_State* L)
    {
        lua_pushboolean(L, g_config.vcr_readonly);
        return 1;
    }

    static int SetVCRReadOnly(lua_State* L)
    {
        g_config.vcr_readonly = lua_toboolean(L, 1);
        Messenger::broadcast(Messenger::Message::ReadonlyChanged, (bool)g_config.vcr_readonly);
        return 0;
    }

    static int begin_seek(lua_State* L)
    {
        auto str = string_to_wstring(lua_tostring(L, 1));
        bool pause_at_end = lua_toboolean(L, 2);

        lua_pushinteger(L, static_cast<int32_t>(core_vcr_begin_seek(str, pause_at_end)));
        return 1;
    }

    static int stop_seek(lua_State* L)
    {
        core_vcr_stop_seek();
        return 0;
    }

    static int is_seeking(lua_State* L)
    {
        lua_pushboolean(L, core_vcr_is_seeking());
        return 1;
    }

    static int get_seek_completion(lua_State* L)
    {
        std::pair<size_t, size_t> pair;
        core_vcr_get_seek_completion(pair);

        lua_newtable(L);
        lua_pushinteger(L, pair.first);
        lua_rawseti(L, -2, 1);
        lua_pushinteger(L, pair.second);
        lua_rawseti(L, -2, 2);

        return 1;
    }

    /**
     * Begins a warp modify operation.
     *
     * Accepts a table of inputs as:
     *
     * {
     *   {A = true, B = true, Z = true },
     *   {A = true, B = true, Start = true },
     * }
     *
     * Inputs which are not specified stay at their default value, 0.
     *
     * \param L
     * \return
     */
    static int begin_warp_modify(lua_State* L)
    {
        std::vector<BUTTONS> inputs;

        luaL_checktype(L, 1, LUA_TTABLE);
        lua_pushnil(L);
        while (lua_next(L, 1))
        {
            luaL_checktype(L, -1, LUA_TTABLE);
            lua_pushnil(L);
            BUTTONS buttons{};
            while (lua_next(L, -2))
            {
                std::string key = lua_tostring(L, -2);
                if (lua_toboolean(L, -1))
                {
                    if (key == "right")
                    {
                        buttons.R_DPAD = 1;
                    }
                    else if (key == "left")
                    {
                        buttons.L_DPAD = 1;
                    }
                    else if (key == "down")
                    {
                        buttons.D_DPAD = 1;
                    }
                    else if (key == "up")
                    {
                        buttons.U_DPAD = 1;
                    }
                    else if (key == "start")
                    {
                        buttons.START_BUTTON = 1;
                    }
                    else if (key == "Z")
                    {
                        buttons.Z_TRIG = 1;
                    }
                    else if (key == "B")
                    {
                        buttons.B_BUTTON = 1;
                    }
                    else if (key == "A")
                    {
                        buttons.A_BUTTON = 1;
                    }
                    else if (key == "Cright")
                    {
                        buttons.R_CBUTTON = 1;
                    }
                    else if (key == "Cleft")
                    {
                        buttons.L_CBUTTON = 1;
                    }
                    else if (key == "Cdown")
                    {
                        buttons.D_CBUTTON = 1;
                    }
                    else if (key == "Cup")
                    {
                        buttons.U_CBUTTON = 1;
                    }
                    else if (key == "R")
                    {
                        buttons.R_TRIG = 1;
                    }
                    else if (key == "L")
                    {
                        buttons.L_TRIG = 1;
                    }
                }
                if (lua_tointeger(L, -1))
                {
                    auto value = luaL_checkinteger(L, -1);
                    if (key == "X")
                    {
                        buttons.X_AXIS = value;
                    }
                    if (key == "Y")
                    {
                        buttons.Y_AXIS = value;
                    }
                }
                lua_pop(L, 1);
            }
            inputs.push_back(buttons);
            lua_pop(L, 1);
        }

        auto result = core_vcr_begin_warp_modify(inputs);

        lua_pushinteger(L, static_cast<int32_t>(result));
        return 1;
    }
}
