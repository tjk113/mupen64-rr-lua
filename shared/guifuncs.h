/**
 * Mupen64 - guifuncs.h
 * Copyright (C) 2002 Hacktarux
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 *
 * If you want to contribute to the project please contact
 * me first (maybe someone is already making what you are
 * planning to do).
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
 * You should have received a copy of the GNU General Public
 * Licence along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
**/

#pragma once

/**
 * \brief Demands user confirmation for an exit action
 * \return Whether the action is allowed
 * \remarks If the user has chosen to not use modals, this function will return true by default
 */
bool confirm_user_exit();


/**
 * \brief Asks the user a yes/no question
 * \param str The dialog content
 * \param title The dialog title
 * \param warning Whether the tone of the message is perceived as a warning
 * \param hwnd The parent window
 * \return Whether the user answered yes
 * \remarks If the user has chosen to not use modals, this function will return true by default
 */
bool show_ask_dialog(const char* str, const char* title = nullptr, bool warning = false, void* hwnd = nullptr);

/**
 * \brief Shows the user a warning dialog
 * \param str The dialog content
 * \param title The dialog title
 * \param hwnd The parent window
 */
void show_warning(const char* str, const char* title = nullptr, void* hwnd = nullptr);

/**
 * \brief Shows the user an error dialog
 * \param str The dialog content
 * \param title The dialog title
 * \param hwnd The parent window
 */
void show_error(const char* str, const char* title = nullptr, void* hwnd = nullptr);

/**
 * \brief Shows the user an information dialog
 * \param str The dialog content
 * \param title The dialog title
 * \param hwnd The parent window
 */
void show_information(const char* str, const char* title = nullptr, void* hwnd = nullptr);

/**
 * \brief Whether the current execution is on the UI thread
 */
bool is_on_gui_thread();

/**
 * \brief Gets the path to the directory containing the mupen64 executable
 */
std::filesystem::path get_app_path();
