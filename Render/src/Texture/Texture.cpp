#include "Texture.h"
#include "../TextureLoader/DDSTextureLoader.h"
#include "../TextureLoader/WICTextureLoader.h"
#include "../TextureLoader/HDRTextureLoader.h"
#include "../Utils/FormatConvert.h"

void Texture::LoadTextureResourceFromFlie(D3D12RHI* d3d12RHI)
{
	std::wstring ext = GetExtension(filePath);
	if (ext == L"dds")
	{
		LoadDDSTexture(d3d12RHI->GetDevice());
	}
	else if (ext == L"png" || ext == L"jpg")
	{
		LoadWICTexture(d3d12RHI->GetDevice());
	}
	else if (ext == L"hdr")
	{
		LoadHDRTexture(d3d12RHI->GetDevice());
	}
}

std::wstring Texture::GetExtension(std::wstring path)
{
	if ((path.rfind('.') != std::wstring::npos) && (path.rfind('.') != (path.length() - 1)))
		return path.substr(path.rfind('.') + 1);
	else
		return L"";
}

void Texture::LoadDDSTexture(Device* device)
{
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile(filePath.c_str(), textureResource.textureInfo,
		textureResource.initData, textureResource.textureData, bSRGB));
}

void Texture::LoadWICTexture(Device* device)
{
	D3D12_SUBRESOURCE_DATA InitData;

	DirectX::WIC_LOADER_FLAGS LoadFlags;
	if (bSRGB)
	{
		LoadFlags = DirectX::WIC_LOADER_FORCE_SRGB;
	}
	else
	{
		LoadFlags = DirectX::WIC_LOADER_IGNORE_SRGB;
	}

	ThrowIfFailed(DirectX::CreateWICTextureFromFile(filePath.c_str(), 0u, D3D12_RESOURCE_FLAG_NONE, LoadFlags,
		textureResource.textureInfo, InitData, textureResource.textureData));

	textureResource.initData.push_back(InitData);
}

void Texture::LoadHDRTexture(Device* device)
{
	D3D12_SUBRESOURCE_DATA InitData;

	CreateHDRTextureFromFile(FormatConvert::WStrToStr(filePath), textureResource.textureInfo, InitData, textureResource.textureData);

	textureResource.initData.push_back(InitData);
}

void Texture::SetTextureResourceDirectly(const TextureInfo& InTextureInfo, const std::vector<uint8_t>& InTextureData, const D3D12_SUBRESOURCE_DATA& InInitData)
{
	textureResource.textureInfo = InTextureInfo;
	textureResource.textureData = InTextureData;

	D3D12_SUBRESOURCE_DATA InitData;
	InitData.pData = textureResource.textureData.data();
	InitData.RowPitch = InInitData.RowPitch;
	InitData.SlicePitch = InInitData.SlicePitch;

	textureResource.initData.push_back(InitData);
}

void Texture::CreateTexture(D3D12RHI* d3d12RHI)
{
	auto CommandList = d3d12RHI->GetDevice()->GetCommandList();

	//Create D3DTexture
	auto& TextureInfo = textureResource.textureInfo;
	TextureInfo.textureType = textureType;
	d3dTexture = d3d12RHI->CreateTexture(TextureInfo, TexCreate_SRV);

	//Upload InitData
	d3d12RHI->UploadTextureData(d3dTexture, textureResource.initData);
}


Texture2D::Texture2D(const std::string& InName, bool InbSRGB, std::wstring InFilePath)
	:Texture(InName, ETextureType::TEXTURE_2D, InbSRGB, InFilePath)
{

}

Texture2D::~Texture2D()
{

}

TextureCube::TextureCube(const std::string& InName, bool InbSRGB, std::wstring InFilePath)
	:Texture(InName, ETextureType::TEXTURE_CUBE, InbSRGB, InFilePath)
{

}

TextureCube::~TextureCube()
{

}

Texture3D::Texture3D(const std::string& InName, bool InbSRGB, std::wstring InFilePath)
	:Texture(InName, ETextureType::TEXTURE_3D, InbSRGB, InFilePath)
{

}

Texture3D::~Texture3D()
{

}