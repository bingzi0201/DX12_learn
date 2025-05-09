#pragma once

#include "../common/d3dx12.h"

enum class ETextureType
{
	TEXTURE_2D,
	TEXTURE_CUBE,
	TEXTURE_3D,
};

struct TextureInfo
{
	ETextureType textureType;
	D3D12_RESOURCE_DIMENSION dimension;
	size_t width;
	size_t height;
	size_t depth;
	size_t arraySize;
	size_t mipCount;
	DXGI_FORMAT format;

	D3D12_RESOURCE_STATES InitState = D3D12_RESOURCE_STATE_GENERIC_READ;

	DXGI_FORMAT srvFormat = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT rtvFormat = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT uavFormat = DXGI_FORMAT_UNKNOWN;
};

enum ETextureCreateFlags
{
	TexCreate_None = 0,
	TexCreate_RTV = 1 << 0,
	TexCreate_CubeRTV = 1 << 1,
	TexCreate_DSV = 1 << 2,
	TexCreate_CubeDSV = 1 << 3,
	TexCreate_SRV = 1 << 4,
	TexCreate_UAV = 1 << 5,
};