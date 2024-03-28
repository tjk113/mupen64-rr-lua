extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <shared/messenger.h>

namespace LuaCore::Movie
{
	// Movie
	static int PlayMovie(lua_State* L)
	{
		const char* fname = lua_tostring(L, 1);
		Config.vcr_readonly = true;
		Messenger::broadcast(Messenger::Message::ReadonlyChanged, (bool)Config.vcr_readonly);
		std::thread([fname] { VCR::start_playback(fname); }).detach();
		return 0;
	}

	static int StopMovie(lua_State* L)
	{
		VCR::stop_all();
		return 0;
	}

	static int GetMovieFilename(lua_State* L)
	{
		if (vcr_is_starting() || vcr_is_playing())
		{
			lua_pushstring(L, g_movie_path.string().c_str());
		} else
		{
			luaL_error(L, "No movie is currently playing");
			lua_pushstring(L, "");
		}
		return 1;
	}

	static int GetVCRReadOnly(lua_State* L)
	{
		lua_pushboolean(L, Config.vcr_readonly);
		return 1;
	}

	static int SetVCRReadOnly(lua_State* L)
	{
		Config.vcr_readonly = lua_toboolean(L, 1);
		Messenger::broadcast(Messenger::Message::ReadonlyChanged, (bool)Config.vcr_readonly);
		return 0;
	}
}
