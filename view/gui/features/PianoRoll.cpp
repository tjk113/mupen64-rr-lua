#include "PianoRoll.h"

#include <cassert>
#include <thread>
#include <Windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "core/r4300/vcr.h"
#include "shared/AsyncExecutor.h"
#include "shared/Messenger.h"
#include "shared/helpers/StlExtensions.h"
#include "shared/services/FrontendService.h"
#include "view/gui/Main.h"
#include "view/helpers/IOHelpers.h"
#include "view/helpers/WinHelpers.h"
#include "winproject/resource.h"

import std;

namespace PianoRoll
{
    const auto JOYSTICK_CLASS = "PianoRollJoystick";

    std::atomic<HWND> g_hwnd = nullptr;
    HWND g_lv_hwnd = nullptr;
    HWND g_joy_hwnd = nullptr;
    HWND g_hist_hwnd = nullptr;

    // Represents the current state of the piano roll.
    struct PianoRollState
    {
        // The input buffer for the piano roll, which is a copy of the inputs from the core and is modified by the user. When editing operations end, this buffer
        // is provided to begin_warp_modify and thereby applied to the core, changing the resulting emulator state.
        std::vector<BUTTONS> inputs;

        // Selected indicies in the piano roll listview.
        std::vector<size_t> selected_indicies;
    };

    // The clipboard buffer for piano roll copy/paste operations. Must be sorted ascendingly.
    //
    // Due to allowing sparse ("extended") selections, we might have gaps in the clipboard buffer as well as the selected indicies when pasting.
    // When a gap-containing clipboard buffer is pasted into the piano roll, the selection acts as a mask.
    //
    // A----
    // -----
    // @@@@@ << Gap!
    // -B---
    // A----
    //
    // results in...
    //
    // $$$$$ << Unaffected, outside of selection!
    // ----- [[[ Selection start
    // $$$$$ << Unaffected
    // -B---
    // $$$$$ <<< Unaffected ]]] Selection end
    //
    // This also applies for the inverse (gapless clipboard buffer and g_piano_roll_state.selected_indicies with gaps).
    //
    std::vector<std::optional<BUTTONS>> g_clipboard{};

    // Whether a drag operation is happening
    bool g_lv_dragging = false;

    // The value of the cell under the mouse at the time when the drag operation started
    bool g_lv_drag_initial_value = false;

    // The column index of the drag operation. Dragging is not supported for non-button columns (n < 3).
    size_t g_lv_drag_column = 0;

    // Whether the drag operation should unset the affected buttons regardless of the initial value
    bool g_lv_drag_unset = false;

    // Whether a joystick drag operation is happening
    bool g_joy_drag = false;

    // The current piano roll state.
    PianoRollState g_piano_roll_state;

    // Undo/redo stack for the piano roll.
    std::deque<PianoRollState> g_piano_roll_states;

    // Stack index for the piano roll undo/redo stack. 0 = top, 1 = 2nd from top, etc...
    size_t g_piano_roll_state_index;

    // Copy of seek savestate frame map from VCR.
    std::unordered_map<size_t, bool> g_seek_savestate_frames;

    /**
     * Gets whether inputs can be modified. Affects both the piano roll and the joystick.
     */
    bool can_modify_inputs()
    {
        return VCR::get_warp_modify_status() == e_warp_modify_status::none
            && VCR::get_seek_completion().second == SIZE_MAX
            && VCR::get_task() == e_task::recording
            && !g_config.vcr_readonly;
    }

    /**
     * Refreshes the piano roll listview and the joystick, re-querying the current inputs from the core.
     */
    void update_inputs()
    {
        if (!g_hwnd)
        {
            return;
        }

        // If VCR is idle, we can't really show anything.
        if (VCR::get_task() == e_task::idle)
        {
            ListView_DeleteAllItems(g_lv_hwnd);
        }

        // In playback mode, the input buffer can't change so we're safe to only pull it once.
        if (VCR::get_task() == e_task::playback)
        {
            SetWindowRedraw(g_lv_hwnd, false);

            ListView_DeleteAllItems(g_lv_hwnd);

            g_piano_roll_state.inputs = VCR::get_inputs();
            ListView_SetItemCount(g_lv_hwnd, g_piano_roll_state.inputs.size());
            std::println("[PianoRoll] Pulled inputs from core for playback mode, count: {}", g_piano_roll_state.inputs.size());

            SetWindowRedraw(g_lv_hwnd, true);
        }

        RedrawWindow(g_joy_hwnd, nullptr, nullptr, RDW_INVALIDATE);
    }

