#include "FrameResource.h"
#include "../Common/DXHelper.h"
#include "../Resource/Texture.h"
#include "../Shader/PSOManager.h"
#include "../Shader/RasterShader.h"
#include "../Resource/Mesh.h"

// 命令分配器和命令列表初始化
FrameResource::FrameResource(Device* device)
	: ubAlloc(TEMP_SIZE, &tempUBVisitor),
	rbAlloc(TEMP_SIZE, &tempRBVisitor),
	dbAlloc(TEMP_SIZE, &tempDBVisitor),
	device(device)
{
	tempDBVisitor.self = this;
	tempRBVisitor.self = this;
	tempUBVisitor.self = this;

	// 创建命令分配器
	ThrowIfFailed(device->DxDevice()->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(cmdAllocator.GetAddressOf())));

	// 创建命令列表
	ThrowIfFailed(device->DxDevice()->CreateCommandList(
		0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		cmdAllocator.Get(), nullptr,
		IID_PPV_ARGS(&cmdList)));

	// 初始关闭命令列表，等待执行
	ThrowIfFailed(cmdList->Close());
}


FrameResource::~FrameResource()
{
}

// 延迟资源释放
// 由于 GPU 资源需要在任务完成后再释放，这里将资源推入 delayDisposeResources，稍后释放。
// 在 Sync() 方法中，会清空这些资源。
void FrameResource::AddDelayDisposeResource(ComPtr<ID3D12Resource> const& ptr)
{
	delayDisposeResources.push_back(ptr);
}

void FrameResource::Signal(ID3D12CommandQueue* queue, ID3D12Fence* fence)
{
	if (!populated)
		return;
	queue->Signal(fence, lastFenceIndex);
}

// GPU 任务提交
void FrameResource::Execute(ID3D12CommandQueue* queue, ID3D12Fence* fence, uint64& fenceIndex)
{
	if (!populated)
		return;
	lastFenceIndex = ++fenceIndex;  // 更新 lastFenceIndex，用于同步 GPU 状态。
	ID3D12CommandList* ppCommandLists[] = { cmdList.Get() };
	queue->ExecuteCommandLists(array_count(ppCommandLists), ppCommandLists);
}

