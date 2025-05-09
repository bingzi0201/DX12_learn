#pragma once

#include "../Utils/D3D12Utils.h"
#include "InputLayout.h"
#include "../Shader/Shader.h"
#include <unordered_map>

//-------------------------------------------------------------------------//
// GraphicsPSO
//-------------------------------------------------------------------------//

struct GraphicsPSODescriptor
{
	bool operator==(const GraphicsPSODescriptor& other) const //TODO
	{
		return other.inputLayoutName == inputLayoutName
			&& other.shader == shader
			&& other.primitiveTopologyType == primitiveTopologyType
			&& other.rasterizerDesc.CullMode == rasterizerDesc.CullMode
			&& other.depthStencilDesc.DepthFunc == depthStencilDesc.DepthFunc;
	}

public:
	std::string inputLayoutName;
	Shader* shader = nullptr;
	DXGI_FORMAT RTVFormats[8] = { DXGI_FORMAT_R8G8B8A8_UNORM };
	bool _4xMsaaState = false;
	UINT _4xMsaaQuality = 0;
	DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	D3D12_RASTERIZER_DESC rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	UINT numRenderTargets = 1;
};

// declare hash<GraphicsPSODescriptor>
namespace std
{
	template <>
	struct hash<GraphicsPSODescriptor>
	{
		std::size_t operator()(const GraphicsPSODescriptor& descriptor) const
		{
			using std::hash;
			using std::string;

			// Compute individual hash values for each item,
			// and combine them using XOR
			// and bit shifting:
			return (hash<string>()(descriptor.inputLayoutName)
				^ (hash<void*>()(descriptor.shader) << 1));
		}
	};
}

class D3D12RHI;

class GraphicsPSOManager
{
public:
	GraphicsPSOManager(D3D12RHI* inD3D12RHI, InputLayoutManager* inInputLayoutManager);
	void TryCreatePSO(const GraphicsPSODescriptor& descriptor);
	ID3D12PipelineState* GetPSO(const GraphicsPSODescriptor& descriptor) const;

private:
	void CreatePSO(const GraphicsPSODescriptor& descriptor);

private:
	D3D12RHI* d3d12RHI = nullptr;
	InputLayoutManager* inputLayoutManager = nullptr;
	std::unordered_map<GraphicsPSODescriptor, Microsoft::WRL::ComPtr<ID3D12PipelineState>> PSOMap;
};

//-------------------------------------------------------------------------//
// ComputePSO
//-------------------------------------------------------------------------//

struct ComputePSODescriptor
{
	bool operator==(const ComputePSODescriptor& Other) const
	{
		return Other.shader == shader
			&& Other.Flags == Flags;
	}

public:
	Shader* shader = nullptr;
	D3D12_PIPELINE_STATE_FLAGS Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
};

// declare hash<TComputePSODescriptor>
namespace std
{
	template <>
	struct hash<ComputePSODescriptor>
	{
		std::size_t operator()(const ComputePSODescriptor& descriptor) const
		{
			using std::hash;
			using std::string;

			// Compute individual hash values for each item,
			// and combine them using XOR
			// and bit shifting:
			return (hash<void*>()(descriptor.shader)
				^ (hash<int>()(descriptor.Flags) << 1));
		}
	};
}

class ComputePSOManager
{
public:
	ComputePSOManager(D3D12RHI* inD3D12RHI);
	void TryCreatePSO(const ComputePSODescriptor& descriptor);
	ID3D12PipelineState* GetPSO(const ComputePSODescriptor& descriptor) const;

private:
	void CreatePSO(const ComputePSODescriptor& descriptor);

private:
	class D3D12RHI* d3d12RHI = nullptr;

	std::unordered_map<ComputePSODescriptor, Microsoft::WRL::ComPtr<ID3D12PipelineState>> PSOMap;
};