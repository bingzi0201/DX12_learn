#pragma once
#include "../Common/stdafx.h"

// ������Դ�����𴴽�Buffer
class IStackAllocVisitor
{
public:
	virtual uint64 Allocate(uint64 size) = 0;
	virtual void DeAllocate(uint64 handle) = 0;
};

// �����ڴ��
class StackAllocator
{
	struct Buffer
	{
		uint64 handle;     // ��Դ�����GPU ��Դ ID��
		uint64 fullSize;   // ��Դ�ܴ�С
		uint64 leftSize;   // ʣ����ÿռ�
	};

	IStackAllocVisitor* visitor;    // ���ʽӿڣ�����ײ� GPU ��Դ�ķ�����ͷ�
	uint64 capacity;                // �ڴ�ص�ǰ����
	std::vector<Buffer> allocatedBuffers; // ��¼�ѷ���� GPU ��Դ

public:
	StackAllocator(uint64 initCapacity, 
		IStackAllocVisitor* visitor);
	~StackAllocator();
	struct Chunk
	{
		uint64 handle;
		uint64 offset;
	};
	// �����е�Buffer�ﻮ�ֿռ�
	Chunk Allocate(uint64 targetSize);  // �����ڴ棨�����Ƕ��룩
	Chunk Allocate(uint64 targetSize, uint64 align);  // ��������ڴ�
	void Clear();
};
