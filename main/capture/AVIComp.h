#pragma once
#include <Windows.h>

typedef struct
{
	long width;
	long height;
	long toolbar_height;
	long statusbar_height;
} t_window_info;

extern t_window_info vcrcomp_window_info;

void VCRComp_init();

void VCRComp_startFile(const char* filename, long width, long height, int fps,
                       int New);
void VCRComp_finishFile();
BOOL VCRComp_addVideoFrame(unsigned char* data);
BOOL VCRComp_addAudioData(unsigned char* data, int len);

unsigned int VCRComp_GetSize();

void get_window_info(HWND hwnd, t_window_info& info);

/**
 * \brief Writes the emulator's current emulation front buffer into the destination buffer
 * \param dest The buffer holding video data of size <c>width * height</c>
 * \param width The buffer's width
 * \param height The buffer's height
 */
void __cdecl vcrcomp_internal_read_screen(void** dest, long* width, long* height);
