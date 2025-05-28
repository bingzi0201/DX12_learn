#include "MaterialRepository.h"

MaterialRepository& MaterialRepository::Get()
{
	static MaterialRepository Instance;
	return Instance;
}

void MaterialRepository::Load()
{
	//--------------------------------------DefaultMat------------------------------------------------------
	{
		// Material
		Material* defaultMat = CreateMaterial("DefaultMat", "BasePassDefault");

		MaterialParameters& parameters = defaultMat->parameters;
		parameters.textureMap.emplace("BaseColorTexture", "NullTex");
		parameters.textureMap.emplace("NormalTexture", "NullTex");
		parameters.textureMap.emplace("MetallicTexture", "NullTex");
		parameters.textureMap.emplace("RoughnessTexture", "NullTex");

		// MaterialInstances
		CreateDefaultMaterialInstance(defaultMat);

		{
			MaterialInstance* gunMatInst = CreateMaterialInstance(defaultMat, "GunInst");

			MaterialParameters& parameters = gunMatInst->parameters;
			parameters.emissiveColor = TVector3(1.0f);
			gunMatInst->SetTextureParamter("BaseColorTexture", "Gun_BaseColor");
			gunMatInst->SetTextureParamter("NormalTexture", "Gun_Normal");
			gunMatInst->SetTextureParamter("MetallicTexture", "Gun_Metallic");
			gunMatInst->SetTextureParamter("RoughnessTexture", "Gun_Roughness");
		}

		{
			MaterialInstance* gunMatInst = CreateMaterialInstance(defaultMat, "SphereInst");

			MaterialParameters& parameters = gunMatInst->parameters;
			parameters.emissiveColor = TVector3(1.0f);
			gunMatInst->SetTextureParamter("BaseColorTexture", "white1x1");
			gunMatInst->SetTextureParamter("NormalTexture", "white1x1");
			gunMatInst->SetTextureParamter("MetallicTexture", "white1x1");
			gunMatInst->SetTextureParamter("RoughnessTexture", "white1x1");
		}

		{
			MaterialInstance* emissiveMatInst = CreateMaterialInstance(defaultMat, "EmissiveMatInst");

			MaterialParameters& parameters = emissiveMatInst->parameters;
			parameters.emissiveColor = TVector3(1.0f);
			emissiveMatInst->SetTextureParamter("BaseColorTexture", "NullTex");
			emissiveMatInst->SetTextureParamter("NormalTexture", "NullTex");
			emissiveMatInst->SetTextureParamter("MetallicTexture", "NullTex");
			emissiveMatInst->SetTextureParamter("RoughnessTexture", "NullTex");
		}
	}



	//--------------------------------------EnviromentMat------------------------------------------------------
	{
		// Material
		Material* enviromentMat = CreateMaterial("HDRSkyMat", "BasePassSky");

		MaterialParameters& parameters = enviromentMat->parameters;
		parameters.textureMap.emplace("SkyCubeTexture", "poolbeg_2k");

		MaterialRenderState& renderState = enviromentMat->renderState;
		renderState.cullMode = D3D12_CULL_MODE_NONE;
		renderState.depthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		enviromentMat->shadingModel = EShadingMode::Unlit;

		// MaterialInstance
		CreateDefaultMaterialInstance(enviromentMat);
	}
}

void MaterialRepository::Unload()
{
	materialInstanceMap.clear();

	materialMap.clear();
}

Material* MaterialRepository::CreateMaterial(const std::string& materialName, const std::string& shaderName)
{
	materialMap.insert({ materialName, std::make_unique<Material>(materialName, shaderName) });

	return materialMap[materialName].get();
}

MaterialInstance* MaterialRepository::CreateMaterialInstance(Material* material, const std::string& materialInstanceName)
{
	materialInstanceMap.insert({ materialInstanceName, std::make_unique<MaterialInstance>(material, materialInstanceName) });

	return materialInstanceMap[materialInstanceName].get();
}

void MaterialRepository::CreateDefaultMaterialInstance(Material* material)
{
	CreateMaterialInstance(material, material->name + "Inst");
}

MaterialInstance* MaterialRepository::GetMaterialInstance(const std::string& materialInstanceName) const
{
	MaterialInstance* result = nullptr;

	auto Iter = materialInstanceMap.find(materialInstanceName);
	if (Iter != materialInstanceMap.end())
	{
		result = Iter->second.get();
	}

	return result;
}