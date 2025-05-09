#include "CameraComponent.h"

CameraComponent::CameraComponent()
{
	SetLens(0.25f * TMath::Pi, 1.0f, 0.1f, 100.0f);
}

CameraComponent::~CameraComponent()
{
}

void CameraComponent::SetWorldLocation(const TVector3& Location)
{
	worldTransform.Location = Location;
	viewDirty = true;
}

float CameraComponent::GetNearZ()const
{
	return nearZ;
}

float CameraComponent::GetFarZ()const
{
	return farZ;
}

float CameraComponent::GetAspect()const
{
	return aspect;
}

float CameraComponent::GetFovY()const
{
	return fovY;
}

float CameraComponent::GetFovX()const
{
	float halfWidth = 0.5f * GetNearWindowWidth();
	return float(2.0f * atan(halfWidth / nearZ));
}

float CameraComponent::GetNearWindowWidth()const
{
	return aspect * nearWindowHeight;
}

float CameraComponent::GetNearWindowHeight()const
{
	return nearWindowHeight;
}

float CameraComponent::GetFarWindowWidth()const
{
	return aspect * farWindowHeight;
}

float CameraComponent::GetFarWindowHeight()const
{
	return farWindowHeight;
}

void CameraComponent::SetLens(float inFovY, float inAspect, float inNearZ, float inNearFar)
{
	// cache properties
	fovY = inFovY;
	aspect = inAspect;
	nearZ = inNearZ;
	farZ = inNearFar;

	nearWindowHeight = 2.0f * nearZ * tanf(0.5f * inFovY);
	farWindowHeight = 2.0f * farZ * tanf(0.5f * inFovY);

	proj = TMatrix::CreatePerspectiveFieldOfView(inFovY, aspect, nearZ, farZ);
}

void CameraComponent::LookAt(const TVector3& pos, const TVector3& target, const TVector3& up)
{
	TVector3 L = target - pos;
	L.Normalize();
	TVector3 R = up.Cross(L);
	R.Normalize();
	TVector3 U = L.Cross(R);

	worldTransform.Location = pos;
	look = L;
	right = R;
	this->up = U;

	viewDirty = true;
}


TMatrix CameraComponent::GetView()const
{
	assert(!viewDirty);
	return view;
}

TMatrix CameraComponent::GetProj()const
{
	return proj;
}

void CameraComponent::MoveRight(float dist)
{
	worldTransform.Location += dist * right;

	viewDirty = true;
}

void CameraComponent::MoveForward(float dist)
{
	worldTransform.Location += dist * look;

	viewDirty = true;
}

void CameraComponent::MoveUp(float dist)
{
	worldTransform.Location += dist * up;

	viewDirty = true;
}

void CameraComponent::Pitch(float degrees)
{
	float Radians = TMath::DegreesToRadians(degrees);

	// Rotate up and look vector about the right vector.
	TMatrix R = TMatrix::CreateFromAxisAngle(right, Radians);

	up = R.TransformNormal(up);
	look = R.TransformNormal(look);

	viewDirty = true;
}

void CameraComponent::RotateY(float degrees)
{
	float radians = TMath::DegreesToRadians(degrees);

	// Rotate the basis vectors about the world y-axis.
	TMatrix R = TMatrix::CreateRotationY(radians);

	right = R.TransformNormal(right);
	up = R.TransformNormal(up);
	look = R.TransformNormal(look);

	viewDirty = true;
}

void CameraComponent::UpdateViewMatrix()
{
	if (viewDirty)
	{
		// Keep camera's axes orthogonal to each other and of unit length.
		look.Normalize();
		up = look.Cross(right);
		up.Normalize();

		// Up, Look already ortho-normal, so no need to normalize cross product.
		right = up.Cross(look);

		// Fill in the view matrix entries.
		float x = -worldTransform.Location.Dot(right);
		float y = -worldTransform.Location.Dot(up);
		float z = -worldTransform.Location.Dot(look);

		view(0, 0) = right.x;
		view(1, 0) = right.y;
		view(2, 0) = right.z;
		view(3, 0) = x;

		view(0, 1) = up.x;
		view(1, 1) = up.y;
		view(2, 1) = up.z;
		view(3, 1) = y;

		view(0, 2) = look.x;
		view(1, 2) = look.y;
		view(2, 2) = look.z;
		view(3, 2) = z;

		view(0, 3) = 0.0f;
		view(1, 3) = 0.0f;
		view(2, 3) = 0.0f;
		view(3, 3) = 1.0f;

		viewDirty = false;
	}
}

void CameraComponent::SetPrevViewProj(const TMatrix& VP)
{
	prevViewProj = VP;
}

TMatrix CameraComponent::GetPrevViewProj() const
{
	return prevViewProj;
}