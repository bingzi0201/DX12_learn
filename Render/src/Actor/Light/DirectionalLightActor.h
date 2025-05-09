#pragma once

#include "LightActor.h"
#include "../../Component/MeshComponent.h"

class DirectionalLightActor : public LightActor
{
public:
	DirectionalLightActor(const std::string& Name);

	~DirectionalLightActor();

public:
	virtual void SetActorTransform(const TTransform& NewTransform) override;

	TVector3 GetLightDirection() const;


private:
	void SetLightDirection(TRotator Rotation);

private:
	TVector3 direction;

	MeshComponent* meshComponent = nullptr;

};