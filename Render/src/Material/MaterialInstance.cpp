#include "MaterialInstance.h"
#include "../Render/RenderProxy.h"
#include "../Resource/D3D12RHI.h"

MaterialInstance::MaterialInstance(Material* parent, const std::string& inName)
	:material(parent), name(inName)
{
	parameters = material->parameters;
}

void MaterialInstance::SetTextureParamter(const std::string& parameter, const std::string& textureName)
{
	auto Iter = parameters.textureMap.find(parameter);

	if (Iter != parameters.textureMap.end())
	{
		Iter->second = textureName;
	}
}

void MaterialInstance::CreateMaterialConstantBuffer(D3D12RHI* D3D12RHI)
{
	MaterialConstants MatConst;

	//Get material params
	MatConst.diffuseAlbedo = parameters.diffuseAlbedo;
	MatConst.fresnelR0 = parameters.fresnelR0;
	MatConst.roughness = parameters.roughness;
	MatConst.matTransform = parameters.matTransform;
	MatConst.emissiveColor = parameters.emissiveColor;
	MatConst.shadingModel = (UINT)material->shadingModel;

	//Create ConstantBuffer
	materialConstantBuffer = D3D12RHI->CreateConstantBuffer(&MatConst, sizeof(MatConst));
}