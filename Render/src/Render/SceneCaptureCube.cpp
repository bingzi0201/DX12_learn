#include "SceneCaptureCube.h"

SceneCaptureCube::SceneCaptureCube(bool renderDepth, UINT Size, DXGI_FORMAT format, D3D12RHI* inD3D12RHI)
	:d3d12RHI(inD3D12RHI), cubeMapSize(Size)
{
	d3d12RHI = inD3D12RHI;
	RTCube = std::make_unique<RenderTargetCube>(d3d12RHI, renderDepth, cubeMapSize, format);
	assert(RTCube && "RenderTargetCube create failed!");
	SetViewportAndScissorRect(cubeMapSize);
}

void SceneCaptureCube::SetViewportAndScissorRect(UINT cubeMapSize)
{
	viewport = { 0.0f, 0.0f, (float)cubeMapSize, (float)cubeMapSize, 0.0f, 1.0f };
	scissorRect = { 0, 0, (int)(viewport.Width), (int)(viewport.Height) };
}

void SceneCaptureCube::CreatePerspectiveViews(const TVector3& eye, float nearPlane, float farPlane)
{
	// Look along each coordinate axis. 
	TVector3 Targets[6] =
	{
		eye + TVector3(1.0f,  0.0f,  0.0f), // +X 
		eye + TVector3(-1.0f, 0.0f,  0.0f), // -X 
		eye + TVector3(0.0f,  1.0f,  0.0f), // +Y 
		eye + TVector3(0.0f,  -1.0f, 0.0f), // -Y 
		eye + TVector3(0.0f,  0.0f,  1.0f), // +Z 
		eye + TVector3(0.0f,  0.0f, -1.0f)  // -Z 
	};

	TVector3 Ups[6] =
	{
		{0.0f, 1.0f, 0.0f},  // +X 
		{0.0f, 1.0f, 0.0f},  // -X 
		{0.0f, 0.0f, -1.0f}, // +Y 
		{0.0f, 0.0f, +1.0f}, // -Y 
		{0.0f, 1.0f, 0.0f},	 // +Z 
		{0.0f, 1.0f, 0.0f}	 // -Z 
	};

	for (int i = 0; i < 6; ++i)
	{
		sceneViews[i].eyePos = eye;
		sceneViews[i].view = TMatrix::CreateLookAt(eye, Targets[i], Ups[i]);

		float fov = 0.5f * TMath::Pi;
		float aspectRatio = 1.0f; //Square
		sceneViews[i].proj = TMatrix::CreatePerspectiveFieldOfView(fov, aspectRatio, nearPlane, farPlane);

		sceneViews[i].Near = nearPlane;
		sceneViews[i].Far = farPlane;
	}
}