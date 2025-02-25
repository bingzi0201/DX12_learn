#pragma once
#include "Device.h"
#include "CommandListHandle.h"
#include "../Resource/UpLoadBuffer.h"
#include "../Resource/ReadbackBuffer.h"
#include "../Resource/DefaultBuffer.h"
#include "../Utility/StackAllocator.h"
#include <functional>
#include "../Resource/DescriptorHeapView.h"
#include "BindProperty.h"

using Microsoft::WRL::ComPtr;

class Texture;
class Mesh;
class PSOManager;
class RasterShader;

class FrameResource
{
	ComPtr<ID3D12CommandAllocator> cmdAllocator;  // 命令分配器
	ComPtr<ID3D12GraphicsCommandList> cmdList;    // 命令列表
	std::vector<ComPtr<ID3D12Resource>> delayDisposeResources; // 延迟执行的事件队列，当 GPU 任务完成后，这些事件会被执行
	std::vector<std::function<void()>> afterSyncEvents;  // 在同步后执行的回调事件
	uint64 lastFenceIndex = 0;  // 最后提交的 Fence 值
	bool populated = false; // 记录命令是否已提交

	static constexpr size_t TEMP_SIZE = 1024ull * 1024ull;

	//  解耦 StackAllocator 与具体的 GPU 资源类型
	template<typename T>
	class Visitor :public IStackAllocVisitor
	{
	public:
		FrameResource* self;
		uint64 Allocate(uint64 size) override;
		void DeAllocate(uint64 handle) override;
	};

	Visitor<UploadBuffer> tempUBVisitor;
	Visitor<ReadbackBuffer> tempRBVisitor;
	Visitor<DefaultBuffer> tempDBVisitor;

	StackAllocator ubAlloc;
	StackAllocator rbAlloc;
	StackAllocator dbAlloc;

	Device* device;
	std::vector<D3D12_VERTEX_BUFFER_VIEW> vertexBufferView;
	BufferView GetTempBuffer(size_t size, size_t align, StackAllocator& alloc);  // 从 StackAllocator 申请一个 UploadBuffer 的 BufferView

public:
	CommandListHandle Command();
	FrameResource(Device* device);
	~FrameResource();
	void AddDelayDisposeResource(ComPtr<ID3D12Resource> const& ptr);
	void Execute(ID3D12CommandQueue* queue, ID3D12Fence* fence, uint64& fenceIndex);
	void Signal(ID3D12CommandQueue* queue, ID3D12Fence* fence);
	void Sync(ID3D12Fence* fence);
	void Upload(BufferView const& buffer, void const* src);  // 资源上传到 UploadBuffer
	void Download(BufferView const& buffer, void* dst);  // 从 GPU 读取数据回 CPU
	BufferView AllocateConstBuffer(std::span<uint8_t const> data);
	void CopyBuffer(Buffer const* src,
		Buffer const* dst,
		uint64 srcOffset,
		uint64 dstOffset,
		uint64 byteSize);  // 在 GPU 内部复制数据
	void SetRenderTarget(Texture const* tex,
		CD3DX12_CPU_DESCRIPTOR_HANDLE const* rtvHandle,
		CD3DX12_CPU_DESCRIPTOR_HANDLE const* dsvHandle = nullptr);
	void ClearRTV(CD3DX12_CPU_DESCRIPTOR_HANDLE const& rtv);
	void ClearDSV(CD3DX12_CPU_DESCRIPTOR_HANDLE const& dsv);
	void DrawMesh(RasterShader const* shader,
		PSOManager* psoManager,
		Mesh* mesh,
		DXGI_FORMAT colorFormat,
		DXGI_FORMAT depthFormat,
		std::span<BindProperty> properties);
};