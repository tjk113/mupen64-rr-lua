#include "Seeker.h"

#include <Windows.h>
#include "../main_win.h"
#include "../../winproject/resource.h"

namespace Seeker
{
	LRESULT CALLBACK SeekerProc(HWND hwnd, UINT Message, WPARAM wParam,
	                            LPARAM lParam)
	{
		switch (Message)
		{
		case WM_CLOSE:
			EndDialog(hwnd, IDCANCEL);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDOK:
				EndDialog(hwnd, IDOK);
				break;
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
		          MAKEINTRESOURCE(IDD_SEEKER), mainHWND,
		          (DLGPROC)SeekerProc);
	}
}
