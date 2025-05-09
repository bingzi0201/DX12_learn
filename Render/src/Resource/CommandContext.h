#pragma once

#include "../Utils/D3D12Utils.h"
#include "DescriptorCache.h"

class Device;

class CommandContext
{
public:
	CommandContext(Device* InDevice);
	~CommandContext();

	void CreateCommandContext();
	void DestroyCommandContext();

	ID3D12CommandQueue* GetCommandQueue() { return commandQueue.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() { return commandList.Get(); }
	DescriptorCache* GetDescriptorCache() { return descriptorCache.get(); }

	void ResetCommandAllocator();
	void ResetCommandList();
	void ExecuteCommandLists();
	void FlushCommandQueue();
	void EndFrame();

private:
	Device* device = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandListAlloc = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;
	std::unique_ptr<DescriptorCache> descriptorCache = nullptr;

private:
	Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;

	UINT64 currentFenceValue = 0;
};