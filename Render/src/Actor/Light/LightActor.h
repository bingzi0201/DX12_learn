#pragma once

#include "../Actor.h"

enum ELightType
{
	None,
	AmbientLight,
	DirectionalLight,
	PointLight,
	SpotLight,
	AreaLight
};

class LightActor : public Actor
{
public:
	LightActor(const std::string& name, ELightType inType);

	~LightActor();

	ELightType GetType()
	{
		return lightType;
	}

public:
	TVector3 GetLightColor() const
	{
		return color;
	}

	virtual void SetLightColor(const TVector3& inColor)
	{
		color = inColor;
	}

	float GetLightIntensity() const
	{
		return intensity;
	}

	virtual void SetLightIntensity(float inIntensity)
	{
		intensity = inIntensity;
	}

	bool IsDrawDebug()
	{
		return bDrawDebug;
	}

	void SetDrawDebug(bool bDraw)
	{
		bDrawDebug = bDraw;
	}

	bool IsDrawMesh()
	{
		return bDrawMesh;
	}

	void SetDrawMesh(bool bDraw)
	{
		bDrawMesh = bDraw;
	}

protected:
	ELightType lightType = ELightType::None;

	TVector3 color = TVector3::One;

	float intensity = 10.0f;

	bool bCastShadows = true;
	bool bDrawDebug = false;
	bool bDrawMesh = false;
};
