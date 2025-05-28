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

	// LUT
	textureMap.emplace("IBL_BRDF_LUT", std::make_shared<Texture2D>("IBL_BRDF_LUT", false, TextureDir + L"IBL_BRDF_LUT.png"));
	// HDR
	//textureMap.emplace("bloem_hill", std::make_shared<Texture2D>("bloem_hill", false, TextureDir + L"bloem_hill_01_2k.hdr"));
	textureMap.emplace("poolbeg_2k", std::make_shared<Texture2D>("poolbeg_2k", false, TextureDir + L"poolbeg_2k.hdr"));

	// Blue Noise
	textureMap.emplace("SRBN_RG", std::make_shared<Texture2D>("SRBN_RG", false, TextureDir + L"stbn_RG.dds"));
}

void TextureRepository::Unload()
{
	textureMap.clear();
}