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
#include "../../lua/LuaConsole.h"

#include "timers.h"
#include "Config.hpp"
#include "../rom.h"
#include "../../memory/pif.h"
#include "../helpers/win_helpers.h"
#include "helpers/collection_helpers.h"

bool frame_changed = true;
extern int m_current_vi;
extern long m_current_sample;

std::chrono::duration<double, std::milli> max_vi_s_ms;

std::deque<time_point> new_frame_times;
std::deque<time_point> new_vi_times;

float target_sleep_time;

void timer_init()
{
	// We precompute the sleep time, since it only changes with the rom
	float multiplier = (float)Config.fps_modifier / 100.0f;
	float target_vis = multiplier * (float)get_vis_per_second(
		ROM_HEADER.Country_code);
	target_sleep_time = 1000.0f / target_vis;

	new_frame_times = {};
	new_vi_times = {};
}

void timer_new_frame()
{
	circular_push(new_frame_times, std::chrono::high_resolution_clock::now());
	frame_changed = true;
}

void timer_new_vi()
{
	if (!fast_forward)
	{
		accurate_sleep(target_sleep_time / 1000.0f);
		// TODO: Reimplement game crash detection, but properly
	}
	circular_push(new_vi_times, std::chrono::high_resolution_clock::now());
}
