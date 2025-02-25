#include "Metalib.h"
#include "stdafx.h"
#include "RenderApp.h"
#include "../DXRunTime/FrameResource.h"
#include "../Component/Camera.h"
#include "../Resource/DefaultBuffer.h"
#include "../Shader/RasterShader.h"
#include "../Shader/PSOManager.h"
#include "../Resource/DescriptorHeap.h"


RenderApp::RenderApp(uint32_t width, uint32_t height, std::wstring name)
	:DXApplication(width, height, name),
	m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
	m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height))
{}

void RenderApp::OnInit()
{
	LoadPipeline();
	LoadAssets();
}

// 加载管线资源
void RenderApp::LoadPipeline()
{
	//创建设备
	device = std::make_unique<Device>();

	// 创建命令队列
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(device->DxDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

	// 创建交换链
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(device->DxgiFactory()->CreateSwapChainForHwnd(m_commandQueue.Get(),
		Win32Application::GetHwnd(), &swapChainDesc, nullptr, nullptr, &swapChain));
		
	
	// 禁用全屏模式
	ThrowIfFailed(device->DxgiFactory()->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain.As(&m_swapChain));
	m_backBufferIndex = 0;

	// 创建描述符堆
	{
		// RTV 描述符堆
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->DxDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
		m_rtvDescriptorSize = device->DxDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		// DSV 描述符堆
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = FrameCount;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->DxDevice()->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));
		m_dsvDescriptorSize = device->DxDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		// SRV 描述符堆
		m_srvHeap = new DescriptorHeap(
			device.get(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,  // 资源类型
			10,                                      // 先分配10个描述符
			true);                                   // bShaderVisible = true
	}

	// 创建帧资源
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

		// 为每帧创建一个RTV
		for (uint32_t n = 0; n < FrameCount; n++)
		{
			m_renderTargets[n] = std::unique_ptr<Texture>(new Texture(device.get(), m_swapChain.Get(), n));
			m_depthTargets[n] = std::unique_ptr<Texture>(
				new Texture(
					device.get(),
					m_scissorRect.right,
					m_scissorRect.bottom,
					DXGI_FORMAT_D32_FLOAT,
					TextureDimension::Tex2D,
					1,
					1,
					Texture::TextureUsage::DepthStencil,
					D3D12_RESOURCE_STATE_DEPTH_READ));

			device->DxDevice()->CreateRenderTargetView(m_renderTargets[n]->GetResource(), nullptr, rtvHandle);
			device->DxDevice()->CreateDepthStencilView(m_depthTargets[n]->GetResource(), nullptr, dsvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);   // 若使用MRT进行渲染，则需要计算偏移，以获取描述符堆中每个rtv的描述符位置
			dsvHandle.Offset(1, m_dsvDescriptorSize);   // 若需要渲染多个dsv使用，则需要计算偏移
		}
	}

	// 初始化帧资源
	for (auto&& i : frameResources)
	{
		i = std::unique_ptr<FrameResource>(new FrameResource(device.get()));
	}
}

static Vertex vertexSample;
// 顶点坐标
static XMFLOAT3 vertices[] = {
	// 前面
	XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(-0.5f, +0.5f, -0.5f),
	XMFLOAT3(+0.5f, +0.5f, -0.5f), XMFLOAT3(+0.5f, -0.5f, -0.5f),

	// 后面
	XMFLOAT3(-0.5f, -0.5f, +0.5f), XMFLOAT3(+0.5f, -0.5f, +0.5f),
	XMFLOAT3(+0.5f, +0.5f, +0.5f), XMFLOAT3(-0.5f, +0.5f, +0.5f),

	// 上面
	XMFLOAT3(-0.5f, +0.5f, -0.5f), XMFLOAT3(-0.5f, +0.5f, +0.5f),
	XMFLOAT3(+0.5f, +0.5f, +0.5f), XMFLOAT3(+0.5f, +0.5f, -0.5f),

	// 下面
	XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(+0.5f, -0.5f, -0.5f),
	XMFLOAT3(+0.5f, -0.5f, +0.5f), XMFLOAT3(-0.5f, -0.5f, +0.5f),

	// 左面
	XMFLOAT3(-0.5f, -0.5f, +0.5f), XMFLOAT3(-0.5f, +0.5f, +0.5f),
	XMFLOAT3(-0.5f, +0.5f, -0.5f), XMFLOAT3(-0.5f, -0.5f, -0.5f),

	// 右面
	XMFLOAT3(+0.5f, -0.5f, -0.5f), XMFLOAT3(+0.5f, +0.5f, -0.5f),
	XMFLOAT3(+0.5f, +0.5f, +0.5f), XMFLOAT3(+0.5f, -0.5f, +0.5f)
};