    /**
     * Gets a button value from a BUTTONS struct at a given column index.
     * \param btn The BUTTONS struct to get the value from
     * \param i The column index. Must be in the range [3, 15] inclusive.
     * \return The button value at the given column index
     */
    unsigned get_input_value_from_column_index(BUTTONS btn, size_t i)
    {
        switch (i)
        {
        case 3:
            return btn.A_BUTTON;
        case 4:
            return btn.B_BUTTON;
        case 5:
            return btn.Z_TRIG;
        case 6:
            return btn.R_TRIG;
        case 7:
            return btn.START_BUTTON;
        case 8:
            return btn.U_CBUTTON;
        case 9:
            return btn.L_CBUTTON;
        case 10:
            return btn.R_CBUTTON;
        case 11:
            return btn.D_CBUTTON;
        case 12:
            return btn.U_DPAD;
        case 13:
            return btn.L_DPAD;
        case 14:
            return btn.R_DPAD;
        case 15:
            return btn.D_DPAD;
        default:
            assert(false);
            return -1;
        }
    }

    /**
     * Sets a button value in a BUTTONS struct at a given column index.
     * \param btn The BUTTONS struct to set the value in
     * \param i The column index. Must be in the range [3, 15] inclusive.
     * \param value The button value to set
     */
    void set_input_value_from_column_index(BUTTONS* btn, size_t i, bool value)
    {
        switch (i)
        {
        case 3:
            btn->A_BUTTON = value;
            break;
        case 4:
            btn->B_BUTTON = value;
            break;
        case 5:
            btn->Z_TRIG = value;
            break;
        case 6:
            btn->R_TRIG = value;
            break;
        case 7:
            btn->START_BUTTON = value;
            break;
        case 8:
            btn->U_CBUTTON = value;
            break;
        case 9:
            btn->L_CBUTTON = value;
            break;
        case 10:
            btn->R_CBUTTON = value;
            break;
        case 11:
            btn->D_CBUTTON = value;
            break;
        case 12:
            btn->U_DPAD = value;
            break;
        case 13:
            btn->L_DPAD = value;
            break;
        case 14:
            btn->R_DPAD = value;
            break;
        case 15:
            btn->D_DPAD = value;
            break;
        default:
            assert(false);
            break;
        }
    }

    /**
     * Gets the button name from a column index.
     * \param i The column index. Must be in the range [3, 15] inclusive.
     * \return The name of the button at the specified column index.
     */
    const char* get_button_name_from_column_index(size_t i)
    {
        switch (i)
        {
        case 3:
            return "A";
        case 4:
            return "B";
        case 5:
            return "Z";
        case 6:
            return "R";
        case 7:
            return "S";
        case 8:
            return "C^";
        case 9:
            return "C<";
        case 10:
            return "C>";
        case 11:
            return "Cv";
        case 12:
            return "D^";
        case 13:
            return "D<";
        case 14:
            return "D>";
        case 15:
            return "Dv";
        default:
            assert(false);
            return nullptr;
        }
    }

    /**
     * Prints a dump of the clipboard
     */
    void print_clipboard_dump()
    {
        std::println("[PianoRoll] ------------- Dump Begin -------------");
        std::println("[PianoRoll] Clipboard of length {}", g_clipboard.size());
        for (auto item : g_clipboard)
        {
            item.has_value() ? std::println("[PianoRoll] {:#06x}", item.value().Value) : std::println("[PianoRoll] ------");
        }
        std::println("[PianoRoll] ------------- Dump End -------------");
    }

    /**
     * Ensures that the currently relevant item is visible in the piano roll listview.
     */
    void ensure_relevant_item_visible()
    {
        int32_t i = ListView_GetNextItem(g_lv_hwnd, -1, LVNI_SELECTED);

        if (i == -1)
        {
            auto current_sample = VCR::get_seek_completion().first;
            ListView_EnsureVisible(g_lv_hwnd, VCR::get_task() == e_task::recording ? current_sample - 1 : current_sample, false);
            return;
        }

        ListView_EnsureVisible(g_lv_hwnd, i, false);
    }

    /**
     * Copies the selected inputs to the clipboard.
     */
    void copy_inputs()
    {
        if (g_piano_roll_state.selected_indicies.empty())
        {
            return;
        }

        if (g_piano_roll_state.selected_indicies.size() == 1)
        {
            g_clipboard = {g_piano_roll_state.inputs[g_piano_roll_state.selected_indicies[0]]};
            return;
        }

        const size_t min = g_piano_roll_state.selected_indicies[0];
        const size_t max = g_piano_roll_state.selected_indicies[g_piano_roll_state.selected_indicies.size() - 1];

        g_clipboard.clear();
        g_clipboard.reserve(max - min);

        for (auto i = min; i <= max; ++i)
        {
            // FIXME: Precompute this, create a map, do anything but not this bru
            const bool gap = std::ranges::find(g_piano_roll_state.selected_indicies, i) == g_piano_roll_state.selected_indicies.end();
            // HACK: nullopt acquired via explicit constructor call...
            std::optional<BUTTONS> opt;
            g_clipboard.push_back(gap ? opt : g_piano_roll_state.inputs[i]);
        }

        print_clipboard_dump();
    }

