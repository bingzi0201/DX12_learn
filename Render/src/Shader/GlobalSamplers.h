#pragma once
#include "../Common/stdafx.h"
#include "../Common/d3dx12.h"

class GlobalSamplers
{
public:
	static std::span<D3D12_STATIC_SAMPLER_DESC> GetSamplers();
};