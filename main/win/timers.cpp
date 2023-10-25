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

#include "features/Statusbar.hpp"

#ifdef _DEBUG
#include <iostream>
#endif

extern bool ffup;

static float VILimit = 60.0;
static auto VILimitMilliseconds = std::chrono::duration<double, std::milli>(1000.0 / 60.0);
float VIs, FPS;

int GetVILimit()
{
	if (ROM_HEADER == NULL) return 60;
	switch (ROM_HEADER->Country_code & 0xFF)
	{
	case 0x44:
	case 0x46:
	case 0x49:
	case 0x50:
	case 0x53:
	case 0x55:
	case 0x58:
	case 0x59:
		return 50;
	case 0x37:
	case 0x41:
	case 0x45:
	case 0x4a:
		return 60;
	default:
		return 60;
	}
}

void InitTimer()
{
	int temp = Config.fps_modifier;
	VILimit = (float)GetVILimit();
	VILimitMilliseconds = std::chrono::duration<double, std::milli>(
		1000.0 / (VILimit * (float)temp / 100));
	Translate("Speed Limit", TempMessage);
	sprintf(TempMessage, "%s: %d%%", TempMessage, temp); // TempMessage into itself?
	statusbar_send_text(std::string(TempMessage));
}


void new_frame()
{
	static std::chrono::high_resolution_clock::time_point prevTime;
	static std::chrono::high_resolution_clock::time_point
		lastStatusbarUpdateTime = std::chrono::high_resolution_clock::now();
	static int frameCount = 0;
	++frameCount;
	auto currentTime = std::chrono::high_resolution_clock::now();
	if (currentTime - lastStatusbarUpdateTime > std::chrono::seconds(1))
	{
		auto timeTaken = std::chrono::duration<double>(
			currentTime - lastStatusbarUpdateTime);
		char mes[100] = {0};
		sprintf(mes, "FPS: %.1f", frameCount / timeTaken.count());
		statusbar_send_text(std::string(mes), 2);
		lastStatusbarUpdateTime = std::chrono::high_resolution_clock::now();
		frameCount = 0;
	}

	prevTime = currentTime;
}

void new_frame_old()
{
	const DWORD CurrentFPSTime = timeGetTime();
	static DWORD LastFPSTime;
	static DWORD CounterTime;
	static int Fps_Counter = 0;

	if (!Config.show_fps) return;
	Fps_Counter++;

	if (CurrentFPSTime - CounterTime >= 1000)
	{
		char mes[100] = {0};
		FPS = (float)(Fps_Counter * 1000.0 / (CurrentFPSTime - CounterTime));
		sprintf(mes, "FPS: %.1f", FPS);
		statusbar_send_text(std::string(mes), 2);
		CounterTime = timeGetTime();
		Fps_Counter = 0;
	}

	LastFPSTime = CurrentFPSTime;
}

extern int m_currentVI;
extern long m_currentSample;

void new_vi()
{
	// fps wont update when emu is stuck so we must check vi/s
	// vi/s shouldn't go over 1000 in normal gameplay while holding down fast forward unless you have repeat speed at uzi speed
	if (!ignoreErrorEmulation && emu_launched && frame_advancing && VIs > 1000)
	{
		int result = 0;
		TaskDialog(mainHWND, app_hInstance, L"Error",
			L"Unusual core state", L"An emulator core timing inaccuracy or game crash has been detected. You can choose to continue emulation.", TDCBF_RETRY_BUTTON | TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, &result);

		if (result == IDCLOSE) {
			frame_advancing = false; //don't pause at next open
			CreateThread(NULL, 0, close_rom, (LPVOID)1, 0, 0);
		}
	}
	typedef std::chrono::high_resolution_clock::time_point timePoint;
	char mes[100] = {0};
	static timePoint LastFPSTime{}; //time point when previous VI happened
	static timePoint CounterTime{};
	//time point when last statusbar update happened
	static std::chrono::duration<double, std::nano> lastSleepError{};
	static unsigned int VI_Counter = 0;

	m_currentVI++;
	if (VCR_isRecording())
		VCR_setLengthVIs(m_currentVI/*VCR_getLengthVIs() + 1*/);

	// frame display / frame counter / framecount


	VCR_updateFrameCounter();
	if (VCR_isPlaying())
	{
		extern int pauseAtFrame;
		if (m_currentSample >= pauseAtFrame && pauseAtFrame >= 0)
		{
			pauseEmu(TRUE); // maybe this is multithreading unsafe?
			pauseAtFrame = -1; // don't pause again
		}
	}


	VI_Counter++;

	auto CurrentFPSTime = std::chrono::high_resolution_clock::now();
	//nanosecond precosion is kept up to the sleep

	auto Dif = CurrentFPSTime - LastFPSTime;
	if (!fast_forward && !frame_advancing)
	{
		if (Dif < VILimitMilliseconds)
		{
			auto time = VILimitMilliseconds - Dif;
			if (ffup)
			{
				time = time.zero();
				CounterTime = std::chrono::high_resolution_clock::now();
				ffup = false;
			}
			if (time.count() > 0 && time < std::chrono::milliseconds(700))
			//700ms
			{
				auto goalSleep = VILimitMilliseconds - Dif - lastSleepError;
				auto startSleep = std::chrono::high_resolution_clock::now();
				std::this_thread::sleep_for(goalSleep);
				auto endSleep = std::chrono::high_resolution_clock::now();
				lastSleepError = (endSleep - startSleep) - goalSleep;
#ifdef _DEBUG
				//auto miliError = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(lastSleepError);
				//std::cout << "Sleep error:" << miliError << std::endl;
#endif
			} else
			{
				auto casted = std::chrono::duration_cast<
					std::chrono::milliseconds>(time).count();
				printf("Invalid timer: %lld ms\n", casted);
				//reset values?
				time = time.zero();
				CounterTime = std::chrono::high_resolution_clock::now();
				ffup = false;
			}
		}
	}
	if (Config.show_vis_per_second)
	{
		if (CurrentFPSTime - CounterTime >= std::chrono::seconds(1))
		{
			//count is in nanoseconds, but statusbar in seconds
			VIs = (float)((double)VI_Counter / std::chrono::duration<double>(
				CurrentFPSTime - CounterTime).count());
			//std::cout << VI_Counter << " in " << std::chrono::duration<double>(CurrentFPSTime - CounterTime) << std::endl;
			if (VIs > 1) //if after fast forwarding pretend statusbar lagged idk
			{
				sprintf(mes, "VI/s: %.1f", VIs);
				statusbar_send_text(std::string(mes), Config.show_fps ? 3 : 2);
			}
			CounterTime = std::chrono::high_resolution_clock::now();
			VI_Counter = 0;
		}
	}
	LastFPSTime = std::chrono::high_resolution_clock::now();
}