    /**
    * Updates the history listbox with the current state of the undo/redo stack.
    */
    void update_history_listbox()
    {
        SetWindowRedraw(g_hist_hwnd, false);
        ListBox_ResetContent(g_hist_hwnd);

        for (size_t i = 0; i < g_piano_roll_states.size(); ++i)
        {
            ListBox_AddString(g_hist_hwnd, std::format("Snapshot {}", i).c_str());
        }

        ListBox_SetCurSel(g_hist_hwnd, g_piano_roll_state_index);
        SetWindowRedraw(g_hist_hwnd, true);
    }

    /**
    * Pushes the current piano roll state to the undo stack. Should be called after operations which change the piano roll state.
    */
    void push_state_to_undo_stack()
    {
        std::println("[PianoRoll] Pushing state to undo stack...");

        if (g_piano_roll_states.size() > g_config.piano_roll_undo_stack_size)
        {
            g_piano_roll_states.pop_back();
        }

        g_piano_roll_states.push_front(g_piano_roll_state);
        g_piano_roll_state_index++;

        update_history_listbox();
    }

    /**
     * Applies the g_piano_roll_state.inputs buffer to the core.
     */
    void apply_input_buffer()
    {
        // This might be called from UI thread, thus grabbing the VCR lock.
        // Problem is that the VCR lock is already grabbed by the core thread because current sample changed message is executed on core thread.
        AsyncExecutor::invoke_async([]
        {
            auto result = VCR::begin_warp_modify(g_piano_roll_state.inputs);

            if (result == VCR::Result::Ok)
            {
                push_state_to_undo_stack();
            }
            else
            {
                // Since we do optimistic updates, we need to revert the changes we made to the input buffer to avoid visual desync
                SetWindowRedraw(g_lv_hwnd, false);

                ListView_DeleteAllItems(g_lv_hwnd);

                g_piano_roll_state.inputs = VCR::get_inputs();
                ListView_SetItemCount(g_lv_hwnd, g_piano_roll_state.inputs.size());
                std::println("[PianoRoll] Pulled inputs from core for recording mode due to warp modify failing, count: {}", g_piano_roll_state.inputs.size());

                SetWindowRedraw(g_lv_hwnd, true);

                FrontendService::show_error(std::format("Failed to initiate the warp modify operation, error code {}.", (int32_t)result).c_str(), "Piano Roll", g_hwnd);
            }
        });
    }

    /**
     * Sets the piano roll state to the specified value, updating everything accordingly and also applying the input buffer.
     * This is an expensive and slow operation.
     */
    void set_piano_roll_state(const PianoRollState& piano_roll_state)
    {
        g_piano_roll_state = piano_roll_state;
        ListView_SetItemCountEx(g_lv_hwnd, g_piano_roll_state.inputs.size(), LVSICF_NOSCROLL);
        set_listview_selection(g_lv_hwnd, g_piano_roll_state.selected_indicies);
        apply_input_buffer();
        update_history_listbox();
    }

    /**
     * Pastes the selected inputs from the clipboard into the piano roll.
     */
    void paste_inputs(bool merge)
    {
        if (g_clipboard.empty() || g_piano_roll_state.selected_indicies.empty() || !can_modify_inputs())
        {
            return;
        }

        bool clipboard_has_gaps = false;
        for (auto item : g_clipboard)
        {
            if (!item.has_value())
            {
                clipboard_has_gaps = true;
                break;
            }
        }

        bool selection_has_gaps = false;
        if (g_piano_roll_state.selected_indicies.size() > 1)
        {
            for (int i = 1; i < g_piano_roll_state.selected_indicies.size(); ++i)
            {
                if (g_piano_roll_state.selected_indicies[i] - g_piano_roll_state.selected_indicies[i - 1] > 1)
                {
                    selection_has_gaps = true;
                    break;
                }
            }
        }

        std::println("[PianoRoll] Clipboard/selection gaps: {}, {}", clipboard_has_gaps, selection_has_gaps);

        SetWindowRedraw(g_lv_hwnd, false);

        if (g_piano_roll_state.selected_indicies.size() == 1)
        {
            // 1-sized selection indicates a bulk copy, where copy all the inputs over (and ignore the clipboard gaps)
            size_t i = g_piano_roll_state.selected_indicies[0];

            for (auto item : g_clipboard)
            {
                if (item.has_value() && i < g_piano_roll_state.inputs.size())
                {
                    g_piano_roll_state.inputs[i] = merge ? BUTTONS{g_piano_roll_state.inputs[i].Value | item.value().Value} : item.value();
                    ListView_Update(g_lv_hwnd, i);
                }

                i++;
            }
        }
        else
        {
            // Standard case: selection is a mask
            size_t i = g_piano_roll_state.selected_indicies[0];

            for (auto item : g_clipboard)
            {
                const bool included = std::ranges::find(g_piano_roll_state.selected_indicies, i) != g_piano_roll_state.selected_indicies.end();

                if (item.has_value() && i < g_piano_roll_state.inputs.size() && included)
                {
                    g_piano_roll_state.inputs[i] = merge ? BUTTONS{g_piano_roll_state.inputs[i].Value | item.value().Value} : item.value();
                    ListView_Update(g_lv_hwnd, i);
                }

                i++;
            }
        }

        // Move selection to allow cool block-wise pasting
        const size_t offset = g_piano_roll_state.selected_indicies[g_piano_roll_state.selected_indicies.size() - 1] - g_piano_roll_state.selected_indicies[0] + 1;
        shift_listview_selection(g_lv_hwnd, offset);

        ensure_relevant_item_visible();

        SetWindowRedraw(g_lv_hwnd, true);

        apply_input_buffer();
    }