// 纹理坐标（UV）
static XMFLOAT2 texcoords[] = {
	// 前面
	XMFLOAT2(0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f),
	XMFLOAT2(1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f),

	// 后面
	XMFLOAT2(1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f),
	XMFLOAT2(0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f),

	// 上面
	XMFLOAT2(0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f),
	XMFLOAT2(1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f),

	// 下面
	XMFLOAT2(1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f),
	XMFLOAT2(0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f),

	// 左面
	XMFLOAT2(0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f),
	XMFLOAT2(1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f),

	// 右面
	XMFLOAT2(0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f),
	XMFLOAT2(1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f)
};

// 索引
static uint indices[] = {
	// 前面
	0, 1, 2, 0, 2, 3,
	// 后面
	4, 5, 6, 4, 6, 7,
	// 上面
	8, 9, 10, 8, 10, 11,
	// 下面
	12, 13, 14, 12, 14, 15,
	// 左面
	16, 17, 18, 16, 18, 19,
	// 右面
	20, 21, 22, 20, 22, 23
};

static UploadBuffer* BuildCubeVertex(Device* device)
{
	constexpr size_t VERTEX_COUNT = array_count(vertices);
	std::vector<vbyte> vertexData(vertexSample.structSize * VERTEX_COUNT);
	vbyte* vertexDataPtr = vertexData.data();
	for (size_t i = 0; i < VERTEX_COUNT; ++i) {
		XMFLOAT3 vert = vertices[i];
		vertexSample.position.Set(vertexDataPtr) = vert;
		XMFLOAT2 texcoord = texcoords[i];
		vertexSample.texCoord.Set(vertexDataPtr) = texcoord;
		XMFLOAT4 color(
			vert.x + 0.5f,
			vert.y + 0.5f,
			vert.z + 0.5f,
			1);
		vertexSample.color.Set(vertexDataPtr) = color;
		vertexDataPtr += vertexSample.structSize;
	}
	UploadBuffer* vertBuffer = new UploadBuffer(device, vertexData.size());
	vertBuffer->CopyData(0, vertexData);
	return vertBuffer;
}

static UploadBuffer* BuildCubeIndices(Device* device)
{
	UploadBuffer* indBuffer = new UploadBuffer(
		device,
		array_count(indices) * sizeof(uint));
	indBuffer->CopyData(
		0,
		{ reinterpret_cast<vbyte const*>(indices), array_count(indices) * sizeof(uint) });
	return indBuffer;
}

