#include "Resource.h"
#include "MemoryAllocator.h"

Resource::Resource(Microsoft::WRL::ComPtr<ID3D12Resource> InD3DResource, D3D12_RESOURCE_STATES InitState)
	:D3DResource(InD3DResource), currentState(InitState)
{
	if (D3DResource->GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		virtualAddressGPU = D3DResource->GetGPUVirtualAddress();
	}
}

void Resource::Map()
{
	ThrowIfFailed(D3DResource->Map(0, nullptr, &mappedBaseAddress));
}

ResourceLocation::ResourceLocation()
{

}

ResourceLocation::~ResourceLocation()
{
	ReleaseResource();
}

void ResourceLocation::ReleaseResource()
{
	switch (resourceLocationType)
	{

	case ResourceLocation::EResourceLocationType::StandAlone:
	{
		delete underlyingResource;
		break;
	}

	case ResourceLocation::EResourceLocationType::SubAllocation:
	{
		if (buddyAllocator)
			buddyAllocator->Deallocate(*this);

		break;
	}

	default:
		break;
	}
}