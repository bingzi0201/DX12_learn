#pragma once

#include <unordered_map>
#include <wrl/client.h>
#include "../Resource/Resource.h"
#include "../Resource/D3D12RHI.h"

using Microsoft::WRL::ComPtr;

enum class EShaderType
{
	VERTEX_SHADER,
	PIXEL_SHADER,
	COMPUTE_SHADER,
};

struct ShaderDefines
{
public:
	void GetD3DShaderMacro(std::vector<D3D_SHADER_MACRO>& outMacros) const;
	bool operator == (const ShaderDefines& other) const;
	void SetDefine(const std::string& name, const std::string& definition);

public:
	std::unordered_map<std::string, std::string> definesMap;
};

// declare hash<ShaderDefines>
namespace std
{
	template <>
	struct hash<ShaderDefines>
	{
		std::size_t operator()(const ShaderDefines& defines) const
		{
			using std::size_t;
			using std::hash;
			using std::string;
			// Compute individual hash values for each string 
			// and combine them using XOR
			// and bit shifting:

			size_t hashValue = 0;
			for (const auto& pair : defines.definesMap)
			{
				hashValue ^= (hash<string>()(pair.first) << 1);
				hashValue ^= (hash<string>()(pair.second) << 1);
			}

			return hashValue;
		}
	};
}

struct ShaderParameter
{
	std::string name;
	EShaderType shaderType;
	UINT bindPoint;
	UINT registerSpace;
};

struct ShaderCBVParameter : ShaderParameter
{
	ConstantBufferRef constantBufferRef;
};

struct ShaderSRVParameter : ShaderParameter
{
	UINT bindCount;
	std::vector<ShaderResourceView*> srvList;
};

struct ShaderUAVParameter : ShaderParameter
{
	UINT bindCount;
	std::vector<UnorderedAccessView*> uavList;
};

struct ShaderSamplerParameter : ShaderParameter
{

};

struct ShaderInfo
{
	std::string shaderName;
	std::string fileName;
	ShaderDefines shaderDefines;

	bool bCreateVS = false;
	std::string vsEntryPoint = "VS";

	bool bCreatePS = false;
	std::string psEntryPoint = "PS";

	bool bCreateCS = false;
	std::string csEntryPoint = "CS";
};

class Shader
{
public:
	Shader(const ShaderInfo& inShaderInfo, D3D12RHI* inD3D12RHI);

	void Initialize();
	bool SetParameter(std::string paramName, ConstantBufferRef constantBufferRef);
	bool SetParameter(std::string paramName, ShaderResourceView* srv);
	bool SetParameter(std::string paramName, const std::vector<ShaderResourceView*>& srvList);
	bool SetParameter(std::string paramName, UnorderedAccessView* uav);
	bool SetParameter(std::string paramName, const std::vector<UnorderedAccessView*>& uavList);
	void BindParameters();

private:
	static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(const std::wstring& filename, const D3D_SHADER_MACRO* defines, const std::string& entrypoint, const std::string& target);
	void GetShaderParameters(ComPtr<ID3DBlob> passBlob, EShaderType shaderType);
	D3D12_SHADER_VISIBILITY GetShaderVisibility(EShaderType shaderType);
	std::vector<CD3DX12_STATIC_SAMPLER_DESC> CreateStaticSamplers();
	void CreateRootSignature();
	void CheckBindings();
	void ClearBindings();

public:
	ShaderInfo shaderInfo;
	std::vector<ShaderCBVParameter> cbvParams;
	std::vector<ShaderSRVParameter> srvParams;
	std::vector<ShaderUAVParameter> uavParams;
	std::vector<ShaderSamplerParameter> samplerParams;

	int cbvSignatureBaseBindSlot = -1;
	int srvSignatureBindSlot = -1;
	UINT srvCount = 0;
	int uavSignatureBindSlot = -1;
	UINT uavCount = 0;
	int samplerSignatureBindSlot = -1;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> shaderPass;
	ComPtr<ID3D12RootSignature> rootSignature;

private:
	D3D12RHI* d3d12RHI = nullptr;
};