// 加载资源
void RenderApp::LoadAssets()
{
	ComPtr<ID3D12CommandAllocator> cmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ThrowIfFailed(
		device
		->DxDevice()
		->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(cmdAllocator.GetAddressOf())));
	ThrowIfFailed(device->DxDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
	ThrowIfFailed(commandList->Close());
	ThrowIfFailed(cmdAllocator->Reset());
	ThrowIfFailed(commandList->Reset(cmdAllocator.Get(), nullptr));
	std::unique_ptr<UploadBuffer> vertexUpload(BuildCubeVertex(device.get()));
	std::unique_ptr<UploadBuffer> indexUpload(BuildCubeIndices(device.get()));
	std::vector<rtti::Struct const*> structs;
	structs.emplace_back(&vertexSample);
	triangleMesh = std::make_unique<Mesh>(
		device.get(),
		structs,
		array_count(vertices),
		array_count(indices));
	commandList->CopyBufferRegion(
			triangleMesh->VertexBuffers()[0].GetResource(),
			0,
			vertexUpload->GetResource(),
			0,
			vertexUpload->GetByteSize());
	commandList->CopyBufferRegion(
		triangleMesh->IndexBuffer().GetResource(),
		0,
		indexUpload->GetResource(),
		0,
		indexUpload->GetByteSize());


	// 创建Texture资源并放到SRV中
	// 加载纹理
	DXGI_FORMAT textureFormat;
	D3D12_RESOURCE_DESC textureDesc;
	m_textureDataSize = LoadImageDataFromFile(&m_textureData, textureDesc, L"Images\\test.jpg", m_textureBytesPerRow, m_textureWidth, m_textureHeight, textureFormat);

	// 创建 GPU DefaulBuffer 中的纹理资源
	 m_texture = std::unique_ptr<Texture>(new Texture(
	 	device.get(),
		m_textureWidth, m_textureHeight,
	 	textureFormat,
	 	TextureDimension::Tex2D,
	 	1, 1,
	 	Texture::TextureUsage::None,
	 	D3D12_RESOURCE_STATE_COPY_DEST));
	 // 创建上传堆
	 CD3DX12_HEAP_PROPERTIES heapPropertiesUpload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	 m_textureBuffer = m_texture->GetResource();
	 const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_textureBuffer.Get(), 0, 1);
	 CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
	 ThrowIfFailed(device->DxDevice()->CreateCommittedResource(
		&heapPropertiesUpload,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_textureBufferUpload)));

	// 上传纹理资源
	// 从上传堆拷贝到默认堆
	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = &m_textureData[0];
	textureData.RowPitch = m_textureBytesPerRow;
	textureData.SlicePitch = m_textureBytesPerRow * textureDesc.Height;
	UpdateSubresources(commandList.Get(), m_textureBuffer.Get(), m_textureBufferUpload.Get(), 0, 0, 1, &textureData);

	// 设置资源屏障（由于这里只需要转换一次状态，因此没有使用stateTracker）
	CD3DX12_RESOURCE_BARRIER resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_textureBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList->ResourceBarrier(1, &resBarrier);

	auto srvDesc = m_texture->GetColorSrvDesc(0);
	// 分配 SRV 描述符索引
	m_textureSrvIndex = m_srvHeap->AllocateIndex();
	// 创建 SRV
	m_srvHeap->CreateSRV(
		m_textureBuffer.Get(),
		srvDesc,
		m_textureSrvIndex);

	// 创建相机
	mainCamera = std::make_unique<Camera>();
	mainCamera->Right = Math::Vector3(0.6877694, -1.622736E-05, 0.7259292);
	mainCamera->Up = Math::Vector3(-0.3181089, 0.8988663, 0.301407);
	mainCamera->Forward = Math::Vector3(-0.6525182, -0.438223, 0.6182076);
	mainCamera->Position = Math::Vector3(2.232773, 1.501817, -1.883978);
	mainCamera->SetAspect(static_cast<float>(m_scissorRect.right) / static_cast<float>(m_scissorRect.bottom));
	mainCamera->UpdateViewMatrix();
	mainCamera->UpdateProjectionMatrix();

	ThrowIfFailed(commandList->Close());

	// Execute 命令
	ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
	m_commandQueue->ExecuteCommandLists(array_count(ppCommandLists), ppCommandLists);

	// 创建根签名
	{
		std::vector<std::pair<std::string, Shader::Property>> properties;
		properties.emplace_back(
			"_Global",
			Shader::Property{ 
				.type = ShaderVariableType::ConstantBuffer,
				.spaceIndex = 0,
				.registerIndex = 0,
				.arrSize = 0 });
		properties.emplace_back(
			"_Texture",
			Shader::Property{
				.type = ShaderVariableType::SRVDescriptorHeap,
				.spaceIndex = 0,
				.registerIndex = 0,
				.arrSize = 1 });
		textureShader = std::unique_ptr<RasterShader>(new RasterShader(properties,
			device.get()));
		psoManager = std::unique_ptr<PSOManager>(new PSOManager(device.get()));

		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		uint32_t compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		uint32_t compileFlags = 0;
#endif
		ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders/shader.hlsl").c_str(), nullptr, nullptr, "VS", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
		ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders/shader.hlsl").c_str(), nullptr, nullptr, "PS", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

		textureShader->vsShader = std::move(vertexShader);
		textureShader->psShader = std::move(pixelShader);
		textureShader->rasterizeState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		textureShader->blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		auto&& depthStencilState = textureShader->depthStencilState;
		depthStencilState.DepthEnable = true;
		depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilState.StencilEnable = false;
	}


	// 同步操作，等待GPU上传完成
	{
		ThrowIfFailed(device->DxDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceValue = 1;
		m_commandQueue->Signal(m_fence.Get(), m_fenceValue);
		if (m_fence->GetCompletedValue() < m_fenceValue) {
			LPCWSTR falseValue = 0;
			HANDLE eventHandle = CreateEventEx(nullptr, falseValue, false, EVENT_ALL_ACCESS);
			ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValue, eventHandle));
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}
}

