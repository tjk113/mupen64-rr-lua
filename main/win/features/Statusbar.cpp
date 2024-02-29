#include "Statusbar.hpp"

#include <Windows.h>
#include <commctrl.h>

#include "messenger.h"
#include "../r4300/r4300.h"
#include "RomBrowser.hpp"
#include "vcr.h"
#include "../../winproject/resource.h"
#include "helpers/win_helpers.h"
#include "win/Config.hpp"
#include "win/main_win.h"


namespace Statusbar
{
	const std::vector emu_parts = {220, 120, 90, 80, 80, 90, -1};
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

		auto parts = value ? emu_parts : idle_parts;

		// We don't want to keep the weird crap from previous state, so let's clear everything
		for (int i = 0; i < 255; ++i)
		{
			SendMessage(statusbar_hwnd, SB_SETTEXT, i, (LPARAM)"");
		}

		set_statusbar_parts(statusbar_hwnd, parts);
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

	void on_rerecords_changed(std::any data)
	{
		auto value = std::any_cast<uint64_t>(data);
		post(std::format("{} rr", value), Section::Rerecords);
	}

	void on_task_changed(std::any data)
	{
		auto value = std::any_cast<e_task>(data);

		if (value == e_task::idle)
		{
			post("", Section::Rerecords);
		}
	}

	void on_slot_changed(std::any data)
	{
		auto value = std::any_cast<size_t>(data);
		post(std::format("Slot {}", value + 1), Section::Slot);
	}

	void init()
	{
		Messenger::subscribe(Messenger::Message::EmuLaunchedChanged,
		                     emu_launched_changed);
		Messenger::subscribe(Messenger::Message::StatusbarVisibilityChanged,
		                     statusbar_visibility_changed);
		Messenger::subscribe(Messenger::Message::TaskChanged,
							 on_task_changed);
		Messenger::subscribe(Messenger::Message::RerecordsChanged,
							 on_rerecords_changed);
		Messenger::subscribe(Messenger::Message::SlotChanged,
							 on_slot_changed);

	}
}
