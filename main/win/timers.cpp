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
#include "../../memory/pif.h"
#include "../helpers/win_helpers.h"
#include "helpers/collection_helpers.h"

bool frame_changed = true;
extern int m_current_vi;
extern long m_current_sample;

std::chrono::duration<double, std::milli> max_vi_s_ms;

std::deque<time_point> new_frame_times;
std::deque<time_point> new_vi_times;

void timer_init(int32_t speed_modifier, t_rom_header* rom_header)
{
	const double max_vi_s = get_vis_per_second(rom_header->Country_code);
	max_vi_s_ms = std::chrono::duration<double, std::milli>(
		1000.0 / (max_vi_s * static_cast<double>(speed_modifier) / 100));

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
		static time_point last_vi_time;
		static time_point counter_time;
		static std::chrono::duration<double, std::nano> last_sleep_error;
		const auto current_vi_time = std::chrono::high_resolution_clock::now();
		// if we're playing game normally with no frame advance or ff and overstepping max time between frames,
		// we need to sleep to compensate the additional time
		if (const auto vi_time_diff = current_vi_time - last_vi_time; !
			fast_forward && !frame_advancing && vi_time_diff < max_vi_s_ms)
		{
			auto sleep_time = max_vi_s_ms - vi_time_diff;
			if (sleep_time.count() > 0 && sleep_time <
				std::chrono::milliseconds(700))
			{
				// we try to sleep for the overstepped time, but must account for sleeping inaccuracies
				const auto goal_sleep = max_vi_s_ms - vi_time_diff -
					last_sleep_error;
				const auto start_sleep =
					std::chrono::high_resolution_clock::now();
				std::this_thread::sleep_for(goal_sleep);
				const auto end_sleep =
					std::chrono::high_resolution_clock::now();

				// sleeping inaccuracy is difference between actual time spent sleeping and the goal sleep
				// this value isnt usually too large
				last_sleep_error = end_sleep - start_sleep - goal_sleep;
			} else
			{
				// sleep time is unreasonable, log it and reset related state
				const auto casted = std::chrono::duration_cast<
					std::chrono::milliseconds>(sleep_time).count();
				printf("Invalid timer: %lld ms\n", casted);
				sleep_time = sleep_time.zero();
				counter_time = std::chrono::high_resolution_clock::now();
			}
		}

		last_vi_time = std::chrono::high_resolution_clock::now();
		// TODO: Reimplement game crash detection, but properly
	}
	circular_push(new_vi_times, std::chrono::high_resolution_clock::now());
}
