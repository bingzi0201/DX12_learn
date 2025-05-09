#pragma once

#include "../Utils/D3D12Utils.h"

class Device;
using Microsoft::WRL::ComPtr;

// shader-visible
// push descriptors from CPU to GPU when we need
class DescriptorCache
{
public:
	DescriptorCache(Device* InDevice);
	~DescriptorCache();

	ComPtr<ID3D12DescriptorHeap> GetCacheCbvSrvUavDescriptorHeap() { return cacheCbvSrvUavDescriptorHeap; }
	// copy descriptors from non-shader-visible heap to shader-visible heap
	CD3DX12_GPU_DESCRIPTOR_HANDLE AppendCbvSrvUavDescriptors(const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& srcDescriptors);

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetCacheRtvDescriptorHeap() { return cacheRtvDescriptorHeap; }
	void AppendRtvDescriptors(const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& rtvDescriptors, CD3DX12_GPU_DESCRIPTOR_HANDLE& outGpuHandle, CD3DX12_CPU_DESCRIPTOR_HANDLE& outCpuHandle);
	void Reset();  // per frame

private:
	void CreateCacheCbvSrvUavDescriptorHeap();
	void CreateCacheRtvDescriptorHeap();
	// per frame
	void ResetCacheCbvSrvUavDescriptorHeap();
	void ResetCacheRtvDescriptorHeap();

private:
	Device* device = nullptr;
	ComPtr<ID3D12DescriptorHeap> cacheCbvSrvUavDescriptorHeap = nullptr;
	UINT cbvSrvUavDescriptorSize;
	static const int maxCbvSrvUavDescripotrCount = 2048;

	uint32_t cbvSrvUavDescriptorOffset = 0;
	ComPtr<ID3D12DescriptorHeap> cacheRtvDescriptorHeap = nullptr;
	UINT rtvDescriptorSize;
	static const int maxRtvDescriptorCount = 1024;
	uint32_t rtvDescriptorOffset = 0;
};