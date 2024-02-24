#include "Seeker.h"

#include <thread>
#include <Windows.h>

#include "messenger.h"
#include "Statusbar.hpp"
#include "vcr.h"
#include "../main_win.h"
#include "../../winproject/resource.h"

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
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDOK:
				{
					BOOL success = false;
					int frame = GetDlgItemInt(hwnd, IDC_SEEKER_FRAME, &success,
											  false);
					if (!success)
					{
						break;
					}

					if (VCR::begin_seek_to(frame) != VCR::Result::Ok)
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
			DialogBox(GetModuleHandle(NULL),
				  MAKEINTRESOURCE(IDD_SEEKER), mainHWND,
				  (DLGPROC)SeekerProc);
			current_hwnd = nullptr;
		}).detach();
	}

	void init()
	{
		Messenger::subscribe(Messenger::Message::SeekCompleted, [] (std::any)
		{
			if (!current_hwnd)
			 return;
			SendMessage(current_hwnd, WM_SEEK_COMPLETED, 0, 0);
		});
	}
}
