/*

*/


#include "LuaConsole.h"
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <string>
#include <map>
#include <list>
#include <filesystem>
#include "../../winproject/resource.h"
#include "win/main_win.h"
#include "win/Config.hpp"
#include "include/lua.hpp"
#include "../memory/memory.h"
#include "../r4300/r4300.h"
#include "../r4300/disasm.h"
#include "../r4300/recomp.h"
#include "../r4300/Plugin.hpp"
#include "../main/vcr.h"
#include "../main/savestates.h"
#include "../main/win/configdialog.h"
#include <helpers/win_helpers.h>
#include "../main/win/wrapper/PersistentPathDialog.h"
#include "../lib/md5.h"
#include <vcr.h>
#include <gdiplus.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>
#include <functional>
#include <queue>
#include <assert.h>

#include <helpers/string_helpers.h>
#include "win/features/Statusbar.hpp"

#include <Windows.h>

#include "LuaCallbacks.h"
#include "modules/avi.h"
#include "modules/d2d.h"
#include "modules/emu.h"
#include "modules/global.h"
#include "modules/input.h"
#include "modules/iohelper.h"
#include "modules/joypad.h"
#include "modules/memory.h"
#include "modules/movie.h"
#include "modules/savestate.h"
#include "modules/wgui.h"
#include "win/timers.h"

extern unsigned long op;
extern void (*interp_ops[64])(void);

extern int fast_memory;

std::vector<std::string> recent_closed_lua;

BUTTONS last_controller_data[4];
BUTTONS new_controller_data[4];
bool overwrite_controller_data[4];

ULONG_PTR gdi_plus_token;
auto d2d_overlay_class = "lua_d2d_overlay";
auto gdi_overlay_class = "lua_gdi_overlay";

std::map<HWND, LuaEnvironment*> hwnd_lua_map;

