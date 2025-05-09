#pragma once

#include "../Utils/D3D12Utils.h"
#include "D3D12Texture.h"

class D3D12RHI;

struct ViewportInfo
{
	HWND windowHandle;

	DXGI_FORMAT backBufferFormat;
	DXGI_FORMAT depthStencilFormat;

	bool bEnable4xMsaa = false;    // 4X MSAA enabled
	UINT qualityOf4xMsaa = 0;      // quality level of 4X MSAA
};

class Viewport
{
public:
	Viewport(D3D12RHI* inD3D12RHI, const ViewportInfo& info, int width, int height);
	~Viewport();

public:
	void OnResize(int newWidth, int newHeight);
	void GetD3DViewport(D3D12_VIEWPORT& outD3DViewPort, D3D12_RECT& outD3DRect);
	void Present();

	Resource* GetCurrentBackBuffer() const;
	RenderTargetView* GetCurrentBackBufferView() const;
	float* GetCurrentBackBufferClearColor() const;
	DepthStencilView* GetDepthStencilView() const;
	ShaderResourceView* GetDepthShaderResourceView() const;
	ViewportInfo GetViewportInfo() const;

private:
	void Initialize();
	void CreateSwapChain();

private:

	D3D12RHI* d3d12RHI = nullptr;

	ViewportInfo viewportInfo;
	int viewportWidth = 0;
	int viewportHeight = 0;

	static const int swapChainBufferCount = 2;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain = nullptr;
	int currBackBuffer = 0;
	D3D12TextureRef renderTargetTextures[swapChainBufferCount];
	D3D12TextureRef depthStencilTexture = nullptr;
};