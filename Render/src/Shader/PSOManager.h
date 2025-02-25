#pragma once
#include "../Common/stdafx.h"
#include <unordered_map>
#include "../Common/d3dx12.h"
#include "../Common/DXHelper.h"
#include "../Utility/Hash.h"
#include "../DXRunTime/Device.h"

// 特化哈希函数
namespace std
{
	template<>
	struct hash<D3D12_GRAPHICS_PIPELINE_STATE_DESC>
	{
		// 定义 argument_type 作为哈希函数的输入类型
		using argument_type = D3D12_GRAPHICS_PIPELINE_STATE_DESC;
		// 定义 result_type 为 size_t（哈希值是 size_t 类型的整数）
		using result_type = size_t;
		size_t operator()(argument_type const& v) const
		{
			return GetHash<argument_type>(v);
		}
	};

	template<>
	struct equal_to<D3D12_GRAPHICS_PIPELINE_STATE_DESC>
	{
		using argument_type = D3D12_GRAPHICS_PIPELINE_STATE_DESC;
		using result_type = size_t;
		bool operator()(D3D12_GRAPHICS_PIPELINE_STATE_DESC const& a, argument_type const& b) const {
			return memcmp(&a, &b, sizeof(argument_type)) == 0;
		}
	};
}

using Microsoft::WRL::ComPtr;
class RasterShader;
class PSOManager
{
	std::unordered_map<D3D12_GRAPHICS_PIPELINE_STATE_DESC, ComPtr<ID3D12PipelineState>> pipelineStates;
	Device* device;
	ID3D12PipelineState* GetPipelineState(D3D12_GRAPHICS_PIPELINE_STATE_DESC const& stateDesc);

public:
	PSOManager(Device* device);
	~PSOManager();
	ID3D12PipelineState* GetPipelineState(
		std::span<D3D12_INPUT_ELEMENT_DESC const> meshLayout,
		RasterShader const* shader,
		std::span<DXGI_FORMAT> rtvFormats,
		DXGI_FORMAT depthFormat,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
};