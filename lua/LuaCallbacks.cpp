#include "LuaCallbacks.h"
#include "LuaConsole.h"
#include "../main/win/main_win.h"

namespace LuaCallbacks
{
	t_window_procedure_params window_proc_params = {0};
	int current_input_n = 0;

	//generic function used for all of the At... callbacks, calls function from stack top.
	int CallTop(lua_State* L)
	{
		return lua_pcall(L, 0, 0, 0);
	}

	int AtUpdateScreen(lua_State* L)
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

	int AtStop(lua_State* L)
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

	void LuaWindowMessage(HWND wnd, UINT msg, WPARAM w, LPARAM l)
	{
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

	void AtUpdateScreenLuaCallback()
	{
		HDC main_dc = GetDC(mainHWND);

		for (auto& pair : hwnd_lua_map)
		{
			/// Let the environment draw to its DCs
			pair.second->draw();

			/// Blit its DCs (GDI, D2D) to the main window with alpha mask
			TransparentBlt(main_dc, 0, 0, pair.second->dc_width,
			               pair.second->dc_height, pair.second->gdi_dc, 0, 0,
			               pair.second->dc_width, pair.second->dc_height,
			               bitmap_color_mask);
			TransparentBlt(main_dc, 0, 0, pair.second->dc_width,
			               pair.second->dc_height, pair.second->d2d_dc, 0, 0,
			               pair.second->dc_width, pair.second->dc_height,
			               bitmap_color_mask);

			// Fill GDI DC with alpha mask
			RECT rect = {0, 0, pair.second->dc_width, pair.second->dc_height};
			HBRUSH brush = CreateSolidBrush(bitmap_color_mask);
			FillRect(pair.second->gdi_dc, &rect, brush);
			DeleteObject(brush);
		}

		ReleaseDC(mainHWND, main_dc);
	}

	void AtVILuaCallback()
	{
		main_dispatcher_invoke([]
		{
			invoke_callbacks_with_key_on_all_instances(
				AtVI, REG_ATVI);
		});
	}

	void AtInputLuaCallback(int n)
	{
		main_dispatcher_invoke([n]
		{
			current_input_n = n;
			invoke_callbacks_with_key_on_all_instances(
				AtInput, REG_ATINPUT);
			inputCount++;
		});
	}

	void AtIntervalLuaCallback()
	{
		main_dispatcher_invoke([]
		{
			invoke_callbacks_with_key_on_all_instances(
				CallTop, REG_ATINTERVAL);
		});
	}

	void AtPlayMovieLuaCallback()
	{
		main_dispatcher_invoke([]
		{
			invoke_callbacks_with_key_on_all_instances(
				CallTop, REG_ATPLAYMOVIE);
		});
	}

	void AtStopMovieLuaCallback()
	{
		main_dispatcher_invoke([]
		{
			invoke_callbacks_with_key_on_all_instances(
				CallTop, REG_ATSTOPMOVIE);
		});
	}

	void AtLoadStateLuaCallback()
	{
		main_dispatcher_invoke([]
		{
			invoke_callbacks_with_key_on_all_instances(
				CallTop, REG_ATLOADSTATE);
		});
	}

	void AtSaveStateLuaCallback()
	{
		main_dispatcher_invoke([]
		{
			invoke_callbacks_with_key_on_all_instances(
				CallTop, REG_ATSAVESTATE);
		});
	}

	void AtResetLuaCallback()
	{
		main_dispatcher_invoke([]
		{
			invoke_callbacks_with_key_on_all_instances(
				CallTop, REG_ATRESET);
		});
	}
}
