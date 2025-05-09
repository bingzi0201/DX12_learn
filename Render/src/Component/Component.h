#pragma once

#include "../Math/Transform.h"

class Component
{
public:
	Component() {}
	virtual ~Component() {}

public:
	virtual void SetWorldLocation(const TVector3& Location)
	{
		worldTransform.Location = Location;
	}

	TVector3 GetWorldLocation() const
	{
		return worldTransform.Location;
	}

	virtual void SetWorldRotation(const TRotator& Rotation)
	{
		worldTransform.Rotation = Rotation;
	}

	TRotator GetWorldRotation() const
	{
		return worldTransform.Rotation;
	}

	void SetWorldTransform(const TTransform& Transform)
	{
		worldTransform = Transform;
	}

	TTransform GetWorldTransform() const
	{
		return worldTransform;
	}

	void SetPrevWorldTransform(const TTransform& Transform)
	{
		prevWorldTransform = Transform;
	}

	TTransform GetPrevWorldTransform() const
	{
		return prevWorldTransform;
	}

protected:
	TTransform relativeTransform; //TODO
	TTransform worldTransform;
	TTransform prevWorldTransform;
};
