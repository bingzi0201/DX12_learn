#pragma once

#include <string>
#include "TextureInfo.h"
#include "../Resource/D3D12Texture.h"
#include "../Resource/D3D12RHI.h"

struct TextureResource
{
	TextureInfo textureInfo;
	std::vector<uint8_t> textureData;
	std::vector<D3D12_SUBRESOURCE_DATA> initData;
};

class Texture
{
public:
	Texture(const std::string& InName, ETextureType InType, bool InbSRGB, std::wstring InFilePath)
		:name(InName), textureType(InType), bSRGB(InbSRGB), filePath(InFilePath)
	{}

	virtual ~Texture()
	{}

	Texture(const Texture& Other) = delete;

	Texture& operator=(const Texture& Other) = delete;

public:
	void LoadTextureResourceFromFlie(D3D12RHI* d3d12RHI);
	void SetTextureResourceDirectly(const TextureInfo& InTextureInfo, const std::vector<uint8_t>& InTextureData,
		const D3D12_SUBRESOURCE_DATA& InInitData);
	void CreateTexture(D3D12RHI* d3d12RHI);
	D3D12TextureRef GetD3DTexture() { return d3dTexture; }

private:
	static std::wstring GetExtension(std::wstring path);

	void LoadDDSTexture(Device* device);
	void LoadWICTexture(Device* device);
	void LoadHDRTexture(Device* device);

public:
	std::string name;
	ETextureType textureType;
	std::wstring filePath;
	bool bSRGB = true;
	TextureResource textureResource;
	D3D12TextureRef d3dTexture = nullptr;
};

class Texture2D : public Texture
{
public:
	Texture2D(const std::string& InName, bool InbSRGB, std::wstring InFilePath);
	~Texture2D();
};

class TextureCube : public Texture
{
public:
	TextureCube(const std::string& InName, bool InbSRGB, std::wstring InFilePath);
	~TextureCube();
};

class Texture3D : public Texture
{
public:
	Texture3D(const std::string& InName, bool InbSRGB, std::wstring InFilePath);
	~Texture3D();
};
