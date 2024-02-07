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

#include <Windows.h>
#include "rom.h"

namespace Rombrowser
{
	/**
	 * \brief Initializes the rombrowser
	 */
	void init();

	/**
	 * \brief Builds the rombrowser contents
	 */
	void build();

	/**
 	* \brief Notifies the rombrowser of a parent receiving the WM_NOTIFY message
 	* \param lparam The lparam value associated with the current message processing pass
 	*/
	void notify(LPARAM lparam);
}