    /**
     * Restores the piano roll state to the last stored state.
     */
    bool undo()
    {
        if (g_piano_roll_states.size() <= 1)
        {
            return false;
        }

        if (g_piano_roll_state_index <= 0)
        {
            return false;
        }

        g_piano_roll_state_index--;
        set_piano_roll_state(g_piano_roll_states[g_piano_roll_state_index]);
        update_history_listbox();

        return true;
    }

    /**
     * Restores the piano roll state to the next stored state.
     */
    bool redo()
    {
        if (g_piano_roll_states.size() <= 1)
        {
            return false;
        }

        if (g_piano_roll_state_index + 1 >= g_piano_roll_states.size())
        {
            return false;
        }

        g_piano_roll_state_index++;
        set_piano_roll_state(g_piano_roll_states[g_piano_roll_state_index]);
        update_history_listbox();

        return true;
    }

    /**
     * Zeroes out all inputs in the current selection
     */
    void clear_inputs_in_selection()
    {
        if (g_piano_roll_state.selected_indicies.empty() || !can_modify_inputs())
        {
            return;
        }

        SetWindowRedraw(g_lv_hwnd, false);

        for (auto i : g_piano_roll_state.selected_indicies)
        {
            g_piano_roll_state.inputs[i] = {0};
            ListView_Update(g_lv_hwnd, i);
        }

        SetWindowRedraw(g_lv_hwnd, true);

        apply_input_buffer();
    }

    /**
     * Deletes all inputs in the current selection, removing them from the input buffer.
     */
    void delete_inputs_in_selection()
    {
        if (g_piano_roll_state.selected_indicies.empty() || !can_modify_inputs())
        {
            return;
        }

        std::vector<size_t> selected_indicies(g_piano_roll_state.selected_indicies.begin(), g_piano_roll_state.selected_indicies.end());
        g_piano_roll_state.inputs = erase_indices(g_piano_roll_state.inputs, selected_indicies);
        ListView_RedrawItems(g_lv_hwnd, 0, ListView_GetItemCount(g_lv_hwnd));
        const int32_t offset = g_piano_roll_state.selected_indicies[g_piano_roll_state.selected_indicies.size() - 1] - g_piano_roll_state.selected_indicies[0] + 1;
        shift_listview_selection(g_lv_hwnd, -offset);

        apply_input_buffer();
    }

    void update_groupbox_status_text()
    {
        if (VCR::get_warp_modify_status() == e_warp_modify_status::warping)
        {
            SetDlgItemText(g_hwnd, IDC_STATIC, "Input - Warping...");
            return;
        }

        if (g_piano_roll_state.selected_indicies.empty())
        {
            SetDlgItemText(g_hwnd, IDC_STATIC, "Input");
        }
        else if (g_piano_roll_state.selected_indicies.size() == 1)
        {
            SetDlgItemText(g_hwnd, IDC_STATIC, std::format("Input - Frame {}", g_piano_roll_state.selected_indicies[0]).c_str());
        }
        else
        {
            SetDlgItemText(g_hwnd, IDC_STATIC, std::format("Input - {} frames selected", g_piano_roll_state.selected_indicies.size()).c_str());
        }
    }

    void on_task_changed(std::any data)
    {
        auto value = std::any_cast<e_task>(data);
        static auto previous_value = value;

        if (value != previous_value)
        {
            std::println("[PianoRoll] Processing TaskChanged from {} to {}", (int32_t)previous_value, (int32_t)value);
            update_inputs();
        }

        previous_value = value;
    }

    void on_current_sample_changed(std::any data)
    {
        auto value = std::any_cast<long>(data);
        static auto previous_value = value;

        if (VCR::get_warp_modify_status() == e_warp_modify_status::warping)
        {
            goto exit;
        }

        if (VCR::get_task() == e_task::idle)
        {
            goto exit;
        }

        if (VCR::get_task() == e_task::recording)
        {
            g_piano_roll_state.inputs = VCR::get_inputs();
            ListView_SetItemCountEx(g_lv_hwnd, g_piano_roll_state.inputs.size(), LVSICF_NOSCROLL);
        }

        ListView_Update(g_lv_hwnd, previous_value);
        ListView_Update(g_lv_hwnd, value);

        ensure_relevant_item_visible();

    exit:
        previous_value = value;
    }

