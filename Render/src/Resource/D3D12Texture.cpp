#include "D3D12Texture.h"
#include "D3D12RHI.h"


D3D12TextureRef D3D12RHI::CreateTexture(const TextureInfo& textureInfo, uint32_t createFlags, TVector4 RTVClearValue)
{
	D3D12TextureRef textureRef = CreateTextureResource(textureInfo, createFlags, RTVClearValue);
	CreateTextureViews(textureRef, textureInfo, createFlags);
	return textureRef;
}

D3D12TextureRef D3D12RHI::CreateTexture(Microsoft::WRL::ComPtr<ID3D12Resource> D3DResource, TextureInfo& textureInfo, uint32_t createFlags)
{
	D3D12TextureRef textureRef = std::make_shared<D3D12Texture>();

	Resource* NewResource = new Resource(D3DResource, textureInfo.InitState);
	textureRef->resourceLocation.underlyingResource = NewResource;
	textureRef->resourceLocation.SetType(ResourceLocation::EResourceLocationType::StandAlone);

	CreateTextureViews(textureRef, textureInfo, createFlags);

	return textureRef;
}

D3D12TextureRef D3D12RHI::CreateTextureResource(const TextureInfo& textureInfo, uint32_t createFlags, TVector4 RTVClearValue)
{
	D3D12TextureRef textureRef = std::make_shared<D3D12Texture>();

	//Create default resource
	D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;

	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = textureInfo.dimension;
	texDesc.Alignment = 0;
	texDesc.Width = textureInfo.width;
	texDesc.Height = (uint32_t)textureInfo.height;
	texDesc.DepthOrArraySize = (textureInfo.depth > 1) ? (uint16_t)textureInfo.depth : (uint16_t)textureInfo.arraySize;
	texDesc.MipLevels = (uint16_t)textureInfo.mipCount;
	texDesc.Format = textureInfo.format;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	bool bCreateRTV = createFlags & (TexCreate_RTV | TexCreate_CubeRTV);
	bool bCreateDSV = createFlags & (TexCreate_DSV | TexCreate_CubeDSV);
	bool bCreateUAV = createFlags & TexCreate_UAV;

	if (bCreateRTV)
	{
		texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET; // | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}
	else if (bCreateDSV)
	{
		texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	}
	else if (bCreateUAV)
	{
		texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}
	else
	{
		texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	}


	bool bReadOnlyTexture = !(bCreateRTV | bCreateDSV | bCreateUAV);
	if (bReadOnlyTexture)
	{
		auto textureResourceAllocator = GetDevice()->GetTextureResourceAllocator();
		textureResourceAllocator->AllocTextureResource(resourceState, texDesc, textureRef->resourceLocation);

		auto textureResource = textureRef->GetD3DResource();
		assert(textureResource);
	}
	else
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> d3dResource;

		CD3DX12_CLEAR_VALUE clearValue = {};
		CD3DX12_CLEAR_VALUE* clearValuePtr = nullptr;

		// Set clear value for RTV and DSV
		if (bCreateRTV)
		{
			clearValue = CD3DX12_CLEAR_VALUE(texDesc.Format, (float*)&RTVClearValue);
			clearValuePtr = &clearValue;

			textureRef->SetRTVClearValue(RTVClearValue);
		}
		else if (bCreateDSV)
		{
			FLOAT Depth = 1.0f;
			UINT8 Stencil = 0;
			clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D24_UNORM_S8_UINT, Depth, Stencil);
			clearValuePtr = &clearValue;
		}

		auto heapPropertiesDefault = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		GetDevice()->GetD3DDevice()->CreateCommittedResource(
			&heapPropertiesDefault,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			textureInfo.InitState,
			clearValuePtr,
			IID_PPV_ARGS(&d3dResource));

		Resource* newResource = new Resource(d3dResource, textureInfo.InitState);
		textureRef->resourceLocation.underlyingResource = newResource;
		textureRef->resourceLocation.SetType(ResourceLocation::EResourceLocationType::StandAlone);
	}

	return textureRef;
}

