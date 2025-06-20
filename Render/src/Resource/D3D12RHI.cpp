#include "D3D12RHI.h"
#include "Device.h"
#include <assert.h>

// If you don't install "Graphics Tools", set InstalledDebugLayers to false
// Ref: https://stackoverflow.com/questions/60157794/dx12-d3d12getdebuginterface-app-requested-interface-depends-on-sdk-component
#define InstalledDebugLayers true

using Microsoft::WRL::ComPtr;

D3D12RHI::D3D12RHI()
{
}

D3D12RHI::~D3D12RHI()
{
	Destroy();
}

bool D3D12RHI::CheckRaytracingSupport()
{
	// Check if the device supports raytracing
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5;
	HRESULT hr = device->GetD3DDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));
	if (FAILED(hr))
	{
		return false;
	}

	return options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
}

void D3D12RHI::Initialize(HWND WindowHandle, int WindowWidth, int WindowHeight)
{
	// D3D12 debug
	UINT DxgiFactoryFlags = 0;

#if (defined(DEBUG) || defined(_DEBUG)) && InstalledDebugLayers 
	{
		ComPtr<ID3D12Debug> DebugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(DebugController.GetAddressOf())));
		DebugController->EnableDebugLayer();
	}

	ComPtr<IDXGIInfoQueue> DxgiInfoQueue;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(DxgiInfoQueue.GetAddressOf()))))
	{
		DxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

		DxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
		DxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
	}

#endif

	// Create DxgiFactory
	ThrowIfFailed(CreateDXGIFactory2(DxgiFactoryFlags, IID_PPV_ARGS(dxgiFactory.GetAddressOf())));

	// Create Device
	device = std::make_unique<Device>(this);

	// Create Viewport
	viewportInfo.windowHandle = WindowHandle;
	viewportInfo.backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	viewportInfo.depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	viewportInfo.bEnable4xMsaa = false;
	viewportInfo.qualityOf4xMsaa = GetSupportMSAAQuality(viewportInfo.backBufferFormat);

	viewport = std::make_unique<Viewport>(this, viewportInfo, WindowWidth, WindowHeight);

#ifdef _DEBUG
	LogAdapters();
#endif

}

void D3D12RHI::Destroy()
{
	EndFrame();

	viewport.reset();

	device.reset();

	//#ifdef _DEBUG
	//	{
	//		ComPtr<IDXGIDebug1> DxgiDebug;
	//		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&DxgiDebug))))
	//		{
	//			DxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
	//		}
	//	}
	//#endif
}

const ViewportInfo& D3D12RHI::GetViewportInfo()
{
	return viewportInfo;
}

IDXGIFactory4* D3D12RHI::GetDxgiFactory()
{
	return dxgiFactory.Get();
}

void D3D12RHI::LogAdapters()
{
	UINT i = 0;
	IDXGIAdapter* adapter = nullptr;
	std::vector<IDXGIAdapter*> adapterList;
	while (dxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);

		std::wstring text = L"***Adapter: ";
		text += desc.Description;
		text += L"\n";

		OutputDebugString(text.c_str());

		adapterList.push_back(adapter);

		++i;
	}

	for (size_t i = 0; i < adapterList.size(); ++i)
	{
		LogAdapterOutputs(adapterList[i]);
		ReleaseCom(adapterList[i]);
	}
}

void D3D12RHI::LogAdapterOutputs(IDXGIAdapter* adapter)
{
	UINT i = 0;
	IDXGIOutput* output = nullptr;
	while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC desc;
		output->GetDesc(&desc);

		std::wstring text = L"***Output: ";
		text += desc.DeviceName;
		text += L"\n";
		OutputDebugString(text.c_str());

		LogOutputDisplayModes(output, viewportInfo.backBufferFormat);

		ReleaseCom(output);

		++i;
	}
}

void D3D12RHI::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
	UINT count = 0;
	UINT flags = 0;

	// Call with nullptr to get list count.
	output->GetDisplayModeList(format, flags, &count, nullptr);

	std::vector<DXGI_MODE_DESC> modeList(count);
	output->GetDisplayModeList(format, flags, &count, &modeList[0]);

	for (auto& x : modeList)
	{
		UINT n = x.RefreshRate.Numerator;
		UINT d = x.RefreshRate.Denominator;
		std::wstring text =
			L"Width = " + std::to_wstring(x.Width) + L" " +
			L"Height = " + std::to_wstring(x.Height) + L" " +
			L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
			L"\n";

		::OutputDebugString(text.c_str());
	}
}

