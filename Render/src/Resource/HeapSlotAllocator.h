#pragma once

#include "../Utils/D3D12Utils.h"
#include <list>

// CPU only
class HeapSlotAllocator
{
public:
	typedef D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle;
	typedef decltype(DescriptorHandle::ptr) DescriptorHandleRaw;

	struct HeapSlot
	{
		uint32_t heapIndex;
		D3D12_CPU_DESCRIPTOR_HANDLE handle;
	};

private:
	struct FreeRange
	{
		DescriptorHandleRaw start;
		DescriptorHandleRaw end;
	};

	struct HeapEntry
	{
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap = nullptr;
		std::list<HeapSlotAllocator::FreeRange> freeList;

		HeapEntry() { }
	};

public:
	HeapSlotAllocator(ID3D12Device5* inDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsPerHeap);
	~HeapSlotAllocator();

	HeapSlot AllocateHeapSlot();
	void FreeHeapSlot(const HeapSlot& slot);

private:
	D3D12_DESCRIPTOR_HEAP_DESC CreateHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsPerHeap);
	void AllocateHeap();

private:
	ID3D12Device5* d3dDevice;
	const D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	const uint32_t descriptorSize;
	std::vector<HeapEntry> heapMap;
};