#pragma once
#include "../Utils/D3D12Utils.h"
#include "../Math/Math.h"

struct SceneView
{
	TVector3 eyePos = TVector3::Zero;
	TMatrix view = TMatrix::Identity;
	TMatrix proj = TMatrix::Identity;

	float Near;
	float Far;
};