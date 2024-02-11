#include "LuaCallbacks.h"
#include "LuaConsole.h"
#include "../main/win/main_win.h"

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

	void call_updatescreen()
	{
		RET_IF_EMPTY;
		HDC main_dc = GetDC(mainHWND);

		// We need to copy the map, since it might be modified during iteration
		auto map = hwnd_lua_map;
		for (auto& pair : map)
		{
			/// Let the environment draw to its DCs

			// Draw to the bound D2D dc
			// Also clear with alpha mask before drawing!
			pair.second->d2d_render_target->BeginDraw();
			pair.second->d2d_render_target->Clear(D2D1::ColorF(bitmap_color_mask));
			pair.second->d2d_render_target->SetTransform(D2D1::Matrix3x2F::Identity());

			bool failed = pair.second->invoke_callbacks_with_key(state_update_screen, REG_ATUPDATESCREEN);

			pair.second->d2d_render_target->EndDraw();

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

			if (failed)
			{
				LuaEnvironment::destroy(pair.second);
			}
		}

		ReleaseDC(mainHWND, main_dc);
	}

	void call_vi()
	{
		RET_IF_EMPTY;
		main_dispatcher_invoke([]
		{
			invoke_callbacks_with_key_on_all_instances(
				AtVI, REG_ATVI);
		});
	}

	void call_input(int n)
	{
		RET_IF_EMPTY;
		main_dispatcher_invoke([n]
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
		main_dispatcher_invoke([]
		{
			invoke_callbacks_with_key_on_all_instances(
				CallTop, REG_ATINTERVAL);
		});
	}

	void call_play_movie()
	{
		RET_IF_EMPTY;
		main_dispatcher_invoke([]
		{
			invoke_callbacks_with_key_on_all_instances(
				CallTop, REG_ATPLAYMOVIE);
		});
	}

	void call_stop_movie()
	{
		RET_IF_EMPTY;
		main_dispatcher_invoke([]
		{
			invoke_callbacks_with_key_on_all_instances(
				CallTop, REG_ATSTOPMOVIE);
		});
	}

	void call_load_state()
	{
		RET_IF_EMPTY;
		main_dispatcher_invoke([]
		{
			invoke_callbacks_with_key_on_all_instances(
				CallTop, REG_ATLOADSTATE);
		});
	}

	void call_save_state()
	{
		RET_IF_EMPTY;
		main_dispatcher_invoke([]
		{
			invoke_callbacks_with_key_on_all_instances(
				CallTop, REG_ATSAVESTATE);
		});
	}

	void call_reset()
	{
		RET_IF_EMPTY;
		main_dispatcher_invoke([]
		{
			invoke_callbacks_with_key_on_all_instances(
				CallTop, REG_ATRESET);
		});
	}
#pragma endregion
}
