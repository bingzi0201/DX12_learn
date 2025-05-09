#pragma once

#include "../Math/Math.h"
#include <string>

struct TSprite
{
	TSprite() {}
	TSprite(const std::string& InTextureName, const UIntPoint& InTextureSize, const RECT& InSourceRect, const RECT& InDestRect)
		:textureName(InTextureName), textureSize(InTextureSize), sourceRect(InSourceRect), destRect(InDestRect)
	{}

	std::string textureName;
	UIntPoint textureSize;
	RECT sourceRect;
	RECT destRect;
};
