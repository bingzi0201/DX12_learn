#include "Shader.h"
#include "../File/FileHelpers.h"
#include <d3d12shader.h>

void ShaderDefines::GetD3DShaderMacro(std::vector<D3D_SHADER_MACRO>& OutMacros) const
{
	for (const auto& pair : definesMap)
	{
		D3D_SHADER_MACRO macro;
		macro.Name = pair.first.c_str();
		macro.Definition = pair.second.c_str();
		OutMacros.push_back(macro);
	}

	D3D_SHADER_MACRO macro;
	macro.Name = NULL;
	macro.Definition = NULL;
	OutMacros.push_back(macro);
}

bool ShaderDefines::operator == (const ShaderDefines& other) const
{
	if (definesMap.size() != other.definesMap.size())
	{
		return false;
	}

	for (const auto& pair : definesMap)
	{
		const std::string key = pair.first;
		const std::string value = pair.second;

		auto iter = other.definesMap.find(key);
		if (iter == other.definesMap.end() || iter->second != value)
		{
			return false;
		}
	}

	return true;
}

void ShaderDefines::SetDefine(const std::string& name, const std::string& definition)
{
	definesMap.insert_or_assign(name, definition);
}


Shader::Shader(const ShaderInfo& inShaderInfo, D3D12RHI* ind3d12RHI)
	: shaderInfo(inShaderInfo), d3d12RHI(ind3d12RHI)
{
	Initialize();

	assert((shaderInfo.bCreateVS | shaderInfo.bCreatePS) ^ shaderInfo.bCreateCS);
}

void Shader::Initialize()
{
	// Compile Shaders
	std::wstring ShaderDir = L"Shaders\\";
	std::wstring filePath = ShaderDir + FormatConvert::StrToWStr(shaderInfo.fileName) + L".hlsl";

	if (!std::filesystem::exists(filePath))
		MessageBoxW(nullptr, filePath.c_str(), L"hlsl not found", MB_OK);


	std::vector<D3D_SHADER_MACRO> shaderMacros;
	shaderInfo.shaderDefines.GetD3DShaderMacro(shaderMacros);

	if (shaderInfo.bCreateVS)
	{
		auto vsBlob = CompileShader(filePath, shaderMacros.data(), shaderInfo.vsEntryPoint, "vs_5_1");
		shaderPass["VS"] = vsBlob;

		GetShaderParameters(vsBlob, EShaderType::VERTEX_SHADER);
	}

	if (shaderInfo.bCreatePS)
	{
		auto psBlob = CompileShader(filePath, shaderMacros.data(), shaderInfo.psEntryPoint, "ps_5_1");
		shaderPass["PS"] = psBlob;

		GetShaderParameters(psBlob, EShaderType::PIXEL_SHADER);
	}

	if (shaderInfo.bCreateCS)
	{
		auto csBlob = CompileShader(filePath, shaderMacros.data(), shaderInfo.csEntryPoint, "cs_5_1");
		shaderPass["CS"] = csBlob;

		GetShaderParameters(csBlob, EShaderType::COMPUTE_SHADER);
	}

	// Create rootSignature
	CreateRootSignature();
}

Microsoft::WRL::ComPtr<ID3DBlob> Shader::CompileShader(const std::wstring& filename, const D3D_SHADER_MACRO* defines, const std::string& entrypoint, const std::string& target)
{
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG) 
	// Enable better shader debugging with the graphics debugging tools.
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr = S_OK;

	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

	if (errors != nullptr)
		OutputDebugStringA((char*)errors->GetBufferPointer());

	ThrowIfFailed(hr);

	return byteCode;
}

void Shader::GetShaderParameters(ComPtr<ID3DBlob> passBlob, EShaderType shaderType)
{
	ID3D12ShaderReflection* reflection = NULL;
	D3DReflect(passBlob->GetBufferPointer(), passBlob->GetBufferSize(), IID_ID3D12ShaderReflection, (void**)&reflection);

	D3D12_SHADER_DESC shaderDesc;
	reflection->GetDesc(&shaderDesc);

	for (UINT i = 0; i < shaderDesc.BoundResources; i++)
	{
		D3D12_SHADER_INPUT_BIND_DESC  resourceDesc;
		reflection->GetResourceBindingDesc(i, &resourceDesc);

		auto shaderVarName = resourceDesc.Name;
		auto resourceType = resourceDesc.Type;
		auto registerSpace = resourceDesc.Space;
		auto bindPoint = resourceDesc.BindPoint;
		auto bindCount = resourceDesc.BindCount;


		if (resourceType == D3D_SHADER_INPUT_TYPE::D3D_SIT_CBUFFER)
		{
			ShaderCBVParameter param;
			param.name = shaderVarName;
			param.shaderType = shaderType;
			param.bindPoint = bindPoint;
			param.registerSpace = registerSpace;

			cbvParams.push_back(param);
		}
		else if (resourceType == D3D_SHADER_INPUT_TYPE::D3D_SIT_STRUCTURED
			|| resourceType == D3D_SHADER_INPUT_TYPE::D3D_SIT_TEXTURE)
		{
			ShaderSRVParameter param;
			param.name = shaderVarName;
			param.shaderType = shaderType;
			param.bindPoint = bindPoint;
			param.bindCount = bindCount;
			param.registerSpace = registerSpace;

			srvParams.push_back(param);
		}
		else if (resourceType == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_RWSTRUCTURED
			|| resourceType == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_RWTYPED)
		{
			assert(shaderType == EShaderType::COMPUTE_SHADER);

			ShaderUAVParameter param;
			param.name = shaderVarName;
			param.shaderType = shaderType;
			param.bindPoint = bindPoint;
			param.bindCount = bindCount;
			param.registerSpace = registerSpace;

			uavParams.push_back(param);
		}
		else if (resourceType == D3D_SHADER_INPUT_TYPE::D3D_SIT_SAMPLER)
		{
			assert(shaderType == EShaderType::PIXEL_SHADER);

			ShaderSamplerParameter param;
			param.name = shaderVarName;
			param.shaderType = shaderType;
			param.bindPoint = bindPoint;
			param.registerSpace = registerSpace;

			samplerParams.push_back(param);
		}
	}
}

