#pragma once

#include "Actor.h"
#include "../Component/CameraComponent.h"

class TCameraActor : public Actor
{
public:
	TCameraActor(const std::string& name);
	~TCameraActor();

	CameraComponent* GetCameraComponent();

private:
	CameraComponent* cameraComponent = nullptr;
};