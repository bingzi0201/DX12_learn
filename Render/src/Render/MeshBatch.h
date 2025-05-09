#pragma once

#include "../Material/Material.h"
#include <string>
#include <unordered_map>

class MeshComponent;

struct MeshBatch
{
	std::string meshName;
	std::string inputLayoutName;

	ConstantBufferRef objConstantBuffer = nullptr;
	MeshComponent* meshComponent = nullptr;

	// Flags
	bool bUseSDF = false;
};

struct MeshCommand
{
	struct MeshShaderParamters
	{
		std::unordered_map<std::string, ConstantBufferRef> cbvParams;
		std::unordered_map<std::string, std::vector<ShaderResourceView*>> srvParams;
	};

public:
	void SetShaderParameter(std::string Param, ConstantBufferRef CBV)
	{
		shaderParameters.cbvParams.insert(std::make_pair(Param, CBV));
	}

	void SetShaderParameter(std::string Param, ShaderResourceView* SRV)
	{
		std::vector<ShaderResourceView*> SRVs;
		SRVs.push_back(SRV);

		shaderParameters.srvParams.insert(std::make_pair(Param, SRVs));
	}

	void SetShaderParameter(std::string Param, std::vector<ShaderResourceView*> SRVs)
	{
		shaderParameters.srvParams.insert(std::make_pair(Param, SRVs));
	}

	void ApplyShaderParamters(Shader* shader) const
	{
		if (shader)
		{
			for (const auto& pair : shaderParameters.cbvParams)
			{
				shader->SetParameter(pair.first, pair.second);
			}

			for (const auto& pair : shaderParameters.srvParams)
			{
				shader->SetParameter(pair.first, pair.second);
			}
		}
	}

public:
	std::string meshName;
	MaterialRenderState renderState;
	MeshShaderParamters shaderParameters;
};

typedef std::vector<MeshCommand> MeshCommandList;