static uint32_t bitmap_color_mask = RGB(255, 0, 255);
static HBRUSH alpha_mask_brush = CreateSolidBrush(bitmap_color_mask);



	uint64_t inputCount = 0;

	int getn(lua_State*);


	int AtPanic(lua_State* L);
	extern const luaL_Reg globalFuncs[];
	extern const luaL_Reg emuFuncs[];
	extern const luaL_Reg wguiFuncs[];
	extern const luaL_Reg d2dFuncs[];
	extern const luaL_Reg memoryFuncs[];
	extern const luaL_Reg inputFuncs[];
	extern const luaL_Reg joypadFuncs[];
	extern const luaL_Reg movieFuncs[];
	extern const luaL_Reg savestateFuncs[];
	extern const luaL_Reg ioHelperFuncs[];
	extern const luaL_Reg aviFuncs[];
	extern const char* const REG_ATSTOP;

	int AtPanic(lua_State* L)
	{
		printf("Lua panic: %s\n", lua_tostring(L, -1));
		MessageBox(mainHWND, lua_tostring(L, -1), "Lua Panic", 0);
		return 0;
	}

	extern const char* const REG_WINDOWMESSAGE;

	void invoke_callbacks_with_key_on_all_instances(
		std::function<int(lua_State*)> function, const char* key)
	{
		// We need to copy the map, since it might be modified during iteration
		auto map = hwnd_lua_map;
		for (auto pair : map)
		{
			if(!pair.second->invoke_callbacks_with_key(function, key))
				continue;

			LuaEnvironment::destroy(pair.second);
		}
	}

	BOOL WmCommand(HWND wnd, WORD id, WORD code, HWND control);

	INT_PTR CALLBACK DialogProc(HWND wnd, UINT msg, WPARAM wParam,
	                            LPARAM lParam)
	{
		switch (msg)
		{
		case WM_INITDIALOG:
			{
				SetWindowText(GetDlgItem(wnd, IDC_TEXTBOX_LUASCRIPTPATH),
				              Config.lua_script_path.c_str());
				return TRUE;
			}
		case WM_CLOSE:
			DestroyWindow(wnd);
			return TRUE;
		case WM_DESTROY:
			{
				if (hwnd_lua_map.contains(wnd)) {
					LuaEnvironment::destroy(hwnd_lua_map[wnd]);
				}
				return TRUE;
			}
		case WM_COMMAND:
			return WmCommand(wnd, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
		case WM_SIZE:
			{
				RECT window_rect = {0};
				GetClientRect(wnd, &window_rect);

				HWND console_hwnd = GetDlgItem(wnd, IDC_TEXTBOX_LUACONSOLE);
				RECT console_rect = get_window_rect_client_space(wnd, console_hwnd);
				SetWindowPos(console_hwnd, nullptr, 0, 0, window_rect.right - console_rect.left * 2, window_rect.bottom - console_rect.top, SWP_NOMOVE);

				HWND path_hwnd = GetDlgItem(wnd, IDC_TEXTBOX_LUASCRIPTPATH);
				RECT path_rect = get_window_rect_client_space(wnd, path_hwnd);
				SetWindowPos(path_hwnd, nullptr, 0, 0, window_rect.right - console_rect.left * 2, path_rect.bottom - path_rect.top, SWP_NOMOVE);
				if (wParam == SIZE_MINIMIZED) SetFocus(mainHWND);
				break;
			}
		}
		return FALSE;
	}

	void SetButtonState(HWND wnd, bool state)
	{
		if (!IsWindow(wnd)) return;
		const HWND state_button = GetDlgItem(wnd, IDC_BUTTON_LUASTATE);
		const HWND stop_button = GetDlgItem(wnd, IDC_BUTTON_LUASTOP);
		if (state)
		{
			SetWindowText(state_button, "Restart");
			EnableWindow(stop_button, TRUE);
		} else
		{
			SetWindowText(state_button, "Run");
			EnableWindow(stop_button, FALSE);
		}
	}

	BOOL WmCommand(HWND wnd, WORD id, WORD code, HWND control)
	{
		switch (id)
		{
		case IDC_BUTTON_LUASTATE:
			{
				char path[MAX_PATH] = {0};
				GetWindowText(GetDlgItem(wnd, IDC_TEXTBOX_LUASCRIPTPATH),
				              path, MAX_PATH);

				// if already running, delete and erase it (we dont want to overwrite the environment without properly disposing it)
				if (hwnd_lua_map.contains(wnd)) {
					LuaEnvironment::destroy(hwnd_lua_map[wnd]);
				}

				// now spool up a new one
				auto status = LuaEnvironment::create(path, wnd);
				Messenger::broadcast(Messenger::Message::ScriptStarted, std::filesystem::path(path));

				if (status.first == nullptr) {
					// failed, we give user some info and thats it
					ConsoleWrite(wnd, (status.second + "\r\n").c_str());
				} else {
					// it worked, we can set up associations and sync ui state
					hwnd_lua_map[wnd] = status.first;
					SetButtonState(wnd, true);
				}

				return TRUE;
			}
		case IDC_BUTTON_LUASTOP:
			{
				if (hwnd_lua_map.contains(wnd)) {
					LuaEnvironment::destroy(hwnd_lua_map[wnd]);
					SetButtonState(wnd, false);
				}
				return TRUE;
			}
		case IDC_BUTTON_LUABROWSE:
			{
				auto path = show_persistent_open_dialog("o_lua", wnd, L"*.lua");

				if (path.empty())
				{
					break;
				}

				SetWindowText(GetDlgItem(wnd, IDC_TEXTBOX_LUASCRIPTPATH), wstring_to_string(path).c_str());
				return TRUE;
			}
		case IDC_BUTTON_LUAEDIT:
			{
				CHAR buf[MAX_PATH];
				GetWindowText(GetDlgItem(wnd, IDC_TEXTBOX_LUASCRIPTPATH),
				              buf, MAX_PATH);
				if (buf == NULL || buf[0] == '\0')
					/* || strlen(buf)>MAX_PATH*/
					return FALSE;
				// previously , clicking edit with empty path will open current directory in explorer, which is very bad

				ShellExecute(0, 0, buf, 0, 0, SW_SHOW);
				return TRUE;
			}
		case IDC_BUTTON_LUACLEAR:
			if (GetAsyncKeyState(VK_MENU))
			{
				// clear path
				SetWindowText(GetDlgItem(wnd, IDC_TEXTBOX_LUASCRIPTPATH), "");
				return TRUE;
			}

			SetWindowText(GetDlgItem(wnd, IDC_TEXTBOX_LUACONSOLE), "");
			return TRUE;
		}
		return FALSE;
	}

	HWND lua_create()
	{
		HWND hwnd = CreateDialogParam(app_instance,
		                             MAKEINTRESOURCE(IDD_LUAWINDOW), mainHWND,
		                             DialogProc,
		                             NULL);
		ShowWindow(hwnd, SW_SHOW);
		return hwnd;
	}




void lua_exit()
{
	Gdiplus::GdiplusShutdown(gdi_plus_token);
}

void lua_create_and_run(const char* path)
	{
		printf("Creating lua window...\n");
		auto hwnd = lua_create();

		printf("Setting path...\n");
		// set the textbox content to match the path
		SetWindowText(GetDlgItem(hwnd, IDC_TEXTBOX_LUASCRIPTPATH), path);

		printf("Simulating run button click...\n");
		// click run button
		SendMessage(hwnd, WM_COMMAND,
						MAKEWPARAM(IDC_BUTTON_LUASTATE, BN_CLICKED),
						(LPARAM)GetDlgItem(hwnd, IDC_BUTTON_LUASTATE));
	}


	void ConsoleWrite(HWND wnd, const char* str)
	{
		HWND console = GetDlgItem(wnd, IDC_TEXTBOX_LUACONSOLE);

		int length = GetWindowTextLength(console);
		if (length >= 0x7000)
		{
			SendMessage(console, EM_SETSEL, 0, length / 2);
			SendMessage(console, EM_REPLACESEL, false, (LPARAM)"");
			length = GetWindowTextLength(console);
		}
		SendMessage(console, EM_SETSEL, length, length);
		SendMessage(console, EM_REPLACESEL, false, (LPARAM)str);
	}

	LRESULT CALLBACK LuaGUIWndProc(HWND wnd, UINT msg, WPARAM wParam,
	                               LPARAM lParam)
	{
		switch (msg)
		{
		case WM_CREATE:
		case WM_DESTROY:
			return 0;
		}
		return DefWindowProc(wnd, msg, wParam, lParam);
	}

	LuaEnvironment* GetLuaClass(lua_State* L)
	{
		lua_getfield(L, LUA_REGISTRYINDEX, REG_LUACLASS);
		auto lua = (LuaEnvironment*)lua_topointer(L, -1);
		lua_pop(L, 1);
		return lua;
	}

	void SetLuaClass(lua_State* L, void* lua)
	{
		lua_pushlightuserdata(L, lua);
		lua_setfield(L, LUA_REGISTRYINDEX, REG_LUACLASS);
		//lua_pop(L, 1); //iteresting, it worked before
	}

	int GetErrorMessage(lua_State* L)
	{
		return 1;
	}

	int getn(lua_State* L)
	{
		lua_pushinteger(L, luaL_len(L, -1));
		return 1;
	}

	int RegisterFunction(lua_State* L, const char* key)
	{
		lua_getfield(L, LUA_REGISTRYINDEX, key);
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			lua_newtable(L);
			lua_setfield(L, LUA_REGISTRYINDEX, key);
			lua_getfield(L, LUA_REGISTRYINDEX, key);
		}
		int i = luaL_len(L, -1) + 1;
		lua_pushinteger(L, i);
		lua_pushvalue(L, -3); //
		lua_settable(L, -3);
		lua_pop(L, 1);
		return i;
	}

	void UnregisterFunction(lua_State* L, const char* key)
	{
		lua_getfield(L, LUA_REGISTRYINDEX, key);
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			lua_newtable(L); //�Ƃ肠����
		}
		int n = luaL_len(L, -1);
		for (LUA_INTEGER i = 0; i < n; i++)
		{
			lua_pushinteger(L, 1 + i);
			lua_gettable(L, -2);
			if (lua_rawequal(L, -1, -3))
			{
				lua_pop(L, 1);
				lua_getglobal(L, "table");
				lua_getfield(L, -1, "remove");
				lua_pushvalue(L, -3);
				lua_pushinteger(L, 1 + i);
				lua_call(L, 2, 0);
				lua_pop(L, 2);
				return;
			}
			lua_pop(L, 1);
		}
		lua_pop(L, 1);
		lua_pushfstring(L, "unregisterFunction(%s): not found function",
		                key);
		lua_error(L);
	}

	// 000000 | 0000 0000 0000 000 | stype(5) = 10101 |001111
	const ULONG BREAKPOINTSYNC_MAGIC_STYPE = 0x15;
	const ULONG BREAKPOINTSYNC_MAGIC = 0x0000000F |
		(BREAKPOINTSYNC_MAGIC_STYPE << 6);


	// these begin and end comments help to generate documentation
	// please don't remove them

	// begin lua funcs
	const luaL_Reg globalFuncs[] = {
		{"print", LuaCore::Global::Print},
		{"printx", LuaCore::Global::PrintX},
		{"tostringex", LuaCore::Global::ToStringExs},
		{"stop", LuaCore::Global::StopScript},
		{NULL, NULL}
	};

	const luaL_Reg emuFuncs[] = {
		{"console", LuaCore::Emu::ConsoleWriteLua},
		{"statusbar", LuaCore::Emu::StatusbarWrite},

		{"atvi", LuaCore::Emu::RegisterVI},
		{"atupdatescreen", LuaCore::Emu::RegisterUpdateScreen},
		{"atdrawd2d", LuaCore::Emu::RegisterAtDrawD2D},
		{"atinput", LuaCore::Emu::RegisterInput},
		{"atstop", LuaCore::Emu::RegisterStop},
		{"atwindowmessage", LuaCore::Emu::RegisterWindowMessage},
		{"atinterval", LuaCore::Emu::RegisterInterval},
		{"atplaymovie", LuaCore::Emu::RegisterPlayMovie},
		{"atstopmovie", LuaCore::Emu::RegisterStopMovie},
		{"atloadstate", LuaCore::Emu::RegisterLoadState},
		{"atsavestate", LuaCore::Emu::RegisterSaveState},
		{"atreset", LuaCore::Emu::RegisterReset},

		{"framecount", LuaCore::Emu::GetVICount},
		{"samplecount", LuaCore::Emu::GetSampleCount},
		{"inputcount", LuaCore::Emu::GetInputCount},

		// DEPRECATE: This is completely useless
		{"getversion", LuaCore::Emu::GetMupenVersion},

		{"pause", LuaCore::Emu::EmuPause},
		{"getpause", LuaCore::Emu::GetEmuPause},
		{"getspeed", LuaCore::Emu::GetSpeed},
		{"get_ff", LuaCore::Emu::GetFastForward},
		{"set_ff", LuaCore::Emu::SetFastForward},
		{"speed", LuaCore::Emu::SetSpeed},
		{"speedmode", LuaCore::Emu::SetSpeedMode},
		// DEPRECATE: This is completely useless
		{"setgfx", LuaCore::Emu::SetGFX},

		{"getaddress", LuaCore::Emu::GetAddress},

		{"screenshot", LuaCore::Emu::Screenshot},

#pragma region WinAPI
		// DEPRECATE: WinAPI coupling
		{"getsystemmetrics", LuaCore::Emu::GetSystemMetricsLua},
		{"play_sound", LuaCore::Emu::LuaPlaySound},
#pragma endregion
		{"ismainwindowinforeground", LuaCore::Emu::IsMainWindowInForeground},

		{NULL, NULL}
	};
	const luaL_Reg memoryFuncs[] = {
		// memory conversion functions
		{"inttofloat", LuaCore::Memory::LuaIntToFloat},
		{"inttodouble", LuaCore::Memory::LuaIntToDouble},
		{"floattoint", LuaCore::Memory::LuaFloatToInt},
		{"doubletoint", LuaCore::Memory::LuaDoubleToInt},
		{"qwordtonumber", LuaCore::Memory::LuaQWordToNumber},

		// word = 2 bytes
		// reading functions
		{"readbytesigned", LuaCore::Memory::LuaReadByteSigned},
		{"readbyte", LuaCore::Memory::LuaReadByteUnsigned},
		{"readwordsigned", LuaCore::Memory::LuaReadWordSigned},
		{"readword", LuaCore::Memory::LuaReadWordUnsigned},
		{"readdwordsigned", LuaCore::Memory::LuaReadDWordSigned},
		{"readdword", LuaCore::Memory::LuaReadDWorldUnsigned},
		{"readqwordsigned", LuaCore::Memory::LuaReadQWordSigned},
		{"readqword", LuaCore::Memory::LuaReadQWordUnsigned},
		{"readfloat", LuaCore::Memory::LuaReadFloat},
		{"readdouble", LuaCore::Memory::LuaReadDouble},
		{"readsize", LuaCore::Memory::LuaReadSize},

		// writing functions
		// all of these are assumed to be unsigned
		{"writebyte", LuaCore::Memory::LuaWriteByteUnsigned},
		{"writeword", LuaCore::Memory::LuaWriteWordUnsigned},
		{"writedword", LuaCore::Memory::LuaWriteDWordUnsigned},
		{"writeqword", LuaCore::Memory::LuaWriteQWordUnsigned},
		{"writefloat", LuaCore::Memory::LuaWriteFloatUnsigned},
		{"writedouble", LuaCore::Memory::LuaWriteDoubleUnsigned},

		{"writesize", LuaCore::Memory::LuaWriteSize},

		{NULL, NULL}
	};

	const luaL_Reg wguiFuncs[] = {
		{"setbrush", LuaCore::Wgui::set_brush},
		{"setpen", LuaCore::Wgui::set_pen},
		{"setcolor", LuaCore::Wgui::set_text_color},
		{"setbk", LuaCore::Wgui::SetBackgroundColor},
		{"setfont", LuaCore::Wgui::SetFont},
		{"text", LuaCore::Wgui::LuaTextOut},
		{"drawtext", LuaCore::Wgui::LuaDrawText},
		{"drawtextalt", LuaCore::Wgui::LuaDrawTextAlt},
		{"gettextextent", LuaCore::Wgui::GetTextExtent},
		{"rect", LuaCore::Wgui::DrawRect},
		{"fillrect", LuaCore::Wgui::FillRect},
		/*<GDIPlus>*/
		// GDIPlus functions marked with "a" suffix
		{"fillrecta", LuaCore::Wgui::FillRectAlpha},
		{"fillellipsea", LuaCore::Wgui::FillEllipseAlpha},
		{"fillpolygona", LuaCore::Wgui::FillPolygonAlpha},
		{"loadimage", LuaCore::Wgui::LuaLoadImage},
		{"deleteimage", LuaCore::Wgui::DeleteImage},
		{"drawimage", LuaCore::Wgui::DrawImage},
		{"loadscreen", LuaCore::Wgui::LoadScreen},
		{"loadscreenreset", LuaCore::Wgui::LoadScreenReset},
		{"getimageinfo", LuaCore::Wgui::GetImageInfo},
		/*</GDIPlus*/
		{"ellipse", LuaCore::Wgui::DrawEllipse},
		{"polygon", LuaCore::Wgui::DrawPolygon},
		{"line", LuaCore::Wgui::DrawLine},
		{"info", LuaCore::Wgui::GetGUIInfo},
		{"resize", LuaCore::Wgui::ResizeWindow},
		{"setclip", LuaCore::Wgui::SetClip},
		{"resetclip", LuaCore::Wgui::ResetClip},
		{NULL, NULL}
	};

	const luaL_Reg d2dFuncs[] = {
		{"create_brush", LuaCore::D2D::create_brush},
		{"free_brush", LuaCore::D2D::free_brush},

		{"clear", LuaCore::D2D::clear},
		{"fill_rectangle", LuaCore::D2D::fill_rectangle},
		{"draw_rectangle", LuaCore::D2D::draw_rectangle},
		{"fill_ellipse", LuaCore::D2D::fill_ellipse},
		{"draw_ellipse", LuaCore::D2D::draw_ellipse},
		{"draw_line", LuaCore::D2D::draw_line},
		{"draw_text", LuaCore::D2D::draw_text},
		{"get_text_size", LuaCore::D2D::measure_text},
		{"push_clip", LuaCore::D2D::push_clip},
		{"pop_clip", LuaCore::D2D::pop_clip},
		{"fill_rounded_rectangle", LuaCore::D2D::fill_rounded_rectangle},
		{"draw_rounded_rectangle", LuaCore::D2D::draw_rounded_rectangle},
		{"load_image", LuaCore::D2D::load_image},
		{"free_image", LuaCore::D2D::free_image},
		{"draw_image", LuaCore::D2D::draw_image},
		{"get_image_info", LuaCore::D2D::get_image_info},
		{"set_text_antialias_mode", LuaCore::D2D::set_text_antialias_mode},
		{"set_antialias_mode", LuaCore::D2D::set_antialias_mode},

		{"draw_to_image", LuaCore::D2D::draw_to_image},
		{NULL, NULL}
	};

	const luaL_Reg inputFuncs[] = {
		{"get", LuaCore::Input::get_keys},
		{"diff", LuaCore::Input::GetKeyDifference},
		{"prompt", LuaCore::Input::InputPrompt},
		{"get_key_name_text", LuaCore::Input::LuaGetKeyNameText},
		// DEPRECATE: WinAPI coupling
		{"map_virtual_key_ex", LuaCore::Input::LuaMapVirtualKeyEx},
		{NULL, NULL}
	};
	const luaL_Reg joypadFuncs[] = {
		{"get", LuaCore::Joypad::lua_get_joypad},
		{"set", LuaCore::Joypad::lua_set_joypad},
		// OBSOLETE: Cross-module reach
		{"count", LuaCore::Emu::GetInputCount},
		{NULL, NULL}
	};

	const luaL_Reg movieFuncs[] = {
		{"play", LuaCore::Movie::PlayMovie},
		{"stop", LuaCore::Movie::StopMovie},
		{"get_filename", LuaCore::Movie::GetMovieFilename},
		{"get_readonly", LuaCore::Movie::GetVCRReadOnly},
		{"set_readonly", LuaCore::Movie::SetVCRReadOnly},
		{NULL, NULL}
	};

	const luaL_Reg savestateFuncs[] = {
		{"savefile", LuaCore::Savestate::SaveFileSavestate},
		{"loadfile", LuaCore::Savestate::LoadFileSavestate},
		{NULL, NULL}
	};
	const luaL_Reg ioHelperFuncs[] = {
		{"filediag", LuaCore::IOHelper::LuaFileDialog},
		{NULL, NULL}
	};
	const luaL_Reg aviFuncs[] = {
		{"startcapture", LuaCore::Avi::StartCapture},
		{"stopcapture", LuaCore::Avi::StopCapture},
		{NULL, NULL}
	};
	// end lua funcs




