#include "Statusbar.hpp"

#include <Windows.h>
#include <commctrl.h>

#include "messenger.h"
#include "../r4300/r4300.h"
#include "RomBrowser.hpp"
#include "../../winproject/resource.h"
#include "helpers/win_helpers.h"
#include "win/Config.hpp"
#include "win/main_win.h"


namespace Statusbar
{
	const std::vector emu_parts = {210, 150, 80, 80, 80, -1};
	const std::vector idle_parts = {400, -1};

	HWND statusbar_hwnd;

	HWND hwnd()
	{
		return statusbar_hwnd;
	}

	void post(const std::string& text, Section section)
	{
		 SendMessage(statusbar_hwnd, SB_SETTEXT, (int)section, (LPARAM)text.c_str());
	}

	void emu_launched_changed(std::any data)
	{
		auto value = std::any_cast<bool>(data);

		if (!statusbar_hwnd) return;

		if (value)
		{
			set_statusbar_parts(statusbar_hwnd, emu_parts);
		} else
		{
			set_statusbar_parts(statusbar_hwnd, idle_parts);
		}
	}

	void create()
	{
		// undocumented behaviour of CCS_BOTTOM: it skips applying SBARS_SIZEGRIP in style pre-computation phase
		statusbar_hwnd = CreateWindowEx(0, STATUSCLASSNAME, nullptr,
		                                WS_CHILD | WS_VISIBLE | CCS_BOTTOM,
		                                0, 0,
		                                0, 0,
		                                mainHWND, (HMENU)IDC_MAIN_STATUS,
		                                app_instance, nullptr);
	}


	void statusbar_visibility_changed(std::any data)
	{
		auto value = std::any_cast<bool>(data);

		if (statusbar_hwnd)
		{
			DestroyWindow(statusbar_hwnd);
			statusbar_hwnd = nullptr;
		}

		if (value)
		{
			create();
		}
	}

	void init()
	{
		Messenger::subscribe(Messenger::Message::EmuLaunchedChanged,
		                     emu_launched_changed);
		Messenger::subscribe(Messenger::Message::StatusbarVisibilityChanged,
		                     statusbar_visibility_changed);
	}
}
