#include "LuaCallbacks.h"
#include "LuaConsole.h"
#include "../main/win/main_win.h"
#include "win/features/Dispatcher.h"

namespace LuaCallbacks
{
	// OPTIMIZATION: If no lua scripts are running, skip the deeper lua path
#define RET_IF_EMPTY { if(hwnd_lua_map.empty()) return; }

	t_window_procedure_params window_proc_params = {0};
	int current_input_n = 0;

	//generic function used for all of the At... callbacks, calls function from stack top.
	int CallTop(lua_State* L)
	{
		return lua_pcall(L, 0, 0, 0);
	}

	int state_update_screen(lua_State* L)
	{
		return lua_pcall(L, 0, 0, 0);
	}

	int AtVI(lua_State* L)
	{
		return lua_pcall(L, 0, 0, 0);
	}


	int AtInput(lua_State* L)
	{
		lua_pushinteger(L, current_input_n);
		return lua_pcall(L, 1, 0, 0);
	}

	int state_stop(lua_State* L)
	{
		return lua_pcall(L, 0, 0, 0);
	}

	int AtWindowMessage(lua_State* L)
	{
		lua_pushinteger(L, (unsigned)window_proc_params.wnd);
		lua_pushinteger(L, window_proc_params.msg);
		lua_pushinteger(L, window_proc_params.w_param);
		lua_pushinteger(L, window_proc_params.l_param);

		return lua_pcall(L, 4, 0, 0);
	}

#pragma region Call Implementations
	void call_window_message(HWND wnd, UINT msg, WPARAM w, LPARAM l)
	{
		RET_IF_EMPTY;
		// Invoking dispatcher here isn't allowed, as it would lead to infinite recursion
		window_proc_params = {
			.wnd = wnd,
			.msg = msg,
			.w_param = w,
			.l_param = l
		};
		invoke_callbacks_with_key_on_all_instances(
			AtWindowMessage, REG_WINDOWMESSAGE);
	}

	void call_vi()
	{
		RET_IF_EMPTY;
		Dispatcher::invoke([]
		{
			invoke_callbacks_with_key_on_all_instances(
				AtVI, REG_ATVI);
		});
	}

	void call_input(int n)
	{
		RET_IF_EMPTY;
		Dispatcher::invoke([n]
		{
			current_input_n = n;
			invoke_callbacks_with_key_on_all_instances(
				AtInput, REG_ATINPUT);
			inputCount++;
		});
	}

	void call_interval()
	{
		RET_IF_EMPTY;
		Dispatcher::invoke([]
		{
			invoke_callbacks_with_key_on_all_instances(
				CallTop, REG_ATINTERVAL);
		});
	}

	void call_play_movie()
	{
		RET_IF_EMPTY;
		Dispatcher::invoke([]
		{
			invoke_callbacks_with_key_on_all_instances(
				CallTop, REG_ATPLAYMOVIE);
		});
	}

	void call_stop_movie()
	{
		RET_IF_EMPTY;
		Dispatcher::invoke([]
		{
			invoke_callbacks_with_key_on_all_instances(
				CallTop, REG_ATSTOPMOVIE);
		});
	}

	void call_load_state()
	{
		RET_IF_EMPTY;
		Dispatcher::invoke([]
		{
			invoke_callbacks_with_key_on_all_instances(
				CallTop, REG_ATLOADSTATE);
		});
	}

	void call_save_state()
	{
		RET_IF_EMPTY;
		Dispatcher::invoke([]
		{
			invoke_callbacks_with_key_on_all_instances(
				CallTop, REG_ATSAVESTATE);
		});
	}

	void call_reset()
	{
		RET_IF_EMPTY;
		Dispatcher::invoke([]
		{
			invoke_callbacks_with_key_on_all_instances(
				CallTop, REG_ATRESET);
		});
	}
#pragma endregion
}
