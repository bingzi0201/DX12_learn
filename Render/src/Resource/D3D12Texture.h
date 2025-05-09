#pragma once

#include "Resource.h"
#include "View.h"
#include "../Math/Math.h"


class D3D12Texture
{
public:
	Resource* GetResource() { return resourceLocation.underlyingResource; }
	ID3D12Resource* GetD3DResource() { return resourceLocation.underlyingResource->D3DResource.Get(); }

	void SetRTVClearValue(TVector4 clearValue) { rtvClearValue = clearValue; }
	TVector4 GetRTVClearValue() { return rtvClearValue; }
	float* GetRTVClearValuePtr() { return (float*)&rtvClearValue; }

	ShaderResourceView* GetSRV(UINT index = 0)
	{
		assert(SRVs.size() > index);
		return SRVs[index].get();
	}
	void AddSRV(std::unique_ptr<ShaderResourceView>& srv)
	{
		SRVs.emplace_back(std::move(srv));
	}
	RenderTargetView* GetRTV(UINT index = 0)
	{
		assert(RTVs.size() > index);
		return RTVs[index].get();
	}
	void AddRTV(std::unique_ptr<RenderTargetView>& rtv)
	{
		RTVs.emplace_back(std::move(rtv));
	}

	DepthStencilView* GetDSV(UINT index = 0)
	{
		assert(DSVs.size() > index);
		return DSVs[index].get();
	}
	void AddDSV(std::unique_ptr<DepthStencilView>& dsv)
	{
		DSVs.emplace_back(std::move(dsv));
	}
	UnorderedAccessView* GetUAV(UINT index = 0)
	{
		assert(UAVs.size() > index);
		return UAVs[index].get();
	}
	void AddUAV(std::unique_ptr<UnorderedAccessView>& uav)
	{
		UAVs.emplace_back(std::move(uav));
	}

public:
	ResourceLocation resourceLocation;

private:
	std::vector<std::unique_ptr<ShaderResourceView>> SRVs;
	std::vector<std::unique_ptr<RenderTargetView>> RTVs;
	std::vector<std::unique_ptr<DepthStencilView>> DSVs;
	std::vector<std::unique_ptr<UnorderedAccessView>> UAVs;

private:
	TVector4 rtvClearValue;
};

typedef std::shared_ptr<D3D12Texture> D3D12TextureRef;