void instrStr1(unsigned long pc, unsigned long w, char* p1)
{
	char*& p = p1;
	INSTDECODE decode;
	const char* const x = "0123456789abcdef";
#define HEX8(n) 	p[0] = x[(r4300word)(n)>>28&0xF];\
	p[1] = x[(r4300word)(n)>>24&0xF];\
	p[2] = x[(r4300word)(n)>>20&0xF];\
	p[3] = x[(r4300word)(n)>>16&0xF];\
	p[4] = x[(r4300word)(n)>>12&0xF];\
	p[5] = x[(r4300word)(n)>>8&0xF];\
	p[6] = x[(r4300word)(n)>>4&0xF];\
	p[7] = x[(r4300word)(n)&0xF];\
	p+=8;

	DecodeInstruction(w, &decode);
	HEX8(pc);
	*(p++) = ':';
	*(p++) = ' ';
	HEX8(w);
	*(p++) = ' ';
	const char* ps = p;
	if (w == 0x00000000)
	{
		*(p++) = 'n';
		*(p++) = 'o';
		*(p++) = 'p';
	} else
	{
		for (const char* q = GetOpecodeString(&decode); *q; q++)
		{
			*(p++) = *q;
		}
		*(p++) = ' ';
		p = GetOperandString(p, &decode, pc);
	}
	for (int i = p - ps + 3; i < 24; i += 4)
	{
		*(p++) = '\t';
	}
	*(p++) = ';';
	INSTOPERAND& o = decode.operand;
#define REGCPU(n) if((n)!=0){\
			for(const char *l = CPURegisterName[n]; *l; l++){\
				*(p++) = *l;\
			}\
			*(p++) = '=';\
			HEX8(reg[n]);\
	}
