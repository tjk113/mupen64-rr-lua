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
#ifndef _WIN32_IE
#define _WIN32_IE 0x0500
#endif
#include <commctrl.h>
#include "../guifuncs.h"
#include "main_win.h"
#include "../rom.h"
#include "translation.h"
#include "../vcr.h"
#include "../../winproject/resource.h"
#include <win/CrashHandlerDialog.h>

//c++!!!
#include <chrono>
#include <thread>

#ifdef _DEBUG
#include <iostream>
#endif

extern bool ffup;

static float VILimit = 60.0;
static auto VILimitMilliseconds = std::chrono::duration<double, std::milli>(1000.0 / 60.0);
float VIs, FPS;

int GetVILimit() {
	if (ROM_HEADER == NULL) return 60;
	switch (ROM_HEADER->Country_code & 0xFF) {
		case 0x44:
		case 0x46:
		case 0x49:
		case 0x50:
		case 0x53:
		case 0x55:
		case 0x58:
		case 0x59:
			return 50;
			break;
		case 0x37:
		case 0x41:
		case 0x45:
		case 0x4a:
			return 60;
			break;
		default:
			return 60;
	}
}

void InitTimer() {
	int temp;
	VILimit = GetVILimit();
	if (Config.is_fps_modifier_enabled) {
		temp = Config.fps_modifier;
	} else {
		temp = 100;
	}
	VILimitMilliseconds = std::chrono::duration<double, std::milli>(1000.0 / (VILimit * temp / 100));
	Translate("Speed Limit", TempMessage);
	sprintf(TempMessage, "%s: %d%%", TempMessage, temp);
	SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)TempMessage);
}


void new_frame() {
	char mes[32];
	static std::chrono::high_resolution_clock::time_point prevTime;
	static std::chrono::high_resolution_clock::time_point lastStatusbarUpdateTime = std::chrono::high_resolution_clock::now();
	static int frameCount = 0;
	++frameCount;
	auto currentTime = std::chrono::high_resolution_clock::now();
	if (currentTime - lastStatusbarUpdateTime > std::chrono::seconds(1)) {
		auto timeTaken = std::chrono::duration<double>(currentTime - lastStatusbarUpdateTime);
		sprintf(mes, "FPS: %.1f", frameCount / timeTaken.count());
		SendMessage(hStatus, SB_SETTEXT, 2, (LPARAM)mes);
		lastStatusbarUpdateTime = std::chrono::high_resolution_clock::now();
		frameCount = 0;
	}

	prevTime = currentTime;
}

void new_frame_old() {

	DWORD CurrentFPSTime;
	char mes[100];
	static DWORD LastFPSTime;
	static DWORD CounterTime;
	static int Fps_Counter = 0;

	if (!Config.show_fps) return;
	Fps_Counter++;

	CurrentFPSTime = timeGetTime();

	if (CurrentFPSTime - CounterTime >= 1000) {
		FPS = (float)(Fps_Counter * 1000.0 / (CurrentFPSTime - CounterTime));
		sprintf(mes, "FPS: %.1f", FPS);
		SendMessage(hStatus, SB_SETTEXT, 2, (LPARAM)mes);
		CounterTime = timeGetTime();
		Fps_Counter = 0;
	}

	LastFPSTime = CurrentFPSTime;
}

extern int m_currentVI;
extern long m_currentSample;

void new_vi() {
	extern DWORD WINAPI closeRom(LPVOID lpParam);
	extern int frame_advancing;

	// fps wont update when emu is stuck so we must check vi/s
	// vi/s shouldn't go over 1000 in normal gameplay while holding down fast forward unless you have repeat speed at uzi speed
	if (!ignoreErrorEmulation && emu_launched && frame_advancing && VIs > 1000) {
		CrashHandlerDialog crashHandlerDialog(CrashHandlerDialog::Types::Ignorable, "An emulator core timing inaccuracy or game crash has been detected.\nPlease choose a way to proceed.");
		auto result = crashHandlerDialog.Show();

		if (result == CrashHandlerDialog::Choices::Exit) {
			frame_advancing = false; //don't pause at next open
			CreateThread(NULL, 0, closeRom, (LPVOID)1, 0, 0);
		}

	}
	typedef std::chrono::high_resolution_clock::time_point timePoint;
	char mes[100];
	static timePoint LastFPSTime{}; //time point when previous VI happened
	static timePoint CounterTime{}; //time point when last statusbar update happened
	static std::chrono::duration<double, std::nano> lastSleepError{};
	static unsigned int VI_Counter = 0;

	m_currentVI++;
	if (VCR_isRecording())
		VCR_setLengthVIs(m_currentVI/*VCR_getLengthVIs() + 1*/);

// frame display / frame counter / framecount
#ifndef __WIN32__   
	if ((m_currentVI % 10) == 0)
		VCR_updateFrameCounter();
#else
	extern int frame_advancing;
	extern BOOL manualFPSLimit;
	if (!Config.is_statusbar_frequent_refresh_enabled) {
		if ((frame_advancing || (m_currentVI % (manualFPSLimit ? 10 : 80)) == 0)) {
			VCR_updateFrameCounter();
		}
	} else
		VCR_updateFrameCounter();

	if (VCR_isPlaying()) {
		extern int pauseAtFrame;
		if (m_currentSample >= pauseAtFrame && pauseAtFrame >= 0) {
			extern void pauseEmu(BOOL quiet);
			pauseEmu(TRUE); // maybe this is multithreading unsafe?
			pauseAtFrame = -1; // don't pause again
		}
	}
#endif

	if ((!Config.show_vis_per_second) && (!Config.is_fps_limited)) return;
	VI_Counter++;

	auto CurrentFPSTime = std::chrono::high_resolution_clock::now(); //nanosecond precosion is kept up to the sleep

	auto Dif = CurrentFPSTime - LastFPSTime;
	if (Config.is_fps_limited && manualFPSLimit && !frame_advancing
	#ifdef LUA_SPEEDMODE
		&& !maximumSpeedMode
	#endif
		) {
		if (Dif < VILimitMilliseconds) {
			auto time = VILimitMilliseconds - Dif;
			if (ffup) {
				time = time.zero(); CounterTime = std::chrono::high_resolution_clock::now(); ffup = false;
			}
			if (time.count() > 0 && time < std::chrono::milliseconds(700)) //700ms
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
			} else {
				auto casted = std::chrono::duration_cast<std::chrono::milliseconds>(time).count();
				printf("Invalid timer: %lld ms\n", casted);
				//reset values?
				time = time.zero(); CounterTime = std::chrono::high_resolution_clock::now(); ffup = false;
			}
		}
	}
	if (Config.show_vis_per_second) {
		if (CurrentFPSTime - CounterTime >= std::chrono::seconds(1)) {
			//count is in nanoseconds, but statusbar in seconds
			VIs = VI_Counter / std::chrono::duration<double>(CurrentFPSTime - CounterTime).count();
			//std::cout << VI_Counter << " in " << std::chrono::duration<double>(CurrentFPSTime - CounterTime) << std::endl;
			if (VIs > 1) //if after fast forwarding pretend statusbar lagged idk
			{
				sprintf(mes, "VI/s: %.1f", VIs);
				if (Config.show_fps) SendMessage(hStatus, SB_SETTEXT, 3, (LPARAM)mes);
				else SendMessage(hStatus, SB_SETTEXT, 2, (LPARAM)mes);
			}
			CounterTime = std::chrono::high_resolution_clock::now();
			VI_Counter = 0;
		}
	}
	LastFPSTime = std::chrono::high_resolution_clock::now();
}
