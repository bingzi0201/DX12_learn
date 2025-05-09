#include "Buffer.h"
#include "D3D12RHI.h"

ConstantBufferRef D3D12RHI::CreateConstantBuffer(const void* contents, uint32_t size)
{
	ConstantBufferRef constantBufferRef = std::make_shared<ConstantBuffer>();

	auto uploadBufferAllocator = GetDevice()->GetUploadBufferAllocator();
	void* mappedData = uploadBufferAllocator->AllocUploadResource(size, UPLOAD_RESOURCE_ALIGNMENT, constantBufferRef->resourceLocation);

	memcpy(mappedData, contents, size);

	return constantBufferRef;
}

StructuredBufferRef D3D12RHI::CreateStructuredBuffer(const void* contents, uint32_t elementSize, uint32_t elementCount)
{
	assert(contents != nullptr && elementSize > 0 && elementCount > 0);

	StructuredBufferRef structuredBufferRef = std::make_shared<StructuredBuffer>();

	auto uploadBufferAllocator = GetDevice()->GetUploadBufferAllocator();
	uint32_t dataSize = elementSize * elementCount;
	// Align to ElementSize
	void* mappedData = uploadBufferAllocator->AllocUploadResource(dataSize, elementSize, structuredBufferRef->resourceLocation);

	memcpy(mappedData, contents, dataSize);


	// Create SRV
	{
		ResourceLocation& resourceLocation = structuredBufferRef->resourceLocation;
		const uint64_t offset = resourceLocation.offsetFromBaseOfResource;
		ID3D12Resource* bufferResource = resourceLocation.underlyingResource->D3DResource.Get();

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		srvDesc.Buffer.StructureByteStride = elementSize;
		srvDesc.Buffer.NumElements = elementCount;
		srvDesc.Buffer.FirstElement = offset / elementSize;
		auto srv = std::make_unique<ShaderResourceView>(GetDevice(), srvDesc, bufferResource);
		structuredBufferRef->SetSRV(srv);
	}


	return structuredBufferRef;
}

RWStructuredBufferRef D3D12RHI::CreateRWStructuredBuffer(uint32_t elementSize, uint32_t elementCount)
{
	RWStructuredBufferRef rwStructuredBufferRef = std::make_shared<RWStructuredBuffer>();

	uint32_t dataSize = elementSize * elementCount;
	// Align to ElementSize
	CreateDefaultBuffer(dataSize, elementSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, rwStructuredBufferRef->resourceLocation);

	ResourceLocation& resourceLocation = rwStructuredBufferRef->resourceLocation;
	const uint64_t Offset = resourceLocation.offsetFromBaseOfResource;
	ID3D12Resource* BufferResource = resourceLocation.underlyingResource->D3DResource.Get();

	// Create SRV
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		srvDesc.Buffer.StructureByteStride = elementSize;
		srvDesc.Buffer.NumElements = elementCount;
		srvDesc.Buffer.FirstElement = Offset / elementSize;

		auto srv = std::make_unique<ShaderResourceView>(GetDevice(), srvDesc, BufferResource);
		rwStructuredBufferRef->SetSRV(srv);
	}

	// Create UAV
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
		UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
		UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
		UAVDesc.Buffer.StructureByteStride = elementSize;
		UAVDesc.Buffer.NumElements = elementCount;
		UAVDesc.Buffer.FirstElement = Offset / elementSize;
		UAVDesc.Buffer.CounterOffsetInBytes = 0;

		auto rwsrv = std::make_unique<UnorderedAccessView>(GetDevice(), UAVDesc, BufferResource);
		rwStructuredBufferRef->SetUAV(rwsrv);
	}

	return rwStructuredBufferRef;
}

VertexBufferRef D3D12RHI::CreateVertexBuffer(const void* Contents, uint32_t Size)
{
	VertexBufferRef vertexBufferRef = std::make_shared<VertexBuffer>();

	CreateAndInitDefaultBuffer(Contents, Size, DEFAULT_RESOURCE_ALIGNMENT, vertexBufferRef->resourceLocation);

	return vertexBufferRef;
}

IndexBufferRef D3D12RHI::CreateIndexBuffer(const void* Contents, uint32_t Size)
{
	IndexBufferRef indexBufferRef = std::make_shared<IndexBuffer>();

	CreateAndInitDefaultBuffer(Contents, Size, DEFAULT_RESOURCE_ALIGNMENT, indexBufferRef->resourceLocation);

	return indexBufferRef;
}

ReadBackBufferRef D3D12RHI::CreateReadBackBuffer(uint32_t size)
{
	ReadBackBufferRef readBackBufferRef = std::make_shared<ReadBackBuffer>();

	Microsoft::WRL::ComPtr<ID3D12Resource> D3DResource;

	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
	auto resourceDescBuffer = CD3DX12_RESOURCE_DESC::Buffer(size);
	HRESULT Hr = device->GetD3DDevice()->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDescBuffer,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&D3DResource));

	ThrowIfFailed(Hr);

	Resource* newResource = new Resource(D3DResource, D3D12_RESOURCE_STATE_COPY_DEST);
	readBackBufferRef->resourceLocation.underlyingResource = newResource;
	readBackBufferRef->resourceLocation.SetType(ResourceLocation::EResourceLocationType::StandAlone);

	return readBackBufferRef;
}

void D3D12RHI::CreateDefaultBuffer(uint32_t size, uint32_t alignment, D3D12_RESOURCE_FLAGS flags, ResourceLocation& resourceLocation)
{
	//Create default resource
	D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(size, flags);
	auto DefaultBufferAllocator = GetDevice()->GetDefaultBufferAllocator();
	DefaultBufferAllocator->AllocDefaultResource(resourceDesc, alignment, resourceLocation);
}

void D3D12RHI::CreateAndInitDefaultBuffer(const void* contents, uint32_t size, uint32_t alignment, ResourceLocation& resourceLocation)
{
	//Create default resource
	CreateDefaultBuffer(size, alignment, D3D12_RESOURCE_FLAG_NONE, resourceLocation);

	//Create upload resource 
	ResourceLocation uploadResourceLocation;
	auto uploadBufferAllocator = GetDevice()->GetUploadBufferAllocator();
	void* mappedData = uploadBufferAllocator->AllocUploadResource(size, UPLOAD_RESOURCE_ALIGNMENT, uploadResourceLocation);

	//Copy contents to upload resource
	memcpy(mappedData, contents, size);

	//Copy data from upload resource to default resource
	Resource* defaultBuffer = resourceLocation.underlyingResource;
	Resource* uploadBuffer = uploadResourceLocation.underlyingResource;

	TransitionResource(defaultBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
	CopyBufferRegion(defaultBuffer, resourceLocation.offsetFromBaseOfResource, uploadBuffer, uploadResourceLocation.offsetFromBaseOfResource, size);
}


