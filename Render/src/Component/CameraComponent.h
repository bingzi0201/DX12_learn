#pragma once

#include "Component.h"

class CameraComponent : public Component
{
public:
	CameraComponent();
	~CameraComponent();

	// Location
	virtual void SetWorldLocation(const TVector3& Location) override;

	// Get frustum properties.
	float GetNearZ()const;
	float GetFarZ()const;
	float GetAspect()const;
	float GetFovY()const;
	float GetFovX()const;

	// Get near and far plane dimensions in view space coordinates.
	float GetNearWindowWidth()const;
	float GetNearWindowHeight()const;
	float GetFarWindowWidth()const;
	float GetFarWindowHeight()const;

	// Set frustum.
	void SetLens(float inFovY, float inAspect, float zn, float zf);

	// Define camera space via LookAt parameters.
	void LookAt(const TVector3& pos, const TVector3& target, const TVector3& up);

	// Get View/Proj matrices.
	TMatrix GetView()const;
	TMatrix GetProj()const;

	// Move the camera a distance.
	void MoveRight(float dist);
	void MoveForward(float dist);
	void MoveUp(float dist);

	// Rotate the camera.
	void Pitch(float degrees);
	void RotateY(float degrees);

	// After modifying camera position/orientation, call to rebuild the view matrix.
	void UpdateViewMatrix();

	void SetPrevViewProj(const TMatrix& vp);
	TMatrix GetPrevViewProj() const;

private:

	// Camera coordinate system with coordinates relative to world space.
	TVector3 right = { 1.0f, 0.0f, 0.0f };
	TVector3 up = { 0.0f, 1.0f, 0.0f };
	TVector3 look = { 0.0f, 0.0f, 1.0f };

	// Cache frustum properties.
	float nearZ = 0.0f;
	float farZ = 0.0f;
	float aspect = 0.0f;
	float fovY = 0.0f;
	float nearWindowHeight = 0.0f;
	float farWindowHeight = 0.0f;

	bool viewDirty = true;

	// Cache View/Proj matrices.
	TMatrix view = TMatrix::Identity;
	TMatrix proj = TMatrix::Identity;

	TMatrix prevViewProj = TMatrix::Identity;
};
