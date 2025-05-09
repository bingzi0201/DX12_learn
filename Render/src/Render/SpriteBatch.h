#pragma once

#include <string>
#include <vector>
#include "../Mesh/Vertex.h"
#include "../Resource/View.h"

struct SpriteItem
{
	ShaderResourceView* spriteSRV = nullptr;
	UIntPoint textureSize;
	RECT sourceRect;
	RECT destRect;
};

struct SpriteBatch
{
	std::vector<SpriteItem> spriteItems;
	VertexBufferRef vertexBufferRef = nullptr;
	IndexBufferRef indexBufferRef = nullptr;
};
