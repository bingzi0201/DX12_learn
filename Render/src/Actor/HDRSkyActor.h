#pragma once

#include "Actor.h"
#include "../Component/MeshComponent.h"

class HDRSkyActor : public Actor
{
public:
	HDRSkyActor(const std::string& name);
	~HDRSkyActor();

	void SetMaterialInstance(std::string materialInstanceName);

public:
	MeshComponent* meshComponent = nullptr;
};