#define REGCPU2(n,m) \
		REGCPU(n);\
		if((n)!=(m)&&(m)!=0){C;REGCPU(m);}
	//10�i��
#define REGFPU(n) *(p++)='f';\
			*(p++)=x[(n)/10];\
			*(p++)=x[(n)%10];\
			*(p++) = '=';\
			p+=sprintf(p,"%f",*reg_cop1_simple[n])
#define REGFPU2(n,m) REGFPU(n);\
		if((n)!=(m)){C;REGFPU(m);}
#define C *(p++) = ','

	if (delay_slot)
	{
		*(p++) = '#';
	}
	switch (decode.format)
	{
	case INSTF_NONE:
		break;
	case INSTF_J:
	case INSTF_0BRANCH:
		break;
	case INSTF_LUI:
		break;
	case INSTF_1BRANCH:
	case INSTF_JR:
	case INSTF_ISIGN:
	case INSTF_IUNSIGN:
		REGCPU(o.i.rs);
		break;
	case INSTF_2BRANCH:
		REGCPU2(o.i.rs, o.i.rt);
		break;
	case INSTF_ADDRW:
		REGCPU(o.i.rt);
		if (o.i.rt != 0) { C; }
	case INSTF_ADDRR:
		*(p++) = '@';
		*(p++) = '=';
		HEX8(reg[o.i.rs] + (r4300halfsigned)o.i.immediate);
		break;
	case INSTF_LFW:
		REGFPU(o.lf.ft);
		C;
	case INSTF_LFR:
		*(p++) = '@';
		*(p++) = '=';
		HEX8(reg[o.lf.base] + (r4300halfsigned)o.lf.offset);
		break;
	case INSTF_R1:
		REGCPU(o.r.rd);
		break;
	case INSTF_R2:
		REGCPU2(o.i.rs, o.i.rt);
		break;
	case INSTF_R3:
		REGCPU2(o.i.rs, o.i.rt);
		break;
	case INSTF_MTC0:
	case INSTF_MTC1:
	case INSTF_SA:
		REGCPU(o.r.rt);
		break;
	case INSTF_R2F:
		REGFPU(o.cf.fs);
		break;
	case INSTF_R3F:
	case INSTF_C:
		REGFPU2(o.cf.fs, o.cf.ft);
		break;
	case INSTF_MFC0:
		break;
	case INSTF_MFC1:
		REGFPU(((FPUREG)o.r.rs));
		break;
	case INSTF_COUNT:
		break;
	}
	p1[strlen(p1)] = '\0';
