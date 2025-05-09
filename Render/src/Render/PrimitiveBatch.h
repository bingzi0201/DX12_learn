#pragma once

#include "../Mesh/Vertex.h"

struct PrimitiveBatch
{
	D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D_PRIMITIVE_TOPOLOGY_LINELIST;

	VertexBufferRef vertexBufferRef = nullptr;

	int currentVertexNum = 0;
};
