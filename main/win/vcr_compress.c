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

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "../win/main_win.h"
#include "../vcr_compress.h"
#include "Config.hpp"
#include "../plugin.hpp"
#include "../../winproject/resource.h"
#include "vfw.h"
#include "features/Toolbar.hpp"
#include "features/Statusbar.hpp"


static int avi_opened = 0;
extern int recording;

static int frame;
static BITMAPINFOHEADER infoHeader;
static PAVIFILE avi_file;
static AVISTREAMINFO video_stream_header;
static PAVISTREAM video_stream;
static PAVISTREAM compressed_video_stream;
static AVICOMPRESSOPTIONS video_options;
static AVICOMPRESSOPTIONS* pvideo_options[1];

static int sample;
static unsigned int AVIFileSize;
static WAVEFORMATEX sound_format;
static AVISTREAMINFO sound_stream_header;
static PAVISTREAM sound_stream;

t_window_info vcrcomp_window_info = {0};

void get_window_info(HWND hwnd, t_window_info& info)
{
    RECT client_rect = {0}, statusbar_rect = {0}, toolbar_rect = {0};

    GetClientRect(hwnd, &client_rect);

    // full client dimensions (including statusbar and toolbar)
    info.width = client_rect.right - client_rect.left;
    info.height = client_rect.bottom - client_rect.top;

    if (toolbar_hwnd)
        GetClientRect(toolbar_hwnd, &toolbar_rect);
    info.toolbar_height = toolbar_rect.bottom - toolbar_rect.top;
    
    if (statusbar_hwnd)
        GetClientRect(statusbar_hwnd, &statusbar_rect);
    info.statusbar_height = statusbar_rect.bottom - statusbar_rect.top;

    //subtract size of toolbar and statusbar from buffer dimensions
    //if video plugin knows about this, whole game screen should be captured. Most of the plugins do.
    info.height -= info.toolbar_height + info.statusbar_height;
}