D3D12_SHADER_VISIBILITY Shader::GetShaderVisibility(EShaderType shaderType)
{
	D3D12_SHADER_VISIBILITY ShaderVisibility;
	if (shaderType == EShaderType::VERTEX_SHADER)
	{
		ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	}
	else if (shaderType == EShaderType::PIXEL_SHADER)
	{
		ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	}
	else if (shaderType == EShaderType::COMPUTE_SHADER)
	{
		ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}
	else
	{
		assert(0);
	}

	return ShaderVisibility;
}

std::vector<CD3DX12_STATIC_SAMPLER_DESC> Shader::CreateStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	CD3DX12_STATIC_SAMPLER_DESC shadow(
		6, // shaderRegister
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,                               // mipLODBias
		16,                                 // maxAnisotropy
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

	CD3DX12_STATIC_SAMPLER_DESC depthMap(
		7, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,   // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,
		0,
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE);


	std::vector<CD3DX12_STATIC_SAMPLER_DESC> staticSamplers;
	staticSamplers.push_back(pointWrap);
	staticSamplers.push_back(pointClamp);
	staticSamplers.push_back(linearWrap);
	staticSamplers.push_back(linearClamp);
	staticSamplers.push_back(anisotropicWrap);
	staticSamplers.push_back(anisotropicClamp);
	staticSamplers.push_back(shadow);
	staticSamplers.push_back(depthMap);

	return staticSamplers;
}

void Shader::CreateRootSignature()
{
	//------------------------------------------------Set SlotRootParameter---------------------------------------
	std::vector<CD3DX12_ROOT_PARAMETER> slotRootParameter;

	// CBV
	for (const ShaderCBVParameter& param : cbvParams)
	{
		if (cbvSignatureBaseBindSlot == -1)
		{
			cbvSignatureBaseBindSlot = (UINT)slotRootParameter.size();
		}

		CD3DX12_ROOT_PARAMETER rootParam;
		rootParam.InitAsConstantBufferView(param.bindPoint, param.registerSpace, GetShaderVisibility(param.shaderType));
		slotRootParameter.push_back(rootParam);
	}

	// SRV
	{
		for (const ShaderSRVParameter& param : srvParams)
		{
			srvCount += param.bindCount;
		}

		if (srvCount > 0)
		{
			srvSignatureBindSlot = (UINT)slotRootParameter.size();

			CD3DX12_DESCRIPTOR_RANGE srvTable;
			srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, srvCount, 0, 0);

			CD3DX12_ROOT_PARAMETER rootParam;
			D3D12_SHADER_VISIBILITY shaderVisibility = shaderInfo.bCreateCS ? D3D12_SHADER_VISIBILITY_ALL : D3D12_SHADER_VISIBILITY_PIXEL;  //TODO
			rootParam.InitAsDescriptorTable(1, &srvTable, shaderVisibility);
			slotRootParameter.push_back(rootParam);
		}
	}

	// UAV
	{
		for (const ShaderUAVParameter& param : uavParams)
		{
			uavCount += param.bindCount;
		}

		if (uavCount > 0)
		{
			uavSignatureBindSlot = (UINT)slotRootParameter.size();

			CD3DX12_DESCRIPTOR_RANGE uavTable;
			uavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, uavCount, 0, 0);

			CD3DX12_ROOT_PARAMETER rootParam;
			D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL; //TODO
			rootParam.InitAsDescriptorTable(1, &uavTable, shaderVisibility);
			slotRootParameter.push_back(rootParam);
		}
	}

	// Sampler
	// TODO
	auto staticSamplers = CreateStaticSamplers();

	//------------------------------------------------SerializeRootSignature---------------------------------------

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc((UINT)slotRootParameter.size(), slotRootParameter.data(),
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(d3d12RHI->GetDevice()->GetD3DDevice()->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature)));
}

