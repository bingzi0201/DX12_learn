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
	// ���� allocatedBuffers���ҵ�һ�������� targetSize �����buffer
	for (auto&& i : allocatedBuffers )
	{
		// ����ʹ����Сʣ��ռ��Buffer
		if (i.leftSize >= targetSize && i.leftSize < minSize)
		{
			minSize = i.leftSize;
			bf = &i;
		}
	}
	// ����ҵ��˺��ʵ� Buffer
	if (bf)
	{
		// ����ƫ�������� fullSize - leftSize ��ʼ���䣩
		auto ofst = bf->fullSize - bf->leftSize;

		// ���� leftSize��������ʹ�õĿռ�
		bf->leftSize -= targetSize;
		return
		{
			bf->handle,
			ofst
		};
	}

	// ����Ҳ������ʵ� Buffer ���ʹ����µ�
	// ��û���㹻�� Buffer ʱ����̬���� capacity
	while (capacity < targetSize)
	{
		capacity = std::max<uint64>(capacity + 1, capacity * 1.5);
	}
	// ���� visitor->Allocate(capacity) �����µ� GPU ��Դ
	auto newHandle = visitor->Allocate(capacity);
	// ���·���� Buffer ��ӵ� allocatedBuffers�����������ľ����ƫ����
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
	// �������
	targetSize = std::max(targetSize, align);
	Buffer* bf = nullptr;
	uint64 offset = 0;
	uint64 minLeftSize = std::numeric_limits<uint64>::max();
	// ���������ƫ����������ȡ���� align ����������
	auto CalcAlign = [](uint64 value, uint64 align) -> uint64 {
		return (value + (align - 1)) & ~(align - 1);
	};
	struct Result
	{
		uint64 offset;
		uint64 leftSize;
	};
	// ���������λ�ã�ȷ�� targetSize �������Ȼ�� fullSize ��Χ��
	auto GetLeftSize = [&](uint64 leftSize, uint64 size) -> std::optional<Result>
	{
		uint64 offset = size - leftSize;
		uint64 alignedOffset = CalcAlign(offset, align);
		uint64 afterAllocSize = targetSize + alignedOffset;
		if (afterAllocSize > size) return {};
		return Result{ alignedOffset, size - afterAllocSize };
	};
	// ���� allocatedBuffers�������ҵ����ʵ� Buffer
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
		bf->leftSize = minLeftSize;   // ȷ���´η����ڴ�ʱ���㹻�ռ�
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