#include "UpLoadBuffer.h"
#include "../Common/DXHelper.h"
#include "../DXRunTime/FrameResource.h"

UploadBuffer::UploadBuffer(Device* device, uint64_t byteSize) :
	Buffer(device), byteSize(byteSize) 
{
	auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto buffer = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
	ThrowIfFailed(device->DxDevice()->CreateCommittedResource(
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&buffer,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&resource)));
}
UploadBuffer::~UploadBuffer(){}

void UploadBuffer::CopyData(uint64 offset, std::span<vbyte const> data) const
{
	if (offset + data.size() > byteSize) // 防止越界写入
	{
		throw std::out_of_range("CopyData out of bounds!");
	}

	void* mappedPtr = nullptr;

	// Map时对于写操作传入nullptr，让D3D12知道我们不会读取GPU数据，可以优化性能
	ThrowIfFailed(resource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPtr)));

	// 进行偏移，并拷贝数据
	memcpy(reinterpret_cast<vbyte*>(mappedPtr) + offset, data.data(), data.size());

	// Unmap 时，通知 DX12 我们不关心 GPU 读取的范围，传 {0, 0}
	D3D12_RANGE emptyRange = { 0, 0 };
	resource->Unmap(0, &emptyRange);
}


void UploadBuffer::DelayDispose(FrameResource* frameRes) const {
	frameRes->AddDelayDisposeResource(resource);
}