#pragma once

#include "../Math/Math.h"

class Ray
{
public:
	Ray()
		:maxDist(TMath::Infinity)
	{}

	Ray(const TVector3& InOrigin, const TVector3& InDirection, float InMaxDist = TMath::Infinity)
		:origin(InOrigin), direction(InDirection), maxDist(InMaxDist)
	{}

public:
	TVector3 origin;
	TVector3 direction;
	mutable float maxDist;
};