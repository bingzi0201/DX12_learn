#include "Device.h"
#include "D3D12RHI.h"

using Microsoft::WRL::ComPtr;

Device::Device(D3D12RHI* InD3D12RHI)
	:d3d12RHI(InD3D12RHI)
{
	Initialize();
}

Device::~Device()
{

}

void Device::Initialize()
{
	// Try to create hardware device.
	HRESULT hardwareResult = D3D12CreateDevice(nullptr/*default adapter*/, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3dDevice));

	// Fallback to WARP device.
	if (FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> WarpAdapter;
		ThrowIfFailed(d3d12RHI->GetDxgiFactory()->EnumWarpAdapter(IID_PPV_ARGS(&WarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(WarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3dDevice)));
	}

	//Create CommandContext
	commandContext = std::make_unique<CommandContext>(this);

	//Create memory allocator
	uploadBufferAllocator = std::make_unique<UploadBufferAllocator>(d3dDevice.Get());
	defaultBufferAllocator = std::make_unique<DefaultBufferAllocator>(d3dDevice.Get());
	textureResourceAllocator = std::make_unique<TextureResourceAllocator>(d3dDevice.Get());

	//Create heapSlot allocator
	RTVHeapSlotAllocator = std::make_unique<HeapSlotAllocator>(d3dDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 200);
	DSVHeapSlotAllocator = std::make_unique<HeapSlotAllocator>(d3dDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 200);
	SRVHeapSlotAllocator = std::make_unique<HeapSlotAllocator>(d3dDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 200);
}

HeapSlotAllocator* Device::GetHeapSlotAllocator(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
{
	switch (heapType)
	{
	case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
		return SRVHeapSlotAllocator.get();
		break;

		//case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
		//	break;

	case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
		return RTVHeapSlotAllocator.get();
		break;

	case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
		return DSVHeapSlotAllocator.get();
		break;

	default:
		return nullptr;
	}
}
