#include "Toolbar.hpp"

#include <Windows.h>
#include <commctrl.h>

#include "messenger.h"
#include "RomBrowser.hpp"
#include "vcr.h"
#include "../r4300/r4300.h"
#include "../Config.hpp"
#include "../main_win.h"
#include "../../winproject/resource.h"

HWND toolbar_hwnd;

namespace Toolbar
{
	void toolbar_create()
	{
		const TBBUTTON tb_buttons[] =
		{
			{
				0, IDM_LOAD_ROM, TBSTATE_ENABLED,
				TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE,
				{0, 0}, 0, 0
			},
			{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0, 0}, 0, 0},
			{
				1, EMU_PLAY, TBSTATE_ENABLED, TBSTYLE_CHECK | TBSTYLE_AUTOSIZE,
				{0, 0}, 0, 0
			},
			{
				2, IDM_PAUSE, TBSTATE_ENABLED, TBSTYLE_CHECK | TBSTYLE_AUTOSIZE,
				{0, 0}, 0, 0
			},
			{
				3, IDM_CLOSE_ROM, TBSTATE_ENABLED,
				TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE,
				{0, 0}, 0, 0
			},
			{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0, 0}, 0, 0},
			{
				4, IDM_FULLSCREEN, TBSTATE_ENABLED,
				TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE,
				{0, 0}, 0, 0
			},
			{
				9, IDM_SETTINGS, TBSTATE_ENABLED,
				TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0
			},
		};

		constexpr auto tb_buttons_count = sizeof(tb_buttons) / sizeof(TBBUTTON);
		toolbar_hwnd = CreateToolbarEx(mainHWND,
		                               WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS,
		                               IDC_TOOLBAR, 10, app_instance,
		                               IDB_TOOLBAR,
		                               tb_buttons,
		                               tb_buttons_count, 16, 16,
		                               static_cast<int>(tb_buttons_count) * 16,
		                               16,
		                               sizeof(TBBUTTON));

		if (toolbar_hwnd == nullptr)
			MessageBox(mainHWND, "Could not create tool bar.", "Error",
			           MB_OK | MB_ICONERROR);

		if (emu_launched)
		{
			if (emu_paused)
			{
				SendMessage(toolbar_hwnd, TB_CHECKBUTTON, IDM_PAUSE, 1);
			} else
			{
				SendMessage(toolbar_hwnd, TB_CHECKBUTTON, EMU_PLAY, 1);
			}
		} else
		{
			SendMessage(toolbar_hwnd, TB_ENABLEBUTTON, IDM_CLOSE_ROM, FALSE);
			SendMessage(toolbar_hwnd, TB_ENABLEBUTTON, IDM_PAUSE, FALSE);
			SendMessage(toolbar_hwnd, TB_ENABLEBUTTON, IDM_FULLSCREEN, FALSE);
		}
	}

	void toolbar_set_visibility(int32_t is_visible)
	{
		if (toolbar_hwnd)
		{
			DestroyWindow(toolbar_hwnd);
			toolbar_hwnd = nullptr;
		}

		if (is_visible)
		{
			toolbar_create();
		}

		rombrowser_update_size();
	}

	void toolbar_on_emu_paused_changed(bool value)
	{
		SendMessage(toolbar_hwnd, TB_CHECKBUTTON, IDM_PAUSE, value);
		SendMessage(toolbar_hwnd, TB_CHECKBUTTON, EMU_PLAY, !value);
	}

	void emu_launched_changed(std::any data)
	{
		auto value = std::any_cast<bool>(data);
		if (!toolbar_hwnd) return;

		SendMessage(toolbar_hwnd, TB_ENABLEBUTTON, IDM_FULLSCREEN, value);
		SendMessage(toolbar_hwnd, TB_ENABLEBUTTON, EMU_PLAY, value);
		SendMessage(toolbar_hwnd, TB_ENABLEBUTTON, IDM_CLOSE_ROM, value);
		SendMessage(toolbar_hwnd, TB_ENABLEBUTTON, IDM_PAUSE, value);
		SendMessage(toolbar_hwnd, TB_ENABLEBUTTON, IDM_PAUSE, value);
	}

	void init()
	{
		Messenger::subscribe(Messenger::Message::EmuLaunchedChanged,
		                     emu_launched_changed);
	}
};
