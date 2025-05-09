#pragma once

#include <unordered_map>
#include <memory>
#include <wrl/client.h>
#include "../Shader/Shader.h"
#include "../Actor/Actor.h"
#include "../Actor/Light/LightActor.h"
#include "../World/World.h"
#include "../Component/MeshComponent.h"
#include "../Component/CameraComponent.h"
#include "../Texture/Texture.h"
#include "../Material/Material.h"
#include "../Engine/GameTimer.h"
#include "RenderProxy.h"
#include "InputLayout.h"
#include "PSO.h"
#include "MeshBatch.h"
#include "PrimitiveBatch.h"
#include "SpriteBatch.h"
#include "SpriteFont.h"
#include "RenderTarget.h"
#include "SceneCaptureCube.h"
#include "../Resource/D3D12RHI.h"

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

#define TAA_SAMPLE_COUNT 8

enum class ERenderPass
{
	SHADOWSPASS,
	BASEPASS,
	PRIMITIVEPASS,
	SPRITEPASS,
	IRRADIANCE,
	DEFERREDLIGHTING,
};

struct ComputeShaderTestData
{
	DirectX::XMFLOAT3 v1;
	DirectX::XMFLOAT2 v2;
};

struct TRenderSettings
{
	bool bUseTBDR = false;
	bool bEnableTAA = false;
	bool bEnableSSR = false;
	bool bEnableSSAO = false;
	bool bDebugSDFScene = false;
	bool bDrawDebugText = false;
};

class Render
{
public:
	Render();
	~Render();

	bool IsInitialize();
	bool Initialize(int windowWidth, int windowHeight, D3D12RHI* inD3D12RHI, World* inWorld, const TRenderSettings& settings);
	void OnResize(int newWidth, int newHeight);
	void Draw(const GameTimer& gt);
	void EndFrame();
	void OnDestroy();

private:
	float AspectRatio() const;
	Resource* CurrentBackBuffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
	float* CurrentBackBufferClearColor() const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

private:
	void CreateRenderResource();
	void CreateNullDescriptors();
	void CreateTextures();
	void CreateGBuffers();
	void CreateColorTextures();
	void CreateMeshProxys();
	void CreateInputLayouts();
	void CreateGlobalShaders();
	void CreateGlobalPSO();

private:
	void SetDescriptorHeaps();

	void GatherAllMeshBatchs();
	TMatrix TextureTransform();
 	void UpdateLightData();
 	void UpdateBasePassCB();
 	void GetBasePassMeshCommandMap();
	void BasePass();
 	void GatherLightDebugPrimitives(std::vector<Line>& outLines);
 	void GatherAllPrimitiveBatchs();
 	void GatherPrimitiveBatchs(const std::vector<PrimitiveVertex>& vertices, D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveType);
 	void PrimitivesPass();
	void DeferredLightingPass();
	void PostProcessPass();


private:
	bool bInitialize;

	int windowWidth;
	int windowHeight;

	World* world;

	ID3D12Device* d3dDevice;
	ID3D12GraphicsCommandList* d3dCommandList;

	std::unique_ptr<ShaderResourceView> texture2DNullDescriptor = nullptr;
	std::unique_ptr<ShaderResourceView> structuredBufferNullDescriptor = nullptr;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> FontHeap;

	const int GBufferCount = 6;
	std::unique_ptr<RenderTarget2D> GBufferBaseColor;
	std::unique_ptr<RenderTarget2D> GBufferNormal;
	std::unique_ptr<RenderTarget2D> GBufferWorldPos;
	std::unique_ptr<RenderTarget2D> GBufferORM;  //Occlusion Roughness Metallic
	std::unique_ptr<RenderTarget2D> GBufferVelocity;
	std::unique_ptr<RenderTarget2D> GBufferEmissive;

	std::unique_ptr<RenderTarget2D> SSAOBuffer;

	D3D12TextureRef colorTexture = nullptr;
	D3D12TextureRef cacheColorTexture = nullptr;
	D3D12TextureRef prevColorTexture = nullptr;
	std::unique_ptr<RenderTarget2D> backDepth = nullptr;

	UINT frameCount = 0;

private:
	// Mesh
	std::unordered_map<std::string,MeshProxy> meshProxyMap;

	// InputLayout
	InputLayoutManager inputLayoutManager;

	// PassCB
	ConstantBufferRef shadowPassCBRef = nullptr;
	ConstantBufferRef basePassCBRef = nullptr;
	ConstantBufferRef deferredLightPassCBRef;

	// Global shader
	std::unique_ptr<Shader> deferredLightingShader = nullptr;
	std::unique_ptr<Shader> primitiveShader = nullptr;
	std::unique_ptr<Shader> postProcessShader = nullptr;

	// PSO
	std::unique_ptr<GraphicsPSOManager> graphicsPSOManager;
	GraphicsPSODescriptor deferredLightingPSODescriptor;
	GraphicsPSODescriptor postProcessPSODescriptor;

	// MeshBatch and MeshCommand
	std::vector<MeshBatch> meshBatchs;
	std::unordered_map<GraphicsPSODescriptor, MeshCommandList> baseMeshCommandMap;
	const int maxRenderMeshCount = 100;

	// PrimitiveBatchs
	std::unordered_map<GraphicsPSODescriptor, PrimitiveBatch> psoPrimitiveBatchMap;

	// SpriteFont
	std::unique_ptr<SpriteFont> spriteFont = nullptr;
	const std::string FONT_TEXTURE_NAME = "FontTex";

	// Light
	StructuredBufferRef lightShaderParametersBuffer = nullptr;
	ConstantBufferRef lightCommonDataBuffer = nullptr;
	UINT lightCount = 0;

	// Culling
	bool bEnableFrustumCulling = false;

	// D3D12RHI
	D3D12RHI* d3d12RHI = nullptr;

	// Render settings
	TRenderSettings renderSettings;
};

