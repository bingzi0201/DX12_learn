#pragma once

#include "HeapSlotAllocator.h"

class Device;

class View
{
public:
	View(Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType, ID3D12Resource* InResource);
	virtual ~View();
	D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle() const { return heapSlot.handle; }

private:
	void Destroy();

protected:
	Device* device = nullptr;
	HeapSlotAllocator* heapSlotAllocator = nullptr;
	ID3D12Resource* resource = nullptr;
	HeapSlotAllocator::HeapSlot heapSlot;
	D3D12_DESCRIPTOR_HEAP_TYPE heapType;
};

class ShaderResourceView : public View
{
public:
	ShaderResourceView(Device* inDevice, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc, ID3D12Resource* inResource);
	virtual ~ShaderResourceView();

protected:
	void CreateShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);
};

class RenderTargetView : public View
{
public:
	RenderTargetView(Device* inDevice, const D3D12_RENDER_TARGET_VIEW_DESC& desc, ID3D12Resource* InResource);
	virtual ~RenderTargetView();

protected:
	void CreateRenderTargetView(const D3D12_RENDER_TARGET_VIEW_DESC& desc);
};


class DepthStencilView : public View
{
public:
	DepthStencilView(Device* InDevice, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc, ID3D12Resource* InResource);
	virtual ~DepthStencilView();

protected:
	void CreateDepthStencilView(const D3D12_DEPTH_STENCIL_VIEW_DESC& desc);
};

class UnorderedAccessView : public View
{
public:
	UnorderedAccessView(Device* InDevice, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc, ID3D12Resource* InResource);
	virtual ~UnorderedAccessView();

protected:
	void CreateUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc);
};