void  D3D12RHI::CreateTextureViews(D3D12TextureRef textureRef, const TextureInfo& textureInfo, uint32_t createFlags)
{
	auto textureResource = textureRef->GetD3DResource();

	// Create SRV
	if (createFlags & TexCreate_SRV)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		if (textureInfo.srvFormat == DXGI_FORMAT_UNKNOWN)
		{
			srvDesc.Format = textureInfo.format;
		}
		else
		{
			srvDesc.Format = textureInfo.srvFormat;
		}

		if (textureInfo.textureType == ETextureType::TEXTURE_2D)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = (uint16_t)textureInfo.mipCount;
		}
		else if (textureInfo.textureType == ETextureType::TEXTURE_CUBE)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube.MostDetailedMip = 0;
			srvDesc.TextureCube.MipLevels = (uint16_t)textureInfo.mipCount;
		}
		else
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			srvDesc.Texture3D.MostDetailedMip = 0;
			srvDesc.Texture3D.MipLevels = (uint16_t)textureInfo.mipCount;
		}
		auto srv = std::make_unique<ShaderResourceView>(GetDevice(), srvDesc, textureResource);
		textureRef->AddSRV(srv);
	}

	// Create RTV
	if (createFlags & TexCreate_RTV)
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;

		if (textureInfo.rtvFormat == DXGI_FORMAT_UNKNOWN)
		{
			rtvDesc.Format = textureInfo.format;
		}
		else
		{
			rtvDesc.Format = textureInfo.rtvFormat;
		}
		auto rtv = std::make_unique<RenderTargetView>(GetDevice(), rtvDesc, textureResource);
		textureRef->AddRTV(rtv);
	}
	else if (createFlags & TexCreate_CubeRTV)
	{
		for (size_t i = 0; i < 6; i++)
		{
			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			rtvDesc.Texture2DArray.MipSlice = 0;
			rtvDesc.Texture2DArray.PlaneSlice = 0;
			rtvDesc.Texture2DArray.FirstArraySlice = (UINT)i;
			rtvDesc.Texture2DArray.ArraySize = 1;

			if (textureInfo.rtvFormat == DXGI_FORMAT_UNKNOWN)
			{
				rtvDesc.Format = textureInfo.format;
			}
			else
			{
				rtvDesc.Format = textureInfo.rtvFormat;
			}
			auto rtv = std::make_unique<RenderTargetView>(GetDevice(), rtvDesc, textureResource);
			textureRef->AddRTV(rtv);
		}
	}

	// Create DSV
	if (createFlags & TexCreate_DSV)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;

		if (textureInfo.dsvFormat == DXGI_FORMAT_UNKNOWN)
		{
			dsvDesc.Format = textureInfo.format;
		}
		else
		{
			dsvDesc.Format = textureInfo.dsvFormat;
		}

		auto dsv = std::make_unique<DepthStencilView>(GetDevice(), dsvDesc, textureResource);
		textureRef->AddDSV(dsv);
	}
	else if (createFlags & TexCreate_CubeDSV)
	{
		for (size_t i = 0; i < 6; i++)
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
			dsvDesc.Texture2DArray.MipSlice = 0;
			dsvDesc.Texture2DArray.FirstArraySlice = (UINT)i;
			dsvDesc.Texture2DArray.ArraySize = 1;

			if (textureInfo.dsvFormat == DXGI_FORMAT_UNKNOWN)
			{
				dsvDesc.Format = textureInfo.format;
			}
			else
			{
				dsvDesc.Format = textureInfo.dsvFormat;
			}

			auto dsv = std::make_unique<DepthStencilView>(GetDevice(), dsvDesc, textureResource);
			textureRef->AddDSV(dsv);
		}
	}

	// Create UAV
	if (createFlags & TexCreate_UAV)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;

		if (textureInfo.uavFormat == DXGI_FORMAT_UNKNOWN)
		{
			uavDesc.Format = textureInfo.format;
		}
		else
		{
			uavDesc.Format = textureInfo.uavFormat;
		}

		auto uav = std::make_unique<UnorderedAccessView>(GetDevice(), uavDesc, textureResource);
		textureRef->AddUAV(uav);
	}
}

void D3D12RHI::UploadTextureData(D3D12TextureRef texture, const std::vector<D3D12_SUBRESOURCE_DATA>& InitData)
{
	auto textureResource = texture->GetResource();
	D3D12_RESOURCE_DESC texDesc = textureResource->D3DResource->GetDesc();

	//GetCopyableFootprints
	const UINT numSubresources = (UINT)InitData.size();
	std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(numSubresources);
	std::vector<uint32_t> numRows(numSubresources);
	std::vector<uint64_t> rowSizesInBytes(numSubresources);

	uint64_t requiredSize = 0;
	device->GetD3DDevice()->GetCopyableFootprints(&texDesc, 0, numSubresources, 0, &layouts[0], &numRows[0], &rowSizesInBytes[0], &requiredSize);

	//Create upload resource
	ResourceLocation uploadResourceLocation;
	auto uploadBufferAllocator = GetDevice()->GetUploadBufferAllocator();
	void* mappedData = uploadBufferAllocator->AllocUploadResource((uint32_t)requiredSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT, uploadResourceLocation);
	ID3D12Resource* uploadBuffer = uploadResourceLocation.underlyingResource->D3DResource.Get();

	//Copy contents to upload resource
	for (uint32_t i = 0; i < numSubresources; ++i)
	{
		if (rowSizesInBytes[i] > SIZE_T(-1))
		{
			assert(0);
		}
		D3D12_MEMCPY_DEST destData = { (BYTE*)mappedData + layouts[i].Offset, layouts[i].Footprint.RowPitch, SIZE_T(layouts[i].Footprint.RowPitch) * SIZE_T(numRows[i]) };
		MemcpySubresource(&destData, &(InitData[i]), static_cast<SIZE_T>(rowSizesInBytes[i]), numRows[i], layouts[i].Footprint.Depth);
	}

	//Copy data from upload resource to default resource
	TransitionResource(textureResource, D3D12_RESOURCE_STATE_COPY_DEST);

	for (UINT i = 0; i < numSubresources; ++i)
	{
		layouts[i].Offset += uploadResourceLocation.offsetFromBaseOfResource;

		CD3DX12_TEXTURE_COPY_LOCATION Src;
		Src.pResource = uploadBuffer;
		Src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		Src.PlacedFootprint = layouts[i];

		CD3DX12_TEXTURE_COPY_LOCATION Dst;
		Dst.pResource = textureResource->D3DResource.Get();
		Dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		Dst.SubresourceIndex = i;

		CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
	}

	TransitionResource(textureResource, D3D12_RESOURCE_STATE_COMMON);
}