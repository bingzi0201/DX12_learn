#ifndef __SHADER_COMMON__
#define __SHADER_COMMON__

#include "Utils.hlsl"
#include "Sampler.hlsl"

// Constant data that varies per frame.
cbuffer cbPerObject
{
    float4x4 gWorld;
    float4x4 gPrevWorld;
    float4x4 gTexTransform;
};

cbuffer cbPass
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float4x4 gPrevViewProj;
    float3 gEyePosW;
    float cbPassPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
};

struct MaterialData
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Roughness;
    float4x4 MatTransform;
	
    float3 EmissiveColor;
    uint ShadingModel;
};

cbuffer cbMaterialData
{
    MaterialData gMaterial;
};

cbuffer CB_EnvCDF
{
    uint width;
    uint height;
    uint groupsPerRow;
    uint groupsPerColumn;
};

#endif //__SHADER_COMMON__