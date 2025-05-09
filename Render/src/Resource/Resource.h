#pragma once
#include "../Utils/D3D12Utils.h"

class BuddyAllocator;

class Resource
{
public:
	Resource(Microsoft::WRL::ComPtr<ID3D12Resource> InD3DResource, D3D12_RESOURCE_STATES InitState = D3D12_RESOURCE_STATE_COMMON);
	void Map();  // resource from GPU to CPU ptr, visited by mappedBaseAddress

public:
	Microsoft::WRL::ComPtr<ID3D12Resource> D3DResource = nullptr;
	D3D12_GPU_VIRTUAL_ADDRESS virtualAddressGPU = 0;
	D3D12_RESOURCE_STATES currentState;

	// For upload buffer
	void* mappedBaseAddress = nullptr;
};

struct BuddyBlockData
{
	uint32_t offset = 0;
	uint32_t order = 0;
	uint32_t actualUsedSize = 0;

	Resource* placedResource = nullptr;
};

// useful for buddyAllocate
class ResourceLocation
{
public:
	enum class EResourceLocationType
	{
		Undefined,
		StandAlone,
		SubAllocation,
	};

public:
	ResourceLocation();
	~ResourceLocation();
	void ReleaseResource();
	void SetType(EResourceLocationType type) { resourceLocationType = type; }

public:
	EResourceLocationType resourceLocationType = EResourceLocationType::Undefined;
	// SubAllocation 
	BuddyAllocator* buddyAllocator = nullptr;
	BuddyBlockData blockData;
	// StandAlone resource 
	Resource* underlyingResource = nullptr;

	union
	{
		uint64_t offsetFromBaseOfResource;   // SubAllocation : ManualSubAllocation
		uint64_t offsetFromBaseOfHeap;       // SubAllocation : PlacedResource
	};

	D3D12_GPU_VIRTUAL_ADDRESS virtualAddressGPU = 0;
	// About mapping, for upload buffer
	void* mappedAddress = nullptr;
};

// RAII
template<typename T>
class ResourceScopeMap
{
public:
	ResourceScopeMap(Resource* resource)
	{
		D3DResource = resource->D3DResource.Get();
		D3DResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
	}

	~ResourceScopeMap() { D3DResource->Unmap(0, nullptr); }

	T* GetMappedData() { return mappedData; }

private:
	ID3D12Resource* D3DResource = nullptr;
	T* mappedData = nullptr;
};
