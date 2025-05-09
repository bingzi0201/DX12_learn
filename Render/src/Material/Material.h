#pragma once

#include <string>
#include <unordered_map>
#include "../Utils/D3D12Utils.h"
#include "../Shader/Shader.h"
#include "../Math/Math.h"

enum class EShadingMode
{
	DefaultLit,
	Unlit,
};

struct MaterialParameters
{
public:
	TVector4 diffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	TVector3 fresnelR0 = { 0.01f, 0.01f, 0.01f };
	float roughness = 64.0f;
	TVector3 emissiveColor = { 0.0f, 0.0f, 0.0f };
	// Used in texture mapping.
	TMatrix matTransform = TMatrix::Identity;

	std::unordered_map<std::string/*parameter*/, std::string/*textureName*/> textureMap;
};

struct MaterialRenderState
{
	D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_NONE;
	D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS;
};


class Material
{
public:
	Material(const std::string& inName, const std::string& inShaderName);

	Shader* GetShader(const ShaderDefines& shaderDefines, D3D12RHI* d3d12RHI);

public:
	std::string name;
	EShadingMode shadingModel = EShadingMode::DefaultLit;
	MaterialParameters parameters;
	MaterialRenderState renderState;

private:
	std::string shaderName;
	std::unordered_map<ShaderDefines, std::unique_ptr<Shader>> shaderMap;
};
