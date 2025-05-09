#pragma once

#include "LightActor.h"
#include "../../Component/MeshComponent.h"

class SpotLightActor : public LightActor
{
public:
	SpotLightActor(const std::string& Name);

	~SpotLightActor();

public:
	virtual void SetActorTransform(const TTransform& NewTransform) override;

	TVector3 GetLightDirection();

	float GetAttenuationRange() const
	{
		return AttenuationRange;
	}

	void SetAttenuationRange(float Range)
	{
		AttenuationRange = Range;
	}

	float GetInnerConeAngle() const
	{
		return InnerConeAngle;
	}

	void SetInnerConeAngle(float Angle)
	{
		InnerConeAngle = Angle;
	}

	float GetOuterConeAngle() const
	{
		return OuterConeAngle;
	}

	void SetOuterConeAngle(float Angle)
	{
		OuterConeAngle = Angle;
	}

	float GetBottomRadius() const
	{
		float Radians = OuterConeAngle * (TMath::Pi / 180.0f);
		float BottomRadius = AttenuationRange * tan(Radians);

		return BottomRadius;
	}

private:
	void SetLightDirection(TRotator Rotation);

private:
	TVector3 Direction = { 0.0f, -1.0f, 0.0f };

	float AttenuationRange = 10.0f; // Cone height
	float InnerConeAngle = 0.0f;  //In degrees
	float OuterConeAngle = 40.0f; //In degrees

private:
	MeshComponent* meshComponent = nullptr;

};