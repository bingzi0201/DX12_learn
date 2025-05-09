#include "RenderTarget.h"

RenderTarget::RenderTarget(D3D12RHI* inD3D12RHI, bool renderDepth, UINT inWidth, UINT inHeight, DXGI_FORMAT inFormat, TVector4 inClearValue)
	: d3d12RHI(inD3D12RHI), bRenderDepth(renderDepth), width(inWidth), height(inHeight), format(inFormat), clearValue(inClearValue)
{

}

RenderTarget::~RenderTarget()
{

}


RenderTarget2D::RenderTarget2D(D3D12RHI* inD3D12RHI, bool renderDepth, UINT inWidth, UINT inHeight, DXGI_FORMAT inFormat, TVector4 inClearValue)
	:RenderTarget(inD3D12RHI, renderDepth, inWidth, inHeight, inFormat, inClearValue)
{
	CreateTexture();
}

void RenderTarget2D::CreateTexture()
{
	//Create D3DTexture
	TextureInfo textureInfo;
	textureInfo.textureType = ETextureType::TEXTURE_2D;
	textureInfo.dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureInfo.width = width;
	textureInfo.height = height;
	textureInfo.depth = 1;
	textureInfo.mipCount = 1;
	textureInfo.arraySize = 1;
	textureInfo.format = format;

	if (bRenderDepth)
	{
		textureInfo.dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		textureInfo.srvFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

		d3dTexture = d3d12RHI->CreateTexture(textureInfo, TexCreate_DSV | TexCreate_SRV);
	}
	else
	{
		d3dTexture = d3d12RHI->CreateTexture(textureInfo, TexCreate_RTV | TexCreate_SRV, clearValue);
	}
}

RenderTargetView* RenderTarget2D::GetRTV() const
{
	if (bRenderDepth)
	{
		return nullptr;
	}
	else
	{
		return d3dTexture->GetRTV();
	}
}

DepthStencilView* RenderTarget2D::GetDSV() const
{
	if (bRenderDepth)
	{
		return d3dTexture->GetDSV();
	}
	else
	{
		return nullptr;
	}
}

ShaderResourceView* RenderTarget2D::GetSRV() const
{
	return d3dTexture->GetSRV();
}

RenderTargetCube::RenderTargetCube(D3D12RHI* InD3D12RHI, bool RenderDepth, UINT Size, DXGI_FORMAT InFormat, TVector4 InClearValue)
	:RenderTarget(InD3D12RHI, RenderDepth, Size, Size, InFormat, InClearValue)
{
	CreateTexture();
}

void RenderTargetCube::CreateTexture()
{
	//Create D3DTexture
	TextureInfo textureInfo;
	textureInfo.textureType = ETextureType::TEXTURE_CUBE;
	textureInfo.dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureInfo.width = width;
	textureInfo.height = height;
	textureInfo.depth = 1;
	textureInfo.mipCount = 1;
	textureInfo.arraySize = 6;
	textureInfo.format = format;

	if (bRenderDepth)
	{
		textureInfo.dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		textureInfo.srvFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

		d3dTexture = d3d12RHI->CreateTexture(textureInfo, TexCreate_CubeDSV | TexCreate_SRV);
	}
	else
	{
		d3dTexture = d3d12RHI->CreateTexture(textureInfo, TexCreate_CubeRTV | TexCreate_SRV, clearValue);
	}
}

RenderTargetView* RenderTargetCube::GetRTV(int Index) const
{
	if (bRenderDepth)
	{
		return nullptr;
	}
	else
	{
		return d3dTexture->GetRTV(Index);
	}
}

DepthStencilView* RenderTargetCube::GetDSV(int Index) const
{
	if (bRenderDepth)
	{
		return d3dTexture->GetDSV(Index);
	}
	else
	{
		return nullptr;
	}
}

ShaderResourceView* RenderTargetCube::GetSRV() const
{
	return d3dTexture->GetSRV();
}