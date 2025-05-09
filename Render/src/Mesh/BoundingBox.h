#pragma once

#include <DirectXCollision.h>
#include <vector>
#include "Vertex.h"
#include "../Math/Transform.h"
#include "Ray.h"

class TBoundingBox
{
public:
	void Init(std::vector<TVector3> points);
	void Init(std::vector<Vertex> vertices);

	TVector3 GetCenter() const { return (boxMin + boxMax) * 0.5f; }
	TVector3 GetExtend() const { return (boxMax - boxMin) * 0.5f; }
	TVector3 GetSize() const { return  (boxMax - boxMin); }

	int GetWidestAxis() const;
	float GetMaxWidth() const;
	float GetSurfaceArea() const;

	static TBoundingBox Union(const TBoundingBox& boxA, const TBoundingBox& boxB);
	static TBoundingBox Union(const TBoundingBox& box, const TVector3& point);

	TBoundingBox Transform(const TTransform& T);

	// If the ray¡¯s origin is inside the box, 0 is returned for Dist0
	bool Intersect(const Ray& ray, float& dist0, float& dist1);

	DirectX::BoundingBox GetD3DBox();

public:
	bool bInit = false;

	TVector3 boxMin = TVector3(TMath::Infinity);
	TVector3 boxMax = TVector3(-TMath::Infinity);
};