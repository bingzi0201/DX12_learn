#include "DirectionalLightActor.h"

using namespace DirectX;

DirectionalLightActor::DirectionalLightActor(const std::string& name)
	:LightActor(name, ELightType::DirectionalLight)
{
	meshComponent = AddComponent<MeshComponent>();

	rootComponent = meshComponent;

}

DirectionalLightActor::~DirectionalLightActor()
{

}

void DirectionalLightActor::SetActorTransform(const TTransform& newTransform)
{
	Actor::SetActorTransform(newTransform);
	SetLightDirection(newTransform.Rotation);
}

void DirectionalLightActor::SetLightDirection(TRotator rotation)
{
	//Calculate Direction
	TMatrix R = TMatrix::CreateFromYawPitchRoll(rotation.Yaw * TMath::Pi / 180.0f, rotation.Pitch * TMath::Pi / 180.0f, rotation.Roll * TMath::Pi / 180.0f);
	direction = R.TransformNormal(TVector3::Up);
}

TVector3 DirectionalLightActor::GetLightDirection() const
{
	return direction;
}
