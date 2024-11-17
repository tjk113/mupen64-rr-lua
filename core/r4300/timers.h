/***************************************************************************
						  timers.c  -  description
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

#include <chrono>
#include <queue>
#include <mutex>

#include <core/r4300/rom.h>

typedef std::chrono::high_resolution_clock::time_point time_point;

const size_t max_deltas = 60;

/// Frame invalidation flag: cleared by consumer
extern bool frame_changed;

/// Time deltas between frames
extern double g_frame_deltas[max_deltas];
extern std::mutex g_frame_deltas_mutex;

/// Time deltas between VIs
extern double g_vi_deltas[max_deltas];
extern std::mutex g_vi_deltas_mutex;

/**
 * \brief Initializes timer code with the specified values
 * \param speed_modifier The current speed modifier
 * \param rom_header The current rom header
 */
void timer_init(int32_t speed_modifier, t_rom_header* rom_header);

/**
 * \brief To be called when a new frame is generated
 */
void timer_new_frame();

/**
 * \brief To be called when a VI is generated
 */
void timer_new_vi();
