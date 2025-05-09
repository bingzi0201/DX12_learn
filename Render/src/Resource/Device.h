#pragma once

#include "CommandContext.h"
#include "MemoryAllocator.h"
#include "HeapSlotAllocator.h"

class D3D12RHI;

class Device
{
public:
	Device(D3D12RHI* InD3D12RHI);
	~Device();

public:
	ID3D12Device* GetD3DDevice() { return d3dDevice.Get(); }
	CommandContext* GetCommandContext() { return commandContext.get(); }
	ID3D12CommandQueue* GetCommandQueue() { return commandContext->GetCommandQueue(); }
	ID3D12GraphicsCommandList* GetCommandList() { return commandContext->GetCommandList(); }
	UploadBufferAllocator* GetUploadBufferAllocator() { return uploadBufferAllocator.get(); }
	DefaultBufferAllocator* GetDefaultBufferAllocator() { return defaultBufferAllocator.get(); }
	TextureResourceAllocator* GetTextureResourceAllocator() { return textureResourceAllocator.get(); }
	HeapSlotAllocator* GetHeapSlotAllocator(D3D12_DESCRIPTOR_HEAP_TYPE HeapType);

private:
	void Initialize();

private:
	D3D12RHI* d3d12RHI = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice = nullptr;
	std::unique_ptr<CommandContext> commandContext = nullptr;

private:
	std::unique_ptr<UploadBufferAllocator> uploadBufferAllocator = nullptr;
	std::unique_ptr<DefaultBufferAllocator> defaultBufferAllocator = nullptr;
	std::unique_ptr<TextureResourceAllocator> textureResourceAllocator = nullptr;
	std::unique_ptr<HeapSlotAllocator> RTVHeapSlotAllocator = nullptr;
	std::unique_ptr<HeapSlotAllocator> DSVHeapSlotAllocator = nullptr;
	std::unique_ptr<HeapSlotAllocator> SRVHeapSlotAllocator = nullptr;

};