    void on_unfreeze_completed(std::any)
    {
        if (g_config.vcr_readonly || VCR::get_warp_modify_status() == e_warp_modify_status::warping)
        {
            return;
        }

        SetWindowRedraw(g_lv_hwnd, false);

        ListView_DeleteAllItems(g_lv_hwnd);

        g_piano_roll_state.inputs = VCR::get_inputs();
        ListView_SetItemCountEx(g_lv_hwnd, min(VCR::get_seek_completion().first, g_piano_roll_state.inputs.size()), LVSICF_NOSCROLL);

        SetWindowRedraw(g_lv_hwnd, true);

        ensure_relevant_item_visible();
    }

    void on_warp_modify_status_changed(std::any)
    {
        update_groupbox_status_text();
    }

    void on_seek_savestate_changed(std::any data)
    {
        if (!g_hwnd)
        {
            return;
        }

        auto value = std::any_cast<size_t>(data);
        g_seek_savestate_frames = VCR::get_seek_savestate_frames();
        ListView_Update(g_lv_hwnd, value);
    }

    /**
     * The window procedure for the joystick control. 
     */
    LRESULT CALLBACK joystick_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_LBUTTONDOWN:
            if (!can_modify_inputs())
            {
                break;
            }
            g_joy_drag = true;
            SetCapture(hwnd);
            goto mouse_move;
        case WM_LBUTTONUP:
            if (!g_joy_drag)
            {
                break;
            }
            goto lmb_up;
        case WM_MOUSEMOVE:
            goto mouse_move;
        case WM_PAINT:
            {
                int32_t i = ListView_GetNextItem(g_lv_hwnd, -1, LVNI_SELECTED);

                BUTTONS input = {0};

                if (i < 0 || i >= g_piano_roll_state.inputs.size())
                {
                    break;
                }

                input = g_piano_roll_state.inputs[i];


                PAINTSTRUCT ps;
                RECT rect;
                HDC hdc = BeginPaint(hwnd, &ps);
                HDC cdc = CreateCompatibleDC(hdc);
                GetClientRect(hwnd, &rect);

                HBITMAP bmp = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
                SelectObject(cdc, bmp);

                const int mid_x = rect.right / 2;
                const int mid_y = rect.bottom / 2;
                const int stick_x = (input.Y_AXIS + 128) * rect.right / 256;
                const int stick_y = (-input.X_AXIS + 128) * rect.bottom / 256;

                HPEN outline_pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
                HPEN line_pen = CreatePen(PS_SOLID, 3, RGB(0, 0, 255));
                HPEN tip_pen = CreatePen(PS_SOLID, 7, RGB(255, 0, 0));

                FillRect(cdc, &rect, GetSysColorBrush(COLOR_BTNFACE));

                SelectObject(cdc, outline_pen);
                Ellipse(cdc, 0, 0, rect.right, rect.bottom);

                MoveToEx(cdc, 0, mid_y, nullptr);
                LineTo(cdc, rect.right, mid_y);
                MoveToEx(cdc, mid_x, 0, nullptr);
                LineTo(cdc, mid_x, rect.bottom);

                SelectObject(cdc, line_pen);
                MoveToEx(cdc, mid_x, mid_y, nullptr);
                LineTo(cdc, stick_x, stick_y);

                SelectObject(cdc, tip_pen);
                MoveToEx(cdc, stick_x, stick_y, nullptr);
                LineTo(cdc, stick_x, stick_y);

                SelectObject(cdc, nullptr);

                BitBlt(hdc, 0, 0, rect.right, rect.bottom, cdc, 0, 0, SRCCOPY);

                EndPaint(hwnd, &ps);

                DeleteDC(cdc);
                DeleteObject(bmp);
                DeleteObject(outline_pen);
                DeleteObject(line_pen);
                DeleteObject(tip_pen);

                return 0;
            }
        default:
            break;
        }

    def:
        return DefWindowProc(hwnd, msg, wParam, lParam);

    lmb_up:
        ReleaseCapture();
        apply_input_buffer();
        g_joy_drag = false;
        goto def;

    mouse_move:
        if (!can_modify_inputs())
        {
            g_joy_drag = false;
        }

        if (!g_joy_drag)
        {
            goto def;
        }

        // Apply the joystick input...
        if (!(GetKeyState(VK_LBUTTON) & 0x100))
        {
            goto lmb_up;
        }

        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hwnd, &pt);

        RECT pic_rect;
        GetWindowRect(hwnd, &pic_rect);
        int x = (pt.x * UINT8_MAX / (signed)(pic_rect.right - pic_rect.left) - INT8_MAX + 1);
        int y = -(pt.y * UINT8_MAX / (signed)(pic_rect.bottom - pic_rect.top) - INT8_MAX + 1);

        if (x > INT8_MAX || y > INT8_MAX || x < INT8_MIN || y < INT8_MIN)
        {
            int div = max(abs(x), abs(y));
            x = x * INT8_MAX / div;
            y = y * INT8_MAX / div;
        }

        if (abs(x) <= 8)
            x = 0;
        if (abs(y) <= 8)
            y = 0;

        SetWindowRedraw(g_lv_hwnd, false);
        for (auto selected_index : g_piano_roll_state.selected_indicies)
        {
            g_piano_roll_state.inputs[selected_index].X_AXIS = y;
            g_piano_roll_state.inputs[selected_index].Y_AXIS = x;
            ListView_Update(g_lv_hwnd, selected_index);
        }
        SetWindowRedraw(g_lv_hwnd, true);

        RedrawWindow(g_joy_hwnd, nullptr, nullptr, RDW_INVALIDATE);
        goto def;
    }

    /**
     * Called when the selection in the piano roll listview changes.
     */
    void on_piano_roll_selection_changed()
    {
        g_piano_roll_state.selected_indicies = get_listview_selection(g_lv_hwnd);

        update_groupbox_status_text();
        RedrawWindow(g_joy_hwnd, nullptr, nullptr, RDW_INVALIDATE);
    }

    /**
     * The window procedure for the listview control. 
     */
    LRESULT CALLBACK list_view_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId, DWORD_PTR)
    {
        switch (msg)
        {
        case WM_CONTEXTMENU:
            {
                HMENU h_menu = CreatePopupMenu();
                const auto base_style = can_modify_inputs() ? MF_ENABLED : MF_DISABLED;
                AppendMenu(h_menu, base_style | MF_STRING, 1, "Copy\tCtrl+C");
                AppendMenu(h_menu, base_style | MF_STRING, 2, "Paste\tCtrl+V");
                AppendMenu(h_menu, base_style | MF_STRING, 3, "Undo\tCtrl+Z");
                AppendMenu(h_menu, base_style | MF_STRING, 4, "Redo\tCtrl+Y");
                AppendMenu(h_menu, base_style | MF_STRING, 5, "Clear\tBackspace");
                AppendMenu(h_menu, base_style | MF_STRING, 6, "Delete\tDelete");

                const int offset = TrackPopupMenuEx(h_menu, TPM_RETURNCMD | TPM_NONOTIFY, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), hwnd, 0);

                switch (offset)
                {
                case 1:
                    copy_inputs();
                    break;
                case 2:
                    paste_inputs(false);
                    break;
                case 3:
                    undo();
                    break;
                case 4:
                    redo();
                    break;
                case 5:
                    clear_inputs_in_selection();
                    break;
                case 6:
                    delete_inputs_in_selection();
                    break;
                default:
                    break;
                }

                break;
            }
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            {
                LVHITTESTINFO lplvhtti{};
                GetCursorPos(&lplvhtti.pt);
                ScreenToClient(hwnd, &lplvhtti.pt);
                ListView_SubItemHitTest(hwnd, &lplvhtti);

                if (lplvhtti.iSubItem <= 2 || lplvhtti.iItem < 0)
                {
                    printf("[PianoRoll] Ignoring WM_LBUTTONDOWN/WM_RBUTTONDOWN with bad iSubItem/iItem\n");
                    break;
                }

                if (!can_modify_inputs())
                {
                    break;
                }

                auto input = g_piano_roll_state.inputs[lplvhtti.iItem];

                g_lv_dragging = true;
                g_lv_drag_column = lplvhtti.iSubItem;
                g_lv_drag_initial_value = !get_input_value_from_column_index(input, g_lv_drag_column);
                g_lv_drag_unset = GetKeyState(VK_RBUTTON) & 0x100;

                goto handle_mouse_move;
            }
        case WM_MOUSEMOVE:
            goto handle_mouse_move;
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
            if (g_lv_dragging)
            {
                g_lv_dragging = false;
                apply_input_buffer();
            }
            break;
        case WM_NCDESTROY:
            RemoveWindowSubclass(hwnd, list_view_proc, sId);
            break;
        case WM_KEYDOWN:
            {
                if (wParam == VK_BACK)
                {
                    clear_inputs_in_selection();
                    break;
                }

                if (wParam == VK_DELETE)
                {
                    delete_inputs_in_selection();
                    break;
                }

                if (!(GetKeyState(VK_CONTROL) & 0x8000))
                {
                    break;
                }

                if (wParam == 'C')
                {
                    copy_inputs();
                    break;
                }

                if (wParam == 'V')
                {
                    paste_inputs(GetKeyState(VK_SHIFT) & 0x8000);
                    break;
                }

                if (wParam == 'Z')
                {
                    undo();
                    break;
                }

                if (wParam == 'Y')
                {
                    redo();
                    break;
                }

                break;
            }

        default:
            break;
        }

    def:
        return DefSubclassProc(hwnd, msg, wParam, lParam);

    handle_mouse_move:

        if (!can_modify_inputs())
        {
            goto def;
        }

        bool prev_lv_dragging = g_lv_dragging;

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
            if (prev_lv_dragging)
            {
                apply_input_buffer();
            }
            goto def;
        }

        LVHITTESTINFO lplvhtti{};
        GetCursorPos(&lplvhtti.pt);
        ScreenToClient(hwnd, &lplvhtti.pt);
        ListView_SubItemHitTest(hwnd, &lplvhtti);

        if (lplvhtti.iItem >= g_piano_roll_state.inputs.size())
        {
            printf("[PianoRoll] iItem out of range\n");
            goto def;
        }

        if (!g_config.piano_roll_constrain_edit_to_column && lplvhtti.iSubItem <= 2)
        {
            goto def;
        }

        // During a drag operation, we just mutate the input vector in memory and update the listview without doing anything with the core.
        // Only when the drag ends do we actually apply the changes to the core via begin_warp_modify

        const auto column = g_config.piano_roll_constrain_edit_to_column ? g_lv_drag_column : lplvhtti.iSubItem;
        const auto new_value = g_lv_drag_unset ? 0 : g_lv_drag_initial_value;

        SetWindowRedraw(g_lv_hwnd, false);

        set_input_value_from_column_index(&g_piano_roll_state.inputs[lplvhtti.iItem], column, new_value);
        ListView_Update(hwnd, lplvhtti.iItem);

        // If we are editing a row inside the selection, we want to apply the same modify operation to the other selected rows.
        if (std::ranges::find(g_piano_roll_state.selected_indicies, lplvhtti.iItem) != g_piano_roll_state.selected_indicies.end())
        {
            for (const auto& i : g_piano_roll_state.selected_indicies)
            {
                set_input_value_from_column_index(&g_piano_roll_state.inputs[i], column, new_value);
                ListView_Update(hwnd, i);
            }
        }

        SetWindowRedraw(g_lv_hwnd, true);
    }

    /**
     * The window procedure for the piano roll dialog. 
     */
    LRESULT CALLBACK dialog_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_INITDIALOG:
            {
                g_hwnd = hwnd;
                g_joy_hwnd = CreateWindowEx(WS_EX_STATICEDGE, JOYSTICK_CLASS, "", WS_CHILD | WS_VISIBLE, 17, 30, 131, 131, g_hwnd, nullptr, g_app_instance, nullptr);
                CreateWindowEx(0, WC_STATIC, "History", WS_CHILD | WS_VISIBLE | WS_GROUP | SS_LEFT | SS_CENTERIMAGE, 17, 166, 131, 15, g_hwnd, nullptr, g_app_instance, nullptr);
                g_hist_hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT, 17, 186, 131, 181, g_hwnd, nullptr, g_app_instance, nullptr);

                EnumChildWindows(hwnd, [](HWND hwnd, LPARAM font)
                {
                    SendMessage(hwnd, WM_SETFONT, (WPARAM)font, 0);
                    return TRUE;
                }, SendMessage(hwnd, WM_GETFONT, 0, 0));

                RECT grid_rect = get_window_rect_client_space(hwnd, GetDlgItem(hwnd, IDC_LIST_PIANO_ROLL));

                auto dwStyle = WS_TABSTOP
                    | WS_VISIBLE
                    | WS_CHILD
                    | LVS_REPORT
                    | LVS_ALIGNTOP
                    | LVS_NOSORTHEADER
                    | LVS_SHOWSELALWAYS
                    | LVS_OWNERDATA;

                g_lv_hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, nullptr,
                                           dwStyle,
                                           grid_rect.left, grid_rect.top,
                                           grid_rect.right - grid_rect.left,
                                           grid_rect.bottom - grid_rect.top,
                                           hwnd, (HMENU)IDC_PIANO_ROLL_LV,
                                           g_app_instance,
                                           nullptr);
                SetWindowSubclass(g_lv_hwnd, list_view_proc, 0, 0);

                ListView_SetExtendedListViewStyle(g_lv_hwnd,
                                                  LVS_EX_GRIDLINES
                                                  | LVS_EX_FULLROWSELECT
                                                  | LVS_EX_DOUBLEBUFFER);

                HIMAGELIST image_list = ImageList_Create(16, 16, ILC_COLORDDB | ILC_MASK, 1, 0);
                ImageList_AddIcon(image_list, LoadIcon(g_app_instance, MAKEINTRESOURCE(IDI_CURRENT)));
                ImageList_AddIcon(image_list, LoadIcon(g_app_instance, MAKEINTRESOURCE(IDI_MARKER)));
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

                for (int i = 3; i <= 15; ++i)
                {
                    lv_column.pszText = (LPTSTR)get_button_name_from_column_index(i);
                    ListView_InsertColumn(g_lv_hwnd, i + 1, &lv_column);
                }

                ListView_DeleteColumn(g_lv_hwnd, 0);

                // Manually call all the setup-related callbacks
                update_inputs();
                on_task_changed(VCR::get_task());
                // ReSharper disable once CppRedundantCastExpression
                on_current_sample_changed(static_cast<long>(VCR::get_seek_completion().first));
                update_groupbox_status_text();
                update_history_listbox();
                SendMessage(hwnd, WM_SIZE, 0, 0);

                break;
            }
        case WM_DESTROY:
            EnumChildWindows(hwnd, [](HWND hwnd, LPARAM)
            {
                DestroyWindow(hwnd);
                return TRUE;
            }, 0);
            g_lv_hwnd = nullptr;
            g_hist_hwnd = nullptr;
            g_hwnd = nullptr;
            break;
        case WM_CLOSE:
            EndDialog(hwnd, IDCANCEL);
            break;
        case WM_SIZE:
            {
                HWND gp_hwnd = GetDlgItem(hwnd, IDC_STATIC);

                RECT rect{};
                GetClientRect(hwnd, &rect);

                RECT lv_rect = get_window_rect_client_space(hwnd, g_lv_hwnd);
                RECT gp_rect = get_window_rect_client_space(hwnd, gp_hwnd);

                SetWindowPos(g_lv_hwnd, nullptr, 0, 0, rect.right - 10 - lv_rect.left, rect.bottom - 10 - lv_rect.top, SWP_NOMOVE | SWP_NOZORDER);
                SetWindowPos(gp_hwnd, nullptr, 0, 0, gp_rect.right - gp_rect.left, rect.bottom - 10 - gp_rect.top, SWP_NOMOVE | SWP_NOZORDER);
                break;
            }
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
                    case LVN_ITEMCHANGED:
                        {
                            const auto nmlv = (NMLISTVIEW*)lParam;

                            if ((nmlv->uNewState ^ nmlv->uOldState) & LVIS_SELECTED)
                            {
                                on_piano_roll_selection_changed();
                            }

                            break;
                        }
                    case LVN_ODSTATECHANGED:
                        on_piano_roll_selection_changed();
                        break;
                    case LVN_GETDISPINFO:
                        {
                            const auto plvdi = (NMLVDISPINFO*)lParam;

                            if (plvdi->item.iItem < 0 || plvdi->item.iItem >= g_piano_roll_state.inputs.size())
                            {
                                printf("[PianoRoll] Ignoring LVN_GETDISPINFO with iItem out of range\n");
                                break;
                            }

                            if (!(plvdi->item.mask & LVIF_TEXT))
                            {
                                break;
                            }

                            auto input = g_piano_roll_state.inputs[plvdi->item.iItem];

                            switch (plvdi->item.iSubItem)
                            {
                            case 0:
                                {
                                    auto current_sample = VCR::get_seek_completion().first;
                                    if (current_sample == plvdi->item.iItem)
                                    {
                                        plvdi->item.iImage = 0;
                                    }
                                    else if (g_seek_savestate_frames.contains(plvdi->item.iItem))
                                    {
                                        plvdi->item.iImage = 1;
                                    }
                                    else
                                    {
                                        plvdi->item.iImage = 999;
                                    }

                                    strcpy(plvdi->item.pszText, std::to_string(plvdi->item.iItem).c_str());
                                    break;
                                }
                            case 1:
                                strcpy(plvdi->item.pszText, std::to_string(input.Y_AXIS).c_str());
                                break;
                            case 2:
                                strcpy(plvdi->item.pszText, std::to_string(input.X_AXIS).c_str());
                                break;
                            default:
                                {
                                    auto value = get_input_value_from_column_index(input, plvdi->item.iSubItem);
                                    auto name = get_button_name_from_column_index(plvdi->item.iSubItem);
                                    strcpy(plvdi->item.pszText, value ? name : "");
                                    break;
                                }
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
        WNDCLASS wndclass = {0};
        wndclass.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
        wndclass.lpfnWndProc = (WNDPROC)joystick_proc;
        wndclass.hInstance = g_app_instance;
        wndclass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wndclass.lpszClassName = JOYSTICK_CLASS;
        RegisterClass(&wndclass);
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
            std::vector<std::function<void()>> unsubscribe_funcs;
            unsubscribe_funcs.push_back(Messenger::subscribe(Messenger::Message::TaskChanged, on_task_changed));
            unsubscribe_funcs.push_back(Messenger::subscribe(Messenger::Message::CurrentSampleChanged, on_current_sample_changed));
            unsubscribe_funcs.push_back(Messenger::subscribe(Messenger::Message::UnfreezeCompleted, on_unfreeze_completed));
            unsubscribe_funcs.push_back(Messenger::subscribe(Messenger::Message::WarpModifyStatusChanged, on_warp_modify_status_changed));
            unsubscribe_funcs.push_back(Messenger::subscribe(Messenger::Message::SeekSavestateChanged, on_seek_savestate_changed));

            DialogBox(g_app_instance, MAKEINTRESOURCE(IDD_PIANO_ROLL), 0, (DLGPROC)dialog_proc);

            std::println("[PianoRoll] Unsubscribing from {} messages...", unsubscribe_funcs.size());
            for (auto unsubscribe_func : unsubscribe_funcs)
            {
                unsubscribe_func();
            }
        }).detach();
    }
}
