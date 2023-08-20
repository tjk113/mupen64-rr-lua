#include <Windows.h>

#include "win/main_win.h"
#include "ffmpeg_capture.hpp"
#include "plugin.hpp"

SWindowInfo gSInfo{};
BITMAPINFO gBMPInfo{}; //Needed for GetDIBits

static unsigned char* buffer = NULL;
static unsigned int bufferSize = 0;

/// <summary>
/// Call this after finishing capture
/// </summary>
void FFMpegCleanup() {
	DllCrtFree(buffer);
	bufferSize = 0;
}

void PrepareBitmapHeader(HWND hMain, HBITMAP bitmap) {
	HDC hdc = GetDC(hMain);
	gBMPInfo.bmiHeader.biSize = sizeof(gBMPInfo.bmiHeader);
	GetDIBits(hdc, bitmap, 0, 0, NULL, &gBMPInfo, DIB_RGB_COLORS);
	gBMPInfo.bmiHeader.biCompression = BI_RGB; //idk if needed
	gBMPInfo.bmiHeader.biBitCount = 24;
	gBMPInfo.bmiHeader.biHeight = -gBMPInfo.bmiHeader.biHeight; // to get upside down output

}

//based on original one for AVI
void FFMpegReadScreen(void** dest, long* width, long* height) {
	HDC mupendc, all, copy; //all - screen; copy - buffer
	//RECT rect, rectS, rectT;
	POINT cli_tl{0, 0}; //mupen client x y 
	HBITMAP bitmap, oldbitmap;

	*width = gSInfo.width;
	*height = gSInfo.height;

	mupendc = GetDC(mainHWND);  //only client area
	if (Config.is_capture_cropped_screen_dc) {
		//get whole screen dc and find out where is mupen's client area
		all = GetDC(NULL);
		ClientToScreen(mainHWND, &cli_tl);
	}

	// copy to a context in memory to speed up process
	copy = CreateCompatibleDC(mupendc);
	bitmap = CreateCompatibleBitmap(mupendc, *width, *height);
	oldbitmap = (HBITMAP)SelectObject(copy, bitmap);
	Sleep(0);
	if (copy) {
		if (Config.is_capture_cropped_screen_dc)
			BitBlt(copy, 0, 0, *width, *height, all, cli_tl.x, cli_tl.y + gSInfo.toolbarHeight + (gSInfo.height - *height), SRCCOPY);
		else
			BitBlt(copy, 0, 0, *width, *height, mupendc, 0, gSInfo.toolbarHeight + (gSInfo.height - *height), SRCCOPY);
	}

	if (!copy || !bitmap) {
		MessageBox(0, "Unexpected AVI error 1", "Error", MB_ICONERROR);
		*dest = NULL;
		SelectObject(copy, oldbitmap); //apparently this leaks 1 pixel bitmap if not used
		if (bitmap)
			DeleteObject(bitmap);
		if (copy)
			DeleteDC(copy);
		if (mupendc)
			ReleaseDC(mainHWND, mupendc);
		return;
	}

	// read the context
	if (!buffer || bufferSize < *width * *height * 3 + 1) //if buffer doesn't exist yet or changed size somehow
	{
		if (buffer) free(buffer);
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
		return;
	}

	if (gBMPInfo.bmiHeader.biSize == 0)
		PrepareBitmapHeader(mainHWND, bitmap);
	if (auto res = GetDIBits(copy, bitmap, 0, *height, buffer, &gBMPInfo, DIB_RGB_COLORS) == 0) {
		printf("GetDIBits error\n");
	}

	*dest = buffer;
	SelectObject(copy, oldbitmap);
	if (bitmap)
		DeleteObject(bitmap);
	if (copy)
		DeleteDC(copy);
	if (mupendc)
		ReleaseDC(mainHWND, mupendc);
}

void InitReadScreenFFmpeg(const SWindowInfo& info) {
#ifdef __WIN32__
	printf((readScreen != NULL) ? (char*)"ReadScreen is implemented by this graphics plugin.\n" : (char*)"ReadScreen not implemented by this graphics plugin (or was forcefully disabled in settings) - substituting...\n");
#endif	

	if (readScreen == NULL) {
		readScreen = FFMpegReadScreen;
		gSInfo = info;
	}
}
