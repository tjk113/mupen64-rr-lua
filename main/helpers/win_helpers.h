#pragma once
#include <Windows.h>
#include <CommCtrl.h>
#include <cstdint>

static void set_checkbox_state(const HWND hwnd, const int32_t id,
                               int32_t is_checked)
{
	SendMessage(GetDlgItem(hwnd, id), BM_SETCHECK,
	            is_checked ? BST_CHECKED : BST_UNCHECKED, 0);
}

static int32_t get_checkbox_state(const HWND hwnd, const int32_t id)
{
	return SendDlgItemMessage(hwnd, id, BM_GETCHECK, 0, 0) == BST_CHECKED
		       ? TRUE
		       : FALSE;
}

static int32_t get_primary_monitor_refresh_rate() {
	DISPLAY_DEVICE dd;
	dd.cb = sizeof(dd);
	EnumDisplayDevices(NULL, 0, &dd, 0);

	DEVMODE dm;
	dm.dmSize = sizeof(dm);
	EnumDisplaySettings(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm);

	return dm.dmDisplayFrequency;
}
