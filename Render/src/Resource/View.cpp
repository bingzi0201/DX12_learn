#include "View.h"
#include "Device.h"
#include <assert.h>

View::View(Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType, ID3D12Resource* InResource)
	:device(InDevice),
	heapType(InType),
	resource(InResource)
{
	heapSlotAllocator = device->GetHeapSlotAllocator(heapType);

	if (heapSlotAllocator)
	{
		heapSlot = heapSlotAllocator->AllocateHeapSlot();
		assert(heapSlot.handle.ptr != 0);
	}
}

View::~View()
{
	Destroy();
}

void View::Destroy()
{
	if (heapSlotAllocator)
	{
		heapSlotAllocator->FreeHeapSlot(heapSlot);
	}
}

ShaderResourceView::ShaderResourceView(Device* InDevice, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc, ID3D12Resource* InResource)
	:View(InDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, InResource)
{
	CreateShaderResourceView(desc);
}

ShaderResourceView::~ShaderResourceView()
{

}

void ShaderResourceView::CreateShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
{
	device->GetD3DDevice()->CreateShaderResourceView(resource, &desc, heapSlot.handle);
}


RenderTargetView::RenderTargetView(Device* InDevice, const D3D12_RENDER_TARGET_VIEW_DESC& desc, ID3D12Resource* InResource)
	:View(InDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, InResource)
{
	CreateRenderTargetView(desc);
}

RenderTargetView::~RenderTargetView()
{

}

void RenderTargetView::CreateRenderTargetView(const D3D12_RENDER_TARGET_VIEW_DESC& desc)
{
	device->GetD3DDevice()->CreateRenderTargetView(resource, &desc, heapSlot.handle);
}


DepthStencilView::DepthStencilView(Device* inDevice, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc, ID3D12Resource* inResource)
	:View(inDevice, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, inResource)
{
	CreateDepthStencilView(desc);
}

DepthStencilView::~DepthStencilView()
{

}

void DepthStencilView::CreateDepthStencilView(const D3D12_DEPTH_STENCIL_VIEW_DESC& desc)
{
	device->GetD3DDevice()->CreateDepthStencilView(resource, &desc, heapSlot.handle);
}


UnorderedAccessView::UnorderedAccessView(Device* inDevice, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc, ID3D12Resource* inResource)
	:View(inDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, inResource)
{
	CreateUnorderedAccessView(desc);
}

UnorderedAccessView::~UnorderedAccessView()
{

}

void UnorderedAccessView::CreateUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc)
{
	device->GetD3DDevice()->CreateUnorderedAccessView(resource, nullptr, &desc, heapSlot.handle);
}