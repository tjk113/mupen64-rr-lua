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

#include <windows.h>
#include <stdio.h>
#include <commctrl.h>
#include "../guifuncs.h"
#include "main_win.h"
#include "Config.hpp"
#include "../rom.h"
#include "translation.h"
#include "../vcr.h"
#include "../../memory/pif.h"
#include "../../winproject/resource.h"

//c++!!!
#include <chrono>
#include <thread>
#include <iostream>

#include "features/Statusbar.hpp"

extern bool ffup;
extern int m_currentVI;
extern long m_currentSample;

std::chrono::duration<double, std::milli> max_vi_s_ms;
float max_vi_s, vi_s, fps;
typedef std::chrono::high_resolution_clock::time_point time_point;

void timer_init()
{
	max_vi_s = (float)get_vis_per_second(ROM_HEADER.Country_code);
	max_vi_s_ms = std::chrono::duration<double, std::milli>(
		1000.0 / (max_vi_s * (float)Config.fps_modifier / 100));
}


// TODO: buffer frame and vi statusbar updates and update all section simultaneously
void timer_new_frame()
{
	static time_point last_statusbar_update_time = std::chrono::high_resolution_clock::now();
	static int frame_count = 0;

	++frame_count;
	auto current_time = std::chrono::high_resolution_clock::now();

	if (current_time - last_statusbar_update_time > std::chrono::seconds(1))
	{
		auto time_taken = std::chrono::duration<double>(
			current_time - last_statusbar_update_time);

		if (Config.show_fps)
		{
			statusbar_send_text(std::format("FPS: {:.0f}", frame_count / time_taken.count()), 2);
		}

		last_statusbar_update_time = std::chrono::high_resolution_clock::now();
		frame_count = 0;
	}
}



void timer_new_vi()
{
	// time when last vi happened
	static time_point last_vi_time;
	static time_point counter_time;
	static std::chrono::duration<double, std::nano> last_sleep_error;
	static unsigned int vi_counter = 0;

	// fps wont update when emu is stuck so we must check vi/s
	// vi/s shouldn't go over 1000 in normal gameplay while holding down fast forward unless you have repeat speed at uzi speed
	if (!ignoreErrorEmulation && emu_launched && frame_advancing && vi_s > 1000)
	{
		int result = 0;
		TaskDialog(mainHWND, app_hInstance, L"Error",
			L"Unusual core state", L"An emulator core timing inaccuracy or game crash has been detected. You can choose to continue emulation.", TDCBF_RETRY_BUTTON | TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, &result);

		if (result == IDCLOSE) {
			frame_advancing = false; //don't pause at next open
			CreateThread(NULL, 0, close_rom, (LPVOID)1, 0, 0);
		}
	}

	m_currentVI++;

	if (VCR_isRecording())
		VCR_setLengthVIs(m_currentVI);

	vcr_update_statusbar();

	if (VCR_isPlaying())
	{
		extern int pauseAtFrame;
		if (m_currentSample >= pauseAtFrame && pauseAtFrame >= 0)
		{
			pauseEmu(TRUE); // maybe this is multithreading unsafe?
			pauseAtFrame = -1; // don't pause again
		}
	}

	vi_counter++;

	auto current_vi_time = std::chrono::high_resolution_clock::now();
	auto vi_time_diff = current_vi_time - last_vi_time;

	// if we're playing game normally with no frame advance or ff and overstepping max time between frames,
	// we need to sleep to compensate the additional time
	if (!fast_forward && !frame_advancing && vi_time_diff < max_vi_s_ms)
	{
		auto sleep_time = max_vi_s_ms - vi_time_diff;

		// if we just released fastforward we dont sleep at all
		// this fixes an old bug where game would sleep for too long after going out of ff mode
		if (ffup)
		{
			sleep_time = sleep_time.zero();
			counter_time = std::chrono::high_resolution_clock::now();
			ffup = false;
		}
		if (sleep_time.count() > 0 && sleep_time < std::chrono::milliseconds(700))
		{
			// we try to sleep for the overstepped time, but must account for sleeping inaccuracies
			auto goal_sleep = max_vi_s_ms - vi_time_diff - last_sleep_error;
			auto start_sleep = std::chrono::high_resolution_clock::now();
			std::this_thread::sleep_for(goal_sleep);
			auto end_sleep = std::chrono::high_resolution_clock::now();

			// sleeping inaccuracy is difference between actual time spent sleeping and the goal sleep
			// this value isnt usually too large
			last_sleep_error = end_sleep - start_sleep - goal_sleep;
		} else
		{
			// sleep time is unreasonable, log it and reset related state
			auto casted = std::chrono::duration_cast<
				std::chrono::milliseconds>(sleep_time).count();
			printf("Invalid timer: %lld ms\n", casted);
			sleep_time = sleep_time.zero();
			counter_time = std::chrono::high_resolution_clock::now();
			ffup = false;
		}
	}

	vi_s = (float)((double)vi_counter / std::chrono::duration<double>(
				current_vi_time - counter_time).count());


	if (current_vi_time - counter_time >= std::chrono::seconds(1))
	{
		// after releasing fast forward it's <1 for one second so we just hide that from user
		if (vi_s > 1)
		{
			if (Config.show_vis_per_second)
			{
				statusbar_send_text(std::format("VI/s: {:.0f}", vi_s), 3);
			}
		}
		counter_time = std::chrono::high_resolution_clock::now();
		vi_counter = 0;
	}


	last_vi_time = std::chrono::high_resolution_clock::now();
}
