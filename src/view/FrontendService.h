/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/*
 *	Interface for calls originating from core or shared layer to the view layer
 *
 *	Must be implemented in the view layer.
 */

#include <core_api.h>

namespace FrontendService
{
    /**
     * Prompts the user to pick a choice from a provided collection of choices.
     * \param choices The collection of choices.
     * \param str The dialog content.
     * \param title The dialog title.
     * \param hwnd The parent window
     * \return The index of the chosen choice
     */
    size_t show_multiple_choice_dialog(const std::vector<std::wstring>& choices, const wchar_t* str, const wchar_t* title = nullptr, fsvc_dialog_type type = fsvc_warning, void* hwnd = nullptr);

    /**
     * \brief Asks the user a yes/no question
     * \param str The dialog content
     * \param title The dialog title
     * \param warning Whether the tone of the message is perceived as a warning
     * \param hwnd The parent window
     * \return Whether the user answered yes
     * \remarks If the user has chosen to not use modals, this function will return true by default
     */
    bool show_ask_dialog(const wchar_t* str, const wchar_t* title = nullptr, bool warning = false, void* hwnd = nullptr);

	/**
	 * \brief Shows the user a dialog.
	 * \param str The dialog content.
	 * \param title The dialog title.
	 * \param type The dialog tone.
	 * \param hwnd The parent window.
	 */
	void show_dialog(const wchar_t* str, const wchar_t* title = nullptr, fsvc_dialog_type type = fsvc_warning, void* hwnd = nullptr);

    /**
     * \brief Shows text in the statusbar
     * \param str The text
     */
    void show_statusbar(const wchar_t* str);

    /**
     * \brief Gets the path to the directory containing the mupen64 executable
     */
    std::filesystem::path get_app_path();

    /**
     * \brief Fills out the default hotkey keys in the config structure
     * \param config The config structure to modify
     * \remarks Because the shared layer is unaware of keycodes, the view must do this.
     * FIXME: While this works for now, storing platform-dependent keycodes in the config will make config files incompatible across platforms.
     */
    void set_default_hotkey_keys(core_cfg* config);

    /**
     * \brief Updates the screen with the current visual information
     */
    void update_screen();

    /**
     * \brief Notifies the frontend of a new VI
     */
    void at_vi();

    /**
     * \brief Notifies the frontend of the audio changing
     */
    void ai_len_changed();

    /**
     * \brief Finds the first rom from the available ROM list which matches the predicate
     * \param predicate A predicate which determines if the rom matches
     * \return The rom's path, or an empty string if no rom was found
     */
    std::wstring find_available_rom(const std::function<bool(const t_rom_header&)>& predicate);
}
