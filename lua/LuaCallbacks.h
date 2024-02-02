#pragma once

// Lua callback interface

#include <Windows.h>
#include "include/lua.hpp"

namespace LuaCallbacks
{
	// TODO: Deprecate this, as it creates windows dependency in header
	void LuaWindowMessage(HWND, UINT, WPARAM, LPARAM);
	void AtUpdateScreenLuaCallback();
	void AtVILuaCallback();
	void AtInputLuaCallback(int n);
	void AtIntervalLuaCallback();
	void AtPlayMovieLuaCallback();
	void AtStopMovieLuaCallback();
	void AtLoadStateLuaCallback();
	void AtSaveStateLuaCallback();
	void AtResetLuaCallback();


	int AtUpdateScreen(lua_State* L);
	int AtStop(lua_State* L);
}
