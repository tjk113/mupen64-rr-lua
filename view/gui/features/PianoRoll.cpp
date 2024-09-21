#include "PianoRoll.h"

#include <assert.h>
#include <thread>
#include <Windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "core/r4300/vcr.h"
#include "shared/Messenger.h"
#include "view/gui/Main.h"
#include "view/helpers/WinHelpers.h"
#include "winproject/resource.h"

namespace PianoRoll
{
    std::atomic<HWND> g_hwnd = nullptr;
    HWND g_lv_hwnd = nullptr;
    std::vector<BUTTONS> g_inputs{};


    // Whether a drag operation is happening
    bool g_lv_dragging = false;

    // The value of the cell under the mouse at the time when the drag operation started
    bool g_lv_drag_initial_value = false;

    // The column index of the drag operation. Dragging is not supported for non-button columns (n < 3).
    size_t g_lv_drag_column = 0;

    // Whether the drag operation should unset the affected buttons regardless of the initial value
    bool g_lv_drag_unset = false;

    void update_enabled_state()
    {
        if (!g_hwnd)
        {
            return;
        }

        const bool enabled = VCR::get_task() != e_task::idle;

        EnableWindow(g_lv_hwnd, enabled);
    }

    void refresh_full()
    {
        if (!g_hwnd)
        {
            return;
        }

        if (VCR::get_task() == e_task::idle)
        {
            ListView_DeleteAllItems(g_lv_hwnd);
            return;
        }

        if (VCR::get_task() == e_task::playback)
        {
            SetWindowRedraw(g_lv_hwnd, false);

            ListView_DeleteAllItems(g_lv_hwnd);

            g_inputs = VCR::get_inputs();
            ListView_SetItemCount(g_lv_hwnd, g_inputs.size());

            SetWindowRedraw(g_lv_hwnd, true);
        }
    }

    unsigned get_input_value_from_column_index(BUTTONS btn, size_t i)
    {
        switch (i)
        {
        case 3:
            return btn.R_DPAD;
        case 4:
            return btn.A_BUTTON;
        case 5:
            return btn.B_BUTTON;
        case 6:
            return btn.Z_TRIG;
        case 7:
            return btn.U_CBUTTON;
        case 8:
            return btn.L_CBUTTON;
        case 9:
            return btn.R_CBUTTON;
        case 10:
            return btn.D_CBUTTON;
        default:
            assert(false);
            break;
        }
    }

    void set_input_value_from_column_index(BUTTONS* btn, size_t i, bool value)
    {
        switch (i)
        {
        case 3:
            btn->R_DPAD = value;
            break;
        case 4:
            btn->A_BUTTON = value;
            break;
        case 5:
            btn->B_BUTTON = value;
            break;
        case 6:
            btn->Z_TRIG = value;
            break;
        case 7:
            btn->U_CBUTTON = value;
            break;
        case 8:
            btn->L_CBUTTON = value;
            break;
        case 9:
            btn->R_CBUTTON = value;
            break;
        case 10:
            btn->D_CBUTTON = value;
            break;
        default:
            assert(false);
            break;
        }
    }

