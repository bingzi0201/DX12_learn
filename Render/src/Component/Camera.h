#pragma once
#include "../Common/stdafx.h"
#include "../DXMath/DXMath.h"
#include "../DXMath/MathHelper.h"
class Camera final
{
	float mNearZ = 0.0f;
	float mFarZ = 0.0f;
	float mAspect = 1.0f;
	float mFovY = 0.0f;
	float mNearWindowHeight = 0.0f;
	float mFarWindowHeight = 0.0f;

public:
	~Camera();
	Camera();
	float NearZ() const { return mNearZ; }
	float FarZ() const { return mFarZ; }
	float Aspect() const { return mAspect; }
	float FovY() const { return mFovY; }
	float FovX() const
	{
		float halfWidth = 0.5 * NearWindowWidth();
		return 2.0 * atan(halfWidth / mNearZ);
	}

	float NearWindowWidth() const { return mAspect * mNearWindowHeight; }
	float NearWindowHeight() const { return mNearWindowHeight; }
	float FarWindowWidth() const { return mAspect * mFarWindowHeight; }
	float FarWindowHeight() const { return mFarWindowHeight; }

	void SetLens(float fovY, float zn, float zf);
	void SetAspect(float aspect);

	void LookAt(const Math::Vector3& pos, const Math::Vector3& target, const Math::Vector3& worldUp);
	void UpdateViewMatrix();
	void UpdateProjectionMatrix();
	Math::Matrix4 View = MathHelper::Identity4x4();
	Math::Matrix4 Proj = MathHelper::Identity4x4();
	Math::Vector3 Position = { 0.0f, 0.0f, 0.0f };
	Math::Vector3 Right = { 1.0f, 0.0f, 0.0f };
	Math::Vector3 Up = { 0.0f, 1.0f, 0.0f };
	Math::Vector3 Forward = { 0.0f, 0.0f, 1.0f };
	float orthoSize = 5;
	bool isOrtho = false;
};