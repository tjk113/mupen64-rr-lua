#include <include/lua.h>
#include <Windows.h>
#include "../../main/win/main_win.h"

namespace LuaCore::Avi
{
	static int StartCapture(lua_State* L)
	{
		const char* fname = lua_tostring(L, 1);
		if (!vcr_is_capturing())
			vcr_start_capture(fname, false);
		else
			luaL_error(
				L,
				"Tried to start AVI capture when one was already in progress");
		return 0;
	}

	static int StopCapture(lua_State* L)
	{
		if (vcr_is_capturing())
			vcr_stop_capture();
		else
			luaL_error(L, "Tried to end AVI capture when none was in progress");
		return 0;
	}
}
