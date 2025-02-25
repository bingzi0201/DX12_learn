#pragma once
#include "Resource.h"
#include "BufferView.h"

class Buffer : public Resource
{
public:
	Buffer(Device* device);
	virtual D3D12_GPU_VIRTUAL_ADDRESS GetAddress() const = 0;
	virtual uint64_t  GetByteSize() const = 0;
	virtual ~Buffer();
	Buffer(Buffer&&) = default;
};
