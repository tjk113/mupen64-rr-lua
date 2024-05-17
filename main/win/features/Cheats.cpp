#include "Cheats.h"

#include <Windows.h>
#include <Windowsx.h>
#include <main/win/main_win.h>
#include <winproject/resource.h>

#include "r4300/gameshark.h"

namespace Cheats
{
	LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam,
	                         LPARAM lParam)
	{
		switch (Message)
		{
		case WM_INITDIALOG:
			{
				goto rebuild_list;
			}
		case WM_DESTROY:

			break;
		case WM_CLOSE:
			EndDialog(hwnd, IDCANCEL);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDC_LIST_CHEATS:
				goto update_selection;
			case IDC_NEW_CHEAT:
				{
					auto script = Gameshark::Script::compile("D033AFA1 0020\n8133B1BC 4220\nD033AFA1 0020\n8133B17C 0300\nD033AFA1 0020\n8133B17E 0880");
					if (script.has_value())
					{
						Gameshark::scripts.push_back(script.value());
						goto rebuild_list;
					}
					break;
				}
			case IDC_CHECK_CHEAT_ENABLED:
				{
					auto lb_hwnd = GetDlgItem(hwnd, IDC_LIST_CHEATS);
					auto selected_index = ListBox_GetCurSel(lb_hwnd);
					if (selected_index == -1)
					{
						break;
					}
					auto script = Gameshark::scripts[selected_index];
					script->set_resumed(IsDlgButtonChecked(hwnd, IDC_CHECK_CHEAT_ENABLED) == BST_CHECKED);
					break;
				}
			default: break;
			}
			break;

		default: break;
		}
		return FALSE;

	update_selection:
		{
			auto selected_index = ListBox_GetCurSel(GetDlgItem(hwnd, IDC_LIST_CHEATS));
			if (selected_index == -1)
			{
				return FALSE;
			}

			CheckDlgButton(hwnd, IDC_CHECK_CHEAT_ENABLED,
			               Gameshark::scripts[selected_index]->resumed() ? BST_CHECKED : BST_UNCHECKED);
			SetDlgItemText(hwnd, IDC_EDIT_CHEAT, Gameshark::scripts[selected_index]->code().c_str());
		}
		return FALSE;


	rebuild_list:
		{
			auto lb_hwnd = GetDlgItem(hwnd, IDC_LIST_CHEATS);
			ListBox_ResetContent(lb_hwnd);
			for (auto script : Gameshark::scripts)
			{
				ListBox_AddString(lb_hwnd, script->name().c_str());
			}
			goto update_selection;
		}
	}

	void show()
	{
		DialogBox(app_instance,
		          MAKEINTRESOURCE(IDD_CHEATS), mainHWND,
		          (DLGPROC)WndProc);
	}
}
