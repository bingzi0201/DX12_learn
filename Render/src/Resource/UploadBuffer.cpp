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
	if (offset + data.size() > byteSize) // ��ֹԽ��д��
	{
		throw std::out_of_range("CopyData out of bounds!");
	}

	void* mappedPtr = nullptr;

	// Mapʱ����д��������nullptr����D3D12֪�����ǲ����ȡGPU���ݣ������Ż�����
	ThrowIfFailed(resource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPtr)));

	// ����ƫ�ƣ�����������
	memcpy(reinterpret_cast<vbyte*>(mappedPtr) + offset, data.data(), data.size());

	// Unmap ʱ��֪ͨ DX12 ���ǲ����� GPU ��ȡ�ķ�Χ���� {0, 0}
	D3D12_RANGE emptyRange = { 0, 0 };
	resource->Unmap(0, &emptyRange);
}


void UploadBuffer::DelayDispose(FrameResource* frameRes) const {
	frameRes->AddDelayDisposeResource(resource);
}