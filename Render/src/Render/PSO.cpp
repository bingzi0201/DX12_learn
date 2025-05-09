#include "PSO.h"
#include "../Shader/Shader.h"
#include "../Resource/D3D12RHI.h"

GraphicsPSOManager::GraphicsPSOManager(D3D12RHI* inD3D12RHI, InputLayoutManager* inInputLayoutManager)
	:d3d12RHI(inD3D12RHI), inputLayoutManager(inInputLayoutManager)
{

}

void GraphicsPSOManager::TryCreatePSO(const GraphicsPSODescriptor& descriptor)
{
	if (PSOMap.find(descriptor) == PSOMap.end())
	{
		CreatePSO(descriptor);
	}
}

void GraphicsPSOManager::CreatePSO(const GraphicsPSODescriptor& descriptor)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	// Inputlayout
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
	inputLayoutManager->GetInputLayout(descriptor.inputLayoutName, inputLayout);
	psoDesc.InputLayout = { inputLayout.data(), (UINT)inputLayout.size() };

	// Shader
	Shader* shader = descriptor.shader;
	auto rootSignature = shader->rootSignature;
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(shader->shaderPass.at("VS")->GetBufferPointer(), shader->shaderPass.at("VS")->GetBufferSize());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(shader->shaderPass.at("PS")->GetBufferPointer(), shader->shaderPass.at("PS")->GetBufferSize());

	psoDesc.RasterizerState = descriptor.rasterizerDesc;
	psoDesc.BlendState = descriptor.blendDesc;
	psoDesc.DepthStencilState = descriptor.depthStencilDesc;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = descriptor.primitiveTopologyType;
	psoDesc.NumRenderTargets = descriptor.numRenderTargets;
	for (int i = 0; i < 8; i++)
	{
		psoDesc.RTVFormats[i] = descriptor.RTVFormats[i];
	}
	psoDesc.SampleDesc.Count = descriptor._4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = descriptor._4xMsaaState ? (descriptor._4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = descriptor.depthStencilFormat;

	// Create PSO
	ComPtr<ID3D12PipelineState> PSO;
	auto d3dDevice = d3d12RHI->GetDevice()->GetD3DDevice();
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&PSO)));
	PSOMap.insert({ descriptor, PSO });
}

ID3D12PipelineState* GraphicsPSOManager::GetPSO(const GraphicsPSODescriptor& descriptor) const
{
	auto Iter = PSOMap.find(descriptor);

	if (Iter == PSOMap.end())
	{
		assert(0); //TODO

		return nullptr;
	}
	else
	{
		return Iter->second.Get();
	}
}


ComputePSOManager::ComputePSOManager(D3D12RHI* InD3D12RHI)
	:d3d12RHI(InD3D12RHI)
{

}

void ComputePSOManager::TryCreatePSO(const ComputePSODescriptor& descriptor)
{
	if (PSOMap.find(descriptor) == PSOMap.end())
	{
		CreatePSO(descriptor);
	}
}

void ComputePSOManager::CreatePSO(const ComputePSODescriptor& descriptor)
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC PsoDesc = {};
	Shader* shader = descriptor.shader;
	PsoDesc.pRootSignature = shader->rootSignature.Get();
	PsoDesc.CS = CD3DX12_SHADER_BYTECODE(shader->shaderPass.at("CS")->GetBufferPointer(), shader->shaderPass.at("CS")->GetBufferSize());
	PsoDesc.Flags = descriptor.Flags;

	ComPtr<ID3D12PipelineState> PSO;
	auto d3dDevice = d3d12RHI->GetDevice()->GetD3DDevice();
	ThrowIfFailed(d3dDevice->CreateComputePipelineState(&PsoDesc, IID_PPV_ARGS(&PSO)));
	PSOMap.insert({ descriptor, PSO });
}

ID3D12PipelineState* ComputePSOManager::GetPSO(const ComputePSODescriptor& Descriptor) const
{
	auto Iter = PSOMap.find(Descriptor);

	if (Iter == PSOMap.end())
	{
		assert(0); //TODO

		return nullptr;
	}
	else
	{
		return Iter->second.Get();
	}
}