UINT D3D12RHI::GetSupportMSAAQuality(DXGI_FORMAT BackBufferFormat)
{
	// Check 4X MSAA quality support for our back buffer format.
	// All Direct3D 11 capable devices support 4X MSAA for all render 
	// target formats, so we only need to check quality support.

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = BackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	ThrowIfFailed(device->GetD3DDevice()->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));

	UINT QualityOf4xMsaa = msQualityLevels.NumQualityLevels;
	assert(QualityOf4xMsaa > 0 && "Unexpected MSAA quality level.");

	return QualityOf4xMsaa;
}


void D3D12RHI::FlushCommandQueue()
{
	GetDevice()->GetCommandContext()->FlushCommandQueue();
}

void D3D12RHI::ExecuteCommandLists()
{
	GetDevice()->GetCommandContext()->ExecuteCommandLists();
}

void D3D12RHI::ResetCommandList()
{
	GetDevice()->GetCommandContext()->ResetCommandList();
}

void D3D12RHI::ResetCommandAllocator()
{
	GetDevice()->GetCommandContext()->ResetCommandAllocator();
}

void D3D12RHI::Present()
{
	GetViewport()->Present();
}

void D3D12RHI::ResizeViewport(int NewWidth, int NewHeight)
{
	GetViewport()->OnResize(NewWidth, NewHeight);
}

void D3D12RHI::TransitionResource(Resource* resource, D3D12_RESOURCE_STATES StateAfter)
{
	D3D12_RESOURCE_STATES StateBefore = resource->currentState;

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource->D3DResource.Get(), StateBefore, StateAfter);
	if (StateBefore != StateAfter)
	{
		GetDevice()->GetCommandList()->ResourceBarrier(1, &barrier);

		resource->currentState = StateAfter;
	}
}

void D3D12RHI::CopyResource(Resource* DstResource, Resource* SrcResource)
{
	GetDevice()->GetCommandList()->CopyResource(DstResource->D3DResource.Get(), SrcResource->D3DResource.Get());
}

void D3D12RHI::CopyBufferRegion(Resource* DstResource, UINT64 DstOffset, Resource* SrcResource, UINT64 SrcOffset, UINT64 Size)
{
	GetDevice()->GetCommandList()->CopyBufferRegion(DstResource->D3DResource.Get(), DstOffset, SrcResource->D3DResource.Get(), SrcOffset, Size);
}

void D3D12RHI::CopyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION* Dst, UINT DstX, UINT DstY, UINT DstZ, const D3D12_TEXTURE_COPY_LOCATION* Src, const D3D12_BOX* SrcBox)
{
	GetDevice()->GetCommandList()->CopyTextureRegion(Dst, DstX, DstY, DstZ, Src, SrcBox);
}

void D3D12RHI::SetVertexBuffer(const VertexBufferRef& VertexBuffer, UINT Offset, UINT Stride, UINT Size)
{
	// Transition resource state
	const ResourceLocation& ResourceLocation = VertexBuffer->resourceLocation;
	Resource* resource = ResourceLocation.underlyingResource;
	TransitionResource(resource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_INDEX_BUFFER);

	// Set vertex buffer
	D3D12_VERTEX_BUFFER_VIEW VBV;
	VBV.BufferLocation = ResourceLocation.virtualAddressGPU + Offset;
	VBV.StrideInBytes = Stride;
	VBV.SizeInBytes = Size;
	GetDevice()->GetCommandList()->IASetVertexBuffers(0, 1, &VBV);
}

void D3D12RHI::SetIndexBuffer(const IndexBufferRef& IndexBuffer, UINT Offset, DXGI_FORMAT Format, UINT Size)
{
	// Transition resource state
	const ResourceLocation& resourceLocation = IndexBuffer->resourceLocation;
	Resource* resource = resourceLocation.underlyingResource;
	TransitionResource(resource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_INDEX_BUFFER);

	// Set vertex buffer
	D3D12_INDEX_BUFFER_VIEW IBV;
	IBV.BufferLocation = resourceLocation.virtualAddressGPU + Offset;
	IBV.Format = Format;
	IBV.SizeInBytes = Size;
	GetDevice()->GetCommandList()->IASetIndexBuffer(&IBV);
}

void D3D12RHI::EndFrame()
{
	// Clean memory allocations
	GetDevice()->GetUploadBufferAllocator()->CleanUpAllocations();

	GetDevice()->GetDefaultBufferAllocator()->CleanUpAllocations();

	GetDevice()->GetTextureResourceAllocator()->CleanUpAllocations();

	// CommandContext
	GetDevice()->GetCommandContext()->EndFrame();
}