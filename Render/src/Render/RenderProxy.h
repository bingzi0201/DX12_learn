#pragma once

#include <string>
#include "../Shader/Shader.h"
#include "../Texture/Texture.h"
#include "../Resource/Resource.h"
#include "../Resource/View.h"
#include "../Math/Math.h"

struct MaterialConstants
{
public:
	TVector4 diffuseAlbedo;
	TVector3 fresnelR0;
	float roughness;

	// Used in texture mapping.
	TMatrix matTransform = TMatrix::Identity;

	TVector3 emissiveColor;
	UINT shadingModel;
};

// Defines a subrange of geometry in a TMeshProxy.  This is for when multiple
// geometries are stored in one vertex and index buffer.  It provides the offsets
// and data needed to draw a subset of geometry stores in the vertex and index 
// buffers
struct SubmeshProxy
{
	UINT indexCount = 0;
	UINT startIndexLocation = 0;
	INT baseVertexLocation = 0;

	// Bounding box of the geometry defined by this submesh. 
	DirectX::BoundingBox bounds;
};

struct CB_EnvCDF
{
	UINT width;
	UINT height;
	UINT groupsPerRow;
	UINT groupsPerColumn;
};

struct MeshProxy
{
	// Give it a name so we can look it up by name.
	std::string Name;

	VertexBufferRef vertexBufferRef;
	IndexBufferRef indexBufferRef;

	ASBufferRef blasBufferRef;

	// Data about the buffers.
	UINT vertexByteStride = 0;
	UINT vertexBufferByteSize = 0;
	DXGI_FORMAT indexFormat = DXGI_FORMAT_R16_UINT;
	UINT indexBufferByteSize = 0;

	// A MeshGeometry may store multiple geometries in one vertex/index buffer.
	// Use this container to define the Submesh geometries so we can draw
	// the Submeshes individually.
	std::unordered_map<std::string, SubmeshProxy> subMeshs;

};

struct LightShaderParameters
{
	TVector3 color;       // All light
	float    intensity;   // All light
	TVector3 position;    // Point/Spot light only
	float    range;       // Point/Spot light only
	TVector3 direction;   // Directional/Spot light only
	float    spotRadius;  // Spot light only
	TVector2 spotAngles;  // Spot light only
	UINT     lightType;
	INT      shadowMapIdx = 0;

	TMatrix lightProj = TMatrix::Identity;
	TMatrix  shadowTransform = TMatrix::Identity;

};
static_assert(sizeof(LightShaderParameters) % 16 == 0, "must be 16-byte aligned");

struct LightCommonData
{
	UINT lightCount = 0;
};

#define MAX_LIGHT_COUNT_IN_TILE 500

struct PassConstants
{
	TMatrix View = TMatrix::Identity;
	TMatrix InvView = TMatrix::Identity;
	TMatrix Proj = TMatrix::Identity;
	TMatrix InvProj = TMatrix::Identity;
	TMatrix ViewProj = TMatrix::Identity;
	TMatrix InvViewProj = TMatrix::Identity;
	TMatrix PrevViewProj = TMatrix::Identity;
	TVector3 EyePosW = { 0.0f, 0.0f, 0.0f };
	float cbPassPad1 = 0.0f;
	TVector2 RenderTargetSize = { 0.0f, 0.0f };
	TVector2 InvRenderTargetSize = { 0.0f, 0.0f };
	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;

	TVector4 FogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
	float gFogStart = 5.0f;
	float gFogRange = 150.0f;
	TVector2 cbPassPad2;
};

struct ObjectConstants
{
	TMatrix World = TMatrix::Identity;
	TMatrix PrevWorld = TMatrix::Identity;
	TMatrix TexTransform = TMatrix::Identity;
};

struct PrefilterEnvironmentConstant
{
	TMatrix view = TMatrix::Identity;
	TMatrix proj = TMatrix::Identity;
	float roughness;
};
