/**
 * Mupen64 - vcr_compress.c
 * Copyright (C) 2002 Hacktarux
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 *
 * If you want to contribute to the project please contact
 * me first (maybe someone is already making what you are
 * planning to do).
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
 * You should have received a copy of the GNU General Public
 * Licence along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
**/

#include <Windows.h>
#include <Vfw.h>
#include <cstdio>
#include "../win/main_win.h"
#include "AVIComp.h"
#include "../win/Config.hpp"
#include "../Plugin.hpp"
#include "../win/features/Statusbar.hpp"

const auto max_avi_size = 0x7B9ACA00;

static AVICOMPRESSOPTIONS* avi_options = new AVICOMPRESSOPTIONS();

AVIComp::VideoParams video_params{};

size_t splits = 0;
bool splitting = false;

bool avi_opened = false;
int frame;
BITMAPINFOHEADER infoHeader;
PAVIFILE avi_file;
AVISTREAMINFO video_stream_header;
PAVISTREAM video_stream;
PAVISTREAM compressed_video_stream;
int sample;
unsigned int AVIFileSize;
WAVEFORMATEX sound_format;
AVISTREAMINFO sound_stream_header;
PAVISTREAM sound_stream;

bool load_options()
{
	FILE* f = fopen("avi.cfg", "rb");

	if (!f)
		return false;

	fseek(f, 0, SEEK_END);

	// Too small...
	if (ftell(f) < sizeof(AVICOMPRESSOPTIONS))
		goto error;

	fseek(f, 0, SEEK_SET);

	fread(avi_options, sizeof(AVICOMPRESSOPTIONS), 1, f);

	{
		void* moreOptions = malloc(avi_options->cbParms);
		fread(moreOptions, avi_options->cbParms, 1, f);
		avi_options->lpParms = moreOptions;
	}

	fclose(f);
	return true;
error:
	fclose(f);
	return false;
}

void save_options()
{
	FILE* f = fopen("avi.cfg", "wb");
	fwrite(avi_options, sizeof(AVICOMPRESSOPTIONS), 1, f);
	fwrite(avi_options->lpParms, avi_options->cbParms, 1, f);
	fclose(f);
}

// "internal" readScreen, used when plugin doesn't implement it
void __cdecl vcrcomp_internal_read_screen(void** dest, long* width,
                                          long* height)
{
	HDC mupendc, all = nullptr, copy; //all - screen; copy - buffer
	POINT cli_tl{0, 0}; //mupen client x y
	HBITMAP bitmap, oldbitmap;

	if (Config.capture_delay)
	{
		Sleep(Config.capture_delay);
	}

	mupendc = GetDC(mainHWND); //only client area
	if (Config.is_capture_cropped_screen_dc)
	{
		//get whole screen dc and find out where is mupen's client area
		all = GetDC(NULL);
		ClientToScreen(mainHWND, &cli_tl);
	}

	auto window_info = get_window_info();
	//real width and height of emulated area must be a multiple of 4, which is apparently important for avi
	*width = window_info.width & ~3;
	*height = window_info.height & ~3;

	// copy to a context in memory to speed up process
	copy = CreateCompatibleDC(mupendc);
	bitmap = CreateCompatibleBitmap(mupendc, *width, *height);
	oldbitmap = (HBITMAP)SelectObject(copy, bitmap);

	if (copy)
	{
		if (Config.is_capture_cropped_screen_dc)
			BitBlt(copy, 0, 0, *width, *height, all, cli_tl.x,
			       cli_tl.y + (window_info.height - *height), SRCCOPY);
		else
			BitBlt(copy, 0, 0, *width, *height, mupendc, 0,
			       (window_info.height - *height), SRCCOPY);
	}

	if ((!avi_opened || !copy || !bitmap))
	{
		if (!*dest) //don't show warning, this is initialisation phase
			MessageBox(0, "Unexpected AVI error 1", "Error", MB_ICONERROR);
		*dest = NULL;
		SelectObject(copy, oldbitmap);
		//apparently this leaks 1 pixel bitmap if not used
		if (bitmap)
			DeleteObject(bitmap);
		if (copy)
			DeleteDC(copy);
		if (mupendc)
			ReleaseDC(mainHWND, mupendc);
		if (all)
			ReleaseDC(NULL, all);
		return;
	}

	// read the context
	static unsigned char* buffer = NULL;
	static unsigned int bufferSize = 0;
	if (!buffer || bufferSize < *width * *height * 3 + 1)
	//if buffer doesn't exist yet or changed size somehow
	{
		free(buffer);
		bufferSize = *width * *height * 3 + 1;
		buffer = (unsigned char*)malloc(bufferSize);
	}

	if (!buffer) //failed to alloc
	{
		MessageBox(0, "Failed to allocate memory for buffer", "Error",
		           MB_ICONERROR);
		*dest = NULL;
		SelectObject(copy, oldbitmap);
		if (bitmap)
			DeleteObject(bitmap);
		if (copy)
			DeleteDC(copy);
		if (mupendc)
			ReleaseDC(mainHWND, mupendc);
		if (all)
			ReleaseDC(NULL, all);
		return;
	}

	BITMAPINFO bmpinfos;
	memcpy(&bmpinfos.bmiHeader, &infoHeader, sizeof(BITMAPINFOHEADER));
	//copy info from avi file init
	GetDIBits(copy, bitmap, 0, *height, buffer, &bmpinfos, DIB_RGB_COLORS);

	*dest = buffer;
	SelectObject(copy, oldbitmap);
	if (bitmap)
		DeleteObject(bitmap);
	if (copy)
		DeleteDC(copy);
	if (mupendc)
		ReleaseDC(mainHWND, mupendc);
	if (all)
		ReleaseDC(NULL, all);
}

