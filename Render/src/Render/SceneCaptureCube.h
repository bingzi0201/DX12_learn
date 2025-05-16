#pragma once

#include "RenderTarget.h"
#include "SceneView.h"

class SceneCaptureCube
{
public:
	SceneCaptureCube(bool renderDepth, UINT size, DXGI_FORMAT format, D3D12RHI* inD3D12RHI);
	RenderTargetCube* GetRTCube() { return RTCube.get(); }
	const SceneView& GetSceneView(UINT index) const
	{
		return sceneViews[index];
	}
	D3D12_VIEWPORT GetViewport() { return viewport; }
	D3D12_RECT GetScissorRect() { return scissorRect; }
	void CreatePerspectiveViews(const TVector3& eye, float nearPlane, float farPlane);
	UINT GetCubeMapSize() { return cubeMapSize; }

private:
	void SetViewportAndScissorRect(UINT cubeMapSize);

private:
	D3D12RHI* d3d12RHI = nullptr;
	UINT cubeMapSize;
	std::unique_ptr<RenderTargetCube> RTCube = nullptr;
	SceneView sceneViews[6];
	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;
};