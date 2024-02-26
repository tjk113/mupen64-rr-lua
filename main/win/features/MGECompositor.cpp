#include "MGECompositor.h"
#include <Windows.h>

#include "messenger.h"
#include "Plugin.hpp"
#include "win/main_win.h"

namespace MGECompositor
{
	const auto class_name = "game_control";

	HWND parent_hwnd;
	HWND control_hwnd;
	long last_width = 0;
	long last_height = 0;
	long width = 0;
	long height = 0;
	void* video_buf = nullptr;

	RECT control_rect{};
	BITMAPINFO bmp_info{};

	LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		switch (msg)
		{
		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hwnd, &ps);

				StretchDIBits(hdc, control_rect.top, control_rect.left, control_rect.right, control_rect.bottom, 0, 0, width, height, video_buf,
							  &bmp_info, DIB_RGB_COLORS, SRCCOPY);

				EndPaint(hwnd, &ps);
				return 0;
			}
		}
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	void create(HWND hwnd)
	{
		parent_hwnd = hwnd;
		control_hwnd = CreateWindow(class_name, "", WS_CHILD | WS_VISIBLE, 0, 0, 1, 1, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
	}

	void init()
	{
		WNDCLASS wndclass = {0};
		wndclass.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc = (WNDPROC)wndproc;
		wndclass.hInstance = GetModuleHandle(nullptr);
		wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndclass.lpszClassName = class_name;
		RegisterClass(&wndclass);

		Messenger::subscribe(Messenger::Message::EmuLaunchedChanged, [](std::any data)
		{
			auto value = std::any_cast<bool>(data);
			ShowWindow(control_hwnd, (value && get_video_size && read_video) ? SW_SHOW : SW_HIDE);
		});
	}

	void update_screen()
	{
		get_video_size(&width, &height);

		if (width != last_width || height != last_height)
		{
			printf("MGE Compositor: Video size %dx%d\n", width, height);
			free(video_buf);
			video_buf = malloc(width * height * 3);

			bmp_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmp_info.bmiHeader.biWidth = width;
			bmp_info.bmiHeader.biHeight = height;
			bmp_info.bmiHeader.biPlanes = 1;
			bmp_info.bmiHeader.biBitCount = 24;
			bmp_info.bmiHeader.biCompression = BI_RGB;
			MoveWindow(control_hwnd, 0, 0, width, height, true);
			GetClientRect(control_hwnd, &control_rect);
		}

		read_video(&video_buf);

		last_width = width;
		last_height = height;

		InvalidateRect(control_hwnd, &control_rect, false);
	}
}
