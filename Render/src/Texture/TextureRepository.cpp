#include "TextureRepository.h"
#include "../File/FileHelpers.h"

TextureRepository& TextureRepository::Get()
{
	static TextureRepository Instance;
	return Instance;
}

void TextureRepository::Load()
{
	std::wstring TextureDir = L"Resources\\Textures\\";

	textureMap.emplace("NullTex", std::make_shared<Texture2D>("NullTex", false, TextureDir + L"white1x1.dds"));

	// PBR textures
	textureMap.emplace("Gun_BaseColor", std::make_shared<Texture2D>("Gun_BaseColor", true, TextureDir + L"Gun_BaseColor.png"));
	textureMap.emplace("Gun_Normal", std::make_shared<Texture2D>("Gun_Normal", false, TextureDir + L"Gun_Normal.png"));
	textureMap.emplace("Gun_Roughness", std::make_shared<Texture2D>("Gun_Roughness", false, TextureDir + L"Gun_Roughness.png"));
	textureMap.emplace("Gun_Metallic", std::make_shared<Texture2D>("Gun_Metallic", false, TextureDir + L"Gun_Metallic.png"));
}

void TextureRepository::Unload()
{
	textureMap.clear();
}