#pragma once
#include "Presenter.h"

#include <d2d1.h>
#include <d2d1_3.h>
#include <dcomp.h>
#include <d3d11.h>

class DCompPresenter : public Presenter
{
public:
	~DCompPresenter() override;
	bool init(HWND hwnd) override;
	ID2D1RenderTarget* dc() const override;
	void begin_present() override;
	void end_present() override;
	void blit(HDC hdc, RECT rect) override;
	D2D1_SIZE_U size() override;

private:
	D2D1_SIZE_U m_size;
	IDXGIFactory2* m_factory;
	IDXGIAdapter1* m_dxgiadapter;
	ID3D11Device* m_d3device;
	IDXGIDevice1* m_dxdevice;
	ID2D1Bitmap1* m_bitmap;
	IDCompositionVisual* m_comp_visual;
	ID3D11DeviceContext* m_d3d_dc;
	IDCompositionDevice* m_comp_device;
	IDCompositionTarget* m_comp_target;
	IDXGISwapChain1* m_dxgi_swapchain;
	ID2D1Factory3* m_d2d_factory;
	ID2D1Device2* m_d2d_device;
	ID2D1DeviceContext2* m_d2d_dc;
	IDXGISurface1* m_dxgi_surface;
	ID3D11Resource* m_dxgi_surface_resource;
	ID3D11Resource* m_front_buffer;
	ID3D11Texture2D* m_d3d_gdi_tex;
};
