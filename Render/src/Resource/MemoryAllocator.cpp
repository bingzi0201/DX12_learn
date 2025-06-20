#include "MemoryAllocator.h"
#include "../File/PlatformHelpers.h"

using namespace DirectX;
using namespace Microsoft::WRL;

BuddyAllocator::BuddyAllocator(ID3D12Device5* device, const AllocatorInitData& initData) : 
	d3dDevice(device), initData(initData)
{
	Initialize();
}

BuddyAllocator::~BuddyAllocator()
{
	if (backingResource)
	{
		delete backingResource;
	}

	if (backingHeap)
	{
		backingHeap->Release();
	}
}

void BuddyAllocator::Initialize()
{
	//Create BackingHeap or BackingResource
	if (initData.allocationStrategy == EAllocationStrategy::PlacedResource)
	{
		CD3DX12_HEAP_PROPERTIES heapProperties(initData.heapType);
		D3D12_HEAP_DESC desc = {};
		desc.SizeInBytes = DEFAULT_POOL_SIZE;
		desc.Properties = heapProperties;
		desc.Alignment = 0;
		desc.Flags = initData.heapFlags;

		// Create BackingHeap, we will create place resources on it.
		ID3D12Heap* heap = nullptr;
		ThrowIfFailed(d3dDevice->CreateHeap(&desc, IID_PPV_ARGS(&heap)));
		heap->SetName(L"BuddyAllocator BackingHeap");

		backingHeap = heap;
	}
	else //ManualSubAllocation
	{
		CD3DX12_HEAP_PROPERTIES heapProperties(initData.heapType);
		D3D12_RESOURCE_STATES heapResourceStates;
		if (initData.heapType == D3D12_HEAP_TYPE_UPLOAD)
		{
			heapResourceStates = D3D12_RESOURCE_STATE_GENERIC_READ;
		}
		else //D3D12_HEAP_TYPE_DEFAULT
		{
			heapResourceStates = D3D12_RESOURCE_STATE_COMMON;
		}

		CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(DEFAULT_POOL_SIZE, initData.resourceFlags);

		// Create committed resource, we will allocate sub regions on it.
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		ThrowIfFailed(d3dDevice->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			heapResourceStates,
			nullptr,
			IID_PPV_ARGS(&resource)));

		resource->SetName(L"BuddyAllocator BackingResource");

		backingResource = new Resource(resource);

		if (initData.heapType == D3D12_HEAP_TYPE_UPLOAD)
		{
			backingResource->Map();
		}
	}

	// Initialize free blocks, add the free block for MaxOrder
	maxOrder = UnitSizeToOrder(SizeToUnitSize(DEFAULT_POOL_SIZE));

	for (uint32_t i = 0; i <= maxOrder; i++)
	{
		freeBlocks.emplace_back(std::set<uint32_t>());
	}

	freeBlocks[maxOrder].insert((uint32_t)0);
}

bool BuddyAllocator::AllocResource(uint32_t size, uint32_t alignment, ResourceLocation& resourceLocation)
{
	uint32_t sizeToAllocate = GetSizeToAllocate(size, alignment);

	if (CanAllocate(sizeToAllocate))
	{
		// Allocate block
		const uint32_t unitSize = SizeToUnitSize(sizeToAllocate);
		const uint32_t order = UnitSizeToOrder(unitSize);
		const uint32_t offset = AllocateBlock(order); // This is the offset in MinBlockSize units
		const uint32_t allocSize = unitSize * minBlockSize;
		totalAllocSize += allocSize;

		//Calculate AlignedOffsetFromResourceBase
		const uint32_t offsetFromBaseOfResource = GetAllocOffsetInBytes(offset);
		uint32_t alignedOffsetFromResourceBase = offsetFromBaseOfResource;
		if (alignment != 0 && offsetFromBaseOfResource % alignment != 0)
		{
			alignedOffsetFromResourceBase = AlignArbitrary(offsetFromBaseOfResource, alignment);

			uint32_t padding = alignedOffsetFromResourceBase - offsetFromBaseOfResource;
			assert((padding + size) <= allocSize);
		}
		assert((alignedOffsetFromResourceBase % alignment) == 0);

		// Save allocation info to ResourceLocation
		resourceLocation.SetType(ResourceLocation::EResourceLocationType::SubAllocation);
		resourceLocation.blockData.order = order;
		resourceLocation.blockData.offset = offset;
		resourceLocation.blockData.actualUsedSize = size;
		resourceLocation.buddyAllocator = this;

		if (initData.allocationStrategy == EAllocationStrategy::ManualSubAllocation)
		{
			resourceLocation.underlyingResource = backingResource;
			resourceLocation.offsetFromBaseOfResource = alignedOffsetFromResourceBase;
			resourceLocation.virtualAddressGPU = backingResource->virtualAddressGPU + alignedOffsetFromResourceBase;

			if (initData.heapType == D3D12_HEAP_TYPE_UPLOAD)
			{
				resourceLocation.mappedAddress = ((uint8_t*)backingResource->mappedBaseAddress + alignedOffsetFromResourceBase);
			}
		}
		else
		{
			resourceLocation.offsetFromBaseOfHeap = alignedOffsetFromResourceBase;

			// Place resource are initialized by caller
		}

		return true;
	}
	else
	{
		return false;
	}
}

