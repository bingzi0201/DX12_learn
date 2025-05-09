#pragma once

#include <string>
#include "../Texture/TextureInfo.h"

bool CreateHDRTextureFromFile(
	std::string FileName,
	TextureInfo& textureInfo,
	D3D12_SUBRESOURCE_DATA& SubResource,
	std::vector<uint8_t>& DecodedData);
