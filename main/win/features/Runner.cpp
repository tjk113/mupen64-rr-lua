#include "Runner.h"

#include <thread>
#include <Windows.h>
#include <Windowsx.h>

#include "../../winproject/resource.h"
#include <win/main_win.h>

#include "LuaConsole.h"
#include "messenger.h"
#include "vcr.h"
#include "win/Config.hpp"
#include "../../r4300/r4300.h"

namespace Runner
{
	void run_auto(int id, std::filesystem::path path)
	{
		switch (id)
		{
		case IDC_LIST_ROMS:
			std::thread([path]
			{
				vr_start_rom(path);
			}).detach();
			break;
		case IDC_LIST_MOVIES:
			Config.vcr_readonly = true;
			Messenger::broadcast(
				Messenger::Message::ReadonlyChanged,
				(bool)Config.vcr_readonly);
			std::thread([=]
			{
				VCR::start_playback(path);
			}).detach();
			break;
		case IDC_LIST_SCRIPTS:
			lua_create_and_run(path.string().c_str());
			break;
		}
	}

	LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam,
	                         LPARAM lParam)
	{
		switch (Message)
		{
		case WM_INITDIALOG:
			{
				auto populate_with_paths = [&](const int id, std::vector<std::string> paths)
				{
					auto ctl = GetDlgItem(hwnd, id);
					for (auto path : paths)
					{
						auto index = ListBox_AddString(ctl, std::filesystem::path(path).string().c_str());

						char* buffer = new char[path.size() + 1]();
						memcpy(buffer, path.data(), path.size());

						ListBox_SetItemData(ctl, index, reinterpret_cast<LPARAM>(buffer));
					}
				};
				populate_with_paths(IDC_LIST_ROMS, Config.recent_rom_paths);
				populate_with_paths(IDC_LIST_MOVIES, Config.recent_movie_paths);
				populate_with_paths(IDC_LIST_SCRIPTS, Config.recent_lua_script_paths);
				break;
			}
		case WM_DESTROY:

			break;
		case WM_CLOSE:
			EndDialog(hwnd, IDCANCEL);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDC_LIST_ROMS:
			case IDC_LIST_MOVIES:
			case IDC_LIST_SCRIPTS:
				if (HIWORD(wParam) == LBN_DBLCLK)
				{
					auto index = ListBox_GetCurSel(GetDlgItem(hwnd, LOWORD(wParam)));
					auto buffer = (char*)ListBox_GetItemData(GetDlgItem(hwnd, LOWORD(wParam)), index);
					std::filesystem::path path(buffer);
					delete buffer;

					EndDialog(hwnd, IDOK);

					run_auto(LOWORD(wParam), path);
				}
				break;
			case IDOK:
				{
					auto ctl_hwnd = GetFocus();
					if (!ctl_hwnd)
					{
						break;
					}
					char name[260]{};
					GetClassName(ctl_hwnd, name, sizeof(name));
					if (lstrcmpi(name, "ListBox"))
					{
						break;
					}

					SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(ctl_hwnd), LBN_DBLCLK), 0);

					break;
				}
			case IDCANCEL:
				EndDialog(hwnd, IDCANCEL);
				break;
			default: break;
			}
			break;
		default: break;
		}
		return FALSE;
	}

	void show()
	{
		DialogBox(GetModuleHandle(NULL),
		          MAKEINTRESOURCE(IDD_RUNNER), mainHWND,
		          (DLGPROC)WndProc);
	}
}
