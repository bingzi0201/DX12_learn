#pragma once

#include "../Resource/D3D12Texture.h"
#include "../Resource/D3D12RHI.h"

class RenderTarget
{
protected:
public:
	RenderTarget(D3D12RHI* inD3D12RHI, bool renderDepth, UINT inWidth, UINT inHeight, DXGI_FORMAT inFormat, TVector4 inClearValue = TVector4::Zero);
	virtual ~RenderTarget();

public:
	D3D12TextureRef GetTexture() const { return d3dTexture; }
	Resource* GetResource() const { return d3dTexture->GetResource(); }
	DXGI_FORMAT GetFormat() const { return format; }
	TVector4 GetClearColor() { return clearValue; }
	float* GetClearColorPtr() { return (float*)&clearValue; }

protected:
	bool bRenderDepth = false;
	D3D12RHI* d3d12RHI = nullptr;
	D3D12TextureRef d3dTexture = nullptr;
	UINT width;
	UINT height;
	DXGI_FORMAT format;
	TVector4 clearValue;
};

class RenderTarget2D : public RenderTarget
{
public:
	RenderTarget2D(D3D12RHI* inD3D12RHI, bool renderDepth, UINT inWidth, UINT inHeight, DXGI_FORMAT inFormat, TVector4 inClearValue = TVector4::Zero);
	RenderTargetView* GetRTV() const;
	DepthStencilView* GetDSV() const;
	ShaderResourceView* GetSRV() const;

private:
	void CreateTexture();
};

class RenderTargetCube : public RenderTarget
{
public:
	RenderTargetCube(D3D12RHI* inD3D12RHI, bool renderDepth, UINT size, DXGI_FORMAT inFormat, TVector4 inClearValue = TVector4::Zero);
	RenderTargetView* GetRTV(int index) const;
	DepthStencilView* GetDSV(int index) const;
	ShaderResourceView* GetSRV() const;

private:
	void CreateTexture();
};