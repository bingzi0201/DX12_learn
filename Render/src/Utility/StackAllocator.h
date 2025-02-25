#pragma once
#include "../Common/stdafx.h"

// 管理资源，负责创建Buffer
class IStackAllocVisitor
{
public:
	virtual uint64 Allocate(uint64 size) = 0;
	virtual void DeAllocate(uint64 handle) = 0;
};

// 管理内存池
class StackAllocator
{
	struct Buffer
	{
		uint64 handle;     // 资源句柄（GPU 资源 ID）
		uint64 fullSize;   // 资源总大小
		uint64 leftSize;   // 剩余可用空间
	};

	IStackAllocVisitor* visitor;    // 访问接口，负责底层 GPU 资源的分配和释放
	uint64 capacity;                // 内存池当前容量
	std::vector<Buffer> allocatedBuffers; // 记录已分配的 GPU 资源

public:
	StackAllocator(uint64 initCapacity, 
		IStackAllocVisitor* visitor);
	~StackAllocator();
	struct Chunk
	{
		uint64 handle;
		uint64 offset;
	};
	// 在已有的Buffer里划分空间
	Chunk Allocate(uint64 targetSize);  // 申请内存（不考虑对齐）
	Chunk Allocate(uint64 targetSize, uint64 align);  // 申请对齐内存
	void Clear();
};
