#include "SpotLightActor.h"


SpotLightActor::SpotLightActor(const std::string& Name)
	:LightActor(Name, ELightType::SpotLight)
{
	meshComponent = AddComponent<MeshComponent>();
	rootComponent = meshComponent;

	//Mesh
	meshComponent->SetMeshName("CylinderMesh");

	//Material
	meshComponent->SetMaterialInstance("DefaultMatInst");
}

SpotLightActor::~SpotLightActor()
{

}

void SpotLightActor::SetActorTransform(const TTransform& newTransform)
{
	Actor::SetActorTransform(newTransform);

	SetLightDirection(newTransform.Rotation);
}

void SpotLightActor::SetLightDirection(TRotator Rotation)
{
	//Calculate Direction
	TMatrix R = TMatrix::CreateFromYawPitchRoll(Rotation.Yaw * TMath::Pi / 180.0f, Rotation.Pitch * TMath::Pi / 180.0f, Rotation.Roll * TMath::Pi / 180.0f);
	Direction = R.TransformNormal(TVector3::Up);
}

TVector3 SpotLightActor::GetLightDirection()
{
	return Direction;
}