// "internal" readScreen, used when plugin doesn't implement it
void __cdecl vcrcomp_internal_read_screen(void** dest, long* width, long* height)
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

    //real width and height of emulated area must be a multiple of 4, which is apparently important for avi
    *width = vcrcomp_window_info.width & ~3;
    *height = vcrcomp_window_info.height & ~3;

    // copy to a context in memory to speed up process
    copy = CreateCompatibleDC(mupendc);
    bitmap = CreateCompatibleBitmap(mupendc, *width, *height);
    oldbitmap = (HBITMAP)SelectObject(copy, bitmap);
    
    if (copy)
    {
        if (Config.is_capture_cropped_screen_dc)
            BitBlt(copy, 0, 0, *width, *height, all, cli_tl.x,
                   cli_tl.y + vcrcomp_window_info.toolbar_height + (vcrcomp_window_info.height - *height), SRCCOPY);
        else
            BitBlt(copy, 0, 0, *width, *height, mupendc, 0, vcrcomp_window_info.toolbar_height + (vcrcomp_window_info.height - *height), SRCCOPY);
    }

    if ((!avi_opened || !copy || !bitmap))
    {
        if (!*dest) //don't show warning, this is initialisation phase
            MessageBox(0, "Unexpected AVI error 1", "Error", MB_ICONERROR);
        *dest = NULL;
        SelectObject(copy, oldbitmap); //apparently this leaks 1 pixel bitmap if not used
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
    if (!buffer || bufferSize < *width * *height * 3 + 1) //if buffer doesn't exist yet or changed size somehow
    {
        free(buffer);
        bufferSize = *width * *height * 3 + 1;
        buffer = (unsigned char*)malloc(bufferSize);
    }

    if (!buffer) //failed to alloc
    {
        MessageBox(0, "Failed to allocate memory for buffer", "Error", MB_ICONERROR);
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
    memcpy(&bmpinfos.bmiHeader, &infoHeader, sizeof(BITMAPINFOHEADER)); //copy info from avi file init
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

void VCRComp_init()
{
}


BOOL VCRComp_addVideoFrame(unsigned char* data)
{
    extern int m_task;
    //if (m_task == 4 || m_task == 5) return 1; //don't record frames that are during emu loading (black frames)
    long int TempLen;
    BOOL ret = AVIStreamWrite(compressed_video_stream, frame++, 1, data, infoHeader.biSizeImage, AVIIF_KEYFRAME, NULL,
                              &TempLen);
    AVIFileSize += TempLen;
    return (0 == ret);
}

BOOL VCRComp_addAudioData(unsigned char* data, int len)
{
    BOOL ok = (0 == AVIStreamWrite(sound_stream, sample, len / sound_format.nBlockAlign, data, len, 0, NULL, NULL));
    sample += len / sound_format.nBlockAlign;
    AVIFileSize += len;
    return ok;
}

bool VRComp_loadOptions()
{
    SetCurrentDirectory(app_path.c_str());
    FILE* f = fopen("avi.cfg", "rb");
    if (f == NULL) return false; //file doesn't exist

    fseek(f, 0, SEEK_END);
    if (ftell(f) == 0) goto error; //empty file
    fseek(f, 0, SEEK_SET);

    pvideo_options[0] = (AVICOMPRESSOPTIONS*)malloc(sizeof(AVICOMPRESSOPTIONS));
    fread(pvideo_options[0], sizeof(AVICOMPRESSOPTIONS), 1, f);

    {
        void* moreOptions = malloc(pvideo_options[0]->cbParms);
        fread(moreOptions, pvideo_options[0]->cbParms, 1, f);
        pvideo_options[0]->lpParms = moreOptions;
    }
    fclose(f);
    return true;
error:
    fclose(f);
    return false;
}

void VRComp_saveOptions()
{
    SetCurrentDirectory(app_path.c_str());
    FILE* f = fopen("avi.cfg", "w");
    fwrite(pvideo_options[0], sizeof(AVICOMPRESSOPTIONS), 1, f);
    fwrite(pvideo_options[0]->lpParms, pvideo_options[0]->cbParms, 1, f);
    fflush(f);
    fclose(f);
}

void VCRComp_startFile(const char* filename, long width, long height, int fps, int New)
{
    avi_opened = 1;
    AVIFileSize = 0;
    frame = 0;
    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = width;
    infoHeader.biHeight = height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = BI_RGB;
    infoHeader.biSizeImage = width * height * 3;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biClrUsed = 0;
    infoHeader.biClrImportant = 0;

    AVIFileInit();
    AVIFileOpen(&avi_file, filename, OF_WRITE | OF_CREATE, NULL);

    ZeroMemory(&video_stream_header, sizeof(AVISTREAMINFO));
    video_stream_header.fccType = streamtypeVIDEO;
    video_stream_header.dwScale = 1;
    video_stream_header.dwRate = fps;
    video_stream_header.dwSuggestedBufferSize = 0;
    AVIFileCreateStream(avi_file, &video_stream, &video_stream_header);

    if (!New && pvideo_options[0] == NULL) //if hide dialog and options not found
    {
        New = !VRComp_loadOptions(); //if failed to load defaults, ask with dialog
    }
    if (New)
    {
        ZeroMemory(&video_options, sizeof(AVICOMPRESSOPTIONS));
        pvideo_options[0] = &video_options;
        extern bool captureMarkedStop;
        captureMarkedStop = !AVISaveOptions(mainHWND, 0, 1, &video_stream, pvideo_options);
    }
    VRComp_saveOptions();
    AVIMakeCompressedStream(&compressed_video_stream, video_stream, pvideo_options[0], NULL);
    AVIStreamSetFormat(compressed_video_stream, 0, &infoHeader,
                       infoHeader.biSize + infoHeader.biClrUsed * sizeof(RGBQUAD));

    // sound
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

    /*ZeroMemory(&sound_options, sizeof(AVICOMPRESSOPTIONS));
    psound_options[0] = &sound_options;
    AVISaveOptions(mainHWND, 0, 1, &sound_stream, psound_options);
    AVIMakeCompressedStream(&compressed_sound_stream, sound_stream, &sound_options, NULL);
    AVIStreamSetFormat(compressed_sound_stream, 0, &sound_format, sizeof(WAVEFORMATEX));*/
}

void VCRComp_finishFile(int split)
{
    //SetWindowPos(mainHWND, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE); // why is this being set when recording is stopped...
    AVIStreamClose(compressed_video_stream);
    AVIStreamRelease(video_stream);
    // AVIStreamClose(compressed_video_stream);
    AVIStreamClose(sound_stream);
    AVIFileClose(avi_file);
    AVIFileExit();
    if (!split)
    {
        HMENU hMenu;
        hMenu = GetMenu(mainHWND);
        EnableMenuItem(hMenu, ID_END_CAPTURE, MF_GRAYED);
        EnableMenuItem(hMenu, ID_START_CAPTURE, MF_ENABLED);
        EnableMenuItem(hMenu, ID_START_CAPTURE_PRESET, MF_ENABLED);
        EnableMenuItem(hMenu, FULL_SCREEN, MF_ENABLED); //Enables fullscreen menu
        SetWindowPos(mainHWND, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE); //Remove the always on top flag
    }

    avi_opened = 0;
    printf("[VCR]: Finished AVI capture.\n");
}

unsigned int VCRComp_GetSize()
{
    return AVIFileSize;
}
