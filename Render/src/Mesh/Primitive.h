#pragma once

#include "Color.h"
#include "BoundingBox.h"
#include "Ray.h"
#include <string>

class Primitive
{
public:
	Primitive() {}

	virtual ~Primitive() {}

	virtual void GenerateBoundingBox() {}
	bool GetLocalBoundingBox(TBoundingBox& outBox) const;
	bool GetWorldBoundingBox(TBoundingBox& outBox) const;
	virtual bool Intersect(const Ray& ray, float& dist, bool& bBackFace) { return false; }

protected:
	TTransform worldTransform;
	TBoundingBox boundingBox;
};

class Point : public Primitive
{
public:
	Point() = default;

	Point(const TVector3& InPoint, const Color& InColor)
		:point(InPoint), color(InColor)
	{}

public:
	TVector3 point;
	Color color;
};

class Line : public Primitive
{
public:
	Line() = default;

	Line(const TVector3& InPointA, const TVector3& InPointB, const Color& InColor)
		:pointA(InPointA), pointB(InPointB), color(InColor)
	{}

	virtual void GenerateBoundingBox() override;

public:
	TVector3 pointA;
	TVector3 pointB;
	Color color;
};


class Triangle : public Primitive
{
public:
	Triangle() = default;

	Triangle(const TVector3& inPointA, const TVector3& inPointB, const TVector3& inPointC, const Color& inColor)
		:pointA(inPointA), pointB(inPointB), pointC(inPointC), color(inColor)
	{}

	Triangle(const Triangle& other) = default;

	virtual void GenerateBoundingBox() override;

	// Don't cull backfacing triangles
	// Negative value will return for Dist when intersect backfacing triangle
	virtual bool Intersect(const Ray& ray, float& Dist, bool& bBackFace) override;

public:
	TVector3 pointA;
	TVector3 pointB;
	TVector3 pointC;
	Color color;
};

class MeshPrimitive : public Primitive
{
public:
	MeshPrimitive(const std::string& InMeshName, const TTransform& InWorldTransform)
		:meshName(InMeshName)
	{
		worldTransform = InWorldTransform;
	}

	virtual void GenerateBoundingBox() override;

private:
	std::string meshName;
};