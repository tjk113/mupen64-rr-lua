/***************************************************************************
						  rombrowser.h  -  description
							 -------------------
	copyright            : (C) 2003 by ShadowPrince
	email                : shadow@emulation64.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <windows.h>
#include "rom.h"

using t_rombrowser_entry = struct s_rombrowser_entry
{
	std::string path;
	size_t size;
	t_rom_header rom_header;
};

/**
 * \brief Creates the rombrowser control
 */
void rombrowser_create();

/**
 * \brief Deletes and rebuilds the rombrowser contents
 */
void rombrowser_build();

/**
 * \brief Sets the rombrowser's visibility
 * \param is_visible Whether the rombrowser should be visible
 */
void rombrowser_set_visibility(int32_t is_visible);

/**
 * \brief Updates the rombrowser control's size
 */
void rombrowser_update_size();

/**
 * \brief Notifies the rombrowser of a parent receiving the WM_NOTIFY message
 * \param lparam The lparam value associated with the current message processing pass
 */
void rombrowser_notify(LPARAM lparam);
