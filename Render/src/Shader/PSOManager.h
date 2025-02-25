#pragma once
#include "../Common/stdafx.h"
#include <unordered_map>
#include "../Common/d3dx12.h"
#include "../Common/DXHelper.h"
#include "../Utility/Hash.h"
#include "../DXRunTime/Device.h"

// �ػ���ϣ����
namespace std
{
	template<>
	struct hash<D3D12_GRAPHICS_PIPELINE_STATE_DESC>
	{
		// ���� argument_type ��Ϊ��ϣ��������������
		using argument_type = D3D12_GRAPHICS_PIPELINE_STATE_DESC;
		// ���� result_type Ϊ size_t����ϣֵ�� size_t ���͵�������
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