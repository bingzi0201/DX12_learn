#include "Render.h"
#include "../Actor/HDRSkyActor.h"
#include "../Actor/CameraActor.h"
#include "../Actor/Light/DirectionalLightActor.h"
#include "../Actor/Light/PointLightActor.h"*
#include "../Actor/Light/SpotLightActor.h"
#include "../Texture/TextureRepository.h"
#include "../Material/MaterialRepository.h"
#include "../Mesh/MeshRepository.h"
#include "../Mesh/KdTree.h"
#include "../Texture/TextureInfo.h"
#include "../Utils/Logger.h"
#include <fstream>
#include <algorithm>
#include "../File/FileHelpers.h"
#include "../File/BinarySaver.h"
#include "../File/BinaryReader.h"
#include "Sampler.h"

using namespace DirectX;

Render::Render()
	:bInitialize(false)
{
}

Render::~Render()
{
	OnDestroy();
}

bool Render::IsInitialize()
{
	return bInitialize;
}

bool Render::Initialize(int windowWidth, int windowHeight, D3D12RHI* inD3D12RHI, World* inWorld, const TRenderSettings& settings)
{
	d3d12RHI = inD3D12RHI;
	world = inWorld;
	renderSettings = settings;

	d3dDevice = d3d12RHI->GetDevice()->GetD3DDevice();
	d3dCommandList = d3d12RHI->GetDevice()->GetCommandList();

	graphicsPSOManager = std::make_unique<GraphicsPSOManager>(d3d12RHI, &inputLayoutManager);
	computePSOManager = std::make_unique<ComputePSOManager>(d3d12RHI);

	// Do the initial resize code.
	OnResize(windowWidth, windowHeight);

	CreateRenderResource();

	bInitialize = true;

	return true;
}

void Render::OnResize(int NewWidth, int NewHeight)
{

	windowWidth = NewWidth;
	windowHeight = NewHeight;

	// Reset viewport
	d3d12RHI->ResizeViewport(NewWidth, NewHeight);

	// Reset camera
	CameraComponent* cameraComponent = world->GetCameraComponent();
	cameraComponent->SetLens(cameraComponent->GetFovY(), AspectRatio(), cameraComponent->GetNearZ(), cameraComponent->GetFarZ());

	// Resize GBuffers
	CreateGBuffers();
	CreateColorTextures();
	CreateComputeShaderResource();
}

float Render::AspectRatio()const
{
	return static_cast<float>(windowWidth) / windowHeight;
}

Resource* Render::CurrentBackBuffer() const
{
	return d3d12RHI->GetViewport()->GetCurrentBackBuffer();
}

D3D12_CPU_DESCRIPTOR_HANDLE Render::CurrentBackBufferView() const
{
	return d3d12RHI->GetViewport()->GetCurrentBackBufferView()->GetDescriptorHandle();
}

float* Render::CurrentBackBufferClearColor() const
{
	return d3d12RHI->GetViewport()->GetCurrentBackBufferClearColor();
}

D3D12_CPU_DESCRIPTOR_HANDLE Render::DepthStencilView() const
{
	return d3d12RHI->GetViewport()->GetDepthStencilView()->GetDescriptorHandle();
}


void Render::CreateRenderResource()
{
	// prepare for recording initialization commands.
	d3d12RHI->ResetCommandList();

	CreateNullDescriptors();
	CreateTextures();
	GetSkyInfo();

	if (skyMeshComponent)
	{
		bEnableIBLEnvLighting = true;
		CreateSceneCaptureCube();
	}

	CreateMeshProxys();
	CreateInputLayouts();
	CreateGlobalShaders();
	CreateGlobalPSO();
	CreateComputePSO();

	// Execute the initialization commands.
	d3d12RHI->ExecuteCommandLists();
	// Wait until initialization is complete.
	d3d12RHI->FlushCommandQueue();
}

void Render::CreateNullDescriptors()
{
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

		texture2DNullDescriptor = std::make_unique<ShaderResourceView>(d3d12RHI->GetDevice(), srvDesc, nullptr);
	}

	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;

		texture3DNullDescriptor = std::make_unique<ShaderResourceView>(d3d12RHI->GetDevice(), srvDesc, nullptr);
	}

	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;

		textureCubeNullDescriptor = std::make_unique<ShaderResourceView>(d3d12RHI->GetDevice(), srvDesc, nullptr);
	}

	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		srvDesc.Buffer.StructureByteStride = 1;
		srvDesc.Buffer.NumElements = 1;
		srvDesc.Buffer.FirstElement = 0;

		structuredBufferNullDescriptor = std::make_unique<ShaderResourceView>(d3d12RHI->GetDevice(), srvDesc, nullptr);
	}
}

void Render::CreateTextures()
{
	const auto& TextureMap = TextureRepository::Get().textureMap;

	// create textures in reposity
	for (const auto& TexturePair : TextureMap)
	{
		TexturePair.second->LoadTextureResourceFromFlie(d3d12RHI);
		TexturePair.second->CreateTexture(d3d12RHI);
	}
}

void Render::CreateSceneCaptureCube()
{
	IBLEnvironmentMap = std::make_unique<SceneCaptureCube>(false, 512, DXGI_FORMAT_R32G32B32A32_FLOAT, d3d12RHI);
	IBLEnvironmentMap->CreatePerspectiveViews({ 0.0f, 0.0f, 0.0f }, 0.1f, 10.0f);

	IBLIrradianceMap = std::make_unique<SceneCaptureCube>(false, 32, DXGI_FORMAT_R32G32B32A32_FLOAT, d3d12RHI);
	IBLIrradianceMap->CreatePerspectiveViews({ 0.0f, 0.0f, 0.0f }, 0.1f, 10.0f);

	for (UINT mip = 0; mip < IBLPrefilterMaxMipLevel; mip++)
	{
		UINT mipWidth = UINT(128 * std::pow(0.5, mip));
		auto prefilterEnvMap = std::make_unique<SceneCaptureCube>(false, mipWidth, DXGI_FORMAT_R16G16B16A16_FLOAT, d3d12RHI);
		prefilterEnvMap->CreatePerspectiveViews({ 0.0f, 0.0f, 0.0f }, 0.1f, 10.0f);

		IBLPrefilterEnvMaps.push_back(std::move(prefilterEnvMap));
	}
}

