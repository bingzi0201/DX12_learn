#pragma once

#include "RenderTarget.h"
#include "SceneView.h"

class SceneCapture2D
{
public:
	SceneCapture2D(bool tenderDepth, UINT inWidth, UINT inHeight, DXGI_FORMAT gormat, D3D12RHI* inD3D12RHI);
	RenderTarget2D* GetRT() { return RT.get(); }
	const SceneView& GetSceneView() const
	{
		return sceneView;
	}
	D3D12_VIEWPORT GetViewport() { return viewport; }
	D3D12_RECT GetScissorRect() { return scissorRect; }

	// FOV: in radians
	void CreatePerspectiveView(const TVector3& Eye, const TVector3& Target, const TVector3& Up,
		float Fov, float AspectRatio, float NearPlane, float FarPlane);
	void CreateOrthographicView(const TVector3& Eye, const TVector3& Target, const TVector3& Up,
		float Left, float Right, float Bottom, float Top, float NearPlane, float FarPlane);

	UINT GetWidth() { return width; }
	UINT GetHeight() { return height; }

private:
	void SetViewportAndScissorRect();

private:
	D3D12RHI* d3d12RHI = nullptr;
	UINT width;
	UINT height;
	std::unique_ptr<RenderTarget2D> RT = nullptr;
	SceneView sceneView;
	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;
};