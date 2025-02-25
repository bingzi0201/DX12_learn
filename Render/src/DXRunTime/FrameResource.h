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
	ComPtr<ID3D12CommandAllocator> cmdAllocator;  // ���������
	ComPtr<ID3D12GraphicsCommandList> cmdList;    // �����б�
	std::vector<ComPtr<ID3D12Resource>> delayDisposeResources; // �ӳ�ִ�е��¼����У��� GPU ������ɺ���Щ�¼��ᱻִ��
	std::vector<std::function<void()>> afterSyncEvents;  // ��ͬ����ִ�еĻص��¼�
	uint64 lastFenceIndex = 0;  // ����ύ�� Fence ֵ
	bool populated = false; // ��¼�����Ƿ����ύ

	static constexpr size_t TEMP_SIZE = 1024ull * 1024ull;

	//  ���� StackAllocator ������ GPU ��Դ����
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
	BufferView GetTempBuffer(size_t size, size_t align, StackAllocator& alloc);  // �� StackAllocator ����һ�� UploadBuffer �� BufferView

public:
	CommandListHandle Command();
	FrameResource(Device* device);
	~FrameResource();
	void AddDelayDisposeResource(ComPtr<ID3D12Resource> const& ptr);
	void Execute(ID3D12CommandQueue* queue, ID3D12Fence* fence, uint64& fenceIndex);
	void Signal(ID3D12CommandQueue* queue, ID3D12Fence* fence);
	void Sync(ID3D12Fence* fence);
	void Upload(BufferView const& buffer, void const* src);  // ��Դ�ϴ��� UploadBuffer
	void Download(BufferView const& buffer, void* dst);  // �� GPU ��ȡ���ݻ� CPU
	BufferView AllocateConstBuffer(std::span<uint8_t const> data);
	void CopyBuffer(Buffer const* src,
		Buffer const* dst,
		uint64 srcOffset,
		uint64 dstOffset,
		uint64 byteSize);  // �� GPU �ڲ���������
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