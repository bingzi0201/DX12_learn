#include "CameraActor.h"

TCameraActor::TCameraActor(const std::string& name)
	:Actor(name)
{
	//CameraComponent
	cameraComponent = AddComponent<CameraComponent>();
	rootComponent = cameraComponent;
}

TCameraActor::~TCameraActor()
{

}

CameraComponent* TCameraActor::GetCameraComponent()
{
	return cameraComponent;
}