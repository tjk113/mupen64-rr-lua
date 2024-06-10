#pragma once
#include <Windows.h>
#include <Windowsx.h>
#include <CommCtrl.h>
#include <Shlwapi.h>
#include <cstdint>
#include <chrono>
#include <gdiplus.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>
#include <functional>
#include <queue>
#include <gdiplus.h>
#include <d2d1.h>
#include <d2d1_3.h>
#include <d2d1helper.h>
#include <dcomp.h>
#include <d3d11.h>
#include <dwrite.h>
#include <wincodec.h>
#include <thread>

static void set_checkbox_state(const HWND hwnd, const int32_t id,
                               int32_t is_checked)
{
	SendMessage(GetDlgItem(hwnd, id), BM_SETCHECK,
	            is_checked ? BST_CHECKED : BST_UNCHECKED, 0);
}

static int32_t get_checkbox_state(const HWND hwnd, const int32_t id)
{
	return SendDlgItemMessage(hwnd, id, BM_GETCHECK, 0, 0) == BST_CHECKED
		       ? TRUE
		       : FALSE;
}

static int32_t get_primary_monitor_refresh_rate()
{
	DISPLAY_DEVICE dd;
	dd.cb = sizeof(dd);
	EnumDisplayDevices(NULL, 0, &dd, 0);

	DEVMODE dm;
	dm.dmSize = sizeof(dm);
	EnumDisplaySettings(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm);

	return dm.dmDisplayFrequency;
}

static void read_combo_box_value(const HWND hwnd, const int resource_id, char* ret)
{
	int index = SendDlgItemMessage(hwnd, resource_id, CB_GETCURSEL, 0, 0);
	SendDlgItemMessage(hwnd, resource_id, CB_GETLBTEXT, index, (LPARAM)ret);
}

/**
 * \brief Accurately sleeps for the specified amount of time
 * \param seconds The seconds to sleep for
 * \remarks https://blat-blatnik.github.io/computerBear/making-accurate-sleep-function/
 */
static void accurate_sleep(double seconds)
{
	using namespace std;
	using namespace std::chrono;

	static double estimate = 5e-3;
	static double mean = 5e-3;
	static double m2 = 0;
	static int64_t count = 1;

	while (seconds > estimate)
	{
		auto start = high_resolution_clock::now();
		this_thread::sleep_for(milliseconds(1));
		auto end = high_resolution_clock::now();

		double observed = (end - start).count() / 1e9;
		seconds -= observed;

		++count;
		double delta = observed - mean;
		mean += delta / count;
		m2 += delta * (observed - mean);
		double stddev = sqrt(m2 / (count - 1));
		estimate = mean + stddev;
	}

	// spin lock
	auto start = high_resolution_clock::now();
	while ((high_resolution_clock::now() - start).count() / 1e9 < seconds);
}


static RECT get_window_rect_client_space(HWND parent, HWND child)
{
	RECT offset_client = {0};
	MapWindowRect(child, parent, &offset_client);

	RECT client = {0};
	GetWindowRect(child, &client);

	return {
		offset_client.left,
		offset_client.top,
		offset_client.left + (client.right - client.left),
		offset_client.top + (client.bottom - client.top)
	};
}

