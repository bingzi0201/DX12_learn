#include "Mesh.h"
#include "../File/FileHelpers.h"
#include <fstream>

Mesh::Mesh()
{
	inputLayoutName = "DefaultInputLayout";
}


void Mesh::CreateBox(float width, float height, float depth, uint32 numSubdivisions)
{
	//
	// Create the Vertices.
	//
	Vertex v[24];

	float w2 = 0.5f * width;
	float h2 = 0.5f * height;
	float d2 = 0.5f * depth;

	// Fill in the front face vertex data.
	v[0] = Vertex(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[1] = Vertex(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[2] = Vertex(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[3] = Vertex(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the back face vertex data.
	v[4] = Vertex(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[5] = Vertex(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[6] = Vertex(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[7] = Vertex(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// Fill in the top face vertex data.
	v[8] = Vertex(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[9] = Vertex(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[10] = Vertex(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[11] = Vertex(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the bottom face vertex data.
	v[12] = Vertex(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[13] = Vertex(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[14] = Vertex(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[15] = Vertex(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// Fill in the left face vertex data.
	v[16] = Vertex(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[17] = Vertex(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[18] = Vertex(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[19] = Vertex(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	// Fill in the right face vertex data.
	v[20] = Vertex(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	v[21] = Vertex(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	v[22] = Vertex(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
	v[23] = Vertex(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

	vertices.assign(&v[0], &v[24]);

	//
	// Create the indices.
	//

	uint32 i[36];

	// Fill in the front face index data
	i[0] = 0; i[1] = 1; i[2] = 2;
	i[3] = 0; i[4] = 2; i[5] = 3;

	// Fill in the back face index data
	i[6] = 4; i[7] = 5; i[8] = 6;
	i[9] = 4; i[10] = 6; i[11] = 7;

	// Fill in the top face index data
	i[12] = 8; i[13] = 9; i[14] = 10;
	i[15] = 8; i[16] = 10; i[17] = 11;

	// Fill in the bottom face index data
	i[18] = 12; i[19] = 13; i[20] = 14;
	i[21] = 12; i[22] = 14; i[23] = 15;

	// Fill in the left face index data
	i[24] = 16; i[25] = 17; i[26] = 18;
	i[27] = 16; i[28] = 18; i[29] = 19;

	// Fill in the right face index data
	i[30] = 20; i[31] = 21; i[32] = 22;
	i[33] = 20; i[34] = 22; i[35] = 23;

	indices32.assign(&i[0], &i[36]);


	// Put a cap on the number of subdivisions.
	numSubdivisions = std::min<uint32>(numSubdivisions, 6u);
	for (uint32 i = 0; i < numSubdivisions; ++i)
		Subdivide();


	GenerateIndices16();
}

void Mesh::Subdivide()
{
	// Save a copy of the input geometry.
	std::vector<Vertex> VerticesCopy = vertices;
	std::vector<uint32> Indices32Copy = indices32;


	vertices.resize(0);
	indices32.resize(0);

	//       v1
	//       *
	//      / \
	//     /   \
	//  m0*-----*m1
	//   / \   / \
	//  /   \ /   \
	// *-----*-----*
	// v0    m2     v2

	uint32 numTris = (uint32)Indices32Copy.size() / 3;
	for (uint32 i = 0; i < numTris; ++i)
	{
		Vertex v0 = VerticesCopy[Indices32Copy[i * 3 + 0]];
		Vertex v1 = VerticesCopy[Indices32Copy[i * 3 + 1]];
		Vertex v2 = VerticesCopy[Indices32Copy[i * 3 + 2]];

		//
		// Generate the midpoints.
		//

		Vertex m0 = MidPoint(v0, v1);
		Vertex m1 = MidPoint(v1, v2);
		Vertex m2 = MidPoint(v0, v2);

		//
		// Add new geometry.
		//

		vertices.push_back(v0); // 0
		vertices.push_back(v1); // 1
		vertices.push_back(v2); // 2
		vertices.push_back(m0); // 3
		vertices.push_back(m1); // 4
		vertices.push_back(m2); // 5

		indices32.push_back(i * 6 + 0);
		indices32.push_back(i * 6 + 3);
		indices32.push_back(i * 6 + 5);

		indices32.push_back(i * 6 + 3);
		indices32.push_back(i * 6 + 4);
		indices32.push_back(i * 6 + 5);

		indices32.push_back(i * 6 + 5);
		indices32.push_back(i * 6 + 4);
		indices32.push_back(i * 6 + 2);

		indices32.push_back(i * 6 + 3);
		indices32.push_back(i * 6 + 1);
		indices32.push_back(i * 6 + 4);
	}
}

Vertex Mesh::MidPoint(const Vertex& v0, const Vertex& v1)
{
	// Compute the midpoints of all the attributes.  Vectors need to be normalized
	// since linear interpolating can make them not unit length.  
	TVector3 pos = 0.5f * (v0.position + v1.position);
	TVector3 normal = 0.5f * (v0.normal + v1.normal);
	normal.Normalize();
	TVector3 tangent = 0.5f * (v0.tangentU + v1.tangentU);
	tangent.Normalize();
	TVector2 tex = 0.5f * (v0.texcoord + v1.texcoord);


	Vertex v(pos, normal, tangent, tex);

	return v;
}

void Mesh::CreateSphere(float radius, uint32 sliceCount, uint32 stackCount)
{
	//
	// Compute the vertices stating at the top pole and moving down the stacks.
	//

	// Poles: note that there will be texture coordinate distortion as there is
	// not a unique point on the texture map to assign to the pole when mapping
	// a rectangular texture onto a sphere.
	Vertex topVertex(0.0f, +radius, 0.0f, 0.0f, +1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	Vertex bottomVertex(0.0f, -radius, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	vertices.push_back(topVertex);

	float phiStep = DirectX::XM_PI / stackCount;
	float thetaStep = 2.0f * DirectX::XM_PI / sliceCount;

	// Compute vertices for each stack ring (do not count the poles as rings).
	for (uint32 i = 1; i <= stackCount - 1; ++i)
	{
		float phi = i * phiStep;

		// Vertices of ring.
		for (uint32 j = 0; j <= sliceCount; ++j)
		{
			float theta = j * thetaStep;

			Vertex v;

			// spherical to cartesian
			v.position.x = radius * sinf(phi) * cosf(theta);
			v.position.y = radius * cosf(phi);
			v.position.z = radius * sinf(phi) * sinf(theta);

			// Partial derivative of P with respect to theta
			v.tangentU.x = -radius * sinf(phi) * sinf(theta);
			v.tangentU.y = 0.0f;
			v.tangentU.z = +radius * sinf(phi) * cosf(theta);
			v.tangentU.Normalize();

			v.normal = v.position;
			v.normal.Normalize();

			v.texcoord.x = theta / DirectX::XM_2PI;
			v.texcoord.y = phi / DirectX::XM_PI;

			vertices.push_back(v);
		}
	}

	vertices.push_back(bottomVertex);

	//
	// Compute indices for top stack.  The top stack was written first to the vertex buffer
	// and connects the top pole to the first ring.
	//

	for (uint32 i = 1; i <= sliceCount; ++i)
	{
		indices32.push_back(0);
		indices32.push_back(i + 1);
		indices32.push_back(i);
	}

	//
	// Compute indices for inner stacks (not connected to poles).
	//

	// Offset the indices to the index of the first vertex in the first ring.
	// This is just skipping the top pole vertex.
	uint32 baseIndex = 1;
	uint32 ringVertexCount = sliceCount + 1;
	for (uint32 i = 0; i < stackCount - 2; ++i)
	{
		for (uint32 j = 0; j < sliceCount; ++j)
		{
			indices32.push_back(baseIndex + i * ringVertexCount + j);
			indices32.push_back(baseIndex + i * ringVertexCount + j + 1);
			indices32.push_back(baseIndex + (i + 1) * ringVertexCount + j);

			indices32.push_back(baseIndex + (i + 1) * ringVertexCount + j);
			indices32.push_back(baseIndex + i * ringVertexCount + j + 1);
			indices32.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
		}
	}

	//
	// Compute indices for bottom stack.  The bottom stack was written last to the vertex buffer
	// and connects the bottom pole to the bottom ring.
	//

	// South pole vertex was added last.
	uint32 southPoleIndex = (uint32)vertices.size() - 1;

	// Offset the indices to the index of the first vertex in the last ring.
	baseIndex = southPoleIndex - ringVertexCount;

	for (uint32 i = 0; i < sliceCount; ++i)
	{
		indices32.push_back(southPoleIndex);
		indices32.push_back(baseIndex + i);
		indices32.push_back(baseIndex + i + 1);
	}

	GenerateIndices16();
}

void Mesh::CreateCylinder(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount)
{
	//
	// Build Stacks.
	// 

	float stackHeight = height / stackCount;

	// Amount to increment radius as we move up each stack level from bottom to top.
	float radiusStep = (topRadius - bottomRadius) / stackCount;

	uint32 ringCount = stackCount + 1;

	// Compute vertices for each stack ring starting at the bottom and moving up.
	for (uint32 i = 0; i < ringCount; ++i)
	{
		float y = -0.5f * height + i * stackHeight;
		float r = bottomRadius + i * radiusStep;

		// vertices of ring
		float dTheta = 2.0f * DirectX::XM_PI / sliceCount;
		for (uint32 j = 0; j <= sliceCount; ++j)
		{
			Vertex vertex;

			float c = cosf(j * dTheta);
			float s = sinf(j * dTheta);

			vertex.position = TVector3(r * c, y, r * s);

			vertex.texcoord.x = (float)j / sliceCount;
			vertex.texcoord.y = 1.0f - (float)i / stackCount;

			// Cylinder can be parameterized as follows, where we introduce v
			// parameter that goes in the same direction as the v tex-coord
			// so that the bitangent goes in the same direction as the v tex-coord.
			//   Let r0 be the bottom radius and let r1 be the top radius.
			//   y(v) = h - hv for v in [0,1].
			//   r(v) = r1 + (r0-r1)v
			//
			//   x(t, v) = r(v)*cos(t)
			//   y(t, v) = h - hv
			//   z(t, v) = r(v)*sin(t)
			// 
			//  dx/dt = -r(v)*sin(t)
			//  dy/dt = 0
			//  dz/dt = +r(v)*cos(t)
			//
			//  dx/dv = (r0-r1)*cos(t)
			//  dy/dv = -h
			//  dz/dv = (r0-r1)*sin(t)

			// This is unit length.
			vertex.tangentU = TVector3(-s, 0.0f, c);

			float dr = bottomRadius - topRadius;
			TVector3 bitangent(dr * c, -height, dr * s);

			vertex.normal = vertex.tangentU.Cross(bitangent);
			vertex.normal.Normalize();

			vertices.push_back(vertex);
		}
	}

	// Add one because we duplicate the first and last vertex per ring
	// since the texture coordinates are different.
	uint32 ringVertexCount = sliceCount + 1;

	// Compute indices for each stack.
	for (uint32 i = 0; i < stackCount; ++i)
	{
		for (uint32 j = 0; j < sliceCount; ++j)
		{
			indices32.push_back(i * ringVertexCount + j);
			indices32.push_back((i + 1) * ringVertexCount + j);
			indices32.push_back((i + 1) * ringVertexCount + j + 1);

			indices32.push_back(i * ringVertexCount + j);
			indices32.push_back((i + 1) * ringVertexCount + j + 1);
			indices32.push_back(i * ringVertexCount + j + 1);
		}
	}

	BuildCylinderTopCap(bottomRadius, topRadius, height, sliceCount, stackCount);
	BuildCylinderBottomCap(bottomRadius, topRadius, height, sliceCount, stackCount);

	GenerateIndices16();
}

void Mesh::BuildCylinderTopCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount)
{
	uint32 baseIndex = (uint32)vertices.size();

	float y = 0.5f * height;
	float dTheta = 2.0f * DirectX::XM_PI / sliceCount;

	// Duplicate cap ring vertices because the texture coordinates and normals differ.
	for (uint32 i = 0; i <= sliceCount; ++i)
	{
		float x = topRadius * cosf(i * dTheta);
		float z = topRadius * sinf(i * dTheta);

		// Scale down by the height to try and make top cap texture coord area
		// proportional to base.
		float u = x / height + 0.5f;
		float v = z / height + 0.5f;

		vertices.push_back(Vertex(x, y, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
	}

	// Cap center vertex.
	vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

	// Index of center vertex.
	uint32 centerIndex = (uint32)vertices.size() - 1;

	for (uint32 i = 0; i < sliceCount; ++i)
	{
		indices32.push_back(centerIndex);
		indices32.push_back(baseIndex + i + 1);
		indices32.push_back(baseIndex + i);
	}
}

void Mesh::BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount)
{
	// 
	// Build bottom cap.
	//

	uint32 baseIndex = (uint32)vertices.size();
	float y = -0.5f * height;

	// vertices of ring
	float dTheta = 2.0f * DirectX::XM_PI / sliceCount;
	for (uint32 i = 0; i <= sliceCount; ++i)
	{
		float x = bottomRadius * cosf(i * dTheta);
		float z = bottomRadius * sinf(i * dTheta);

		// Scale down by the height to try and make top cap texcoord area
		// proportional to base.
		float u = x / height + 0.5f;
		float v = z / height + 0.5f;

		vertices.push_back(Vertex(x, y, z, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
	}

	// Cap center vertex.
	vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

	// Cache the index of center vertex.
	uint32 centerIndex = (uint32)vertices.size() - 1;

	for (uint32 i = 0; i < sliceCount; ++i)
	{
		indices32.push_back(centerIndex);
		indices32.push_back(baseIndex + i);
		indices32.push_back(baseIndex + i + 1);
	}
}

void Mesh::CreateGrid(float width, float depth, uint32 m, uint32 n)
{
	uint32 vertexCount = m * n;
	uint32 faceCount = (m - 1) * (n - 1) * 2;

	//
	// Create the vertices.
	//

	float halfWidth = 0.5f * width;
	float halfDepth = 0.5f * depth;

	float dx = width / (n - 1);
	float dz = depth / (m - 1);

	float du = 1.0f / (n - 1);
	float dv = 1.0f / (m - 1);

	vertices.resize(vertexCount);
	for (uint32 i = 0; i < m; ++i)
	{
		float z = halfDepth - i * dz;
		for (uint32 j = 0; j < n; ++j)
		{
			float x = -halfWidth + j * dx;

			vertices[i * n + j].position = TVector3(x, 0.0f, z);
			vertices[i * n + j].normal = TVector3(0.0f, 1.0f, 0.0f);
			vertices[i * n + j].tangentU = TVector3(1.0f, 0.0f, 0.0f);

			// Stretch texture over grid.
			vertices[i * n + j].texcoord.x = j * du;
			vertices[i * n + j].texcoord.y = i * dv;
		}
	}

	//
	// Create the indices.
	//

	indices32.resize(faceCount * 3); // 3 indices per face

	// Iterate over each quad and compute indices.
	uint32 k = 0;
	for (uint32 i = 0; i < m - 1; ++i)
	{
		for (uint32 j = 0; j < n - 1; ++j)
		{
			indices32[k] = i * n + j;
			indices32[k + 1] = i * n + j + 1;
			indices32[k + 2] = (i + 1) * n + j;

			indices32[k + 3] = (i + 1) * n + j;
			indices32[k + 4] = i * n + j + 1;
			indices32[k + 5] = (i + 1) * n + j + 1;

			k += 6; // next quad
		}
	}

	GenerateIndices16();
}

void Mesh::CreateQuad(float x, float y, float w, float h, float depth)
{
	vertices.resize(4);
	indices32.resize(6);

	// Position coordinates specified in NDC space.
	vertices[0] = Vertex(
		x, y - h, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f);

	vertices[1] = Vertex(
		x, y, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 0.0f);

	vertices[2] = Vertex(
		x + w, y, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f);

	vertices[3] = Vertex(
		x + w, y - h, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 1.0f);

	indices32[0] = 0;
	indices32[1] = 1;
	indices32[2] = 2;

	indices32[3] = 0;
	indices32[4] = 2;
	indices32[5] = 3;


	GenerateIndices16();
}

void Mesh::GenerateIndices16()
{
	if (indices32.size() > 0xFFFF)      // exceed range
	{
		// reserve 32 indices
		indices16.clear();
		return;
	}

	indices16.resize(indices32.size());
	for (size_t i = 0; i < indices32.size(); ++i)
		indices16[i] = static_cast<uint16_t>(indices32[i]);

}

const std::vector<std::uint16_t>& Mesh::GetIndices16() const
{
	return indices16;
}

std::string Mesh::GetInputLayoutName() const
{
	return inputLayoutName;
}

void Mesh::GenerateBoundingBox()
{
	boundingBox.Init(vertices);
}