/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "Cheats.h"

#include "FrontendService.h"

#include <Windows.h>
#include <Windowsx.h>
#include <gui/Main.h>
#include <resource.h>
#include <core_api.h>

namespace Cheats
{
    // LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam,
    //                          LPARAM lParam)
    // {
    //     switch (Message)
    //     {
    //     case WM_INITDIALOG:
    //         {
    //             goto rebuild_list;
    //         }
    //     case WM_DESTROY:
    //
    //         break;
    //     case WM_CLOSE:
    //         EndDialog(hwnd, IDCANCEL);
    //         break;
    //     case WM_COMMAND:
    //         switch (LOWORD(wParam))
    //         {
    //         case IDC_LIST_CHEATS:
    //             goto update_selection;
    //         case IDC_NEW_CHEAT:
    //             {
    //                 auto script = GSScript::compile(L"D033AFA1 0020\n8133B1BC 4220\nD033AFA1 0020\n8133B17C 0300\nD033AFA1 0020\n8133B17E 0880");
    //                 if (script.has_value())
    //                 {
    //                     g_core.gs_scripts->push_back(script.value());
    //                     goto rebuild_list;
    //                 }
    //                 break;
    //             }
    //         case IDC_REMOVE_CHEAT:
    //             {
    //                 auto lb_hwnd = GetDlgItem(hwnd, IDC_LIST_CHEATS);
    //                 auto selected_index = ListBox_GetCurSel(lb_hwnd);
    //                 if (selected_index == -1)
    //                 {
    //                     break;
    //                 }
    //
    //                 g_core.gs_scripts->erase(g_core.gs_scripts->begin() + selected_index);
    //                 goto rebuild_list;
    //                 break;
    //             }
    //         case IDC_CHECK_CHEAT_ENABLED:
    //             {
    //                 auto lb_hwnd = GetDlgItem(hwnd, IDC_LIST_CHEATS);
    //                 auto selected_index = ListBox_GetCurSel(lb_hwnd);
    //                 if (selected_index == -1)
    //                 {
    //                     break;
    //                 }
    //                 auto script = g_core.gs_scripts->at(selected_index);
    //                 script->set_resumed(IsDlgButtonChecked(hwnd, IDC_CHECK_CHEAT_ENABLED) == BST_CHECKED);
    //                 goto rebuild_list;
    //             }
    //
    //         case IDC_CHEAT_APPLY:
    //             {
    //                 auto lb_hwnd = GetDlgItem(hwnd, IDC_LIST_CHEATS);
    //                 auto selected_index = ListBox_GetCurSel(lb_hwnd);
    //                 if (selected_index == -1)
    //                 {
    //                     break;
    //                 }
    //
    //                 const bool prev_resumed = g_core.gs_scripts->at(selected_index)->resumed();
    //
    //                 wchar_t code[4096] = {};
    //                 Edit_GetText(GetDlgItem(hwnd, IDC_EDIT_CHEAT), code, sizeof(code));
    //
    //                 wchar_t name[256] = {};
    //                 Edit_GetText(GetDlgItem(hwnd, IDC_EDIT_CHEAT_NAME), name, sizeof(name));
    //
    //                 // Replace with script recompiled with new code, while keeping some properties
    //                 // Old script goes out of ref
    //                 auto script = GSScript::compile(code);
    //
    //                 if (!script.has_value())
    //                 {
    //                     FrontendService::show_dialog(L"Cheat code could not be compiled.\r\nVerify that the syntax is correct", L"Cheats", fsvc_error);
    //                     break;
    //                 }
    //
    //                 script.value()->set_name(name);
    //                 script.value()->set_resumed(prev_resumed);
    //                 g_core.gs_scripts->assign(selected_index, script.value());
    //                 goto rebuild_list;
    //             }
    //         default:
    //             break;
    //         }
    //         break;
    //
    //     default:
    //         break;
    //     }
    //     return FALSE;
    //
    // update_selection:
    //     {
    //         auto selected_index = ListBox_GetCurSel(GetDlgItem(hwnd, IDC_LIST_CHEATS));
    //         if (selected_index == -1)
    //         {
    //             return FALSE;
    //         }
    //
    //         CheckDlgButton(hwnd, IDC_CHECK_CHEAT_ENABLED,
    //                        g_core.gs_scripts->at(selected_index)->resumed() ? BST_CHECKED : BST_UNCHECKED);
    //         SetDlgItemText(hwnd, IDC_EDIT_CHEAT, g_core.gs_scripts->at(selected_index)->code().c_str());
    //         Edit_SetText(GetDlgItem(hwnd, IDC_EDIT_CHEAT_NAME), g_core.gs_scripts->at(selected_index)->name().c_str());
    //     }
    //     return FALSE;
    //
    //
    // rebuild_list:
    //     {
    //         auto lb_hwnd = GetDlgItem(hwnd, IDC_LIST_CHEATS);
    //         auto prev_index = ListBox_GetCurSel(lb_hwnd);
    //         ListBox_ResetContent(lb_hwnd);
    //         for (const auto script : *g_core.gs_scripts)
    //         {
    //             auto name = !script->resumed() ? script->name() + L" (Disabled)" : script->name();
    //             ListBox_AddString(lb_hwnd, name.c_str());
    //         }
    //         ListBox_SetCurSel(lb_hwnd, prev_index);
    //         goto update_selection;
    //     }
    // }

    void show()
    {
        // DialogBox(g_app_instance,
        //           MAKEINTRESOURCE(IDD_CHEATS), g_main_hwnd,
        //           (DLGPROC)WndProc);
    }
} // namespace Cheats
