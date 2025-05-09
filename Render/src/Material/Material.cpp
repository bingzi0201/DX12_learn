#include "Material.h"

Material::Material(const std::string& inName, const std::string& inShaderName)
	:name(inName),
	shaderName(inShaderName)
{
}

Shader* Material::GetShader(const ShaderDefines& shaderDefines, D3D12RHI* d3d12RHI)
{
	auto iter = shaderMap.find(shaderDefines);

	if (iter == shaderMap.end())
	{
		// Create new shader
		ShaderInfo shaderInfo;
		shaderInfo.shaderName = shaderName;
		shaderInfo.fileName = shaderName;
		shaderInfo.shaderDefines = shaderDefines;
		shaderInfo.bCreateVS = true;
		shaderInfo.bCreatePS = true;
		std::unique_ptr<Shader> newShader = std::make_unique<Shader>(shaderInfo, d3d12RHI);

		shaderMap.insert({ shaderDefines, std::move(newShader) });

		return shaderMap[shaderDefines].get();
	}
	else
	{
		return iter->second.get();
	}
}