#pragma once

#include "Resource.h"
#include <stdint.h>
#include <set>

#define DEFAULT_POOL_SIZE (512 * 1024 * 512)

#define DEFAULT_RESOURCE_ALIGNMENT 4
#define UPLOAD_RESOURCE_ALIGNMENT 256

class BuddyAllocator
{
public:
	enum class EAllocationStrategy
	{
		PlacedResource,
		ManualSubAllocation
	};

	struct AllocatorInitData
	{
		EAllocationStrategy allocationStrategy;
		D3D12_HEAP_TYPE heapType;
		D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;    // only for PlacedResource
		D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;    // only for ManualSubAllocation
	};

public:
	BuddyAllocator(ID3D12Device* device, const AllocatorInitData& initData);
	~BuddyAllocator();

	bool AllocResource(uint32_t size, uint32_t alignment, ResourceLocation& resourceLocation);
	void Deallocate(ResourceLocation& resourceLocation);
	void CleanUpAllocations();

	ID3D12Heap* GetBackingHeap() { return backingHeap; }
	EAllocationStrategy GetAllocationStrategy() { return initData.allocationStrategy; }

private:
	void Initialize();
	uint32_t AllocateBlock(uint32_t order);
	uint32_t GetSizeToAllocate(uint32_t size, uint32_t alignment);
	bool CanAllocate(uint32_t sizeToAllocate);

	void DeallocateInternal(const BuddyBlockData& block);
	void DeallocateBlock(uint32_t Offset, uint32_t Order);

	uint32_t SizeToUnitSize(uint32_t size) const { return (size + (minBlockSize - 1)) / minBlockSize; }
	uint32_t UnitSizeToOrder(uint32_t size) const
	{
		unsigned long result;
		_BitScanReverse(&result, size + size - 1); // ceil(log2(size)), only for Windows MSVC
		return result;
	}
	uint32_t OrderToUnitSize(uint32_t order) const { return ((uint32_t)1) << order; }  // result = 2^order

	uint32_t GetBuddyOffset(const uint32_t& offset, const uint32_t& size) { return offset ^ size; }
	uint32_t GetAllocOffsetInBytes(uint32_t Offset) const { return Offset * minBlockSize; }

private:
	AllocatorInitData initData;
	const uint32_t minBlockSize = 256;
	uint32_t maxOrder;
	uint32_t totalAllocSize = 0;
	std::vector<std::set<uint32_t>> freeBlocks;
	std::vector<BuddyBlockData> deferredDeletionQueue;
	ID3D12Device* d3dDevice;
	Resource* backingResource = nullptr;
	ID3D12Heap* backingHeap = nullptr;
};

class MultiBuddyAllocator
{
public:
	MultiBuddyAllocator(ID3D12Device* device, const BuddyAllocator::AllocatorInitData& initData);
	~MultiBuddyAllocator();
	bool AllocResource(uint32_t size, uint32_t alignment, ResourceLocation& resourceLocation);
	void CleanUpAllocations();

private:
	std::vector<std::shared_ptr<BuddyAllocator>> allocators;
	ID3D12Device* d3dDevice;
	BuddyAllocator::AllocatorInitData initData;
};

class UploadBufferAllocator
{
public:
	UploadBufferAllocator(ID3D12Device* InDevice);
	void* AllocUploadResource(uint32_t size, uint32_t alignment, ResourceLocation& resourceLocation);
	void CleanUpAllocations();

private:
	std::unique_ptr<MultiBuddyAllocator> allocator = nullptr;
	ID3D12Device* d3dDevice = nullptr;
};

class DefaultBufferAllocator
{
public:
	DefaultBufferAllocator(ID3D12Device* InDevice);
	void AllocDefaultResource(const D3D12_RESOURCE_DESC& resourceDesc, uint32_t alignment, ResourceLocation& resourceLocation);
	void CleanUpAllocations();

private:
	std::unique_ptr<MultiBuddyAllocator> allocator = nullptr;
	std::unique_ptr<MultiBuddyAllocator> uavAllocator = nullptr;
	ID3D12Device* d3dDevice = nullptr;
};

class TextureResourceAllocator
{
public:
	TextureResourceAllocator(ID3D12Device* InDevice);
	void AllocTextureResource(const D3D12_RESOURCE_STATES& resourceState, const D3D12_RESOURCE_DESC& resourceDesc, ResourceLocation& resourceLocation);
	void CleanUpAllocations();

private:
	std::unique_ptr<MultiBuddyAllocator> allocator = nullptr;
	ID3D12Device* d3dDevice = nullptr;
};