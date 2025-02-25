#include "Buffer.h"

BufferView::BufferView(Buffer const* buffer):
	buffer(buffer),
	byteSize(buffer?buffer->GetByteSize():0),
	offset(0){}

BufferView::BufferView(
	Buffer const* buffer, 
	uint64_t offset, 
	uint64_t byteSize):
	buffer(buffer),
	offset(offset),
	byteSize(byteSize){}

Buffer::Buffer(Device* device):
	Resource(device){}

Buffer::~Buffer(){}