#undef HEX8
#undef REGCPU
#undef REGFPU
#undef REGCPU2
#undef REGFPU2
#undef C
}

EmulationLock::EmulationLock()
{
	printf("Emulation Lock\n");
	pause_emu();
}

EmulationLock::~EmulationLock()
{
	resume_emu();
	printf("Emulation Unlock\n");
}

void close_all_scripts() {
	assert(IsGUIThread(false));

	// we mutate the map's nodes while iterating, so we have to make a copy
	for (auto copy = std::map(hwnd_lua_map); const auto fst : copy | std::views::keys) {
		SendMessage(fst, WM_CLOSE, 0, 0);
	}
	assert(hwnd_lua_map.empty());
}

void stop_all_scripts()
{
	assert(IsGUIThread(false));
	// we mutate the map's nodes while iterating, so we have to make a copy
	auto copy = std::map(hwnd_lua_map);
	for (const auto key : copy | std::views::keys) {
		SendMessage(key, WM_COMMAND,
					MAKEWPARAM(IDC_BUTTON_LUASTOP, BN_CLICKED),
					(LPARAM)GetDlgItem(key, IDC_BUTTON_LUASTOP));

	}
	assert(hwnd_lua_map.empty());
}

LRESULT CALLBACK d2d_overlay_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_PAINT:
		{
			auto lua = (LuaEnvironment*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

			PAINTSTRUCT ps;
			RECT rect;
			BeginPaint(hwnd, &ps);
			GetClientRect(hwnd, &rect);

			lua->d2d_dc->BeginDraw();
			lua->d3d_dc->CopyResource(lua->dxgi_surface_resource, lua->front_buffer);
			lua->d2d_dc->SetTransform(D2D1::Matrix3x2F::Identity());

			bool failed = lua->invoke_callbacks_with_key(LuaCallbacks::state_update_screen, REG_ATDRAWD2D);

			lua->d2d_dc->EndDraw();
			lua->dxgi_swapchain->Present(0, 0);
			lua->comp_device->Commit();

			if (failed)
			{
				LuaEnvironment::destroy(lua);
			}

			EndPaint(hwnd, &ps);
			return 0;
		}
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

LRESULT CALLBACK gdi_overlay_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_PAINT:
		{
			auto lua = (LuaEnvironment*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

			PAINTSTRUCT ps;
			RECT rect;
			HDC dc = BeginPaint(hwnd, &ps);
			GetClientRect(hwnd, &rect);

			bool failed = lua->invoke_callbacks_with_key(LuaCallbacks::state_update_screen, REG_ATUPDATESCREEN);

			BitBlt(dc, 0, 0, lua->dc_size.width, lua->dc_size.height, lua->gdi_back_dc, 0, 0, SRCCOPY);

			if (failed)
			{
				LuaEnvironment::destroy(lua);
			}

			EndPaint(hwnd, &ps);
			return 0;
		}
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}


void lua_init()
{

	Gdiplus::GdiplusStartupInput startup_input;
	GdiplusStartup(&gdi_plus_token, &startup_input, NULL);

	WNDCLASS wndclass = {0};
	wndclass.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = (WNDPROC)d2d_overlay_wndproc;
	wndclass.hInstance = GetModuleHandle(nullptr);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.lpszClassName = d2d_overlay_class;
	RegisterClass(&wndclass);

	wndclass.lpfnWndProc = (WNDPROC)gdi_overlay_wndproc;
	wndclass.lpszClassName = gdi_overlay_class;
	RegisterClass(&wndclass);
}


void LuaEnvironment::create_renderer()
{
	if (gdi_back_dc != nullptr)
	{
		return;
	}
	printf("Creating multi-target renderer for Lua...\n");

	RECT window_rect;
	GetClientRect(mainHWND, &window_rect);
	if (Statusbar::hwnd())
	{
		// We don't want to paint over statusbar
		RECT rc{};
		GetWindowRect(Statusbar::hwnd(), &rc);
		window_rect.bottom -= (WORD)(rc.bottom - rc.top);
	}
	dc_size = { (UINT32)abs(window_rect.right), (UINT32)abs(window_rect.bottom) };

	d2d_overlay_hwnd = CreateWindowEx(WS_EX_LAYERED, d2d_overlay_class, "", WS_CHILD | WS_VISIBLE, 0, 0, dc_size.width, dc_size.height, mainHWND, nullptr, GetModuleHandle(nullptr), nullptr);
	SetWindowLongPtr(d2d_overlay_hwnd, GWLP_USERDATA, (LONG_PTR)this);

	DWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(dw_factory),
		reinterpret_cast<IUnknown**>(&dw_factory)
	);

	if(!create_composition_surface(d2d_overlay_hwnd, dc_size, &factory, &dxgiadapter, &d3device, &dxdevice, &bitmap, &comp_visual, &comp_device, &comp_target, &dxgi_swapchain, &d2d_factory, &d2d_device, &d3d_dc, &d2d_dc, &dxgi_surface, &dxgi_surface_resource, &front_buffer))
	{
		printf("Failed to set up composition\n");
		return;
	}

	d2d_render_target_stack.push(d2d_dc);

	auto gdi_dc = GetDC(mainHWND);
	gdi_back_dc = CreateCompatibleDC(gdi_dc);
	gdi_bmp = CreateCompatibleBitmap(gdi_dc,dc_size.width, dc_size.height);
	SelectObject(gdi_back_dc, gdi_bmp);
	ReleaseDC(mainHWND, gdi_dc);

	gdi_overlay_hwnd = CreateWindowEx(WS_EX_LAYERED, gdi_overlay_class, "", WS_CHILD | WS_VISIBLE, 0, 0, dc_size.width, dc_size.height, mainHWND, nullptr, GetModuleHandle(nullptr), nullptr);
	SetWindowLongPtr(gdi_overlay_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	SetLayeredWindowAttributes(gdi_overlay_hwnd, bitmap_color_mask, 0, LWA_COLORKEY);

	// If we don't fill up the DC with the key first, it never becomes "transparent"
	FillRect(gdi_back_dc, &window_rect, alpha_mask_brush);
}

void LuaEnvironment::destroy_renderer()
{
	if (!d2d_dc)
	{
		return;
	}

	printf("Destroying Lua renderer...\n");

	for (auto const& [_, val] : dw_text_layouts) {
		val->Release();
	}
	dw_text_layouts.clear();

	while (!d2d_render_target_stack.empty()) {
		d2d_render_target_stack.pop();
	}

	for (auto x : image_pool) {
		delete x;
	}
	image_pool.clear();

	DestroyWindow(d2d_overlay_hwnd);
	front_buffer->Release();
	dxgi_surface_resource->Release();
	dxgi_surface->Release();
	d2d_dc->Release();
	d3d_dc->Release();
	comp_device->Release();
	comp_target->Release();
	dxgi_swapchain->Release();
	d2d_factory->Release();
	d2d_device->Release();
	comp_visual->Release();
	bitmap->Release();
	dxdevice->Release();
	d3device->Release();
	dxgiadapter->Release();
	factory->Release();

	DestroyWindow(gdi_overlay_hwnd);
	SelectObject(gdi_back_dc, nullptr);
	DeleteDC(gdi_back_dc);
	DeleteObject(gdi_bmp);
	gdi_back_dc = nullptr;
}

void LuaEnvironment::destroy(LuaEnvironment* lua_environment) {
	hwnd_lua_map.erase(lua_environment->hwnd);
	delete lua_environment;
}

std::pair<LuaEnvironment*, std::string> LuaEnvironment::create(std::filesystem::path path, HWND wnd)
{
	auto lua_environment = new LuaEnvironment();

	lua_environment->hwnd = wnd;
	lua_environment->path = path;

	lua_environment->brush = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
	lua_environment->pen = static_cast<HPEN>(GetStockObject(BLACK_PEN));
	lua_environment->font = static_cast<HFONT>(GetStockObject(SYSTEM_FONT));
	lua_environment->col = lua_environment->bkcol = 0;
	lua_environment->bkmode = TRANSPARENT;
	lua_environment->L = luaL_newstate();
	lua_atpanic(lua_environment->L, AtPanic);
	SetLuaClass(lua_environment->L, lua_environment);
	lua_environment->register_functions();
	lua_environment->create_renderer();

	bool error = luaL_dofile(lua_environment->L, lua_environment->path.string().c_str());

	std::string error_str;
	if (error)
	{
		error_str = lua_tostring(lua_environment->L, -1);
		delete lua_environment;
		lua_environment = nullptr;
	}

	return std::make_pair(lua_environment, error_str);
}

LuaEnvironment::~LuaEnvironment() {
	invoke_callbacks_with_key(LuaCallbacks::state_stop, REG_ATSTOP);
	SelectObject(gdi_back_dc, nullptr);
	DeleteObject(brush);
	DeleteObject(pen);
	DeleteObject(font);
	lua_close(L);
	L = NULL;
	SetButtonState(hwnd, false);
	this->destroy_renderer();
	printf("Lua destroyed\n");
}

//calls all functions that lua script has defined as callbacks, reads them from registry
//returns true at fail
bool LuaEnvironment::invoke_callbacks_with_key(std::function<int(lua_State*)> function,
							   const char* key) {
	assert(IsGUIThread(false));
	lua_getfield(L, LUA_REGISTRYINDEX, key);
	//shouldn't ever happen but could cause kernel panic
	if lua_isnil(L, -1) {
		lua_pop(L, 1);
		return false;
	}
	int n = luaL_len(L, -1);
	for (LUA_INTEGER i = 0; i < n; i++) {
		lua_pushinteger(L, 1 + i);
		lua_gettable(L, -2);
		if (function(L)) {\
			const char* str = lua_tostring(L, -1);
			ConsoleWrite(hwnd, str);
			ConsoleWrite(hwnd, "\r\n");
			printf("Lua error: %s\n", str);
			return true;
		}
	}
	lua_pop(L, 1);
	return false;
}

void LuaEnvironment::LoadScreenDelete()
{
	ReleaseDC(mainHWND, hwindowDC);
	DeleteDC(hsrcDC);

	LoadScreenInitialized = false;
}

void LuaEnvironment::invalidate_visuals()
{
	RECT rect;
 	GetClientRect(this->d2d_overlay_hwnd, &rect);

 	InvalidateRect(this->d2d_overlay_hwnd, &rect, false);
 	InvalidateRect(this->gdi_overlay_hwnd, &rect, false);
}

void LuaEnvironment::LoadScreenInit()
{
	if (LoadScreenInitialized) LoadScreenDelete();

	hwindowDC = GetDC(mainHWND);
	// Create a handle to the main window Device Context
	hsrcDC = CreateCompatibleDC(hwindowDC); // Create a DC to copy the screen to

	auto info = get_window_info();
	info.height -= 1; // ¯\_(ツ)_/¯
	printf("LoadScreen Size: %d x %d\n", info.width, info.height);

	// create an hbitmap
	hbitmap = CreateCompatibleBitmap(hwindowDC, info.width, info.height);

	LoadScreenInitialized = true;
}

void register_as_package(lua_State* lua_state, const char* name, const luaL_Reg regs[]) {
	if (name == nullptr)
	{
		const luaL_Reg* p = regs;
		do {
			lua_register(lua_state, p->name, p->func);
		} while ((++p)->func);
		return;
	}
	luaL_newlib(lua_state, regs);
	lua_setglobal(lua_state, name);
}

void LuaEnvironment::register_functions() {
	luaL_openlibs(L);

	register_as_package(L, nullptr, globalFuncs);
	register_as_package(L, "emu", emuFuncs);
	register_as_package(L, "memory", memoryFuncs);
	register_as_package(L, "wgui", wguiFuncs);
	register_as_package(L, "d2d", d2dFuncs);
	register_as_package(L, "input", inputFuncs);
	register_as_package(L, "joypad", joypadFuncs);
	register_as_package(L, "movie", movieFuncs);
	register_as_package(L, "savestate", savestateFuncs);
	register_as_package(L, "iohelper", ioHelperFuncs);
	register_as_package(L, "avi", aviFuncs);

	// COMPAT: table.getn deprecated, replaced by # prefix
	luaL_dostring(L, "table.getn = function(t) return #t end");

	// COMPAT: emu.debugview deprecated, forwarded to print
	luaL_dostring(L, "emu.debugview = print");

	// COMPAT: movie.playmovie deprecated, forwarded to movie.play
	luaL_dostring(L, "movie.playmovie = movie.play");

	// COMPAT: movie.stopmovie deprecated, forwarded to movie.stop
	luaL_dostring(L, "movie.stopmovie = movie.stop");

	// COMPAT: movie.getmoviefilename deprecated, forwarded to movie.get_filename
	luaL_dostring(L, "movie.getmoviefilename = movie.get_filename");

	// COMPAT: movie.isreadonly deprecated, forwarded to movie.get_readonly
	luaL_dostring(L, "movie.isreadonly = movie.get_readonly");

	// COMPAT: emu.isreadonly deprecated, forwarded to movie.get_readonly
	luaL_dostring(L, "emu.isreadonly = movie.get_readonly");

	// os.execute poses security risks
	luaL_dostring(L, "os.execute = function() print('os.execute is disabled') end");
}
