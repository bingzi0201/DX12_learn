#pragma once

#include "Material.h"
#include "../Resource/Buffer.h"

class D3D12RHI;

class MaterialInstance
{
public:
	MaterialInstance(Material* parent, const std::string& inName);

public:
	void SetTextureParamter(const std::string& parameter, const std::string& textureName);

	void CreateMaterialConstantBuffer(D3D12RHI* d3d12RHI);

public:
	Material* material = nullptr;

	std::string name;

	MaterialParameters parameters;

	ConstantBufferRef materialConstantBuffer = nullptr;
};