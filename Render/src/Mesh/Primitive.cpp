#include "Primitive.h"
#include "MeshRepository.h"

bool Primitive::GetLocalBoundingBox(TBoundingBox& outBox) const
{
	if (boundingBox.bInit)
	{
		outBox = boundingBox;
		return true;
	}
	else
	{
		return false;
	}
}

bool Primitive::GetWorldBoundingBox(TBoundingBox& outBox) const
{
	TBoundingBox localBox;

	if (GetLocalBoundingBox(localBox))
	{
		outBox = localBox.Transform(worldTransform);
		return true;
	}
	else
	{
		return false;
	}
}

void Line::GenerateBoundingBox()
{
	std::vector<TVector3> points;
	points.push_back(pointA);
	points.push_back(pointB);

	boundingBox.Init(points);
}

void Triangle::GenerateBoundingBox()
{
	std::vector<TVector3> points;
	points.push_back(pointA);
	points.push_back(pointB);
	points.push_back(pointC);

	boundingBox.Init(points);
}

// Ref: "Fast, Minimum Storage Ray-Triangle Intersection"
// https://cadxfem.org/inf/Fast%20MinimumStorage%20RayTriangle%20Intersection.pdf
bool Triangle::Intersect(const Ray& ray, float& distance, bool& bBackFace)
{
	const float EPSILON = 0.000001f;

	TVector3 Dir = ray.direction;
	TVector3 Orig = ray.origin;

	// calculate E1 and E2
	TVector3 Edge1 = pointB - pointA;
	TVector3 Edge2 = pointC - pointA;

	// calculate P
	TVector3 PVec = Dir.Cross(Edge2);

	// If determinant is near zero, ray lies in plane of f triangle
	float Det = Edge1.Dot(PVec);

	if (Det > -EPSILON && Det < EPSILON)
	{
		return false;
	}

	float inverseDet = 1.0f / Det;

	// Calculate distance from vert to ray origin
	TVector3 TVec = Orig - pointA;

	// Calculate U parameter and test bounds 
	float u = TVec.Dot(PVec) * inverseDet;
	if (u < 0.0f || u > 1.0f)
	{
		return false;
	}

	// calculate Q = T x E1
	TVector3 QVec = TVec.Cross(Edge1);

	// calculate v parameter and test bounds
	float v = Dir.Dot(QVec) * inverseDet;
	if (v < 0.0f || u + v > 1.0f)
	{
		return false;
	}

	// Calculate t
	float t = Edge2.Dot(QVec) * inverseDet;

	if (t < 0.0f)
	{
		return false;
	}

	float tValue = std::abs(t);
	if (tValue > ray.maxDist)
	{
		return false;
	}

	distance = tValue;
	bBackFace = Det < 0.0f ? true : false;

	return true;
}

void MeshPrimitive::GenerateBoundingBox()
{
	Mesh& mesh = MeshRepository::Get().meshMap.at(meshName);
	boundingBox = mesh.GetBoundingBox();
}