#pragma once

#include "Device.h"
#include "Viewport.h"
#include "D3D12Texture.h"
#include "Buffer.h"
#include "../Texture/TextureInfo.h"
#include "../Math/Math.h"
#include <memory>

class Device;
class Viewport;

class D3D12RHI
{
public:
	D3D12RHI();
	~D3D12RHI();

	void Initialize(HWND windowHandle, int windowWidth, int windowHeight);
	void Destroy();

public:
	Device* GetDevice() { return device.get(); }
	Viewport* GetViewport() { return viewport.get(); }
	const ViewportInfo& GetViewportInfo();
	IDXGIFactory4* GetDxgiFactory();

public:
	//---------------------------RHI CommandList-----------------------------
	void FlushCommandQueue();
	void ExecuteCommandLists();
	void ResetCommandList();
	void ResetCommandAllocator();
	void Present();
	void ResizeViewport(int newWidth, int newHeight);
	void TransitionResource(Resource* resource, D3D12_RESOURCE_STATES stateAfter);
	void CopyResource(Resource* dstResource, Resource* srcResource);
	void CopyBufferRegion(Resource* dstResource, UINT64 dstOffset, Resource* srcResource, UINT64 srcOffset, UINT64 size);
	void CopyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION* dst, UINT dstX, UINT dstY, UINT dstZ, const D3D12_TEXTURE_COPY_LOCATION* src, const D3D12_BOX* srcBox);

	// Buffer.cpp
	ConstantBufferRef CreateConstantBuffer(const void* contents, uint32_t size);
	StructuredBufferRef CreateStructuredBuffer(const void* contents, uint32_t elementSize, uint32_t elementCount);
	RWStructuredBufferRef CreateRWStructuredBuffer(uint32_t elementSize, uint32_t elementCount);
	VertexBufferRef CreateVertexBuffer(const void* contents, uint32_t size);
	IndexBufferRef CreateIndexBuffer(const void* contents, uint32_t size);
	ReadBackBufferRef CreateReadBackBuffer(uint32_t size);
	void SetVertexBuffer(const VertexBufferRef& vertexBuffer, UINT offset, UINT stride, UINT size);
	void SetIndexBuffer(const IndexBufferRef& indexBuffer, UINT offset, DXGI_FORMAT format, UINT size);

	// D3D12Texture.cpp
	D3D12TextureRef CreateTexture(const TextureInfo& textureInfo, uint32_t createFlags, TVector4 rtvClearValue = TVector4::Zero);
	// Use D3DResource to create texture, texture will manage this D3DResource
	D3D12TextureRef CreateTexture(Microsoft::WRL::ComPtr<ID3D12Resource> D3DResource, TextureInfo& textureInfo, uint32_t createFlags);
	void UploadTextureData(D3D12TextureRef texture, const std::vector<D3D12_SUBRESOURCE_DATA>& InitData);

	void EndFrame();

	//-----------------------------------------------------------------------

private:
	void CreateDefaultBuffer(uint32_t size, uint32_t alignment, D3D12_RESOURCE_FLAGS flags, ResourceLocation& resourceLocation);  // only create default buffer
	void CreateAndInitDefaultBuffer(const void* contents, uint32_t size, uint32_t alignment, ResourceLocation& resourceLocation); // create default buffer and upload to uploadBuffer
	D3D12TextureRef CreateTextureResource(const TextureInfo& textureInfo, uint32_t createFlags, TVector4 rtvClearValue);
	void CreateTextureViews(D3D12TextureRef textureRef, const TextureInfo& textureInfo, uint32_t CreateFlags);

private:
	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter* adapter);
	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);
	UINT GetSupportMSAAQuality(DXGI_FORMAT backBufferFormat);

private:
	std::unique_ptr<Device> device = nullptr;
	std::unique_ptr<Viewport> viewport = nullptr;
	ViewportInfo viewportInfo;
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory = nullptr;
};