uint32_t BuddyAllocator::GetSizeToAllocate(uint32_t size, uint32_t alignment)
{
	uint32_t sizeToAllocate = size;

	// If the alignment doesn't match the block size
	if (alignment != 0 && minBlockSize % alignment != 0)
	{
		sizeToAllocate = size + alignment;
	}

	return sizeToAllocate;
}


bool BuddyAllocator::CanAllocate(uint32_t sizeToAllocate)
{
	if (totalAllocSize == DEFAULT_POOL_SIZE)
	{
		return false;
	}

	uint32_t blockSize = DEFAULT_POOL_SIZE;

	for (int i = (int)freeBlocks.size() - 1; i >= 0; i--)
	{
		if (freeBlocks[i].size() && blockSize >= sizeToAllocate)
		{
			return true;
		}

		// Halve the block size;
		blockSize = blockSize >> 1;

		if (blockSize < sizeToAllocate) return false;
	}
	return false;
}

uint32_t BuddyAllocator::AllocateBlock(uint32_t order)
{
	uint32_t offset;

	if (order > maxOrder)
	{
		assert(false);
	}

	if (freeBlocks[order].size() == 0)
	{
		// No free nodes in the requested pool.  Try to find a higher-order block and split it.  
		uint32_t left = AllocateBlock(order + 1);
		uint32_t unitSize = OrderToUnitSize(order);
		uint32_t right = left + unitSize;

		freeBlocks[order].insert(right); // Add the right block to the free pool  

		offset = left; // Return the left block  
	}
	else
	{
		auto It = freeBlocks[order].cbegin();
		offset = *It;

		// Remove the block from the free list
		freeBlocks[order].erase(*It);
	}

	return offset;
}

void BuddyAllocator::Deallocate(ResourceLocation& resourceLocation)
{
	deferredDeletionQueue.push_back(resourceLocation.blockData);
}

void BuddyAllocator::CleanUpAllocations()
{
	for (int32_t i = 0; i < deferredDeletionQueue.size(); i++)
	{
		const BuddyBlockData& block = deferredDeletionQueue[i];

		DeallocateInternal(block);
	}

	// clear out all of the released blocks, don't allow the array to shrink
	deferredDeletionQueue.clear();
}

void BuddyAllocator::DeallocateInternal(const BuddyBlockData& block)
{
	DeallocateBlock(block.offset, block.order);

	uint32_t size = OrderToUnitSize(block.order) * minBlockSize;
	totalAllocSize -= size;

	if (initData.allocationStrategy == EAllocationStrategy::PlacedResource)
	{
		// Release place resource
		assert(block.placedResource != nullptr);
		delete block.placedResource;
	}
}

void BuddyAllocator::DeallocateBlock(uint32_t offset, uint32_t order)
{
	// Get buddy block
	uint32_t size = OrderToUnitSize(order);
	uint32_t buddy = GetBuddyOffset(offset, size);

	auto It = freeBlocks[order].find(buddy);
	if (It != freeBlocks[order].end()) // If buddy block is free, merge it
	{
		// Deallocate merged blocks
		DeallocateBlock(min(offset, buddy), order + 1);

		// Remove the buddy from the free list  
		freeBlocks[order].erase(*It);
	}
	else
	{
		// Add the block to the free list
		freeBlocks[order].insert(offset);
	}
}

