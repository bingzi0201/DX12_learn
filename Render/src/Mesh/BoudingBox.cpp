#include "BoundingBox.h"

void TBoundingBox::Init(std::vector<TVector3> points)
{
	if (points.size() > 0)
	{
		bInit = true;

		for (const TVector3& point : points)
		{
			boxMin = TVector3::Min(boxMin, point);
			boxMax = TVector3::Max(boxMax, point);
		}
	}
}

void TBoundingBox::Init(std::vector<Vertex> vertices)
{
	if (vertices.size() > 0)
	{
		bInit = true;

		for (const Vertex& vertex : vertices)
		{
			TVector3 P = vertex.position;

			boxMin = TVector3::Min(boxMin, P);
			boxMax = TVector3::Max(boxMax, P);
		}
	}
}

DirectX::BoundingBox TBoundingBox::GetD3DBox()
{
	DirectX::BoundingBox d3dBox;

	d3dBox.Center = (boxMin + boxMax) * 0.5f;
	d3dBox.Extents = (boxMax - boxMin) * 0.5f;

	return d3dBox;
}

int TBoundingBox::GetWidestAxis() const
{
	TVector3 V = boxMax - boxMin;

	if (V.x > V.y && V.x > V.z)
		return 0;
	else if (V.y > V.z)
		return 1;
	else
		return 2;
}

float TBoundingBox::GetMaxWidth() const
{
	TVector3 V = boxMax - boxMin;

	if (V.x > V.y && V.x > V.z)
		return V.x;
	else if (V.y > V.z)
		return V.y;
	else
		return V.z;
}

float TBoundingBox::GetSurfaceArea() const
{
	if (bInit)
	{
		TVector3 V = boxMax - boxMin;

		return 2.0f * (V.x * V.y + V.x * V.z + V.y * V.z);
	}
	else
	{
		return 0.0f;
	}
}

TBoundingBox TBoundingBox::Union(const TBoundingBox& boxA, const TBoundingBox& boxB)
{
	TBoundingBox unionBox;

	if (boxA.bInit || boxB.bInit)
	{
		unionBox.bInit = true;

		unionBox.boxMin = TVector3::Min(boxA.boxMin, boxB.boxMin);
		unionBox.boxMax = TVector3::Max(boxA.boxMax, boxB.boxMax);
	}

	return unionBox;
}

TBoundingBox TBoundingBox::Union(const TBoundingBox& box, const TVector3& point)
{
	TBoundingBox unionBox;

	unionBox.bInit = true;

	unionBox.boxMin = TVector3::Min(box.boxMin, point);
	unionBox.boxMax = TVector3::Max(box.boxMax, point);

	return unionBox;
}

TBoundingBox TBoundingBox::Transform(const TTransform& T)
{
	TBoundingBox box;

	if (bInit)
	{
		box.bInit = true;

		// Transform eight corner points, and calculate new AABB
		TMatrix M = T.GetTransformMatrix();

		box = Union(box, M.Transform(TVector3(boxMin.x, boxMin.y, boxMin.z)));
		box = Union(box, M.Transform(TVector3(boxMax.x, boxMin.y, boxMin.z)));
		box = Union(box, M.Transform(TVector3(boxMin.x, boxMax.y, boxMin.z)));
		box = Union(box, M.Transform(TVector3(boxMin.x, boxMin.y, boxMax.z)));

		box = Union(box, M.Transform(TVector3(boxMin.x, boxMax.y, boxMax.z)));
		box = Union(box, M.Transform(TVector3(boxMax.x, boxMax.y, boxMin.z)));
		box = Union(box, M.Transform(TVector3(boxMax.x, boxMin.y, boxMax.z)));
		box = Union(box, M.Transform(TVector3(boxMax.x, boxMax.y, boxMax.z)));
	}

	return box;
}

// Ref: pbrt-v3
bool TBoundingBox::Intersect(const Ray& ray, float& dist0, float& dist1)
{
	float t0 = 0, t1 = ray.maxDist;
	for (int i = 0; i < 3; ++i)
	{
		// Update interval for x/y/z bounding box slab
		float InvRayDir = 1.0f / ray.direction[i];
		float tNear = (boxMin[i] - ray.origin[i]) * InvRayDir;
		float tFar = (boxMax[i] - ray.origin[i]) * InvRayDir;

		// Update parametric interval from slab intersection t values
		if (tNear > tFar) std::swap(tNear, tFar);

		// Update tFar to ensure robust ray--bounds intersection
		tFar *= 1 + 2 * TMath::gamma(3);
		t0 = tNear > t0 ? tNear : t0;
		t1 = tFar < t1 ? tFar : t1;
		if (t0 > t1) return false;
	}

	dist0 = t0;
	dist1 = t1;

	return true;
}