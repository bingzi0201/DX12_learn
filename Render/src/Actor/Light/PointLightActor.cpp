#include "PointLightActor.h"


PointLightActor::PointLightActor(const std::string& Name)
	:LightActor(Name, ELightType::PointLight)
{
	meshComponent = AddComponent<MeshComponent>();

	rootComponent = meshComponent;

	//Mesh
	meshComponent->SetMeshName("SphereMesh");

	//Material
	meshComponent->SetMaterialInstance("DefaultMatInst");
}

PointLightActor::~PointLightActor()
{

}

