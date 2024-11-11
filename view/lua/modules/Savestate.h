extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <core/memory/savestates.h>

namespace LuaCore::Savestate
{
    static int SaveFileSavestate(lua_State* L)
    {
        ::Savestates::do_file(lua_tostring(L, 1), ::Savestates::Job::Save);
        return 0;
    }

    static int LoadFileSavestate(lua_State* L)
    {
        ::Savestates::do_file(lua_tostring(L, 1), ::Savestates::Job::Load);
        return 0;
    }
}
