#ifndef __SHADER_LIGHTUTIL__
#define __SHADER_LIGHTUTIL__

#include "Utils.hlsl"

struct LightParameters
{
    float3 Color; // All light
    float Intensity; // All light
    float3 Position; // Point/Spot light only
    float Range; // Point/Spot light only
    float3 Direction; // Directional/Spot light only
    float SpotRadius; // Spot light only
    float2 SpotAngles; // Spot light only
    uint LightType;
    int ShadowMapIdx;
    
    float4x4 LightProj;
    float4x4 ShadowTransform;
    
    // AreaLight
    float3 AreaLightPoint0InWorld;
    float LightPad2;
    float3 AreaLightPoint1InWorld;
    float LightPad3;
    float3 AreaLightPoint2InWorld;
    float LightPad4;
    float3 AreaLightPoint3InWorld;
    float LightPad5;
};

cbuffer LightCommonData
{
    uint LightCount;
};

#define TILE_BLOCK_SIZE 16
#define MAX_LIGHT_COUNT_IN_TILE 500
struct TileLightInfo
{
    uint LightIndices[MAX_LIGHT_COUNT_IN_TILE];
    uint LightCount;
};

struct Material
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Shininess;
};


float CalcDistanceAttenuation(float3 WorldPos, float3 LightPos, float Range)
{
    float3 WorldLightVector = LightPos - WorldPos;
    float DistanceSqr = dot(WorldLightVector, WorldLightVector);
    
    float DistanceAttenuation = 1 / (DistanceSqr + 1);
    
    float InvRange = rcp(max(Range, 0.001f));
    float LightRadiusMask = Square(saturate(1 - Square(DistanceSqr * InvRange * InvRange)));
    DistanceAttenuation *= LightRadiusMask;
    
    return DistanceAttenuation;
}

/**
 * LightDir is the direction from shader point to spot light.
 * SpotDirection is the direction of the spot light.
 * SpotAngles.x is CosOuterCone, SpotAngles.y is InvCosConeDifference. 
*/
float SpotAttenuation(float3 LightDir, float3 SpotDirection, float2 SpotAngles)
{
    float ConeAngleFalloff = Square(saturate((dot(-LightDir, SpotDirection) - SpotAngles.x) * SpotAngles.y));
    return ConeAngleFalloff;
}

#endif //__SHADER_LIGHTUTIL__