MultiBuddyAllocator::MultiBuddyAllocator(ID3D12Device5* inDevice, const BuddyAllocator::AllocatorInitData& inInitData)
	:d3dDevice(inDevice), initData(inInitData)
{

}

MultiBuddyAllocator::~MultiBuddyAllocator()
{

}

bool MultiBuddyAllocator::AllocResource(uint32_t size, uint32_t alignment, ResourceLocation& resourceLocation)
{
	for (auto& allocator : allocators) // Try to use existing allocators 
	{
		if (allocator->AllocResource(size, alignment, resourceLocation))
		{
			return true;
		}
	}

	// Create new allocator
	auto allocator = std::make_shared<BuddyAllocator>(d3dDevice, initData);
	allocators.push_back(allocator);

	bool result = allocator->AllocResource(size, alignment, resourceLocation);
	assert(result);

	return true;
}

void MultiBuddyAllocator::CleanUpAllocations()
{
	for (auto& allocator : allocators)
	{
		allocator->CleanUpAllocations();
	}
}

UploadBufferAllocator::UploadBufferAllocator(ID3D12Device5* InDevice)
{
	BuddyAllocator::AllocatorInitData initData;
	initData.allocationStrategy = BuddyAllocator::EAllocationStrategy::ManualSubAllocation;
	initData.heapType = D3D12_HEAP_TYPE_UPLOAD;
	initData.resourceFlags = D3D12_RESOURCE_FLAG_NONE;

	allocator = std::make_unique<MultiBuddyAllocator>(InDevice, initData);

	d3dDevice = InDevice;
}

void* UploadBufferAllocator::AllocUploadResource(uint32_t size, uint32_t alignment, ResourceLocation& resourceLocation)
{
	allocator->AllocResource(size, alignment, resourceLocation);

	return resourceLocation.mappedAddress;
}

void UploadBufferAllocator::CleanUpAllocations()
{
	allocator->CleanUpAllocations();
}



DefaultBufferAllocator::DefaultBufferAllocator(ID3D12Device5* InDevice)
{
	{
		BuddyAllocator::AllocatorInitData initData;
		initData.allocationStrategy = BuddyAllocator::EAllocationStrategy::ManualSubAllocation;
		initData.heapType = D3D12_HEAP_TYPE_DEFAULT;
		initData.resourceFlags = D3D12_RESOURCE_FLAG_NONE;

		allocator = std::make_unique<MultiBuddyAllocator>(InDevice, initData);
	}

	{
		BuddyAllocator::AllocatorInitData initData;
		initData.allocationStrategy = BuddyAllocator::EAllocationStrategy::ManualSubAllocation;
		initData.heapType = D3D12_HEAP_TYPE_DEFAULT;
		initData.resourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		uavAllocator = std::make_unique<MultiBuddyAllocator>(InDevice, initData);
	}

	d3dDevice = InDevice;
}

void DefaultBufferAllocator::AllocDefaultResource(const D3D12_RESOURCE_DESC& resourceDesc, uint32_t alignment, ResourceLocation& resourceLocation)
{
	if (resourceDesc.Flags == D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
	{
		uavAllocator->AllocResource((uint32_t)resourceDesc.Width, alignment, resourceLocation);
	}
	else
	{
		allocator->AllocResource((uint32_t)resourceDesc.Width, alignment, resourceLocation);
	}
}

void DefaultBufferAllocator::CleanUpAllocations()
{
	allocator->CleanUpAllocations();
}



TextureResourceAllocator::TextureResourceAllocator(ID3D12Device5* inDevice)
{
	BuddyAllocator::AllocatorInitData initData;
	initData.allocationStrategy = BuddyAllocator::EAllocationStrategy::PlacedResource;
	initData.heapType = D3D12_HEAP_TYPE_DEFAULT;
	initData.heapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;

	allocator = std::make_unique<MultiBuddyAllocator>(inDevice, initData);

	d3dDevice = inDevice;
}

void TextureResourceAllocator::AllocTextureResource(const D3D12_RESOURCE_STATES& resourceState, const D3D12_RESOURCE_DESC& resourceDesc, ResourceLocation& resourceLocation)
{
	const D3D12_RESOURCE_ALLOCATION_INFO info = d3dDevice->GetResourceAllocationInfo(0, 1, &resourceDesc);

	allocator->AllocResource((uint32_t)info.SizeInBytes, DEFAULT_RESOURCE_ALIGNMENT, resourceLocation);

	// Create placed resource
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		ID3D12Heap* backingHeap = resourceLocation.buddyAllocator->GetBackingHeap();
		uint64_t heapOffset = resourceLocation.offsetFromBaseOfHeap;
		d3dDevice->CreatePlacedResource(backingHeap, heapOffset, &resourceDesc, resourceState, nullptr, IID_PPV_ARGS(&resource));

		Resource* newResource = new Resource(resource);
		resourceLocation.underlyingResource = newResource;
		resourceLocation.blockData.placedResource = newResource;  // Will delete Resource when ResourceLocation was destroyed
	}
}

