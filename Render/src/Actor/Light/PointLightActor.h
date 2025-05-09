#pragma once

#include "LightActor.h"
#include "../../Component/MeshComponent.h"

class PointLightActor : public LightActor
{
public:
	PointLightActor(const std::string& Name);

	~PointLightActor();

	float GetAttenuationRange() const
	{
		return attenuationRange;
	}

	void SetAttenuationRange(float Radius)
	{
		attenuationRange = Radius;
	}


private:
	float attenuationRange = 10.0f;
	MeshComponent* meshComponent = nullptr;

};