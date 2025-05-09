#include "DescriptorCache.h"
#include "Device.h"

DescriptorCache::DescriptorCache(Device* InDevice)
	:device(InDevice)
{
	CreateCacheCbvSrvUavDescriptorHeap();

	CreateCacheRtvDescriptorHeap();
}

DescriptorCache::~DescriptorCache()
{

}

void DescriptorCache::CreateCacheCbvSrvUavDescriptorHeap()
{
	// Create the descriptor heap.
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = maxCbvSrvUavDescripotrCount;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ThrowIfFailed(device->GetD3DDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&cacheCbvSrvUavDescriptorHeap)));
	SetDebugName(cacheCbvSrvUavDescriptorHeap.Get(), L"DescriptorCache cacheCbvSrvUavDescriptorHeap");

	cbvSrvUavDescriptorSize = device->GetD3DDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}


CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorCache::AppendCbvSrvUavDescriptors(const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& srcDescriptors)
{
	// Append to heap
	uint32_t slotsNeeded = (uint32_t)srcDescriptors.size();
	assert(cbvSrvUavDescriptorOffset + slotsNeeded < maxCbvSrvUavDescripotrCount);

	auto cpuDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(cacheCbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), cbvSrvUavDescriptorOffset, cbvSrvUavDescriptorSize);
	device->GetD3DDevice()->CopyDescriptors(1, &cpuDescriptorHandle, &slotsNeeded, slotsNeeded, srcDescriptors.data(), nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Get GpuDescriptorHandle
	auto gpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(cacheCbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), cbvSrvUavDescriptorOffset, cbvSrvUavDescriptorSize);

	// Increase descriptor offset
	cbvSrvUavDescriptorOffset += slotsNeeded;

	return gpuDescriptorHandle;
}

void DescriptorCache::ResetCacheCbvSrvUavDescriptorHeap()
{
	cbvSrvUavDescriptorOffset = 0;
}

void DescriptorCache::CreateCacheRtvDescriptorHeap()
{
	// Create the descriptor heap.
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = maxRtvDescriptorCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	ThrowIfFailed(device->GetD3DDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&cacheRtvDescriptorHeap)));
	SetDebugName(cacheRtvDescriptorHeap.Get(), L"TD3D12DescriptorCache CacheRtvDescriptorHeap");

	rtvDescriptorSize = device->GetD3DDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

void DescriptorCache::AppendRtvDescriptors(const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& rtvDescriptors, CD3DX12_GPU_DESCRIPTOR_HANDLE& outGpuHandle, CD3DX12_CPU_DESCRIPTOR_HANDLE& outCpuHandle)
{
	// Append to heap
	uint32_t slotsNeeded = (uint32_t)rtvDescriptors.size();
	assert(rtvDescriptorOffset + slotsNeeded < maxRtvDescriptorCount);

	auto CpuDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(cacheRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), rtvDescriptorOffset, rtvDescriptorSize);
	device->GetD3DDevice()->CopyDescriptors(1, &CpuDescriptorHandle, &slotsNeeded, slotsNeeded, rtvDescriptors.data(), nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	outGpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(cacheRtvDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), rtvDescriptorOffset, rtvDescriptorSize);

	outCpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(cacheRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), rtvDescriptorOffset, rtvDescriptorSize);

	// Increase descriptor offset
	rtvDescriptorOffset += slotsNeeded;
}

void DescriptorCache::ResetCacheRtvDescriptorHeap()
{
	rtvDescriptorOffset = 0;
}

void DescriptorCache::Reset()
{
	ResetCacheCbvSrvUavDescriptorHeap();

	ResetCacheRtvDescriptorHeap();
}