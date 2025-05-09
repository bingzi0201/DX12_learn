#include "CommandContext.h"
#include "Device.h"

CommandContext::CommandContext(Device* InDevice)
	:device(InDevice)
{
	CreateCommandContext();
	descriptorCache = std::make_unique<DescriptorCache>(device);
}

CommandContext::~CommandContext()
{
	DestroyCommandContext();
}

void CommandContext::CreateCommandContext()
{
	//Create fence
	ThrowIfFailed(device->GetD3DDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

	//Create direct type commandQueue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(device->GetD3DDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

	//Create direct type commandAllocator
	ThrowIfFailed(device->GetD3DDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(commandListAlloc.GetAddressOf())));

	//Create direct type commandList
	ThrowIfFailed(device->GetD3DDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandListAlloc.Get(),
		nullptr, IID_PPV_ARGS(commandList.GetAddressOf())));

	// Start off in a closed state. 
	// This is because the first time we refer to the command list we will Reset it,
	// and it needs to be closed before calling Reset.
	ThrowIfFailed(commandList->Close());
}

void CommandContext::DestroyCommandContext()
{
	//Don't need to do anything
	//Microsoft::WRL::ComPtr will destroy resource automatically
}

void CommandContext::ResetCommandAllocator()
{
	// Command list allocators can only be reset when the associated command lists have finished execution on the GPU.
	// So we should use fences to determine GPU execution progress(see FlushCommandQueue()).
	ThrowIfFailed(commandListAlloc->Reset());
}

void CommandContext::ResetCommandList()
{
	// Reusing the command list can reuses memory.
	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Before an app calls Reset, the command list must be in the "closed" state. 
	// After Reset succeeds, the command list is left in the "recording" state. 
	ThrowIfFailed(commandList->Reset(commandListAlloc.Get(), nullptr));
}

void CommandContext::ExecuteCommandLists()
{
	// Done recording commands.
	ThrowIfFailed(commandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
}

void CommandContext::FlushCommandQueue()
{
	// Advance the fence value to mark commands up to this fence point.
	currentFenceValue++;

	// Add an instruction to the command queue to set a new fence point.  
	// Because we are on the GPU timeline, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
	ThrowIfFailed(commandQueue->Signal(fence.Get(), currentFenceValue));

	// Wait until the GPU has completed commands up to this fence point.
	if (fence->GetCompletedValue() < currentFenceValue)
	{
		HANDLE eventHandle = CreateEvent(nullptr, false, false, nullptr);

		// Fire event when GPU hits current fence.  
		ThrowIfFailed(fence->SetEventOnCompletion(currentFenceValue, eventHandle));

		// Wait until the GPU hits current fence event is fired.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void CommandContext::EndFrame()
{
	descriptorCache->Reset();
}

