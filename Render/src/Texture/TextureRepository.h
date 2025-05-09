#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include "Texture.h"

class TextureRepository
{
public:
	static TextureRepository& Get();

	void Load();

	void Unload();

public:
	std::unordered_map<std::string /*TextureName*/, std::shared_ptr<Texture>> textureMap;
};
