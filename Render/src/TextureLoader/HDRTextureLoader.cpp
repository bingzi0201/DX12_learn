#include "HDRTextureLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

bool CreateHDRTextureFromFile(
	std::string FileName,
	TextureInfo& textureInfo,
	D3D12_SUBRESOURCE_DATA& SubResource,
	std::vector<uint8_t>& DecodedData)
{
	stbi_set_flip_vertically_on_load(true);
	int Width, Height, Components;
	float* Data = stbi_loadf(FileName.c_str(), &Width, &Height, &Components, 0);
	assert(Components == 3);

	if (Data)
	{
		LONG RowBytes = Width * Components * sizeof(float);
		LONG NumBytes = Width * Height * Components * sizeof(float);

		// DecodedData
		DecodedData.resize(NumBytes);
		memcpy_s(DecodedData.data(), NumBytes, Data, NumBytes);

		stbi_image_free(Data);

		// SubResource
		SubResource.pData = DecodedData.data();
		SubResource.RowPitch = static_cast<LONG>(RowBytes);
		SubResource.SlicePitch = static_cast<LONG>(NumBytes);

		// TextureInfo
		textureInfo.arraySize = 1;
		textureInfo.format = DXGI_FORMAT_R32G32B32_FLOAT;
		textureInfo.width = Width;
		textureInfo.height = Height;
		textureInfo.depth = 1;
		textureInfo.mipCount = 1;
		textureInfo.dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		return true;
	}

	return false;
}