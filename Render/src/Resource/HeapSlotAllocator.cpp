#include "HeapSlotAllocator.h"
#include <assert.h>


HeapSlotAllocator::HeapSlotAllocator(ID3D12Device* inDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsPerHeap)
	:d3dDevice(inDevice),
	heapDesc(CreateHeapDesc(type, numDescriptorsPerHeap)),
	descriptorSize(d3dDevice->GetDescriptorHandleIncrementSize(heapDesc.Type))
{

}

HeapSlotAllocator::~HeapSlotAllocator()
{

}

D3D12_DESCRIPTOR_HEAP_DESC HeapSlotAllocator::CreateHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsPerHeap)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = type;
	desc.NumDescriptors = numDescriptorsPerHeap;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // This heap will not be bound to the shader
	desc.NodeMask = 0;

	return desc;
}

void HeapSlotAllocator::AllocateHeap()
{
	// Create a new descriptorHeap
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap)));
	SetDebugName(heap.Get(), L"HeapSlotAllocator Descriptor Heap");

	// Add an entry covering the entire heap.
	DescriptorHandle heapBase = heap->GetCPUDescriptorHandleForHeapStart();
	assert(heapBase.ptr != 0);

	HeapEntry entry;
	entry.heap = heap;
	entry.freeList.push_back({ heapBase.ptr, heapBase.ptr + (SIZE_T)heapDesc.NumDescriptors * descriptorSize });

	// Add the entry to HeapMap
	heapMap.push_back(entry);
}

HeapSlotAllocator::HeapSlot HeapSlotAllocator::AllocateHeapSlot()
{
	// Find the entry with free list
	int entryIndex = -1;
	for (int i = 0; i < heapMap.size(); i++)
	{
		if (heapMap[i].freeList.size() > 0)
		{
			entryIndex = i;
			break;
		}
	}

	// If all entries are full, create a new one
	if (entryIndex == -1)
	{
		AllocateHeap();

		entryIndex = int(heapMap.size() - 1);
	}

	HeapEntry& entry = heapMap[entryIndex];
	assert(entry.freeList.size() > 0);

	// Allocate  a slot
	FreeRange& range = entry.freeList.front();
	HeapSlot slot = { (uint32_t)entryIndex, range.start };

	// Remove this range if all slot has been allocated.
	range.start += descriptorSize;
	if (range.start == range.end)
	{
		entry.freeList.pop_front();
	}

	return slot;
}

void HeapSlotAllocator::FreeHeapSlot(const HeapSlot& slot)
{
	assert(slot.heapIndex < heapMap.size());
	HeapEntry& entry = heapMap[slot.heapIndex];

	FreeRange newRange =
	{
		slot.handle.ptr,
		slot.handle.ptr + descriptorSize
	};

	bool bFound = false;
	for (auto node = entry.freeList.begin(); node != entry.freeList.end() && !bFound; node++)
	{
		FreeRange& range = *node;
		assert(range.start < range.end);

		if (range.start == newRange.end) //Merge NewRange and Range
		{
			range.start = newRange.start;
			bFound = true;
		}
		else if (range.end == newRange.start) // Merge Range and NewRange
		{
			range.end = newRange.end;
			bFound = true;
		}
		else
		{
			assert(range.end < newRange.start || range.start > newRange.start);
			if (range.start > newRange.start) // Insert NewRange before Range
			{
				entry.freeList.insert(node, newRange);
				bFound = true;
			}
		}
	}

	if (!bFound)
	{
		// Add  NewRange to tail
		entry.freeList.push_back(newRange);
	}
}


