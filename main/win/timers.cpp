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
#include <Windows.h>
#include <cstdio>
#include <commctrl.h>
#include "main_win.h"
#include "Config.hpp"
#include "../rom.h"
#include "../helpers/win_helpers.h"
#include "../../memory/pif.h"

//c++!!!
#include <thread>

#include "features/Statusbar.hpp"
#include "helpers/collection_helpers.h"


extern bool ffup;
extern int m_current_vi;
extern long m_current_sample;

std::chrono::duration<double, std::milli> max_vi_s_ms;
float max_vi_s;

std::deque<time_point> new_frame_times;
std::deque<time_point> new_vi_times;

void timer_init()
{
	max_vi_s = (float)get_vis_per_second(ROM_HEADER.Country_code);
	max_vi_s_ms = std::chrono::duration<double, std::milli>(
		1000.0 / (max_vi_s * (float)Config.fps_modifier / 100));
	new_frame_times = {};
	new_vi_times = {};
	statusbar_post_text(std::format("Speed limit: {}%", Config.fps_modifier));
}

void timer_new_frame()
{
	circular_push(new_frame_times, std::chrono::high_resolution_clock::now());
	is_primary_statusbar_invalidated = true;
}

void timer_new_vi()
{
	if (!fast_forward)
	{
		float multiplier = (float)Config.fps_modifier / 100.0f;
		float target_vis = multiplier * (float)get_vis_per_second(
			ROM_HEADER.Country_code);
		float target_sleep_time = 1000.0f / target_vis;
		accurate_sleep(target_sleep_time / 1000.0f);

		// TODO: Reimplement game crash detection, but properly
	}
	circular_push(new_vi_times, std::chrono::high_resolution_clock::now());
}
