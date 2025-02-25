#pragma once
#include "DXApplication.h"
#include "../Resource/Mesh.h"
#include "../DXRunTime/Device.h"
#include "../Resource/UpLoadBuffer.h"
#include "../Resource/Texture.h"
#include "../DXRunTime/ResourceStateTracker.h"
#include "../DXRunTime/BindProperty.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;
class FrameResource;
class Camera;
class PSOManager;
class RasterShader;
class CommandListHandle;
class DefaultBuffer;

struct Vertex : public rtti::Struct
{
	rtti::Var<XMFLOAT3> position = "POSITION";
	rtti::Var<XMFLOAT4> color = "COLOR";
	rtti::Var<XMFLOAT2> texCoord = "TEXCOORD";
};

class RenderApp : public DXApplication
{
public:
	RenderApp(uint32_t width, uint32_t height, std::wstring name);
	RenderApp(RenderApp const&) = delete;
	RenderApp(RenderApp&&) = delete;
	void OnInit() override;
	void OnUpdate() override;
	void OnRender() override;
	void OnDestroy() override;
	~RenderApp();

private:
	static const uint32_t FrameCount = 3;
	std::unique_ptr<Device> device;
	std::unique_ptr<Camera> mainCamera;
	// 管线资源
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;
	ComPtr<IDXGISwapChain3> m_swapChain;
	std::unique_ptr<Texture> m_renderTargets[FrameCount];
	std::unique_ptr<Texture> m_depthTargets[FrameCount];
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	std::unique_ptr<PSOManager> psoManager;
	std::unique_ptr<RasterShader> textureShader;
	uint32_t m_rtvDescriptorSize;
	uint32_t m_dsvDescriptorSize;
	std::unique_ptr<FrameResource> frameResources[FrameCount];
	ResourceStateTracker stateTracker;
	// 资源
	std::unique_ptr<Mesh> triangleMesh;
	// 同步对象
	uint32_t m_backBufferIndex;
	ComPtr<ID3D12Fence> m_fence;
	uint64_t m_fenceValue;
	std::vector<BindProperty> bindProperties;
	// 纹理资源
	DescriptorHeap* m_srvHeap;
	std::unique_ptr<Texture> m_texture;
	uint m_textureWidth = 0, m_textureHeight = 0;
	uint m_textureSrvIndex = 0;
	BYTE* m_textureData = nullptr;
	int m_textureDataSize = 0, m_textureBytesPerRow = 0;

	ComPtr<ID3D12Resource> m_textureBuffer;
	ComPtr<ID3D12Resource> m_textureBufferUpload;

	void LoadPipeline();
	void LoadAssets();
	void PopulateCommandList(FrameResource& frameRes, uint frameIndex);
};