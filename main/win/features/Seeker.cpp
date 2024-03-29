#include "Seeker.h"

#include <thread>
#include <Windows.h>

#include <shared/messenger.h>
#include "Statusbar.hpp"
#include "../../r4300/vcr.h"
#include "../main_win.h"
#include "../../winproject/resource.h"
#include <shared/Config.hpp>

#define WM_SEEK_COMPLETED (WM_USER + 11)
#define WM_UPDATE_SEEK_ALLOWED (WM_USER + 12)

namespace Seeker
{
	HWND current_hwnd;

	LRESULT CALLBACK SeekerProc(HWND hwnd, UINT Message, WPARAM wParam,
	                            LPARAM lParam)
	{
		switch (Message)
		{
		case WM_INITDIALOG:
			current_hwnd = hwnd;
			SetDlgItemText(hwnd, IDC_SEEKER_FRAME, Config.seeker_value.c_str());
			SetFocus(GetDlgItem(hwnd, IDC_SEEKER_FRAME));
			break;
		case WM_DESTROY:
			VCR::stop_seek();
			break;
		case WM_CLOSE:
			EndDialog(hwnd, IDCANCEL);
			break;
		case WM_SEEK_COMPLETED:
			EnableWindow(GetDlgItem(hwnd, IDOK), true);
			SendMessage(hwnd, WM_UPDATE_SEEK_ALLOWED, 0, 0);
			break;
		case WM_UPDATE_SEEK_ALLOWED:
			try
			{
				bool relative = Config.seeker_value[0] == '-' || Config.seeker_value[0] == '+';
				int32_t frame = std::stoi(Config.seeker_value);
				EnableWindow(GetDlgItem(hwnd, IDOK), Config.seeker_value.size() > 1 && VCR::can_seek_to(frame, relative));
			} catch (...)
			{
				EnableWindow(GetDlgItem(hwnd, IDOK), false);
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDC_SEEKER_FRAME:
				{
					char str[260] = {0};
					GetDlgItemText(hwnd, IDC_SEEKER_FRAME, str, std::size(str));
					Config.seeker_value = str;
					SendMessage(hwnd, WM_UPDATE_SEEK_ALLOWED, 0, 0);
				}
				break;
			case IDOK:
				{
					// Relative seek is activated by typing + or - in front of number
					bool relative = Config.seeker_value[0] == '-' || Config.seeker_value[0] == '+';

					int32_t frame;
					try
					{
						frame = std::stoi(Config.seeker_value);
					} catch (...)
					{
						break;
					}

					if (VCR::begin_seek_to(frame, relative) != VCR::Result::Ok)
					{
						MessageBox(
							hwnd,
							"Failed to begin seeking.\r\nVerify the inputs and try again.",
							nullptr, MB_ICONERROR);
						break;
					}

					EnableWindow(GetDlgItem(hwnd, IDOK), false);
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
		// Let's not have 2 instances of a singleton...
		if (current_hwnd)
		{
			return;
		}

		// We need to run the dialog on another thread, since we don't want to block the main one
		std::thread([]
		{
			DialogBox(app_instance,
			          MAKEINTRESOURCE(IDD_SEEKER), mainHWND,
			          (DLGPROC)SeekerProc);
			current_hwnd = nullptr;
		}).detach();
	}

	void init()
	{
		Messenger::subscribe(Messenger::Message::SeekCompleted, [](std::any)
		{
			if (!current_hwnd)
				return;
			SendMessage(current_hwnd, WM_SEEK_COMPLETED, 0, 0);
		});
	}
}