void RenderApp::OnUpdate()
{
	
}

void RenderApp::OnRender()
{
	// 遍历所有需要渲染的帧
	auto curFrame = m_backBufferIndex;
	auto nextFrame = (curFrame + 1) % FrameCount;
	auto lastFrame = (nextFrame + 1) % FrameCount;

	// Execute 和 Present
	frameResources[curFrame]->Execute(
		m_commandQueue.Get(),
		m_fence.Get(),
		m_fenceValue);
	ThrowIfFailed(m_swapChain->Present(0, 0));
	m_backBufferIndex = (m_backBufferIndex + 1) % FrameCount;
	// Signal
	frameResources[curFrame]->Signal(m_commandQueue.Get(), m_fence.Get());
	
	// Populate
	PopulateCommandList(*frameResources[nextFrame], nextFrame);

	frameResources[lastFrame]->Sync(m_fence.Get());
}

void RenderApp::OnDestroy()
{
	for (auto&& i:frameResources)
	{
		i->Sync(m_fence.Get());
	}
}

void RenderApp::PopulateCommandList(FrameResource& frameRes, uint frameIndex)
{
	auto cmdListHandle = frameRes.Command();
	auto cmdList = cmdListHandle.CmdList();

	// 绑定描述符堆到命令列表
	ID3D12DescriptorHeap* heaps[] = { m_srvHeap->GetHeap() }; // 假设 m_srvHeap 有 Heap() 方法返回 ID3D12DescriptorHeap*
	cmdList->SetDescriptorHeaps(1, heaps);

	// 绑定渲染目标状态
	stateTracker.RecordState(m_renderTargets[frameIndex].get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	stateTracker.RecordState(m_depthTargets[frameIndex].get(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
	stateTracker.UpdateState(cmdList);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, m_rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, m_dsvDescriptorSize);
	frameRes.SetRenderTarget(
		m_renderTargets[frameIndex].get(),
		&rtvHandle,
		&dsvHandle);
	frameRes.ClearRTV(rtvHandle);
	frameRes.ClearDSV(dsvHandle);
	// Record commands.
	DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT depthFormat = DXGI_FORMAT_D32_FLOAT;

	Math::Matrix4 viewProjMatrix = mainCamera->Proj * mainCamera->View;
	auto constBuffer = frameRes.AllocateConstBuffer({ reinterpret_cast<uint8_t const*>(&viewProjMatrix), sizeof(viewProjMatrix) });
	bindProperties.clear();
	bindProperties.emplace_back("_Global", constBuffer);

	// 绑定 SRV（纹理）
	DescriptorHeapView srvView(m_srvHeap, m_textureSrvIndex);  // 创建 DescriptorHeapView
	bindProperties.emplace_back("_Texture", srvView);          // 提交到 bindProperties

	frameRes.DrawMesh(
		textureShader.get(),
		psoManager.get(),
		triangleMesh.get(),
		colorFormat,
		depthFormat,
		bindProperties);
	

	stateTracker.RestoreState(cmdList);

}

RenderApp::~RenderApp() {}