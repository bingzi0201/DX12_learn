#include "HDRSkyActor.h"

using namespace DirectX;

HDRSkyActor::HDRSkyActor(const std::string& name)
	:Actor(name)
{
	meshComponent = AddComponent<MeshComponent>();
	rootComponent = meshComponent;

	meshComponent->SetMeshName("SphereMesh");
}

HDRSkyActor::~HDRSkyActor()
{}

void HDRSkyActor::SetMaterialInstance(std::string materialInstanceName)
{
	meshComponent->SetMaterialInstance(materialInstanceName);
}