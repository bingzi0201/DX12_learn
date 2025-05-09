#include "SceneCapture2D.h"

SceneCapture2D::SceneCapture2D(bool renderDepth, UINT inWidth, UINT inHeight, DXGI_FORMAT format, D3D12RHI* inD3D12RHI)
	:d3d12RHI(inD3D12RHI), width(inWidth), height(inHeight)
{
	RT = std::make_unique<RenderTarget2D>(d3d12RHI, renderDepth, width, height, format);
	SetViewportAndScissorRect();
}

void SceneCapture2D::SetViewportAndScissorRect()
{
	viewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	scissorRect = { 0, 0, (int)(viewport.Width), (int)(viewport.Height) };
}

void SceneCapture2D::CreatePerspectiveView(const TVector3& eye, const TVector3& target, const TVector3& up, float fov, float aspectRatio, float nearPlane, float farPlane)
{
	sceneView.eyePos = eye;
	sceneView.view = TMatrix::CreateLookAt(eye, target, up);
	sceneView.proj = TMatrix::CreatePerspectiveFieldOfView(fov, aspectRatio, nearPlane, farPlane);

	sceneView.Near = nearPlane;
	sceneView.Far = farPlane;
}

void SceneCapture2D::CreateOrthographicView(const TVector3& eye, const TVector3& target, const TVector3& up, float left, float right, float bottom, float top, float nearPlane, float farPlane)
{
	sceneView.eyePos = eye;
	sceneView.view = TMatrix::CreateLookAt(eye, target, up);
	sceneView.proj = TMatrix::CreateOrthographicOffCenter(left, right, bottom, top, nearPlane, farPlane);

	sceneView.Near = nearPlane;
	sceneView.Far = farPlane;
}