bool AVIComp::add_video_data(uint8_t* data)
{
	if (AVIFileSize > max_avi_size)
	{
		// If AVI file gets too big, it will corrupt or crash. Since this is a limitation the caller shouldn't care about, we split the recording silently.
		// When splitting, filenames follow this pattern: <fname> <n>.avi
		VideoParams new_params = video_params;
		new_params.path = with_name(new_params.path,
		                            get_name(video_params.path) + " " +
		                            std::to_string(++splits));

		splitting = true;
		AVIComp::stop();
		AVIComp::start(new_params, false);
		splitting = false;
	}
	LONG written_len;
	BOOL ret = AVIStreamWrite(compressed_video_stream, frame++, 1, data,
	                          infoHeader.biSizeImage, AVIIF_KEYFRAME, NULL,
	                          &written_len);
	AVIFileSize += written_len;
	return !ret;
}

bool AVIComp::add_audio_data(uint8_t* data, size_t len)
{
	BOOL ok = (0 == AVIStreamWrite(sound_stream, sample,
	                               len / sound_format.nBlockAlign, data, len, 0,
	                               NULL, NULL));
	sample += len / sound_format.nBlockAlign;
	AVIFileSize += len;
	return ok;
}

AVIComp::Result AVIComp::start(VideoParams params,
                               bool show_codec_picker)
{
	if (!splitting)
	{
		video_params = params;
	}
	avi_opened = true;
	AVIFileSize = 0;
	frame = 0;
	infoHeader.biSize = sizeof(BITMAPINFOHEADER);
	infoHeader.biWidth = params.width;
	infoHeader.biHeight = params.height;
	infoHeader.biPlanes = 1;
	infoHeader.biBitCount = 24;
	infoHeader.biCompression = BI_RGB;
	infoHeader.biSizeImage = params.width * params.height * 3;
	infoHeader.biXPelsPerMeter = 0;
	infoHeader.biYPelsPerMeter = 0;
	infoHeader.biClrUsed = 0;
	infoHeader.biClrImportant = 0;

	AVIFileInit();
	AVIFileOpen(&avi_file, params.path.string().c_str(), OF_WRITE | OF_CREATE,
	            NULL);

	ZeroMemory(&video_stream_header, sizeof(AVISTREAMINFO));
	video_stream_header.fccType = streamtypeVIDEO;
	video_stream_header.dwScale = 1;
	video_stream_header.dwRate = params.fps;
	video_stream_header.dwSuggestedBufferSize = 0;
	AVIFileCreateStream(avi_file, &video_stream, &video_stream_header);

	if (show_codec_picker)
	{
		if (!AVISaveOptions(mainHWND, 0, 1, &video_stream, &avi_options))
		{
			return Result::Cancelled;
		}
	} else
	{
		if (!load_options())
		{
			return Result::InvalidPreset;
		}
	}

	save_options();
	AVIMakeCompressedStream(&compressed_video_stream, video_stream,
	                        avi_options, NULL);
	AVIStreamSetFormat(compressed_video_stream, 0, &infoHeader,
	                   infoHeader.biSize + infoHeader.biClrUsed * sizeof(
		                   RGBQUAD));

	sample = 0;
	sound_format.wFormatTag = WAVE_FORMAT_PCM;
	sound_format.nChannels = 2;
	sound_format.nSamplesPerSec = 44100;
	sound_format.nAvgBytesPerSec = 44100 * (2 * 16 / 8);
	sound_format.nBlockAlign = 2 * 16 / 8;
	sound_format.wBitsPerSample = 16;
	sound_format.cbSize = 0;

	ZeroMemory(&sound_stream_header, sizeof(AVISTREAMINFO));
	sound_stream_header.fccType = streamtypeAUDIO;
	sound_stream_header.dwQuality = (DWORD)-1;
	sound_stream_header.dwScale = sound_format.nBlockAlign;
	sound_stream_header.dwInitialFrames = 1;
	sound_stream_header.dwRate = sound_format.nAvgBytesPerSec;
	sound_stream_header.dwSampleSize = sound_format.nBlockAlign;
	AVIFileCreateStream(avi_file, &sound_stream, &sound_stream_header);
	AVIStreamSetFormat(sound_stream, 0, &sound_format, sizeof(WAVEFORMATEX));
	return Result::Ok;
}

AVIComp::Result AVIComp::stop()
{
	AVIStreamClose(compressed_video_stream);
	AVIStreamRelease(video_stream);
	AVIStreamClose(sound_stream);
	AVIFileClose(avi_file);
	AVIFileExit();

	avi_opened = false;
	return Result::Ok;
}
