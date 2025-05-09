#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "Vertex.h"
#include "BoundingBox.h"
#include "../Texture/Texture.h"

struct MeshSDFDescriptor
{
	TVector3 center;
	float extent;

	int resolution;
	int pad1;
	int pad2;
	int pad3;
};

class Mesh
{
public:
	using uint16 = std::uint16_t;
	using uint32 = std::uint32_t;

	Mesh();

	Mesh(Mesh&&) = default;

	Mesh(const Mesh&) = delete;

	Mesh& operator=(const Mesh&) = delete;

public:

	/// Creates a box centered at the origin with the given dimensions, where each
	/// face has m rows and n columns of vertices.
	void CreateBox(float width, float height, float depth, uint32 numSubdivisions);
	/// Creates a sphere centered at the origin with the given radius.  The
	/// slices and stacks parameters control the degree of tessellation.
	void CreateSphere(float radius, uint32 sliceCount, uint32 stackCount);
	/// Creates a cylinder parallel to the y-axis, and centered about the origin.  
	/// The bottom and top radius can vary to form various cone shapes rather than true
	// cylinders.  The slices and stacks parameters control the degree of tessellation.
	void CreateCylinder(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount);
	/// Creates an mxn grid in the xz-plane with m rows and n columns, centered
	/// at the origin with the specified width and depth.
	void CreateGrid(float width, float depth, uint32 m, uint32 n);
	/// Creates a quad aligned with the screen.  This is useful for postprocessing and screen effects.
	void CreateQuad(float x, float y, float w, float h, float depth);

	const std::vector<uint16>& GetIndices16() const;
	std::string GetInputLayoutName() const;
	void GenerateIndices16();

public:
	void GenerateBoundingBox();
	TBoundingBox GetBoundingBox() { return boundingBox; }
	void SetSDFTexture(std::unique_ptr<Texture3D>& inSDFTexture)
	{
		SDFTexture = std::move(inSDFTexture);
	}

	Texture3D* GetSDFTexture()
	{
		if (SDFTexture)
		{
			return SDFTexture.get();
		}
		else
		{
			return nullptr;
		}
	}

private:

	void Subdivide();
	Vertex MidPoint(const Vertex& v0, const Vertex& v1);
	void BuildCylinderTopCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount);
	void BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount);

public:
	std::string meshName;
	std::vector<Vertex> vertices;
	std::vector<uint32> indices32;
	std::vector<uint16> indices16;
	std::string inputLayoutName;
	TBoundingBox boundingBox;

	// SDF
	std::unique_ptr<Texture3D> SDFTexture = nullptr;

	MeshSDFDescriptor SDFDescriptor;
};