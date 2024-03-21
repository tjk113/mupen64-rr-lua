#pragma once

// Lua callback interface

#include <lua/include/lua.hpp>

namespace LuaCallbacks
{
	/**
	 * \brief Notifies all lua instances of a window message
	 */
	void call_window_message(void*, unsigned int, unsigned int, long);

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
