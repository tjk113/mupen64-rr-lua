/***************************************************************************
						  commandline.h  -  description
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
#ifndef COMMANDLINE_H
#define COMMANDLINE_H
#include <string>

/**
 * \brief Sets and readies state from commandline arguments
 */
void commandline_set();

/**
 * \brief Load the commandline-specified rom
 */
void commandline_load_rom();

#endif