    LRESULT CALLBACK ListViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId, DWORD_PTR dwRefData)
    {
        switch (msg)
        {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            {
                LVHITTESTINFO lplvhtti{};
                GetCursorPos(&lplvhtti.pt);
                ScreenToClient(hwnd, &lplvhtti.pt);
                ListView_SubItemHitTest(hwnd, &lplvhtti);

                if (lplvhtti.iSubItem <= 2)
                {
                    break;
                }

                auto input = g_inputs[lplvhtti.iItem];

                g_lv_dragging = true;
                g_lv_drag_column = lplvhtti.iSubItem;
                g_lv_drag_initial_value = !get_input_value_from_column_index(input, g_lv_drag_column);
                g_lv_drag_unset = GetKeyState(VK_RBUTTON) & 0x100;
                
                goto handle_mouse_move;
            }
        case WM_MOUSEMOVE:
            goto handle_mouse_move;
        case WM_NCDESTROY:
            RemoveWindowSubclass(hwnd, ListViewProc, sId);
            break;
        default:
            break;
        }

    def:
        return DefSubclassProc(hwnd, msg, wParam, lParam);

    handle_mouse_move:

        // Disable dragging if the corresponding mouse button was released. More reliable to do this here instead of MOUSE_XBUTTONDOWN.
        if (!g_lv_drag_unset && !(GetKeyState(VK_LBUTTON) & 0x100))
        {
            g_lv_dragging = false;
        }
        
        if (g_lv_drag_unset && !(GetKeyState(VK_RBUTTON) & 0x100))
        {
            g_lv_dragging = false;
        }

        if (!g_lv_dragging)
        {
            goto def;
        }

        LVHITTESTINFO lplvhtti{};
        GetCursorPos(&lplvhtti.pt);
        ScreenToClient(hwnd, &lplvhtti.pt);
        ListView_SubItemHitTest(hwnd, &lplvhtti);

        if (lplvhtti.iItem >= g_inputs.size())
        {
            printf("[PianoRoll] iItem out of range\n");
            goto def;
        }

        set_input_value_from_column_index(&g_inputs[lplvhtti.iItem], g_lv_drag_column, g_lv_drag_unset ? 0 : g_lv_drag_initial_value);

        ListView_Update(hwnd, lplvhtti.iItem);
    }

    LRESULT CALLBACK PianoRollProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
    {
        switch (Message)
        {
        case WM_INITDIALOG:
            {
                g_hwnd = hwnd;

                RECT grid_rect = get_window_rect_client_space(hwnd, GetDlgItem(hwnd, IDC_LIST_PIANO_ROLL));

                auto dwStyle = WS_TABSTOP
                    | WS_VISIBLE
                    | WS_CHILD
                    | LVS_NOSORTHEADER
                    | LVS_REPORT
                    | LVS_SHOWSELALWAYS
                    | LVS_ALIGNTOP
                    | LVS_OWNERDATA;

                g_lv_hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL,
                                           dwStyle,
                                           grid_rect.left, grid_rect.top,
                                           grid_rect.right - grid_rect.left,
                                           grid_rect.bottom - grid_rect.top,
                                           hwnd, (HMENU)IDC_PIANO_ROLL_LV,
                                           g_app_instance,
                                           NULL);
                SetWindowSubclass(g_lv_hwnd, ListViewProc, 0, 0);
                ListView_SetExtendedListViewStyle(g_lv_hwnd,
                                                  LVS_EX_GRIDLINES |
                                                  LVS_EX_FULLROWSELECT |
                                                  LVS_EX_DOUBLEBUFFER);

                HIMAGELIST image_list = ImageList_Create(16, 16, ILC_COLORDDB | ILC_MASK, 1, 0);
                ImageList_AddIcon(image_list, LoadIcon(g_app_instance, MAKEINTRESOURCE(IDI_CURRENT)));
                ListView_SetImageList(g_lv_hwnd, image_list, LVSIL_SMALL);

                LVCOLUMN lv_column{};
                lv_column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
                lv_column.fmt = LVCFMT_CENTER;

                // HACK: Insert and then delete dummy column to have all columns center-aligned
                // https://learn.microsoft.com/en-us/windows/win32/api/commctrl/ns-commctrl-lvcolumnw#remarks
                lv_column.cx = 1;
                lv_column.pszText = (LPTSTR)"";
                ListView_InsertColumn(g_lv_hwnd, 0, &lv_column);

                lv_column.cx = 65;

                lv_column.pszText = (LPTSTR)"Frame";
                ListView_InsertColumn(g_lv_hwnd, 1, &lv_column);

                lv_column.cx = 40;

                lv_column.pszText = (LPTSTR)"X";
                ListView_InsertColumn(g_lv_hwnd, 2, &lv_column);
                lv_column.pszText = (LPTSTR)"Y";
                ListView_InsertColumn(g_lv_hwnd, 3, &lv_column);

                lv_column.cx = 30;

                lv_column.pszText = (LPTSTR)"R";
                ListView_InsertColumn(g_lv_hwnd, 4, &lv_column);
                lv_column.pszText = (LPTSTR)"A";
                ListView_InsertColumn(g_lv_hwnd, 5, &lv_column);
                lv_column.pszText = (LPTSTR)"B";
                ListView_InsertColumn(g_lv_hwnd, 6, &lv_column);
                lv_column.pszText = (LPTSTR)"Z";
                ListView_InsertColumn(g_lv_hwnd, 7, &lv_column);
                lv_column.pszText = (LPTSTR)"C^";
                ListView_InsertColumn(g_lv_hwnd, 8, &lv_column);
                lv_column.pszText = (LPTSTR)"C<";
                ListView_InsertColumn(g_lv_hwnd, 9, &lv_column);
                lv_column.pszText = (LPTSTR)"C>";
                ListView_InsertColumn(g_lv_hwnd, 10, &lv_column);
                lv_column.pszText = (LPTSTR)"Cv";
                ListView_InsertColumn(g_lv_hwnd, 11, &lv_column);

                ListView_DeleteColumn(g_lv_hwnd, 0);

                refresh_full();
                update_enabled_state();

                break;
            }
        case WM_DESTROY:
            DestroyWindow(g_lv_hwnd);
            g_lv_hwnd = nullptr;
            g_hwnd = nullptr;
            break;
        case WM_CLOSE:
            EndDialog(hwnd, IDCANCEL);
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                EndDialog(hwnd, IDCANCEL);
                break;
            default: break;
            }
            break;
        case WM_NOTIFY:
            {
                if (wParam == IDC_PIANO_ROLL_LV)
                {
                    switch (((LPNMHDR)lParam)->code)
                    {
                    case LVN_GETDISPINFO:
                        {
                            const auto plvdi = (NMLVDISPINFO*)lParam;

                            if (plvdi->item.iItem == -1)
                            {
                                printf("[PianoRoll] LVN_GETDISPINFO with -1 iItem?\n");
                                break;
                            }

                            if (!(plvdi->item.mask & LVIF_TEXT))
                            {
                                break;
                            }

                            auto input = g_inputs[plvdi->item.iItem];

                            switch (plvdi->item.iSubItem)
                            {
                            case 0:
                                {
                                    auto current_sample = VCR::get_seek_completion().first;
                                    plvdi->item.iImage = current_sample == plvdi->item.iItem ? 0 : 999;
                                    strcpy(plvdi->item.pszText, std::to_string(plvdi->item.iItem).c_str());
                                    break;
                                }
                            case 1:
                                strcpy(plvdi->item.pszText, std::to_string(input.Y_AXIS).c_str());
                                break;
                            case 2:
                                strcpy(plvdi->item.pszText, std::to_string(input.X_AXIS).c_str());
                                break;
                            case 3:
                                strcpy(plvdi->item.pszText, input.R_DPAD ? "R" : "");
                                break;
                            case 4:
                                strcpy(plvdi->item.pszText, input.A_BUTTON ? "A" : "");
                                break;
                            case 5:
                                strcpy(plvdi->item.pszText, input.B_BUTTON ? "B" : "");
                                break;
                            case 6:
                                strcpy(plvdi->item.pszText, input.Z_TRIG ? "Z" : "");
                                break;
                            case 7:
                                strcpy(plvdi->item.pszText, input.U_CBUTTON ? "C^" : "");
                                break;
                            case 8:
                                strcpy(plvdi->item.pszText, input.L_CBUTTON ? "C<" : "");
                                break;
                            case 9:
                                strcpy(plvdi->item.pszText, input.R_CBUTTON ? "C>" : "");
                                break;
                            case 10:
                                strcpy(plvdi->item.pszText, input.D_CBUTTON ? "Cv" : "");
                                break;
                            default:
                                break;
                            }
                        }
                        break;
                    }
                }
                break;
            }
        default:
            break;
        }
        return FALSE;
    }

    void init()
    {
        Messenger::subscribe(Messenger::Message::TaskChanged, [](std::any data)
        {
            auto value = std::any_cast<e_task>(data);
            static auto previous_value = value;

            printf("[PianoRoll] TaskChanged from %d to %d\n", previous_value, value);

            if (value != previous_value)
            {
                refresh_full();
                update_enabled_state();
            }

            previous_value = value;
        });

        Messenger::subscribe(Messenger::Message::CurrentSampleChanged, [](std::any data)
        {
            auto value = std::any_cast<long>(data);
            static auto previous_value = value;

            printf("Sample: %d, Buffer size: %d\n", value, VCR::get_inputs().size());

            if (VCR::get_task() == e_task::recording)
            {
                g_inputs = VCR::get_inputs();
                ListView_SetItemCount(g_lv_hwnd, g_inputs.size());
            }

            ListView_Update(g_lv_hwnd, previous_value);
            ListView_Update(g_lv_hwnd, value);

            if (VCR::get_task() == e_task::recording)
            {
                ListView_EnsureVisible(g_lv_hwnd, value - 1, false);
            }
            else
            {
                ListView_EnsureVisible(g_lv_hwnd, value, false);
            }

            previous_value = value;
        });

        Messenger::subscribe(Messenger::Message::UnfreezeCompleted, [](std::any)
        {
            if (g_config.vcr_readonly)
            {
                return;
            }

            SetWindowRedraw(g_lv_hwnd, false);

            ListView_DeleteAllItems(g_lv_hwnd);

            g_inputs = VCR::get_inputs();
            ListView_SetItemCount(g_lv_hwnd, g_inputs.size());

            ListView_EnsureVisible(g_lv_hwnd, VCR::get_seek_completion().first, false);

            SetWindowRedraw(g_lv_hwnd, true);
        });
    }

    void show()
    {
        if (g_hwnd)
        {
            BringWindowToTop(g_hwnd);
            return;
        }

        std::thread([]
        {
            DialogBox(g_app_instance, MAKEINTRESOURCE(IDD_PIANO_ROLL), 0, (DLGPROC)PianoRollProc);
        }).detach();
    }
}