bool Shader::SetParameter(std::string paramName, ConstantBufferRef constantBufferRef)
{
	bool findParam = false;

	for (ShaderCBVParameter& param : cbvParams)
	{
		if (param.name == paramName)
		{
			param.constantBufferRef = constantBufferRef;
			findParam = true;
		}
	}

	return findParam;
}

bool Shader::SetParameter(std::string paramName, ShaderResourceView* srv)
{
	std::vector<ShaderResourceView*> srvList;
	srvList.push_back(srv);

	return SetParameter(paramName, srvList);
}

bool Shader::SetParameter(std::string paramName, const std::vector<ShaderResourceView*>& srvList)
{
	bool findParam = false;

	for (ShaderSRVParameter& param : srvParams)
	{
		if (param.name == paramName)
		{
			assert(srvList.size() == param.bindCount);
			param.srvList = srvList;
			findParam = true;
		}
	}

	return findParam;
}

bool Shader::SetParameter(std::string ParamName, UnorderedAccessView* uav)
{
	std::vector<UnorderedAccessView*> uavList;
	uavList.push_back(uav);

	return SetParameter(ParamName, uavList);
}

bool Shader::SetParameter(std::string paramName, const std::vector<UnorderedAccessView*>& uavList)
{
	bool findParam = false;

	for (ShaderUAVParameter& param : uavParams)
	{
		if (param.name == paramName)
		{
			assert(uavList.size() == param.bindCount);
			param.uavList = uavList;
			findParam = true;
		}
	}

	return findParam;
}

void Shader::BindParameters()
{
	auto commandList = d3d12RHI->GetDevice()->GetCommandList();
	auto descriptorCache = d3d12RHI->GetDevice()->GetCommandContext()->GetDescriptorCache();

	CheckBindings();

	bool bComputeShader = shaderInfo.bCreateCS;

	// CBV binding
	for (int i = 0; i < cbvParams.size(); i++)
	{
		UINT rootParamIdx = cbvSignatureBaseBindSlot + i;
		D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress = cbvParams[i].constantBufferRef->resourceLocation.virtualAddressGPU;

		if (bComputeShader)
		{
			commandList->SetComputeRootConstantBufferView(rootParamIdx, gpuVirtualAddress);
		}
		else
		{
			commandList->SetGraphicsRootConstantBufferView(rootParamIdx, gpuVirtualAddress);
		}
	}


	// SRV binding
	if (srvCount > 0)
	{
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> srcDescriptors;
		srcDescriptors.resize(srvCount);

		for (const ShaderSRVParameter& param : srvParams)
		{
			for (UINT i = 0; i < param.srvList.size(); i++)
			{
				UINT index = param.bindPoint + i;
				srcDescriptors[index] = param.srvList[i]->GetDescriptorHandle();
			}
		}

		UINT rootParamIdx = srvSignatureBindSlot;
		auto gpuDescriptorHandle = descriptorCache->AppendCbvSrvUavDescriptors(srcDescriptors);

		if (bComputeShader)
		{
			commandList->SetComputeRootDescriptorTable(rootParamIdx, gpuDescriptorHandle);
		}
		else
		{
			commandList->SetGraphicsRootDescriptorTable(rootParamIdx, gpuDescriptorHandle);
		}
	}

	// UAV binding
	if (uavCount > 0)
	{
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> srcDescriptors;
		srcDescriptors.resize(uavCount);

		for (const ShaderUAVParameter& param : uavParams)
		{
			for (UINT i = 0; i < param.uavList.size(); i++)
			{
				UINT index = param.bindPoint + i;
				srcDescriptors[index] = param.uavList[i]->GetDescriptorHandle();
			}
		}

		UINT rootParamIdx = uavSignatureBindSlot;
		auto gpuDescriptorHandle = descriptorCache->AppendCbvSrvUavDescriptors(srcDescriptors);

		if (bComputeShader)
		{
			commandList->SetComputeRootDescriptorTable(rootParamIdx, gpuDescriptorHandle);
		}
		else
		{
			assert(0);
		}
	}

	ClearBindings();
}

void Shader::CheckBindings()
{
	for (ShaderCBVParameter& param : cbvParams)
	{
		assert(param.constantBufferRef);
	}

	for (ShaderSRVParameter& param : srvParams)
	{
		assert(param.srvList.size() > 0);
	}

	for (ShaderUAVParameter& param : uavParams)
	{
		assert(param.uavList.size() > 0);
	}
}

void Shader::ClearBindings()
{
	for (ShaderCBVParameter& param : cbvParams)
	{
		param.constantBufferRef = nullptr;
	}

	for (ShaderSRVParameter& param : srvParams)
	{
		param.srvList.clear();
	}

	for (ShaderUAVParameter& param : uavParams)
	{
		param.uavList.clear();
	}
}