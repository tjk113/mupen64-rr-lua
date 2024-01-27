#include <include/lua.h>
#include <Windows.h>
#include "../../main/win/main_win.h"

namespace LuaCore::Movie
{
	// Movie
	static int PlayMovie(lua_State* L)
	{
		const char* fname = lua_tostring(L, 1);
		vcr_set_read_only(true);
		vcr_start_playback(fname, "", "");
		return 0;
	}

	static int StopMovie(lua_State* L)
	{
		vcr_stop_playback();
		return 0;
	}

	static int GetMovieFilename(lua_State* L)
	{
		if (vcr_is_starting() || vcr_is_playing())
		{
			lua_pushstring(L, vcr_get_movie_filename());
		} else
		{
			luaL_error(L, "No movie is currently playing");
			lua_pushstring(L, "");
		}
		return 1;
	}

	static int GetVCRReadOnly(lua_State* L)
	{
		lua_pushboolean(L, vcr_get_read_only());
		return 1;
	}

}