static bool create_composition_surface(HWND hwnd, D2D1_SIZE_U size, IDXGIFactory2** factory, IDXGIAdapter1** dxgiadapter, ID3D11Device** d3device,
                                       IDXGIDevice1** dxdevice, ID2D1Bitmap1** bitmap, IDCompositionVisual** comp_visual, IDCompositionDevice** comp_device,
                                       IDCompositionTarget** comp_target, IDXGISwapChain1** swapchain, ID2D1Factory3** d2d_factory, ID2D1Device2** d2d_device,
                                       ID3D11DeviceContext** d3d_dc, ID2D1DeviceContext2** d2d_dc, IDXGISurface1** dxgi_surface,
                                       ID3D11Resource** dxgi_surface_resource, ID3D11Resource** front_buffer, ID3D11Texture2D** d3d_gdi_tex)
{
	CreateDXGIFactory2(0, IID_PPV_ARGS(factory));

	(*factory)->EnumAdapters1(0, dxgiadapter);

	D3D11CreateDevice(*dxgiadapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_SINGLETHREADED, nullptr, 0, D3D11_SDK_VERSION, d3device, nullptr,
	                  d3d_dc);

	(*d3device)->QueryInterface(dxdevice);

	(*dxdevice)->SetMaximumFrameLatency(1);

	{
		DCompositionCreateDevice(*dxdevice, IID_PPV_ARGS(comp_device));

		(*comp_device)->CreateTargetForHwnd(hwnd, true, comp_target);

		(*comp_device)->CreateVisual(comp_visual);

		DXGI_SWAP_CHAIN_DESC1 swapdesc{};
		swapdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		swapdesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
		swapdesc.SampleDesc.Count = 1;
		swapdesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapdesc.BufferCount = 2;
		swapdesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		swapdesc.Width = size.width;
		swapdesc.Height = size.height;
		swapdesc.Flags = DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE;

		(*factory)->CreateSwapChainForComposition(*d3device, &swapdesc, nullptr, swapchain);

		(*comp_visual)->SetContent(*swapchain);
		(*comp_target)->SetRoot(*comp_visual);
	}
	
	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, {}, d2d_factory);
	(*d2d_factory)->CreateDevice(*dxdevice, d2d_device);
	{
		(*d2d_device)->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, d2d_dc);

		(*swapchain)->GetBuffer(0, IID_PPV_ARGS(dxgi_surface));

		const UINT dpi = GetDpiForWindow(hwnd);
		const D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
			D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
			dpi,
			dpi
		);

		(*d2d_dc)->CreateBitmapFromDxgiSurface(*dxgi_surface, props, bitmap);
		
		(*d2d_dc)->SetTarget(*bitmap);
	}

	// Since the swapchain can't be gdi-compatible, we need a gdi-compatible texture which we blit the swapbuffer onto
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = size.width;
	desc.Height = size.height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.SampleDesc = { .Count = 1, .Quality = 0 };
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET;
	desc.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;

	HRESULT hr = (*d3device)->CreateTexture2D(&desc, nullptr, d3d_gdi_tex);

	(*swapchain)->GetBuffer(1, IID_PPV_ARGS(front_buffer));
	(*dxgi_surface)->QueryInterface(dxgi_surface_resource);

	return true;
}


static void set_statusbar_parts(HWND hwnd, std::vector<int32_t> parts)
{
	auto new_parts = parts;
	auto accumulator = 0;
	for (int i = 0; i < parts.size(); ++i)
	{
		accumulator += parts[i];
		new_parts[i] = accumulator;
	}
	SendMessage(hwnd, SB_SETPARTS,
	            new_parts.size(),
	            (LPARAM)new_parts.data());
}

static HWND create_tooltip(HWND hwnd, int id, const char* text)
{
	HWND hwnd_tip = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL,
	                              WS_POPUP | TTS_NOPREFIX,
	                              CW_USEDEFAULT, CW_USEDEFAULT,
	                              CW_USEDEFAULT, CW_USEDEFAULT,
	                              hwnd, NULL,
	                              GetModuleHandle(0), NULL);

	TOOLINFO info = {0};
	info.cbSize = sizeof(info);
	info.hwnd = hwnd;
	info.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	info.uId = (UINT_PTR)GetDlgItem(hwnd, id);
	// FIXME: why does ms want mutable string for this
	info.lpszText = const_cast<LPSTR>(text);
	SendMessage(hwnd_tip, TTM_ADDTOOL, 0, (LPARAM)&info);
	// Multiline tooltips only work if you specify a max width because of course they do
	SendMessage(hwnd_tip, TTM_SETMAXTIPWIDTH, 0, 999999);

	return hwnd_tip;
}
