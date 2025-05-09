#include "Viewport.h"
#include "D3D12RHI.h"

Viewport::Viewport(D3D12RHI* InD3D12RHI, const ViewportInfo& Info, int width, int height)
	:d3d12RHI(InD3D12RHI), viewportInfo(Info), viewportWidth(width), viewportHeight(height)
{
	Initialize();
}

Viewport::~Viewport()
{

}

void Viewport::Initialize()
{
	CreateSwapChain();
}

void Viewport::CreateSwapChain()
{
	// Release the previous swapchain we will be recreating.
	swapChain.Reset();

	DXGI_SWAP_CHAIN_DESC desc;
	desc.BufferDesc.Width = viewportWidth;
	desc.BufferDesc.Height = viewportHeight;
	desc.BufferDesc.RefreshRate.Numerator = 60;
	desc.BufferDesc.RefreshRate.Denominator = 1;
	desc.BufferDesc.Format = viewportInfo.backBufferFormat;
	desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	desc.SampleDesc.Count = viewportInfo.bEnable4xMsaa ? 4 : 1;
	desc.SampleDesc.Quality = viewportInfo.bEnable4xMsaa ? (viewportInfo.qualityOf4xMsaa - 1) : 0;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = swapChainBufferCount;
	desc.OutputWindow = viewportInfo.windowHandle;
	desc.Windowed = true;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Note: Swap chain uses queue to perform flush.
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue = d3d12RHI->GetDevice()->GetCommandQueue();

	ThrowIfFailed(d3d12RHI->GetDxgiFactory()->CreateSwapChain(commandQueue.Get(), &desc, swapChain.GetAddressOf()));
}

void Viewport::OnResize(int newWidth, int newHeight)
{
	viewportWidth = newWidth;
	viewportHeight = newHeight;

	// Flush before changing any resources.
	d3d12RHI->GetDevice()->GetCommandContext()->FlushCommandQueue();
	d3d12RHI->GetDevice()->GetCommandContext()->ResetCommandList();

	// Release the previous resources
	for (UINT i = 0; i < swapChainBufferCount; i++)
	{
		renderTargetTextures[i].reset();
	}
	depthStencilTexture.reset();

	// Resize the swap chain.
	ThrowIfFailed(swapChain->ResizeBuffers(swapChainBufferCount, viewportWidth, viewportHeight,
		viewportInfo.backBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	currBackBuffer = 0;

	// Create RenderTargetTextures
	for (UINT i = 0; i < swapChainBufferCount; i++)
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> swapChainBuffer = nullptr;
		ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&swapChainBuffer)));

		D3D12_RESOURCE_DESC backBufferDesc = swapChainBuffer->GetDesc();

		TextureInfo textureInfo;
		textureInfo.rtvFormat = backBufferDesc.Format;
		textureInfo.InitState = D3D12_RESOURCE_STATE_PRESENT;
		renderTargetTextures[i] = d3d12RHI->CreateTexture(swapChainBuffer, textureInfo, TexCreate_RTV);
	}

	// Create DepthStencilTexture
	TextureInfo textureInfo;
	textureInfo.textureType = ETextureType::TEXTURE_2D;
	textureInfo.dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureInfo.width = viewportWidth;
	textureInfo.height = viewportHeight;
	textureInfo.depth = 1;
	textureInfo.mipCount = 1;
	textureInfo.arraySize = 1;
	textureInfo.InitState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	textureInfo.format = DXGI_FORMAT_R24G8_TYPELESS;  // Create with a typeless format, support DSV and SRV(for SSAO)
	textureInfo.dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	textureInfo.srvFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

	depthStencilTexture = d3d12RHI->CreateTexture(textureInfo, TexCreate_DSV | TexCreate_SRV);


	// Execute the resize commands.
	d3d12RHI->GetDevice()->GetCommandContext()->ExecuteCommandLists();

	// Wait until resize is complete.
	d3d12RHI->GetDevice()->GetCommandContext()->FlushCommandQueue();
}

void Viewport::GetD3DViewport(D3D12_VIEWPORT& outD3DViewPort, D3D12_RECT& outD3DRect)
{
	outD3DViewPort.TopLeftX = 0;
	outD3DViewPort.TopLeftY = 0;
	outD3DViewPort.Width = static_cast<float>(viewportWidth);
	outD3DViewPort.Height = static_cast<float>(viewportHeight);
	outD3DViewPort.MinDepth = 0.0f;
	outD3DViewPort.MaxDepth = 1.0f;

	outD3DRect = { 0, 0, viewportWidth, viewportHeight };
}

void Viewport::Present()
{
	// swap the back and front buffers
	ThrowIfFailed(swapChain->Present(0, 0));
	currBackBuffer = (currBackBuffer + 1) % swapChainBufferCount;
}

Resource* Viewport::GetCurrentBackBuffer() const
{
	return renderTargetTextures[currBackBuffer]->GetResource();
}

RenderTargetView* Viewport::GetCurrentBackBufferView() const
{
	return renderTargetTextures[currBackBuffer]->GetRTV();
}

float* Viewport::GetCurrentBackBufferClearColor() const
{
	return renderTargetTextures[currBackBuffer]->GetRTVClearValuePtr();
}

DepthStencilView* Viewport::GetDepthStencilView() const
{
	return depthStencilTexture->GetDSV();
}

ShaderResourceView* Viewport::GetDepthShaderResourceView() const
{
	return depthStencilTexture->GetSRV();
}

ViewportInfo Viewport::GetViewportInfo() const
{
	return viewportInfo;
}