// GPU 同步
void FrameResource::Sync(ID3D12Fence* fence)
{
	// 确保 GPU 执行完 lastFenceIndex 之前的所有任务。
	if (!populated || lastFenceIndex == 0) return;
	if (fence->GetCompletedValue() < lastFenceIndex)
	{
		LPCWSTR falseValue = 0;
		HANDLE eventHandle = CreateEventEx(nullptr, falseValue, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(fence->SetEventOnCompletion(lastFenceIndex, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	// 释放 delayDisposeResources 资源，防止内存泄漏。
	delayDisposeResources.clear();

	// 执行 afterSyncEvents 事件队列，用于回调 CPU 任务。
	for (auto&& i : afterSyncEvents)
	{
		i();
	}

	// 清空 ubAlloc、dbAlloc、rbAlloc，释放分配的缓冲区。
	afterSyncEvents.clear();
	ubAlloc.Clear();
	dbAlloc.Clear();
	rbAlloc.Clear();
}

CommandListHandle FrameResource::Command() 
{
	populated = true;
	return { cmdAllocator.Get(), cmdList.Get() };
}

template<typename T>
uint64 FrameResource::Visitor<T>::Allocate(uint64 size)
{
	auto packPtr = new T(self->device, size);
	// 将 T* 转换为 Buffer*，再转换为 uint64（表示这个 Buffer 资源的唯一句柄）。
	// 这样 StackAllocator 可以存储 uint64 句柄，而不必知道 T 的具体类型。
	return reinterpret_cast<uint64>(static_cast<Buffer*>(packPtr));
}

template<typename T>
void FrameResource::Visitor<T>::DeAllocate(uint64 handle)
{
	delete reinterpret_cast<T*>(handle);
}

// 从 StackAllocator 申请一个 UploadBuffer 的 BufferView
BufferView FrameResource::GetTempBuffer(size_t size, size_t align, StackAllocator& alloc)
{
	auto chunk = [&]
	{
		if (align <= 1)
		{
			return alloc.Allocate(size);
		}
		return alloc.Allocate(size, align);
	}();
	auto package = reinterpret_cast<UploadBuffer*>(chunk.handle);
	return
	{
		package,
		chunk.offset,
		size
	};
}

// 资源上传到 UploadBuffer
void FrameResource::Upload(BufferView const& buffer, void const* src)
{
	auto uBuffer = GetTempBuffer(buffer.byteSize, 0, ubAlloc);
	static_cast<UploadBuffer const*>(uBuffer.buffer)
		->CopyData(uBuffer.offset,
			{ reinterpret_cast<vbyte const*>(src), size_t(uBuffer.byteSize) });
	CopyBuffer(uBuffer.buffer,
		buffer.buffer,
		uBuffer.offset,
		buffer.offset,
		buffer.byteSize);
}

BufferView FrameResource::AllocateConstBuffer(std::span<uint8_t const> data)
{
	auto tempBuffer = GetTempBuffer(data.size(), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, dbAlloc);
	Upload(tempBuffer, data.data());
	return tempBuffer;
}

// 从 GPU 读取数据回 CPU
void FrameResource::Download(BufferView const& buffer, void* dst)
{
	// 先把数据拷贝到 ReadBack Buffer
	auto rBuffer = GetTempBuffer(buffer.byteSize, 0, rbAlloc);
	// 等GPU任务完成后，再从 ReadBack Buffer 拷贝回dst
	afterSyncEvents.emplace_back([rBuffer, dst] {
		static_cast<ReadbackBuffer const*>(rBuffer.buffer)
			->CopyData(rBuffer.offset,
				{ reinterpret_cast<vbyte*>(dst), size_t(rBuffer.byteSize) });
		});
}

// 在 GPU 内部复制数据
void FrameResource::CopyBuffer(Buffer const* src, Buffer const* dst, uint64 srcOffset, uint64 dstOffset, uint64 byteSize)
{
	auto c = cmdList.Get();
	c->CopyBufferRegion(
		dst->GetResource(),
		dstOffset,
		src->GetResource(),
		srcOffset,
		byteSize);
}

// 设置渲染目标
void FrameResource::SetRenderTarget(Texture const* tex, CD3DX12_CPU_DESCRIPTOR_HANDLE const* rtvHandle, CD3DX12_CPU_DESCRIPTOR_HANDLE const* dsvHandle)
{
	auto desc = tex->GetResource()->GetDesc();
	CD3DX12_VIEWPORT viewPort(0.0f, 0.0f, desc.Width, desc.Height);
	CD3DX12_RECT scissorRect(0, 0, desc.Width, desc.Height);
	cmdList->RSSetViewports(1, &viewPort);
	cmdList->RSSetScissorRects(1, &scissorRect);
	cmdList->OMSetRenderTargets(1, rtvHandle, FALSE, dsvHandle);  // 绑定 RTV 和 DSV
}

void FrameResource::ClearRTV(CD3DX12_CPU_DESCRIPTOR_HANDLE const& rtv)
{
	cmdList->ClearRenderTargetView(rtv, Texture::CLEAR_COLOR, 0, nullptr);
}

void FrameResource::ClearDSV(CD3DX12_CPU_DESCRIPTOR_HANDLE const& dsv)
{
	cmdList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, Texture::CLEAR_DEPTH, Texture::CLEAR_STENCIL, 0, nullptr);
}

// 执行Draw Call
void FrameResource::DrawMesh(RasterShader const* shader, PSOManager* psoManager, Mesh* mesh, DXGI_FORMAT colorFormat, DXGI_FORMAT depthFormat, std::span<BindProperty> properties)
{
	// 绑定 Pipeline State
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->SetPipelineState(
		psoManager->GetPipelineState(mesh->Layout(),
			shader,
			{ &colorFormat, 1 },
			depthFormat));
	// 绑定 Vertex Buffer 和 Index Buffer。
	mesh->GetVertexBufferView(vertexBufferView);
	cmdList->IASetVertexBuffers(0, vertexBufferView.size(), vertexBufferView.data());
	D3D12_INDEX_BUFFER_VIEW indexBufferView = mesh->GetIndexBufferView();
	cmdList->IASetIndexBuffer(&indexBufferView);
	struct PropertyBinder 
	{
		ID3D12GraphicsCommandList* cmdList;
		Shader const* shader;
		std::string const* name;
		void operator()(BufferView const& bfView) const 
		{
			if (!shader->SetResource(
				*name,
				cmdList,
				bfView)) {
				throw "Invalid shader binding";
			}
		}
		void operator()(DescriptorHeapView const& descView) const 
		{
			if (!shader->SetResource(
				*name,
				cmdList,
				descView)) {
				throw "Invalid shader binding";
			}
		}
	};
	cmdList->SetGraphicsRootSignature(shader->RootSig());
	PropertyBinder binder
	{
		.cmdList = cmdList.Get(),
		.shader = shader };
	for (auto&& i :properties)
	{
		binder.name = &i.name;
		std::visit(binder, i.prop);
	}
	// 提交 Draw Call
	cmdList->DrawIndexedInstanced(
		mesh->IndexBuffer().GetByteSize() / 4,
		1,
		0,
		0,
		0);
}