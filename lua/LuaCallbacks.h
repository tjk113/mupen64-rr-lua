#pragma once

// Lua callback interface

#include <Windows.h>
#include "include/lua.hpp"

namespace LuaCallbacks
{
	/**
	 * \brief Notifies all lua instances of a window message
	 * \remark TODO: Deprecate this, as it creates windows dependency in header
	 */
	void call_window_message(HWND, UINT, WPARAM, LPARAM);

	/**
	 * \brief Notifies all lua instances of a screen update
	 */
	void call_updatescreen();

	/**
	 * \brief Notifies all lua instances of a visual interrupt
	 */
	void call_vi();

	/**
	 * \brief Notifies all lua instances of an input poll
	 * \param n The polled controller's index
	 */
	void call_input(int n);

	/**
	 * \brief Notifies all lua instances of the heartbeat while paused
	 */
	void call_interval();

	/**
	 * \brief Notifies all lua instances of movie playback starting
	 */
	void call_play_movie();

	/**
	 * \brief Notifies all lua instances of movie playback stopping
	 */
	void call_stop_movie();

	/**
	 * \brief Notifies all lua instances of a state being saves
	 */
	void call_save_state();

	/**
	 * \brief Notifies all lua instances of a state being loaded
	 */
	void call_load_state();

	/**
	 * \brief Notifies all lua instances of the rom being reset
	 */
	void call_reset();

#pragma region Raw Calls
	int state_update_screen(lua_State* L);
	int state_stop(lua_State* L);
#pragma endregion
}