void Render::CreateGBuffers()
{
	GBufferBaseColor = std::make_unique<RenderTarget2D>(d3d12RHI, false, windowWidth, windowHeight, DXGI_FORMAT_R32G32B32A32_FLOAT);
	GBufferNormal = std::make_unique<RenderTarget2D>(d3d12RHI, false, windowWidth, windowHeight, DXGI_FORMAT_R8G8B8A8_SNORM);
	GBufferWorldPos = std::make_unique<RenderTarget2D>(d3d12RHI, false, windowWidth, windowHeight, DXGI_FORMAT_R32G32B32A32_FLOAT);
	GBufferORM = std::make_unique<RenderTarget2D>(d3d12RHI, false, windowWidth, windowHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
	GBufferVelocity = std::make_unique<RenderTarget2D>(d3d12RHI, false, windowWidth, windowHeight, DXGI_FORMAT_R16G16_FLOAT);
	GBufferEmissive = std::make_unique<RenderTarget2D>(d3d12RHI, false, windowWidth, windowHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
}

void Render::CreateColorTextures()
{
	TextureInfo textureInfo;
	textureInfo.textureType = ETextureType::TEXTURE_2D;
	textureInfo.dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureInfo.width = windowWidth;
	textureInfo.height = windowHeight;
	textureInfo.depth = 1;
	textureInfo.arraySize = 1;
	textureInfo.mipCount = 1;
	textureInfo.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	textureInfo.InitState = D3D12_RESOURCE_STATE_COMMON;

	colorTexture = d3d12RHI->CreateTexture(textureInfo, TexCreate_SRV | TexCreate_RTV);
	cacheColorTexture = d3d12RHI->CreateTexture(textureInfo, TexCreate_SRV);
	prevColorTexture = d3d12RHI->CreateTexture(textureInfo, TexCreate_SRV);
}

void Render::CreateMeshProxys()
{
	auto& meshMap = MeshRepository::Get().meshMap;

	for (auto& meshPair : meshMap)
	{
		Mesh& mesh = meshPair.second;

		// ------------------------------------------------------------------
		// 1. vertex buffer
		// ------------------------------------------------------------------
		const UINT vbByteSize = static_cast<UINT>(mesh.vertices.size() * sizeof(Vertex));

		MeshProxy& meshProxy = meshProxyMap
			.emplace(mesh.meshName, MeshProxy{})
			.first->second;

		meshProxy.vertexBufferRef = d3d12RHI->CreateVertexBuffer(
			mesh.vertices.data(), vbByteSize);
		meshProxy.vertexByteStride = sizeof(Vertex);
		meshProxy.vertexBufferByteSize = vbByteSize;

		// ------------------------------------------------------------------
		// 2. choose indices for 16 or 32
		// ------------------------------------------------------------------
		bool canUse16 = !mesh.indices32.empty() &&
			*std::max_element(mesh.indices32.begin(), mesh.indices32.end()) < 0x10000;

		const void* indexData = nullptr;
		UINT        indexCount = 0;
		UINT        indexStride = 0;

		if (canUse16)
		{
			mesh.GenerateIndices16();                 // only in safe conditions
			indexData = mesh.indices16.data();
			indexCount = static_cast<UINT>(mesh.indices16.size());
			indexStride = sizeof(uint16_t);
			meshProxy.indexFormat = DXGI_FORMAT_R16_UINT;
		}
		else
		{
			indexData = mesh.indices32.data();
			indexCount = static_cast<UINT>(mesh.indices32.size());
			indexStride = sizeof(uint32_t);
			meshProxy.indexFormat = DXGI_FORMAT_R32_UINT;
		}

		const UINT ibByteSize = indexCount * indexStride;

		meshProxy.indexBufferRef = d3d12RHI->CreateIndexBuffer(indexData, ibByteSize);
		meshProxy.indexBufferByteSize = ibByteSize;

		// ------------------------------------------------------------------
		// 3. Sub-mesh
		// ------------------------------------------------------------------
		SubmeshProxy submesh;
		submesh.indexCount = indexCount;
		submesh.startIndexLocation = 0;
		submesh.baseVertexLocation = 0;

		meshProxy.subMeshs["Default"] = submesh;

		assert(meshProxy.vertexBufferRef && meshProxy.indexBufferRef);
		assert(submesh.indexCount > 0);
	}
}


void Render::CreateInputLayouts()
{
	// DefaultInputLayout
	std::vector<D3D12_INPUT_ELEMENT_DESC>  DefaultInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	inputLayoutManager.AddInputLayout("DefaultInputLayout", DefaultInputLayout);


	// PositionColorInputLayout
	std::vector<D3D12_INPUT_ELEMENT_DESC>  PositionColorInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	inputLayoutManager.AddInputLayout("PositionColorInputLayout", PositionColorInputLayout);

	// PositionTexcoordInputLayout
	std::vector<D3D12_INPUT_ELEMENT_DESC> PositionTexcoordInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	inputLayoutManager.AddInputLayout("PositionTexcoordInputLayout", PositionTexcoordInputLayout);
}

void Render::CreateGlobalShaders()
{
	// IBL
	if (bEnableIBLEnvLighting)
	{
		ShaderInfo shaderInfo;
		shaderInfo.shaderName = "IBLEnvironment";
		shaderInfo.fileName = "IBLEnvironment";
		shaderInfo.bCreateVS = true;
		shaderInfo.bCreatePS = true;
		IBLEnvironmentShader = std::make_unique<Shader>(shaderInfo, d3d12RHI);
	}
	if (bEnableIBLEnvLighting)
	{
		ShaderInfo shaderInfo;
		shaderInfo.shaderName = "IBLIrradiance";
		shaderInfo.fileName = "IBLIrradiance";
		shaderInfo.bCreateVS = true;
		shaderInfo.bCreatePS = true;
		IBLIrradianceShader = std::make_unique<Shader>(shaderInfo, d3d12RHI);
	}
	if (bEnableIBLEnvLighting)
	{
		ShaderInfo shaderInfo;
		shaderInfo.shaderName = "IBLPrefilterEnv";
		shaderInfo.fileName = "IBLPrefilterEnv";
		shaderInfo.bCreateVS = true;
		shaderInfo.bCreatePS = true;
		IBLPrefilterEnvShader = std::make_unique<Shader>(shaderInfo, d3d12RHI);
	}

	{
		ShaderInfo shaderInfo;
		shaderInfo.shaderName = "DeferredLighting";
		shaderInfo.fileName = "DeferredLighting";
		shaderInfo.bCreateVS = true;
		shaderInfo.bCreatePS = true;
		deferredLightingShader = std::make_unique<Shader>(shaderInfo, d3d12RHI);
	}

	{
		ShaderInfo shaderInfo;
		shaderInfo.shaderName = "Primitive";
		shaderInfo.fileName = "Primitive";
		shaderInfo.bCreateVS = true;
		shaderInfo.bCreatePS = true;
		primitiveShader = std::make_unique<Shader>(shaderInfo, d3d12RHI);
	}

	{
		ShaderInfo shaderInfo;
		shaderInfo.shaderName = "PostProcess";
		shaderInfo.fileName = "PostProcess";
		shaderInfo.bCreateVS = true;
		shaderInfo.bCreatePS = true;
		postProcessShader = std::make_unique<Shader>(shaderInfo, d3d12RHI);
	}

	// Compute Shader
	{
		ShaderInfo shaderInfo;
		shaderInfo.shaderName = "LocalCondCDF";
		shaderInfo.fileName = "LocalCondCDF";
		shaderInfo.bCreateCS = true;
		localCondCDFShader = std::make_unique<Shader>(shaderInfo, d3d12RHI);
	}
	{
		ShaderInfo shaderInfo;
		shaderInfo.shaderName = "GlobalCondCDF";
		shaderInfo.fileName = "GlobalCondCDF";
		shaderInfo.bCreateCS = true;
		globalCondCDFShader = std::make_unique<Shader>(shaderInfo, d3d12RHI);
	}
	{
		ShaderInfo shaderInfo;
		shaderInfo.shaderName = "BroadcastConCDF";
		shaderInfo.fileName = "BroadcastConCDF";
		shaderInfo.bCreateCS = true;
		broadcastCondCDFShader = std::make_unique<Shader>(shaderInfo, d3d12RHI);
	}

	{
		ShaderInfo shaderInfo;
		shaderInfo.shaderName = "LocalEdgeCDF";
		shaderInfo.fileName = "LocalEdgeCDF";
		shaderInfo.bCreateCS = true;
		localEdgeCDFShader = std::make_unique<Shader>(shaderInfo, d3d12RHI);
	}
	{
		ShaderInfo shaderInfo;
		shaderInfo.shaderName = "GlobalEdgeCDF";
		shaderInfo.fileName = "GlobalEdgeCDF";
		shaderInfo.bCreateCS = true;
		globalEdgeCDFShader = std::make_unique<Shader>(shaderInfo, d3d12RHI);
	}
	{
		ShaderInfo shaderInfo;
		shaderInfo.shaderName = "CDFResult";
		shaderInfo.fileName = "CDFResult";
		shaderInfo.bCreateCS = true;
		resultCDFShader = std::make_unique<Shader>(shaderInfo, d3d12RHI);
	}

	// TODO

	{
		ShaderInfo shaderInfo;
		shaderInfo.shaderName = "TemporalAccumCS";
		shaderInfo.fileName = "TemporalAccumCS";
		shaderInfo.bCreateCS = true;
		temporalAccumShader = std::make_unique<Shader>(shaderInfo, d3d12RHI);
	}

	{
		ShaderInfo shaderInfo;
		shaderInfo.shaderName = "VarianceCS";
		shaderInfo.fileName = "VarianceCS";
		shaderInfo.bCreateCS = true;
		varianceShader = std::make_unique<Shader>(shaderInfo, d3d12RHI);
	}

	{
		ShaderInfo shaderInfo;
		shaderInfo.shaderName = "SVGFSpatFilterCS";
		shaderInfo.fileName = "SVGFSpatFilterCS";
		shaderInfo.bCreateCS = true;
		SVGFSpatFilterShader = std::make_unique<Shader>(shaderInfo, d3d12RHI);
	}

}

void Render::CreateGlobalPSO()
{
	// IBLEnvironmentPSODescriptor
	if (bEnableIBLEnvLighting)
	{
		IBLEnvironmentPSODescriptor.inputLayoutName = std::string("DefaultInputLayout");
		IBLEnvironmentPSODescriptor.shader = IBLEnvironmentShader.get();
		IBLEnvironmentPSODescriptor.RTVFormats[0] = IBLEnvironmentMap->GetRTCube()->GetFormat();
		IBLEnvironmentPSODescriptor.numRenderTargets = 1;
		// The camera is inside the cube, so just turn off culling.
		IBLEnvironmentPSODescriptor.rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
		IBLEnvironmentPSODescriptor.depthStencilDesc.DepthEnable = false;
		IBLEnvironmentPSODescriptor.depthStencilFormat = DXGI_FORMAT_UNKNOWN;

		graphicsPSOManager->TryCreatePSO(IBLEnvironmentPSODescriptor);
	}

	// IBLIrradiance PSO
	if (bEnableIBLEnvLighting)
	{
		IBLIrradiancePSODescriptor.inputLayoutName = std::string("DefaultInputLayout");
		IBLIrradiancePSODescriptor.shader = IBLIrradianceShader.get();
		IBLIrradiancePSODescriptor.RTVFormats[0] = IBLIrradianceMap->GetRTCube()->GetFormat();
		IBLIrradiancePSODescriptor.numRenderTargets = 1;
		// The camera is inside the cube, so just turn off culling.
		IBLIrradiancePSODescriptor.rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
		IBLIrradiancePSODescriptor.depthStencilDesc.DepthEnable = false;
		IBLIrradiancePSODescriptor.depthStencilFormat = DXGI_FORMAT_UNKNOWN;

		graphicsPSOManager->TryCreatePSO(IBLIrradiancePSODescriptor);
	}

	// IBLPrefilterEnv PSO
	if (bEnableIBLEnvLighting)
	{
		IBLPrefilterEnvPSODescriptor.inputLayoutName = std::string("DefaultInputLayout");
		IBLPrefilterEnvPSODescriptor.shader = IBLPrefilterEnvShader.get();
		IBLPrefilterEnvPSODescriptor.RTVFormats[0] = IBLPrefilterEnvMaps[0]->GetRTCube()->GetFormat();
		IBLPrefilterEnvPSODescriptor.numRenderTargets = 1;
		// The camera is inside the cube, so just turn off culling.
		IBLPrefilterEnvPSODescriptor.rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
		IBLPrefilterEnvPSODescriptor.depthStencilDesc.DepthEnable = false;
		IBLPrefilterEnvPSODescriptor.depthStencilFormat = DXGI_FORMAT_UNKNOWN;

		graphicsPSOManager->TryCreatePSO(IBLPrefilterEnvPSODescriptor);
	}

	// DeferredLighting
	{
		D3D12_DEPTH_STENCIL_DESC lightPassDSD;
		lightPassDSD.DepthEnable = false;
		lightPassDSD.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		lightPassDSD.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		lightPassDSD.StencilEnable = true;
		lightPassDSD.StencilReadMask = 0xff;
		lightPassDSD.StencilWriteMask = 0x0;
		lightPassDSD.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		lightPassDSD.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		lightPassDSD.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		lightPassDSD.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		// We are not rendering back-facing polygons, so these settings do not matter. 
		lightPassDSD.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		lightPassDSD.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		lightPassDSD.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		lightPassDSD.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

		auto blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		blendState.AlphaToCoverageEnable = false;
		blendState.IndependentBlendEnable = false;

		blendState.RenderTarget[0].BlendEnable = true;
		blendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;

		auto rasterizer = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		//rasterizer.CullMode = D3D12_CULL_MODE_FRONT; // Front culling for point light
		rasterizer.CullMode = D3D12_CULL_MODE_NONE;
		rasterizer.DepthClipEnable = false;

		deferredLightingPSODescriptor.inputLayoutName = std::string("DefaultInputLayout");
		deferredLightingPSODescriptor.shader = deferredLightingShader.get();
		deferredLightingPSODescriptor.blendDesc = blendState;
		deferredLightingPSODescriptor.depthStencilDesc = lightPassDSD;
		deferredLightingPSODescriptor.rasterizerDesc = rasterizer;
		deferredLightingPSODescriptor.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		deferredLightingPSODescriptor.numRenderTargets = 1;
		deferredLightingPSODescriptor._4xMsaaState = false; //can't use msaa in deferred rendering.

		graphicsPSOManager->TryCreatePSO(deferredLightingPSODescriptor);
	}

	// PostProcess
	{
		postProcessPSODescriptor.inputLayoutName = std::string("DefaultInputLayout");
		postProcessPSODescriptor.shader = postProcessShader.get();
		postProcessPSODescriptor.rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
		postProcessPSODescriptor.depthStencilDesc.DepthEnable = false;
		postProcessPSODescriptor.depthStencilFormat = DXGI_FORMAT_UNKNOWN;
		postProcessPSODescriptor.primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		postProcessPSODescriptor.RTVFormats[0] = d3d12RHI->GetViewportInfo().backBufferFormat;

		graphicsPSOManager->TryCreatePSO(postProcessPSODescriptor);
	}
}

void Render::CreateComputePSO()
{
	// environment-CDF
	localCondCDFPSODescriptor.shader = localCondCDFShader.get();
	localCondCDFPSODescriptor.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	computePSOManager->TryCreatePSO(localCondCDFPSODescriptor);

	globalCondCDFPSODescriptor.shader = globalCondCDFShader.get();
	globalCondCDFPSODescriptor.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	computePSOManager->TryCreatePSO(globalCondCDFPSODescriptor);

	broadcastCondCDFPSODescriptor.shader = broadcastCondCDFShader.get();
	broadcastCondCDFPSODescriptor.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	computePSOManager->TryCreatePSO(broadcastCondCDFPSODescriptor);

	localEdgeCDFPSODescriptor.shader = localEdgeCDFShader.get();
	localEdgeCDFPSODescriptor.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	computePSOManager->TryCreatePSO(localEdgeCDFPSODescriptor);

	globalEdgeCDFPSODescriptor.shader = globalEdgeCDFShader.get();
	globalEdgeCDFPSODescriptor.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	computePSOManager->TryCreatePSO(globalEdgeCDFPSODescriptor);

	resultCDFPSODescriptor.shader = resultCDFShader.get();
	resultCDFPSODescriptor.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	computePSOManager->TryCreatePSO(resultCDFPSODescriptor);

	// TAA
	temporalAccumPSODescriptor.shader = temporalAccumShader.get();
	temporalAccumPSODescriptor.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	computePSOManager->TryCreatePSO(temporalAccumPSODescriptor);

	// SVFG
	variancePSODescriptor.shader = varianceShader.get();
	variancePSODescriptor.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	computePSOManager->TryCreatePSO(variancePSODescriptor);

	SVGFSpatFilterPSODescriptor.shader = SVGFSpatFilterShader.get();
	SVGFSpatFilterPSODescriptor.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	computePSOManager->TryCreatePSO(SVGFSpatFilterPSODescriptor);
}

void Render::CreateComputeShaderResource()
{

}

void Render::Draw(const GameTimer& gt)
{
	d3d12RHI->ResetCommandAllocator();
	d3d12RHI->ResetCommandList();

	SetDescriptorHeaps();

	if (bEnableIBLEnvLighting && frameCount == 0)
	{
		CreateIBLEnviromentMap();
 		CreateIBLIrradianceMap();
 		CreateIBLPrefilterEnvMap();
		CreateEnviromentCDF();
	}

	GatherAllMeshBatchs();
	UpdateLightData();
	BasePass();
 	PrimitivesPass();
	DeferredLightingPass();
 	PostProcessPass();

	d3d12RHI->ExecuteCommandLists();
	d3d12RHI->Present();
	d3d12RHI->FlushCommandQueue();
}

void Render::EndFrame()
{
	d3d12RHI->EndFrame();
	frameCount++;
}

void Render::SetDescriptorHeaps()
{
	auto cacheCbvSrvUavDescriptorHeap = d3d12RHI->GetDevice()->GetCommandContext()->GetDescriptorCache()->GetCacheCbvSrvUavDescriptorHeap();
	ID3D12DescriptorHeap* d3dDescriptorHeaps[] = { cacheCbvSrvUavDescriptorHeap.Get() };
	d3dCommandList->SetDescriptorHeaps(1, d3dDescriptorHeaps);
}

void Render::GetSkyInfo()
{
	auto hdrSkyActors = world->GetAllActorsOfClass<HDRSkyActor>();

	if (hdrSkyActors.size() > 0)
	{
		assert(hdrSkyActors.size() == 1);
		auto hdrSky = hdrSkyActors[0];
		skyMeshComponent = hdrSky->meshComponent;
		skyCubeTextureName = skyMeshComponent->GetMaterialInstance()->parameters.textureMap["SkyCubeTexture"];
	}
}

void Render::UpdateIBLEnviromentPassCB()
{
	for (int i = 0; i < 6; i++)
	{
		TMatrix view = IBLEnvironmentMap->GetSceneView(i).view;
		TMatrix proj = IBLEnvironmentMap->GetSceneView(i).proj;

		PassConstants passCB;
		passCB.View = view.Transpose();
		passCB.Proj = proj.Transpose();

		IBLEnviromentPassCBRef[i] = d3d12RHI->CreateConstantBuffer(&passCB, sizeof(passCB));
	}
}

void Render::CreateIBLEnviromentMap()
{
	UpdateIBLEnviromentPassCB();

	d3d12RHI->TransitionResource(IBLEnvironmentMap->GetRTCube()->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);

	// Set viewport
	auto viewport = IBLEnvironmentMap->GetViewport();
	auto rect = IBLEnvironmentMap->GetScissorRect();
	d3dCommandList->RSSetViewports(1, &viewport);
	d3dCommandList->RSSetScissorRects(1, &rect);

	// Set PSO
	d3dCommandList->SetPipelineState(graphicsPSOManager->GetPSO(IBLEnvironmentPSODescriptor));

	// Set RootSignature
	Shader* shader = IBLEnvironmentPSODescriptor.shader;
	d3dCommandList->SetGraphicsRootSignature(shader->rootSignature.Get());

	for (UINT i = 0; i < 6; i++)
	{
		// Set renderTarget
		auto RTV = IBLEnvironmentMap->GetRTCube()->GetRTV(i);
		float* clearValue = IBLEnvironmentMap->GetRTCube()->GetClearColorPtr();
		auto descHandle = RTV->GetDescriptorHandle();
		d3dCommandList->ClearRenderTargetView(RTV->GetDescriptorHandle(), clearValue, 0, nullptr);
		d3dCommandList->OMSetRenderTargets(1, &descHandle, true, nullptr);

		// Draw Box
		{
			shader->SetParameter("cbPass", IBLEnviromentPassCBRef[i]);

			auto& textureMap = TextureRepository::Get().textureMap;
			auto equirectangularSRV = textureMap[skyCubeTextureName]->GetD3DTexture()->GetSRV();
			shader->SetParameter("EquirectangularMap", equirectangularSRV);

			// Bind paramters
			shader->BindParameters();

			const MeshProxy& meshProxy = meshProxyMap.at("BoxMesh");

			// Set vertex buffer
			d3d12RHI->SetVertexBuffer(meshProxy.vertexBufferRef, 0, meshProxy.vertexByteStride, meshProxy.vertexBufferByteSize);

			// Set index buffer
			d3d12RHI->SetIndexBuffer(meshProxy.indexBufferRef, 0, meshProxy.indexFormat, meshProxy.indexBufferByteSize);

			D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			d3dCommandList->IASetPrimitiveTopology(primitiveType);

			// Draw 
			auto& SubMesh = meshProxy.subMeshs.at("Default");
			d3dCommandList->DrawIndexedInstanced(SubMesh.indexCount, 1, SubMesh.startIndexLocation, SubMesh.baseVertexLocation, 0);
		}
	}

	// Change back to GENERIC_READ so we can read the texture in a shader.
	d3d12RHI->TransitionResource(IBLEnvironmentMap->GetRTCube()->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
}

void Render::UpdateIBLIrradiancePassCB()
{
	for (int i = 0; i < 6; i++)
	{
		TMatrix view = IBLIrradianceMap->GetSceneView(i).view;
		TMatrix proj = IBLIrradianceMap->GetSceneView(i).proj;

		PassConstants PassCB;
		PassCB.View = view.Transpose();
		PassCB.Proj = proj.Transpose();

		IBLIrradiancePassCBRef[i] = d3d12RHI->CreateConstantBuffer(&PassCB, sizeof(PassCB));
	}
}

void Render::CreateIBLIrradianceMap()
{
	UpdateIBLIrradiancePassCB();

	d3d12RHI->TransitionResource(IBLIrradianceMap->GetRTCube()->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);

	// Set viewport
	auto viewport = IBLIrradianceMap->GetViewport();
	auto rect = IBLIrradianceMap->GetScissorRect();
	d3dCommandList->RSSetViewports(1, &viewport);
	d3dCommandList->RSSetScissorRects(1, &rect);

	// Set PSO
	d3dCommandList->SetPipelineState(graphicsPSOManager->GetPSO(IBLIrradiancePSODescriptor));

	// Set RootSignature
	Shader* shader = IBLIrradiancePSODescriptor.shader;
	d3dCommandList->SetGraphicsRootSignature(shader->rootSignature.Get()); //should before binding

	for (UINT i = 0; i < 6; i++)
	{
		// Set renderTarget
		auto RTV = IBLIrradianceMap->GetRTCube()->GetRTV(i);
		float* ClearValue = IBLIrradianceMap->GetRTCube()->GetClearColorPtr();
		auto descHandle = RTV->GetDescriptorHandle();
		d3dCommandList->ClearRenderTargetView(RTV->GetDescriptorHandle(), ClearValue, 0, nullptr);
		d3dCommandList->OMSetRenderTargets(1, &descHandle, true, nullptr);

		// Draw Box
		{
			shader->SetParameter("cbPass", IBLIrradiancePassCBRef[i]);

			ShaderResourceView* environmentSRV = IBLEnvironmentMap->GetRTCube()->GetSRV();
			shader->SetParameter("EnvironmentMap", environmentSRV);

			// Bind paramters
			shader->BindParameters();

			const MeshProxy& meshProxy = meshProxyMap.at("BoxMesh");

			// Set vertex buffer 
			d3d12RHI->SetVertexBuffer(meshProxy.vertexBufferRef, 0, meshProxy.vertexByteStride, meshProxy.vertexBufferByteSize);

			// Set index buffer
			d3d12RHI->SetIndexBuffer(meshProxy.indexBufferRef, 0, meshProxy.indexFormat, meshProxy.indexBufferByteSize);

			D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			d3dCommandList->IASetPrimitiveTopology(PrimitiveType);

			// Draw 
			auto& SubMesh = meshProxy.subMeshs.at("Default");
			d3dCommandList->DrawIndexedInstanced(SubMesh.indexCount, 1, SubMesh.startIndexLocation, SubMesh.baseVertexLocation, 0);
		}
	}

	// Change back to GENERIC_READ so we can read the texture in a shader.
	d3d12RHI->TransitionResource(IBLIrradianceMap->GetRTCube()->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
}

void Render::UpdateIBLPrefilterEnvCB()
{
	for (UINT mip = 0; mip < IBLPrefilterMaxMipLevel; mip++)
	{
		float roughness = (float)mip / (float)(IBLPrefilterMaxMipLevel - 1);

		for (UINT i = 0; i < 6; i++)
		{
			TMatrix view = IBLPrefilterEnvMaps[mip]->GetSceneView(i).view;
			TMatrix proj = IBLPrefilterEnvMaps[mip]->GetSceneView(i).proj;

			PrefilterEnvironmentConstant passCB;
			passCB.view = view.Transpose();
			passCB.proj = proj.Transpose();
			passCB.roughness = roughness;

			IBLPrefilterEnvPassCBRef[mip * 6 + i] = d3d12RHI->CreateConstantBuffer(&passCB, sizeof(passCB));
		}
	}
}

void Render::CreateIBLPrefilterEnvMap()
{
	UpdateIBLPrefilterEnvCB();

	for (UINT mip = 0; mip < IBLPrefilterMaxMipLevel; mip++)
	{
		d3d12RHI->TransitionResource(IBLPrefilterEnvMaps[mip]->GetRTCube()->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);

		// Set viewport
		auto viewport = IBLPrefilterEnvMaps[mip]->GetViewport();
		auto rect = IBLPrefilterEnvMaps[mip]->GetScissorRect();
		d3dCommandList->RSSetViewports(1, &viewport);
		d3dCommandList->RSSetScissorRects(1, &rect);

		// Set PSO
		d3dCommandList->SetPipelineState(graphicsPSOManager->GetPSO(IBLPrefilterEnvPSODescriptor));

		// Set RootSignature
		d3dCommandList->SetGraphicsRootSignature(IBLPrefilterEnvShader->rootSignature.Get()); //should before binding	

		for (UINT i = 0; i < 6; i++)
		{
			// Set renderTarget
			auto RTV = IBLPrefilterEnvMaps[mip]->GetRTCube()->GetRTV(i);
			float* ClearValue = IBLPrefilterEnvMaps[mip]->GetRTCube()->GetClearColorPtr();
			auto descHandle = RTV->GetDescriptorHandle();
			d3dCommandList->ClearRenderTargetView(RTV->GetDescriptorHandle(), ClearValue, 0, nullptr);
			d3dCommandList->OMSetRenderTargets(1, &descHandle, true, nullptr);

			// Draw Box
			{
				IBLPrefilterEnvShader->SetParameter("cbPrefilterEnvPass", IBLPrefilterEnvPassCBRef[mip * 6 + i]);


				ShaderResourceView* environmentSRV = IBLEnvironmentMap->GetRTCube()->GetSRV();
				IBLPrefilterEnvShader->SetParameter("EnvironmentMap", environmentSRV);

				// Bind paramters
				IBLPrefilterEnvShader->BindParameters();

				const MeshProxy& meshProxy = meshProxyMap.at("BoxMesh");

				// Set vertex buffer
				d3d12RHI->SetVertexBuffer(meshProxy.vertexBufferRef, 0, meshProxy.vertexByteStride, meshProxy.vertexBufferByteSize);

				// Set index buffer
				d3d12RHI->SetIndexBuffer(meshProxy.indexBufferRef, 0, meshProxy.indexFormat, meshProxy.indexBufferByteSize);

				D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				d3dCommandList->IASetPrimitiveTopology(primitiveType);

				// Draw 
				auto& SubMesh = meshProxy.subMeshs.at("Default");
				d3dCommandList->DrawIndexedInstanced(SubMesh.indexCount, 1, SubMesh.startIndexLocation, SubMesh.baseVertexLocation, 0);
			}
		}

		// Change back to GENERIC_READ so we can read the texture in a shader.
		d3d12RHI->TransitionResource(IBLPrefilterEnvMaps[mip]->GetRTCube()->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	}
}

void Render::CreateEnviromentCDF()
{
	auto& textureMap = TextureRepository::Get().textureMap;
	auto equirectangularSRV = textureMap[skyCubeTextureName]->GetD3DTexture()->GetSRV();
	auto eqTexDesc = textureMap[skyCubeTextureName]->GetD3DTexture()->GetResource()->D3DResource->GetDesc();
	UINT height = eqTexDesc.Height;
	UINT width = eqTexDesc.Width;

	// CDF output
	// only for first frame, so create in this pass
	{
		TextureInfo textureInfo;
		textureInfo.textureType = ETextureType::TEXTURE_2D;
		textureInfo.dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		textureInfo.width = width;
		textureInfo.height = height;
		textureInfo.depth = 1;
		textureInfo.arraySize = 1;
		textureInfo.mipCount = 1;
		textureInfo.format = DXGI_FORMAT_R32G32_FLOAT;
		textureInfo.InitState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

		enviromentCDFTex0 = d3d12RHI->CreateTexture(textureInfo, TexCreate_SRV | TexCreate_UAV);
	}
	{
		TextureInfo textureInfo;
		textureInfo.textureType = ETextureType::TEXTURE_2D;
		textureInfo.dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		textureInfo.width = width;
		textureInfo.height = height;
		textureInfo.depth = 1;
		textureInfo.arraySize = 1;
		textureInfo.mipCount = 1;
		textureInfo.format = DXGI_FORMAT_R32G32_FLOAT;
		textureInfo.InitState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

		enviromentCDFTex1 = d3d12RHI->CreateTexture(textureInfo, TexCreate_SRV | TexCreate_UAV);
	}

	UINT groupsPerRow = (UINT)ceilf(width / 64.0f);
	UINT groupsPerColumn = (UINT)ceilf(height / 64.0f);

	RWStructuredBufferRef localRowSumsBuf = d3d12RHI->CreateRWStructuredBuffer(sizeof(float), ((UINT)ceilf(width / 64.0f) * height));
	RWStructuredBufferRef groupPrefixSumsBuf = d3d12RHI->CreateRWStructuredBuffer(sizeof(float), groupsPerRow * height);
	RWStructuredBufferRef globalRowSumsBuf = d3d12RHI->CreateRWStructuredBuffer(sizeof(float), height);
	RWStructuredBufferRef localColumnSumsBuf = d3d12RHI->CreateRWStructuredBuffer(sizeof(float), ((UINT)ceilf(height / 64.0f)));
	RWStructuredBufferRef columnPrefixSumsBuf = d3d12RHI->CreateRWStructuredBuffer(sizeof(float), height);
	RWStructuredBufferRef colGroupOffsetSumsBuf = d3d12RHI->CreateRWStructuredBuffer(sizeof(float), height/64.0f);

	// CBV, count of blockSum
	CB_EnvCDF cbData = {};
	cbData.width = width;
	cbData.height = height;
	cbData.groupsPerRow = groupsPerRow;
	cbData.groupsPerColumn = groupsPerColumn;

	auto cbEnvCDF = d3d12RHI->CreateConstantBuffer(&cbData, sizeof(cbData));

	// Local condCDF
	{
		// Set PSO
		d3dCommandList->SetPipelineState(computePSOManager->GetPSO(localCondCDFPSODescriptor));
		// Set RootSignature
		auto shader = localCondCDFPSODescriptor.shader;
		d3dCommandList->SetComputeRootSignature(shader->rootSignature.Get()); // should before binding

		// CBV
		shader->SetParameter("CB_EnvCDF", cbEnvCDF);

		shader->SetParameter("EquirectangularMap", equirectangularSRV);

		auto localRowSumsUAV = localRowSumsBuf->GetUAV();
		shader->SetParameter("LocalRowSums", localRowSumsUAV);

		auto environmentCDFUAV = enviromentCDFTex0->GetUAV();
		shader->SetParameter("EnvironmentCDF_Out", environmentCDFUAV);

		// Bind parameters
		shader->BindParameters();

		// Dispatch (16¡Á16 group)
		UINT gx = (UINT)ceilf(width / 64.0f);
		UINT gy = height;
		d3dCommandList->Dispatch(gx, gy, 1);

		d3d12RHI->TransitionResource(localRowSumsBuf->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
		d3d12RHI->TransitionResource(enviromentCDFTex0->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
		d3d12RHI->TransitionResource(enviromentCDFTex1->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		d3d12RHI->TransitionResource(globalRowSumsBuf->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	// Global condCDF
	{
		// Set PSO
		d3dCommandList->SetPipelineState(computePSOManager->GetPSO(globalCondCDFPSODescriptor));
		// Set RootSignature
		auto shader = globalCondCDFPSODescriptor.shader;
		d3dCommandList->SetComputeRootSignature(shader->rootSignature.Get()); // should before binding

		// CBV
		shader->SetParameter("CB_EnvCDF", cbEnvCDF);

		auto localRowSumSRV = localRowSumsBuf->GetSRV();
		shader->SetParameter("LocalRowSums", localRowSumSRV);

		auto groupPrefixSumsUAV = groupPrefixSumsBuf->GetUAV();
		shader->SetParameter("GroupPrefixSums", groupPrefixSumsUAV);

		auto globalRowSumsUAV = globalRowSumsBuf->GetUAV();
		shader->SetParameter("GlobalRowSums", globalRowSumsUAV);

		// Bind parameters
		shader->BindParameters();

		// Dispatch
		UINT gx = 1;
		UINT gy = height;
		d3dCommandList->Dispatch(gx, gy, 1);

		d3d12RHI->TransitionResource(groupPrefixSumsBuf->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
		d3d12RHI->TransitionResource(globalRowSumsBuf->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	}

	// BroadCast condCDF
	{
		// Set PSO
		d3dCommandList->SetPipelineState(computePSOManager->GetPSO(broadcastCondCDFPSODescriptor));
		// Set RootSignature
		auto shader = broadcastCondCDFPSODescriptor.shader;
		d3dCommandList->SetComputeRootSignature(shader->rootSignature.Get()); // should before binding

		// CBV

		shader->SetParameter("CB_EnvCDF", cbEnvCDF);

		auto environmentCDFSRV = enviromentCDFTex0->GetSRV();
		shader->SetParameter("EnvironmentCDF_In", environmentCDFSRV);

		auto environmentCDFUAV = enviromentCDFTex1->GetUAV();
		shader->SetParameter("EnvironmentCDF_Out", environmentCDFUAV);

		auto groupPrefixSumsSRV = groupPrefixSumsBuf->GetSRV();
		shader->SetParameter("GroupPrefixSums", groupPrefixSumsSRV);

		auto globalRowSumsSRV = globalRowSumsBuf->GetSRV();
		shader->SetParameter("GlobalRowSums", globalRowSumsSRV);

		// Bind parameters
		shader->BindParameters();

		// Dispatch
		UINT gx = (UINT)ceilf(width / 64.0f);
		UINT gy = height;
		d3dCommandList->Dispatch(gx, gy, 1);

		d3d12RHI->TransitionResource(globalRowSumsBuf->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
		d3d12RHI->TransitionResource(enviromentCDFTex1->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
		d3d12RHI->TransitionResource(enviromentCDFTex0->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	// Local EdgeCDF
	{
		// Set PSO
		d3dCommandList->SetPipelineState(computePSOManager->GetPSO(localEdgeCDFPSODescriptor));
		// Set RootSignature
		auto shader = localEdgeCDFPSODescriptor.shader;
		d3dCommandList->SetComputeRootSignature(shader->rootSignature.Get()); // should before binding

		// CBV
		shader->SetParameter("CB_EnvCDF", cbEnvCDF);

		auto globalRowSumsSRV = globalRowSumsBuf->GetSRV();
		shader->SetParameter("GlobalRowSums", globalRowSumsSRV);

		auto localColumnSumsUAV = localColumnSumsBuf->GetUAV();
		shader->SetParameter("LocalColSums", localColumnSumsUAV);

		auto columnPrefixSumsUAV = columnPrefixSumsBuf->GetUAV();
		shader->SetParameter("ColumnPrefixSums", columnPrefixSumsUAV);

		// Bind parameters
		shader->BindParameters();

		// Dispatch (numBlocks/256 ¡Á 1 group)

		UINT gx = (UINT)ceilf(height / 64.0f);
		UINT gy = 1;
		d3dCommandList->Dispatch(gx, gy, 1);

		d3d12RHI->TransitionResource(localColumnSumsBuf->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
		d3d12RHI->TransitionResource(columnPrefixSumsBuf->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	}

	// Global EdgeCDF
	{
		// Set PSO
		d3dCommandList->SetPipelineState(computePSOManager->GetPSO(globalEdgeCDFPSODescriptor));
		// Set RootSignature
		auto shader = globalEdgeCDFPSODescriptor.shader;
		d3dCommandList->SetComputeRootSignature(shader->rootSignature.Get()); // should before binding

		// CBV
		shader->SetParameter("CB_EnvCDF", cbEnvCDF);

		auto localColumnSumSRV = localColumnSumsBuf->GetSRV();
		shader->SetParameter("LocalColSums", localColumnSumSRV);

		auto colGroupOffsetSumsUAV = colGroupOffsetSumsBuf->GetUAV();
		shader->SetParameter("GroupColOffset", colGroupOffsetSumsUAV);

		// Bind parameters
		shader->BindParameters();

		// Dispatch
		UINT gx = (UINT)ceilf(height / 64.0f / 64.0f);
		UINT gy = 1;
		d3dCommandList->Dispatch(gx, gy, 1);
		d3d12RHI->TransitionResource(colGroupOffsetSumsBuf->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	}

	// CDF Result
	{
		// Set PSO
		d3dCommandList->SetPipelineState(computePSOManager->GetPSO(resultCDFPSODescriptor));
		// Set RootSignature
		auto shader = resultCDFPSODescriptor.shader;
		d3dCommandList->SetComputeRootSignature(shader->rootSignature.Get()); // should before binding

		// CBV
		shader->SetParameter("CB_EnvCDF", cbEnvCDF);

		auto environmentCDFSRV = enviromentCDFTex1->GetSRV();
		shader->SetParameter("EnvironmentCDF_In", environmentCDFSRV);

		auto environmentCDFUAV = enviromentCDFTex0->GetUAV();
		shader->SetParameter("EnvironmentCDF_Out", environmentCDFUAV);

		auto columnPrefixSumsSRV = columnPrefixSumsBuf->GetSRV();
		shader->SetParameter("ColumnPrefixSums", columnPrefixSumsSRV);

		auto colGroupPrefixSumsSRV = colGroupOffsetSumsBuf->GetSRV();
		shader->SetParameter("GroupColOffset", colGroupPrefixSumsSRV);

		// Bind parameters
		shader->BindParameters();

		// Dispatch
		UINT gx = width;
		UINT gy = (UINT)ceilf(height / 64.0f);
		d3dCommandList->Dispatch(gx, gy, 1);
	}

	// UAV ¡ú SRV for runtime sampling
	d3d12RHI->TransitionResource(enviromentCDFTex0->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
}

void Render::IntegratePass()
{

}

void Render::GatherAllMeshBatchs()
{
	meshBatchs.clear();

	// Get all mesh components in world
	auto actors = world->GetActors();
	std::vector<MeshComponent*> allMeshComponents;

	for (auto actor : actors)
	{
		auto Light = dynamic_cast<LightActor*>(actor);
		if (Light && !Light->IsDrawMesh())
		{
			continue;
		}

		auto meshComponents = actor->GetComponentsOfClass<MeshComponent>();
		for (auto meshComponent : meshComponents)
		{
			if (meshComponent->IsMeshValid())
			{
				allMeshComponents.push_back(meshComponent);
			}
		}
	}

	// Calculate BoundingFrustum in view space
	CameraComponent* cameraComponent = world->GetCameraComponent();
	TMatrix ViewToWorld = cameraComponent->GetView().Invert();

	BoundingFrustum ViewSpaceFrustum;
	BoundingFrustum::CreateFromMatrix(ViewSpaceFrustum, cameraComponent->GetProj());


	std::vector<MeshComponent*> MeshComponentsAfterCulling;
	for (auto meshComponent : allMeshComponents)
	{
		TMatrix WorldToLocal = meshComponent->GetWorldTransform().GetTransformMatrix().Invert();
		TMatrix ViewToLocal = ViewToWorld * WorldToLocal;

		// Transform the frustum from view space to the object's local space.
		BoundingFrustum LocalSpaceFrustum;

		// Note: BoundingFrustum::Transform( BoundingFrustum& Out, FXMMATRIX M) cannot contain a scale transform.
		// Ref: https://docs.microsoft.com/en-us/windows/win32/api/directxcollision/nf-directxcollision-boundingfrustum-transform
		// So it will have problems when actor has scale transform !!!
		// TODO: fix it
		ViewSpaceFrustum.Transform(LocalSpaceFrustum, ViewToLocal);

		TBoundingBox BoundingBox;
		if (bEnableFrustumCulling && meshComponent->GetLocalBoundingBox(BoundingBox))
		{
			if (LocalSpaceFrustum.Contains(BoundingBox.GetD3DBox()) != DirectX::DISJOINT)
			{
				MeshComponentsAfterCulling.push_back(meshComponent);
			}
		}
		else
		{
			MeshComponentsAfterCulling.push_back(meshComponent);
		}
	}

	// Generate MeshBatchs
	for (auto meshComponent : MeshComponentsAfterCulling)
	{
		std::string meshName = meshComponent->GetMeshName();

		MeshBatch meshBatch;
		meshBatch.meshName = meshName;
		meshBatch.inputLayoutName = MeshRepository::Get().meshMap.at(meshName).GetInputLayoutName();

		//Create Object ConstantBuffer		
		TMatrix World = meshComponent->GetWorldTransform().GetTransformMatrix();
		TMatrix PrevWorld = meshComponent->GetPrevWorldTransform().GetTransformMatrix();
		TMatrix TexTransform = meshComponent->TexTransform;

		ObjectConstants objConst;
		objConst.World = World.Transpose();
		objConst.PrevWorld = PrevWorld.Transpose();
		objConst.TexTransform = TexTransform.Transpose();
		meshBatch.objConstantBuffer = d3d12RHI->CreateConstantBuffer(&objConst, sizeof(objConst));

		meshBatch.meshComponent = meshComponent;
		meshBatch.bUseSDF = meshComponent->bUseSDF;

		//Add to list
		meshBatchs.emplace_back(meshBatch);
	}
}

TMatrix Render::TextureTransform()
{
	TMatrix T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	return T;
}

void Render::UpdateLightData()
{
	std::vector<LightShaderParameters> lightShaderParametersArray;

	auto Lights = world->GetAllActorsOfClass<LightActor>();

	for (UINT LightIdx = 0; LightIdx < Lights.size(); LightIdx++)
	{
		auto Light = Lights[LightIdx];

		if (Light->GetType() == ELightType::DirectionalLight)
		{
			auto DirectionalLight = dynamic_cast<DirectionalLightActor*>(Light);
			assert(DirectionalLight);

			LightShaderParameters lightShaderParameter;
			lightShaderParameter.color = DirectionalLight->GetLightColor();
			lightShaderParameter.intensity = DirectionalLight->GetLightIntensity();
			lightShaderParameter.position = DirectionalLight->GetActorLocation();
			lightShaderParameter.direction = DirectionalLight->GetLightDirection();
			lightShaderParameter.lightType = ELightType::DirectionalLight;

			lightShaderParametersArray.push_back(lightShaderParameter);
		}
		else if (Light->GetType() == ELightType::PointLight)
		{
			auto PointLight = dynamic_cast<PointLightActor*>(Light);
			assert(PointLight);

			LightShaderParameters lightShaderParameter;
			lightShaderParameter.color = PointLight->GetLightColor();
			lightShaderParameter.intensity = PointLight->GetLightIntensity();
			lightShaderParameter.position = PointLight->GetActorLocation();
			lightShaderParameter.range = PointLight->GetAttenuationRange();
			lightShaderParameter.lightType = ELightType::PointLight;

			lightShaderParametersArray.push_back(lightShaderParameter);
		}
		else if (Light->GetType() == ELightType::SpotLight)
		{
			auto SpotLight = dynamic_cast<SpotLightActor*>(Light);
			assert(SpotLight);

			LightShaderParameters lightShaderParameter;
			lightShaderParameter.color = SpotLight->GetLightColor();
			lightShaderParameter.intensity = SpotLight->GetLightIntensity();
			lightShaderParameter.position = SpotLight->GetActorLocation();
			lightShaderParameter.range = SpotLight->GetAttenuationRange();
			lightShaderParameter.direction = SpotLight->GetLightDirection();

			float ClampedInnerConeAngle = std::clamp(SpotLight->GetInnerConeAngle(), 0.0f, 89.0f);
			float ClampedOuterConeAngle = std::clamp(SpotLight->GetOuterConeAngle(), ClampedInnerConeAngle + 0.001f, 89.0f + 0.001f);
			ClampedInnerConeAngle *= (TMath::Pi / 180.0f);
			ClampedOuterConeAngle *= (TMath::Pi / 180.0f);
			float CosInnerCone = cos(ClampedInnerConeAngle);
			float CosOuterCone = cos(ClampedOuterConeAngle);
			float InvCosConeDifference = 1.0f / (CosInnerCone - CosOuterCone);

			lightShaderParameter.spotAngles = TVector2(CosOuterCone, InvCosConeDifference);
			lightShaderParameter.spotRadius = SpotLight->GetBottomRadius();
			lightShaderParameter.lightType = ELightType::SpotLight;

			lightShaderParametersArray.push_back(lightShaderParameter);
		}

	}

	lightCount = (UINT)lightShaderParametersArray.size();
	if (lightCount > 0)
	{
		lightShaderParametersBuffer = d3d12RHI->CreateStructuredBuffer(lightShaderParametersArray.data(),
			(uint32_t)(sizeof(LightShaderParameters)), (uint32_t)(lightShaderParametersArray.size()));
	}
	else
	{
		lightShaderParametersBuffer = nullptr;
	}

	//---------------------------IBL ambient Light----------------------------------------------------------//	
	if (bEnableIBLEnvLighting)
	{
		LightShaderParameters lightShaderParameter;
		lightShaderParameter.lightType = ELightType::AmbientLight;
		lightShaderParametersArray.push_back(lightShaderParameter);
	}

	lightCount = (UINT)lightShaderParametersArray.size();
	if (lightCount > 0)
	{
		lightShaderParametersBuffer = d3d12RHI->CreateStructuredBuffer(lightShaderParametersArray.data(),
			(uint32_t)(sizeof(LightShaderParameters)), (uint32_t)(lightShaderParametersArray.size()));
	}
	else
	{
		lightShaderParametersBuffer = nullptr;
	}

	LightCommonData lightCommonData;
	lightCommonData.lightCount = lightCount;
	lightCommonDataBuffer = d3d12RHI->CreateConstantBuffer(&lightCommonData, sizeof(lightCommonData));

}

void Render::UpdateBasePassCB()
{
	CameraComponent* cameraComponent = world->GetCameraComponent();

	TMatrix View = cameraComponent->GetView();
	TMatrix Proj = cameraComponent->GetProj();

	if (renderSettings.bEnableTAA)
	{
		UINT SampleIdx = frameCount % TAA_SAMPLE_COUNT;
		double JitterX = Halton_2[SampleIdx] / (double)windowWidth;
		double JitterY = Halton_3[SampleIdx] / (double)windowHeight;
		Proj(2, 0) += (float)JitterX;
		Proj(2, 1) += (float)JitterY;
	}

	TMatrix ViewProj = View * Proj;
	TMatrix InvView = View.Invert();
	TMatrix InvProj = Proj.Invert();
	TMatrix InvViewProj = ViewProj.Invert();
	TMatrix PrevViewProj = cameraComponent->GetPrevViewProj();

	PassConstants BasePassCB;
	BasePassCB.View = View.Transpose();
	BasePassCB.Proj = Proj.Transpose();
	BasePassCB.ViewProj = ViewProj.Transpose();
	BasePassCB.InvView = InvView.Transpose();
	BasePassCB.InvProj = InvProj.Transpose();
	BasePassCB.InvViewProj = InvViewProj.Transpose();
	BasePassCB.PrevViewProj = PrevViewProj.Transpose();
	BasePassCB.EyePosW = cameraComponent->GetWorldLocation();
	BasePassCB.RenderTargetSize = TVector2((float)windowWidth, (float)windowHeight);
	BasePassCB.InvRenderTargetSize = TVector2(1.0f / windowWidth, 1.0f / windowHeight);
	BasePassCB.NearZ = cameraComponent->GetNearZ();
	BasePassCB.FarZ = cameraComponent->GetFarZ();

	basePassCBRef = d3d12RHI->CreateConstantBuffer(&BasePassCB, sizeof(BasePassCB));
}

void Render::GetBasePassMeshCommandMap()
{
	baseMeshCommandMap.clear();

	for (const MeshBatch& meshBatch : meshBatchs)
	{
		// Create MeshCommand
		MeshCommand meshCommand;
		meshCommand.meshName = meshBatch.meshName;

		auto materialInstance = meshBatch.meshComponent->GetMaterialInstance();
		meshCommand.renderState = materialInstance->material->renderState;

		// Get material constanct buffer
		if (materialInstance->materialConstantBuffer == nullptr)
		{
			materialInstance->CreateMaterialConstantBuffer(d3d12RHI);
		}

		// Set shader parameters
		meshCommand.SetShaderParameter("cbMaterialData", materialInstance->materialConstantBuffer);
		meshCommand.SetShaderParameter("cbPass", basePassCBRef);
		meshCommand.SetShaderParameter("cbPerObject", meshBatch.objConstantBuffer);
		for (const auto& Pair : materialInstance->parameters.textureMap)
		{
			std::string TextureName = Pair.second;
			ShaderResourceView* SRV = nullptr;

			if (TextureName == skyCubeTextureName)
			{
				SRV = IBLEnvironmentMap->GetRTCube()->GetSRV();
			}
			else
			{
				SRV = TextureRepository::Get().textureMap[TextureName]->GetD3DTexture()->GetSRV();
			}

			meshCommand.SetShaderParameter(Pair.first, SRV);
		}

		// Get PSO descriptor of this mesh
		GraphicsPSODescriptor Descriptor;
		Descriptor.inputLayoutName = meshBatch.inputLayoutName;
		Descriptor.rasterizerDesc.CullMode = meshCommand.renderState.cullMode;
		Descriptor.depthStencilDesc.DepthFunc = meshCommand.renderState.depthFunc;

		Material* material = materialInstance->material;
		ShaderDefines EmptyShaderDefines;
		Descriptor.shader = material->GetShader(EmptyShaderDefines, d3d12RHI);

		// GBuffer PSO common settings
		Descriptor.RTVFormats[0] = GBufferBaseColor->GetFormat();
		Descriptor.RTVFormats[1] = GBufferNormal->GetFormat();
		Descriptor.RTVFormats[2] = GBufferWorldPos->GetFormat();
		Descriptor.RTVFormats[3] = GBufferORM->GetFormat();
		Descriptor.RTVFormats[4] = GBufferVelocity->GetFormat();
		Descriptor.RTVFormats[5] = GBufferEmissive->GetFormat();
		Descriptor.numRenderTargets = GBufferCount;
		Descriptor.depthStencilFormat = d3d12RHI->GetViewportInfo().depthStencilFormat;
		Descriptor._4xMsaaState = false; //can't use msaa in deferred rendering.

		// Create a new PSO if we don't have the pso with this descriptor
		graphicsPSOManager->TryCreatePSO(Descriptor);

		// Save MeshCommand according to PSO type
		baseMeshCommandMap.insert({ Descriptor, MeshCommandList() });
		baseMeshCommandMap[Descriptor].emplace_back(meshCommand);
	}
}

void Render::BasePass()
{
 	UpdateBasePassCB();
 
 	GetBasePassMeshCommandMap();

	// Use screen viewport 
	D3D12_VIEWPORT ScreenViewport;
	D3D12_RECT ScissorRect;
	d3d12RHI->GetViewport()->GetD3DViewport(ScreenViewport, ScissorRect);
	d3dCommandList->RSSetViewports(1, &ScreenViewport);
	d3dCommandList->RSSetScissorRects(1, &ScissorRect);

	// Transit to render target state
	d3d12RHI->TransitionResource(GBufferBaseColor->GetTexture()->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	d3d12RHI->TransitionResource(GBufferNormal->GetTexture()->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	d3d12RHI->TransitionResource(GBufferWorldPos->GetTexture()->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	d3d12RHI->TransitionResource(GBufferORM->GetTexture()->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	d3d12RHI->TransitionResource(GBufferVelocity->GetTexture()->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	d3d12RHI->TransitionResource(GBufferEmissive->GetTexture()->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);

	// Clear renderTargets
	d3dCommandList->ClearRenderTargetView(GBufferBaseColor->GetRTV()->GetDescriptorHandle(), GBufferBaseColor->GetClearColorPtr(), 0, nullptr);
	d3dCommandList->ClearRenderTargetView(GBufferNormal->GetRTV()->GetDescriptorHandle(), GBufferNormal->GetClearColorPtr(), 0, nullptr);
	d3dCommandList->ClearRenderTargetView(GBufferWorldPos->GetRTV()->GetDescriptorHandle(), GBufferWorldPos->GetClearColorPtr(), 0, nullptr);
	d3dCommandList->ClearRenderTargetView(GBufferORM->GetRTV()->GetDescriptorHandle(), GBufferORM->GetClearColorPtr(), 0, nullptr);
	d3dCommandList->ClearRenderTargetView(GBufferVelocity->GetRTV()->GetDescriptorHandle(), GBufferVelocity->GetClearColorPtr(), 0, nullptr);
	d3dCommandList->ClearRenderTargetView(GBufferEmissive->GetRTV()->GetDescriptorHandle(), GBufferEmissive->GetClearColorPtr(), 0, nullptr);

	// Clear depthstencil
	d3dCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the renderTargets we are going to render to.
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> RtvDescriptors;
	RtvDescriptors.push_back(GBufferBaseColor->GetRTV()->GetDescriptorHandle());
	RtvDescriptors.push_back(GBufferNormal->GetRTV()->GetDescriptorHandle());
	RtvDescriptors.push_back(GBufferWorldPos->GetRTV()->GetDescriptorHandle());
	RtvDescriptors.push_back(GBufferORM->GetRTV()->GetDescriptorHandle());
	RtvDescriptors.push_back(GBufferVelocity->GetRTV()->GetDescriptorHandle());
	RtvDescriptors.push_back(GBufferEmissive->GetRTV()->GetDescriptorHandle());

	auto DescriptorCache = d3d12RHI->GetDevice()->GetCommandContext()->GetDescriptorCache();
	CD3DX12_GPU_DESCRIPTOR_HANDLE GpuHandle;
	CD3DX12_CPU_DESCRIPTOR_HANDLE CpuHandle;
	DescriptorCache->AppendRtvDescriptors(RtvDescriptors, GpuHandle, CpuHandle);

	auto dsv = DepthStencilView();
	d3dCommandList->OMSetRenderTargets(GBufferCount, &CpuHandle, true, &dsv);

	// Draw all mesh
	for (const auto& Pair : baseMeshCommandMap)
	{

		const GraphicsPSODescriptor& PSODescriptor = Pair.first;
		const MeshCommandList& meshCommandList = Pair.second;

		// Set PSO
		d3dCommandList->SetPipelineState(graphicsPSOManager->GetPSO(PSODescriptor));

		// Set RootSignature
		Shader* shader = PSODescriptor.shader;
		d3dCommandList->SetGraphicsRootSignature(shader->rootSignature.Get()); //should before binding

		for (const MeshCommand& meshCommand : meshCommandList)
		{
			auto& textureMap = TextureRepository::Get().textureMap;

			// Set paramters
			meshCommand.ApplyShaderParamters(shader);

			// Bind paramters
			shader->BindParameters();

			const MeshProxy& meshProxy = meshProxyMap.at(meshCommand.meshName);

			// Set vertex buffer
			d3d12RHI->SetVertexBuffer(meshProxy.vertexBufferRef, 0, meshProxy.vertexByteStride, meshProxy.vertexBufferByteSize);

			// Set index buffer
			d3d12RHI->SetIndexBuffer(meshProxy.indexBufferRef, 0, meshProxy.indexFormat, meshProxy.indexBufferByteSize);

			D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			d3dCommandList->IASetPrimitiveTopology(PrimitiveType);

			// Draw 
			auto& SubMesh = meshProxy.subMeshs.at("Default");
			d3dCommandList->DrawIndexedInstanced(SubMesh.indexCount, 1, SubMesh.startIndexLocation, SubMesh.baseVertexLocation, 0);
		}
	}

	// Transit to generic read state
	d3d12RHI->TransitionResource(GBufferBaseColor->GetTexture()->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	d3d12RHI->TransitionResource(GBufferNormal->GetTexture()->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	d3d12RHI->TransitionResource(GBufferWorldPos->GetTexture()->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	d3d12RHI->TransitionResource(GBufferORM->GetTexture()->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	d3d12RHI->TransitionResource(GBufferVelocity->GetTexture()->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	d3d12RHI->TransitionResource(GBufferEmissive->GetTexture()->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
}

void Render::GatherLightDebugPrimitives(std::vector<Line>& outLines)
{
	// Lights debug primitives
	{
		auto lights = world->GetAllActorsOfClass<LightActor>();
		for (UINT lightIdx = 0; lightIdx < lights.size(); lightIdx++)
		{
			auto light = lights[lightIdx];

			if (!light->IsDrawDebug())
			{
				continue;
			}

			if (light->GetType() == ELightType::DirectionalLight)
			{
				auto directionalLight = dynamic_cast<DirectionalLightActor*>(light);
				assert(directionalLight);

				TVector3 direction = directionalLight->GetLightDirection();
				TVector3 startPos = directionalLight->GetActorLocation();
				float debugLength = 3.0f;
				TVector3 endPos = startPos + direction * debugLength;

				TVector3 v1 = direction.Cross(TVector3::Up);
				v1.Normalize();
				TVector3 Offset = v1 * 0.5f;

				outLines.push_back(Line(startPos - Offset, endPos - Offset, Color::White));
				outLines.push_back(Line(startPos, endPos, Color::White));
				outLines.push_back(Line(startPos + Offset, endPos + Offset, Color::White));

			}
			else if (light->GetType() == ELightType::PointLight)
			{
				auto pointLight = dynamic_cast<PointLightActor*>(light);
				assert(pointLight);

				TVector3 centerPos = pointLight->GetActorLocation();
				float radius = pointLight->GetAttenuationRange();
				TVector3 v1 = TVector3(1.0f, 0.0f, 0.0f);
				TVector3 v2 = TVector3(0.0f, 0.0f, 1.0f);
				TVector3 v3 = TVector3(0.0f, 1.0f, 0.0f);

				// Horizon circle
				TVector3 lastPoint = TVector3::Zero;
				bool bFirstPoint = true;
				for (float deltaAngle = 0; deltaAngle <= 360; deltaAngle += 20)
				{
					float radian = deltaAngle * (TMath::Pi / 180.0f);
					TVector3 point = centerPos + (v1 * sin(radian) + v2 * cos(radian)) * radius;

					if (bFirstPoint)
					{
						bFirstPoint = false;
					}
					else
					{
						outLines.push_back(Line(lastPoint, point, Color::Yellow));
					}

					lastPoint = point;
				}

				// Vertical circle
				lastPoint = TVector3::Zero;
				bFirstPoint = true;
				for (float deltaAngle = 0; deltaAngle <= 360; deltaAngle += 20)
				{
					float radian = deltaAngle * (TMath::Pi / 180.0f);
					TVector3 point = centerPos + (v1 * sin(radian) + v3 * cos(radian)) * radius;

					if (bFirstPoint)
					{
						bFirstPoint = false;
					}
					else
					{
						outLines.push_back(Line(lastPoint, point, Color::Yellow));
					}

					lastPoint = point;
				}

			}
			else if (light->GetType() == ELightType::SpotLight)
			{
				auto spotLight = dynamic_cast<SpotLightActor*>(light);
				assert(spotLight);

				TVector3 tipPos = spotLight->GetActorLocation();
				float height = spotLight->GetAttenuationRange();
				TVector3 direction = spotLight->GetLightDirection();
				TVector3 directionEnd = tipPos + height * direction;
				float bottomRadius = spotLight->GetBottomRadius();

				// Direction line
				outLines.push_back(Line(tipPos, directionEnd, Color::White));

				// Slant lines
				TVector3 v1 = direction.Cross(TVector3::Up);
				v1.Normalize();
				TVector3 v2 = direction.Cross(v1);
				v2.Normalize();

				for (float deltaAngle = 0; deltaAngle <= 360; deltaAngle += 20)
				{
					float radian = deltaAngle * (TMath::Pi / 180.0f);
					TVector3 slantPoint = directionEnd + (v1 * sin(radian) + v2 * cos(radian)) * bottomRadius;

					outLines.push_back(Line(tipPos, slantPoint, Color::Yellow));
				}
			}
		}
	}
}

void Render::GatherAllPrimitiveBatchs()
{
	psoPrimitiveBatchMap.clear();

	// Points
	{
		// Get all points
		std::vector<PrimitiveVertex> vertices;
		const auto& points = world->GetPoints();
		for (const auto& point : points)
		{
			vertices.push_back(PrimitiveVertex(point.point, point.color));
		}

		GatherPrimitiveBatchs(vertices, D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
	}

	// Lines
	{
		// Get all lines
		std::vector<Line> lines;
		const auto& worldLines = world->GetLines();
		lines.insert(lines.end(), worldLines.begin(), worldLines.end());

		std::vector<Line> lightDebugLines;
		GatherLightDebugPrimitives(lightDebugLines);
		lines.insert(lines.end(), lightDebugLines.begin(), lightDebugLines.end());

		std::vector<PrimitiveVertex> vertices;
		for (const auto& line : lines)
		{
			vertices.push_back(PrimitiveVertex(line.pointA, line.color));
			vertices.push_back(PrimitiveVertex(line.pointB, line.color));
		}

		GatherPrimitiveBatchs(vertices, D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
	}


	// Triangles
	{
		// Get all triangles
		std::vector<PrimitiveVertex> vertices;
		const auto& Triangles = world->GetTriangles();
		for (const auto& triangle : Triangles) {
			vertices.push_back(PrimitiveVertex(triangle.pointA, triangle.color));
			vertices.push_back(PrimitiveVertex(triangle.pointB, triangle.color));
			vertices.push_back(PrimitiveVertex(triangle.pointC, triangle.color));
		}

		GatherPrimitiveBatchs(vertices, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	}
}

void Render::GatherPrimitiveBatchs(const std::vector<PrimitiveVertex>& vertices, D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveType)
{
	// Primitive PSO
	GraphicsPSODescriptor psoDescriptor;
	psoDescriptor.inputLayoutName = std::string("PositionColorInputLayout");
	psoDescriptor.shader = primitiveShader.get();
	psoDescriptor.primitiveTopologyType = primitiveType;

	// GBuffer PSO common settings
	psoDescriptor.RTVFormats[0] = GBufferBaseColor->GetFormat();
	psoDescriptor.RTVFormats[1] = GBufferNormal->GetFormat();
	psoDescriptor.RTVFormats[2] = GBufferWorldPos->GetFormat();
	psoDescriptor.RTVFormats[3] = GBufferORM->GetFormat();
	psoDescriptor.RTVFormats[4] = GBufferVelocity->GetFormat();
	psoDescriptor.RTVFormats[5] = GBufferEmissive->GetFormat();
	psoDescriptor.numRenderTargets = GBufferCount;
	psoDescriptor.depthStencilFormat = d3d12RHI->GetViewportInfo().depthStencilFormat;
	psoDescriptor._4xMsaaState = false; //can't use msaa in deferred rendering.

	// If don't find this PSO, create new PSO and PrimitiveBatch
	graphicsPSOManager->TryCreatePSO(psoDescriptor);

	if (psoPrimitiveBatchMap.find(psoDescriptor) == psoPrimitiveBatchMap.end())
	{
		psoPrimitiveBatchMap.emplace(std::make_pair(psoDescriptor, PrimitiveBatch()));
		PrimitiveBatch& primitiveBatch = psoPrimitiveBatchMap[psoDescriptor];

		if (primitiveType == D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT)
		{
			primitiveBatch.primitiveType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		}
		else if (primitiveType == D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE)
		{
			primitiveBatch.primitiveType = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		}
		else if (primitiveType == D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE)
		{
			primitiveBatch.primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		}
		else
		{
			assert(0);
		}
	}

	// Update vertex buffer
	PrimitiveBatch& primitiveBatch = psoPrimitiveBatchMap[psoDescriptor];
	primitiveBatch.currentVertexNum = (int)vertices.size();

	if (primitiveBatch.currentVertexNum > 0)
	{
		const UINT VbByteSize = (UINT)vertices.size() * sizeof(PrimitiveVertex);
		primitiveBatch.vertexBufferRef = d3d12RHI->CreateVertexBuffer(vertices.data(), VbByteSize);
	}
	else
	{
		primitiveBatch.vertexBufferRef = nullptr;
	}
}

void Render::PrimitivesPass()
{
	GatherAllPrimitiveBatchs();

	// Transit to render target state
	d3d12RHI->TransitionResource(GBufferBaseColor->GetTexture()->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	d3d12RHI->TransitionResource(GBufferNormal->GetTexture()->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	d3d12RHI->TransitionResource(GBufferWorldPos->GetTexture()->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	d3d12RHI->TransitionResource(GBufferORM->GetTexture()->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	d3d12RHI->TransitionResource(GBufferVelocity->GetTexture()->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	d3d12RHI->TransitionResource(GBufferEmissive->GetTexture()->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);

	// Specify the renderTargets we are going to render to.
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvDescriptors;
	rtvDescriptors.push_back(GBufferBaseColor->GetRTV()->GetDescriptorHandle());
	rtvDescriptors.push_back(GBufferNormal->GetRTV()->GetDescriptorHandle());
	rtvDescriptors.push_back(GBufferWorldPos->GetRTV()->GetDescriptorHandle());
	rtvDescriptors.push_back(GBufferORM->GetRTV()->GetDescriptorHandle());
	rtvDescriptors.push_back(GBufferVelocity->GetRTV()->GetDescriptorHandle());
	rtvDescriptors.push_back(GBufferEmissive->GetRTV()->GetDescriptorHandle());

	auto descriptorCache = d3d12RHI->GetDevice()->GetCommandContext()->GetDescriptorCache();
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle;
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	descriptorCache->AppendRtvDescriptors(rtvDescriptors, gpuHandle, cpuHandle);

	auto depthDes = DepthStencilView();
	d3dCommandList->OMSetRenderTargets(GBufferCount, &cpuHandle, true, &depthDes);

	// Draw all PrimitiveBatchs
	for (const auto& pair : psoPrimitiveBatchMap)
	{
		const GraphicsPSODescriptor& psoDescriptor = pair.first;
		const PrimitiveBatch& primitiveBatch = pair.second;

		if (primitiveBatch.currentVertexNum > 0)
		{
			// Set PSO	
			d3dCommandList->SetPipelineState(graphicsPSOManager->GetPSO(psoDescriptor));

			// Set RootSignature
			Shader* shader = psoDescriptor.shader;
			d3dCommandList->SetGraphicsRootSignature(shader->rootSignature.Get()); //should before binding

			// Set paramters
			shader->SetParameter("cbPass", basePassCBRef);

			// Bind paramters
			shader->BindParameters();

			// Set vertex buffer
			d3d12RHI->SetVertexBuffer(primitiveBatch.vertexBufferRef, 0, sizeof(PrimitiveVertex), primitiveBatch.currentVertexNum * sizeof(PrimitiveVertex));

			d3dCommandList->IASetPrimitiveTopology(primitiveBatch.primitiveType);

			// Draw 
			d3dCommandList->DrawInstanced(primitiveBatch.currentVertexNum, 1, 0, 0);
		}
	}

	// Transit to generic read state
	d3d12RHI->TransitionResource(GBufferBaseColor->GetTexture()->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	d3d12RHI->TransitionResource(GBufferNormal->GetTexture()->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	d3d12RHI->TransitionResource(GBufferWorldPos->GetTexture()->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	d3d12RHI->TransitionResource(GBufferORM->GetTexture()->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	d3d12RHI->TransitionResource(GBufferVelocity->GetTexture()->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	d3d12RHI->TransitionResource(GBufferEmissive->GetTexture()->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
}


void Render::DeferredLightingPass()
{
	// Indicate a state transition on the resource usage.
	d3d12RHI->TransitionResource(colorTexture->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);

	// Set the viewport and scissor rect.
	D3D12_VIEWPORT ScreenViewport;
	D3D12_RECT ScissorRect;
	d3d12RHI->GetViewport()->GetD3DViewport(ScreenViewport, ScissorRect);
	d3dCommandList->RSSetViewports(1, &ScreenViewport);
	d3dCommandList->RSSetScissorRects(1, &ScissorRect);

	// Clear the colorTexture and depth buffer.
	float* clearValue = colorTexture->GetRTVClearValuePtr();
	D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle = colorTexture->GetRTV()->GetDescriptorHandle();
	d3dCommandList->ClearRenderTargetView(RTVHandle, clearValue, 0, nullptr);

	// Specify the buffers we are going to render to.
	auto depthStencilView = DepthStencilView();
	d3dCommandList->OMSetRenderTargets(1, &RTVHandle, true, &depthStencilView);

	// Set DeferredLighting PSO
	d3dCommandList->SetPipelineState(graphicsPSOManager->GetPSO(deferredLightingPSODescriptor));

	// Set RootSignature
	Shader* shader = deferredLightingPSODescriptor.shader;
	d3dCommandList->SetGraphicsRootSignature(shader->rootSignature.Get()); //should before binding

	auto& textureMap = TextureRepository::Get().textureMap;

	//-------------------------------------Set paramters-------------------------------------------

	shader->SetParameter("cbPass", basePassCBRef);
	shader->SetParameter("cbDeferredLighting", deferredLightPassCBRef);
	shader->SetParameter("LightCommonData", lightCommonDataBuffer);
	shader->SetParameter("BaseColorGbuffer", GBufferBaseColor->GetTexture()->GetSRV());
	shader->SetParameter("NormalGbuffer", GBufferNormal->GetTexture()->GetSRV());
	shader->SetParameter("WorldPosGbuffer", GBufferWorldPos->GetTexture()->GetSRV());
	shader->SetParameter("OrmGbuffer", GBufferORM->GetTexture()->GetSRV());
	shader->SetParameter("EmissiveGbuffer", GBufferEmissive->GetTexture()->GetSRV());

	if (lightCount > 0)
	{
		shader->SetParameter("Lights", lightShaderParametersBuffer->GetSRV());
	}
	else
	{
		shader->SetParameter("Lights", structuredBufferNullDescriptor.get());
	}

	if (bEnableIBLEnvLighting)
	{
		shader->SetParameter("IBLIrradianceMap", IBLIrradianceMap->GetRTCube()->GetSRV());

		auto BRDFIntegrationMapSRV = textureMap["IBL_BRDF_LUT"]->GetD3DTexture()->GetSRV();
		shader->SetParameter("BrdfLUT", BRDFIntegrationMapSRV);

		std::vector<ShaderResourceView*> IBLPrefilterEnvMapSRVs;
		for (UINT i = 0; i < IBLPrefilterMaxMipLevel; i++)
		{
			IBLPrefilterEnvMapSRVs.push_back(IBLPrefilterEnvMaps[i]->GetRTCube()->GetSRV());
		}
		shader->SetParameter("IBLPrefilterEnvMaps", IBLPrefilterEnvMapSRVs);
	}
	else
	{
		shader->SetParameter("IBLIrradianceMap", textureCubeNullDescriptor.get());
		shader->SetParameter("BrdfLUT", texture2DNullDescriptor.get());

		std::vector<ShaderResourceView*> NullSRVs;
		for (UINT i = 0; i < IBLPrefilterMaxMipLevel; i++)
		{
			NullSRVs.push_back(textureCubeNullDescriptor.get());
		}
		shader->SetParameter("IBLPrefilterEnvMaps", NullSRVs);
	}

	//-------------------------------------------------------------------------------------------

	// Bind paramters
	shader->BindParameters();

	// Draw ScreenQuad
	{
		const MeshProxy& meshProxy = meshProxyMap.at("ScreenQuadMesh");

		// Set vertex buffer
		d3d12RHI->SetVertexBuffer(meshProxy.vertexBufferRef, 0, meshProxy.vertexByteStride, meshProxy.vertexBufferByteSize);
		// Set index buffer
		d3d12RHI->SetIndexBuffer(meshProxy.indexBufferRef, 0, meshProxy.indexFormat, meshProxy.indexBufferByteSize);

		D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		d3dCommandList->IASetPrimitiveTopology(primitiveType);

		// Draw 
		auto& subMesh = meshProxy.subMeshs.at("Default");
		d3dCommandList->DrawIndexedInstanced(subMesh.indexCount, 1, subMesh.startIndexLocation, subMesh.baseVertexLocation, 0);
	}

	// Transition to PRESENT state.
	d3d12RHI->TransitionResource(colorTexture->GetResource(), D3D12_RESOURCE_STATE_PRESENT);
}

void Render::PostProcessPass()
{
	d3d12RHI->TransitionResource(colorTexture->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);

	d3d12RHI->TransitionResource(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET);

	// Set the viewport and scissor rect.
	D3D12_VIEWPORT ScreenViewport;
	D3D12_RECT ScissorRect;
	d3d12RHI->GetViewport()->GetD3DViewport(ScreenViewport, ScissorRect);
	d3dCommandList->RSSetViewports(1, &ScreenViewport);
	d3dCommandList->RSSetScissorRects(1, &ScissorRect);

	// Clear the back buffer.
	float* ClearColor = CurrentBackBufferClearColor();
	d3dCommandList->ClearRenderTargetView(CurrentBackBufferView(), ClearColor, 0, nullptr);

	// Specify the buffers we are going to render to.
	auto currentBackBufferView = CurrentBackBufferView();
	d3dCommandList->OMSetRenderTargets(1, &currentBackBufferView, true, nullptr);

	// Set PSO
	d3dCommandList->SetPipelineState(graphicsPSOManager->GetPSO(postProcessPSODescriptor));

	// Set RootSignature
	d3dCommandList->SetGraphicsRootSignature(postProcessShader->rootSignature.Get()); //should before binding

	// Set paramters
	postProcessShader->SetParameter("ColorTexture", colorTexture->GetSRV());

	// Bind paramters
	postProcessShader->BindParameters();

	// Draw ScreenQuad
	{
		const MeshProxy& meshProxy = meshProxyMap.at("ScreenQuadMesh");

		// Set vertex buffer
		d3d12RHI->SetVertexBuffer(meshProxy.vertexBufferRef, 0, meshProxy.vertexByteStride, meshProxy.vertexBufferByteSize);

		// Set index buffer
		d3d12RHI->SetIndexBuffer(meshProxy.indexBufferRef, 0, meshProxy.indexFormat, meshProxy.indexBufferByteSize);

		D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		d3dCommandList->IASetPrimitiveTopology(PrimitiveType);

		// Draw 
		auto& SubMesh = meshProxy.subMeshs.at("Default");
		d3dCommandList->DrawIndexedInstanced(SubMesh.indexCount, 1, SubMesh.startIndexLocation, SubMesh.baseVertexLocation, 0);
	}

	// Transition back-buffer to PRESENT state.
	d3d12RHI->TransitionResource(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT);
}

void Render::OnDestroy()
{
	d3d12RHI->FlushCommandQueue();
}
