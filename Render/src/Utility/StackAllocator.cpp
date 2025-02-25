#pragma vengine_package vengine_dll
#include "StackAllocator.h"
#include <optional>

StackAllocator::StackAllocator(uint64 initCapacity,
	IStackAllocVisitor* visitor)
	: capacity(initCapacity),
	  visitor(visitor){}

StackAllocator::Chunk StackAllocator::Allocate(uint64 targetSize)
{
	Buffer* bf = nullptr;
	uint64 minSize = std::numeric_limits<uint64>::max();
	// 遍历 allocatedBuffers，找到一个能满足 targetSize 需求的buffer
	for (auto&& i : allocatedBuffers )
	{
		// 优先使用最小剩余空间的Buffer
		if (i.leftSize >= targetSize && i.leftSize < minSize)
		{
			minSize = i.leftSize;
			bf = &i;
		}
	}
	// 如果找到了合适的 Buffer
	if (bf)
	{
		// 计算偏移量（从 fullSize - leftSize 开始分配）
		auto ofst = bf->fullSize - bf->leftSize;

		// 更新 leftSize，减少已使用的空间
		bf->leftSize -= targetSize;
		return
		{
			bf->handle,
			ofst
		};
	}

	// 如果找不到合适的 Buffer ，就创建新的
	// 当没有足够的 Buffer 时，动态扩容 capacity
	while (capacity < targetSize)
	{
		capacity = std::max<uint64>(capacity + 1, capacity * 1.5);
	}
	// 调用 visitor->Allocate(capacity) 申请新的 GPU 资源
	auto newHandle = visitor->Allocate(capacity);
	// 将新分配的 Buffer 添加到 allocatedBuffers，并返回它的句柄和偏移量
	allocatedBuffers.push_back(Buffer{
		newHandle,
		capacity,
		capacity - targetSize });
	return
	{
		newHandle,
		0
	};
}

StackAllocator::Chunk StackAllocator::Allocate(uint64 targetSize, uint64 align)
{
	// 计算对齐
	targetSize = std::max(targetSize, align);
	Buffer* bf = nullptr;
	uint64 offset = 0;
	uint64 minLeftSize = std::numeric_limits<uint64>::max();
	// 计算对齐后的偏移量（向上取整到 align 的整数倍）
	auto CalcAlign = [](uint64 value, uint64 align) -> uint64 {
		return (value + (align - 1)) & ~(align - 1);
	};
	struct Result
	{
		uint64 offset;
		uint64 leftSize;
	};
	// 计算对齐后的位置，确保 targetSize 分配后仍然在 fullSize 范围内
	auto GetLeftSize = [&](uint64 leftSize, uint64 size) -> std::optional<Result>
	{
		uint64 offset = size - leftSize;
		uint64 alignedOffset = CalcAlign(offset, align);
		uint64 afterAllocSize = targetSize + alignedOffset;
		if (afterAllocSize > size) return {};
		return Result{ alignedOffset, size - afterAllocSize };
	};
	// 遍历 allocatedBuffers，尝试找到合适的 Buffer
	for (auto&& i:allocatedBuffers)
	{
		auto result = GetLeftSize(i.leftSize, i.fullSize);
		if(!result.has_value()) continue;
		auto resultValue = result.value();
		if (resultValue.leftSize < minLeftSize)
		{
			minLeftSize = resultValue.leftSize;
			offset = resultValue.offset;
			bf = &i;
		}
	}
	if (bf)
	{
		bf->leftSize = minLeftSize;   // 确保下次分配内存时有足够空间
		return{
			bf->handle,
			offset };
	}
	while (capacity < targetSize)
	{
		capacity = std::max<uint64>(capacity + 1, capacity * 1.5);
	}
	auto newHandle = visitor->Allocate(capacity);
	allocatedBuffers.push_back(Buffer{
		newHandle,
		capacity,
		capacity - targetSize });
	return{
		newHandle,
		0
	};
}

void StackAllocator::Clear()
{
	for (auto&& i : allocatedBuffers)
	{
		i.leftSize = i.fullSize;
	}
}

StackAllocator::~StackAllocator()
{
	for (auto&& i : allocatedBuffers)
	{
		visitor->DeAllocate(i.handle);
	}
}