void TextureResourceAllocator::CleanUpAllocations()
{
	allocator->CleanUpAllocations();
}

DXRResourceAllocator::DXRResourceAllocator(ID3D12Device5* inDevice)
    : d3dDevice(inDevice)
{
    BuddyAllocator::AllocatorInitData initData;
    initData.allocationStrategy = BuddyAllocator::EAllocationStrategy::PlacedResource;
    initData.heapType = D3D12_HEAP_TYPE_DEFAULT;
    // DXR Acceleration Structures are buffers, so this heap flag is appropriate.
    initData.heapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
    initData.resourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    allocator = std::make_unique<MultiBuddyAllocator>(inDevice, initData);
}

void DXRResourceAllocator::AllocAccelerationStructureResource(
    UINT64 sizeInBytes,
    const std::wstring& resourceName,
    ResourceLocation& resourceLocation)
{
    // DXR AS require their GPU Virtual Address to be 256-byte aligned.
    // The sizeInBytes parameter is also expected to be 256-byte aligned by the caller.
    // The alignment passed to AllocResource ensures the offset within the heap is 256-byte aligned.
    allocator->AllocResource(
        (uint32_t)sizeInBytes,
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, // 256 bytes
        resourceLocation
    );

    // Create the Placed Resource for the Acceleration Structure
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment = 0; // For PlacedResource, alignment is determined by heap offset
    resourceDesc.Width = sizeInBytes;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; // AS requires UAV access

    Microsoft::WRL::ComPtr<ID3D12Resource> placedD3DResource;
    ID3D12Heap* backingHeap = resourceLocation.buddyAllocator->GetBackingHeap(); // Assumes this getter exists
    uint64_t heapOffset = resourceLocation.offsetFromBaseOfHeap;

    HRESULT hr = d3dDevice->CreatePlacedResource(
        backingHeap,
        heapOffset,
        &resourceDesc,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, // Initial state for AS
        nullptr, // No optimized clear value for buffers
        IID_PPV_ARGS(&placedD3DResource)
    );

    Resource* newUnderlyingResource = new Resource(placedD3DResource);
    resourceLocation.underlyingResource = newUnderlyingResource;
    resourceLocation.blockData.placedResource = newUnderlyingResource; // Ensure ownership is clear
}

void DXRResourceAllocator::AllocScratchResource(
    UINT64 sizeInBytes,
    const std::wstring& resourceName,
    ResourceLocation& resourceLocation)
{
    // Scratch buffers also require UAV access and are typically placed in default heaps.
    // 256-byte alignment for their GPUVA is good practice and often required implicitly by drivers.
    allocator->AllocResource(
        (uint32_t)sizeInBytes,
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, // Using 256 for consistency, or D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT
        resourceLocation
    );

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = sizeInBytes;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; // Scratch buffers are UAVs

    Microsoft::WRL::ComPtr<ID3D12Resource> placedD3DResource;
    ID3D12Heap* backingHeap = resourceLocation.buddyAllocator->GetBackingHeap();
    uint64_t heapOffset = resourceLocation.offsetFromBaseOfHeap;

    HRESULT hr = d3dDevice->CreatePlacedResource(
        backingHeap,
        heapOffset,
        &resourceDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, // Initial state for scratch buffers
        nullptr,
        IID_PPV_ARGS(&placedD3DResource)
    );

    if (!resourceName.empty())
    {
        placedD3DResource->SetName(resourceName.c_str());
    }

    Resource* newUnderlyingResource = new Resource(placedD3DResource);
    resourceLocation.underlyingResource = newUnderlyingResource;
    resourceLocation.blockData.placedResource = newUnderlyingResource;
}

void DXRResourceAllocator::CleanUpAllocations()
{
    if (allocator)
    {
        allocator->CleanUpAllocations();
    }
}