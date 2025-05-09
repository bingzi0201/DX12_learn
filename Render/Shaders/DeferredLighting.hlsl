#include "PBRLighting.hlsl"
#include "Common.hlsl"
#include "LightingUtil.hlsl"

StructuredBuffer<LightParameters> Lights;

Texture2D BaseColorGbuffer;
Texture2D NormalGbuffer;
Texture2D WorldPosGbuffer;
Texture2D OrmGbuffer;
Texture2D EmissiveGbuffer;

#define DIRECTIONAL_LIGHT_PIXEL_WIDTH       5.0f
#define SPOT_LIGHT_PIXEL_WIDTH              10.0f

cbuffer cbDeferredLighting
{
    uint EnableSSAO;
};

struct VertexIn
{
    float3 PosL : POSITION;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 TexC : TEXCOORD;
};


VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;

	// Already in homogeneous clip space.
    vout.PosH = float4(vin.PosL, 1.0f);

    vout.TexC = vin.TexC;

    return vout;
}

float4 PS(VertexOut pin) : SV_TARGET
{
    //return float4(float(lights[0].lighttype)/4, 0.0f, 0.0f, 1);
    //return float4(0.5, 0.0f, 0.0f, 1);
    
    float3 FinalColor = 0.0f;

	// Get Gbuffer data
    float3 BaseColor = BaseColorGbuffer.Sample(gsamPointClamp, pin.TexC).rgb;
    float3 Normal = NormalGbuffer.Sample(gsamPointClamp, pin.TexC).rgb;
    float3 WorldPos = WorldPosGbuffer.Sample(gsamPointClamp, pin.TexC).rgb;
    float ShadingModelValue = WorldPosGbuffer.Sample(gsamPointClamp, pin.TexC).a;
    uint ShadingModel = (uint) round(ShadingModelValue * (float) 0xF);
    float Roughness = OrmGbuffer.Sample(gsamPointClamp, pin.TexC).g;
    float Metallic = OrmGbuffer.Sample(gsamPointClamp, pin.TexC).b;
    //float3 EmissiveColor = EmissiveGbuffer.Sample(gsamPointClamp, pin.TexC).rgb;
	
    float3 CameraPosition = gEyePosW;
    float3 ViewDir = normalize(CameraPosition - WorldPos);
    Normal = normalize(Normal);
	
    float3 LightDir = normalize(-Lights[0].Direction);
    float3 LightPosition = WorldPos + LightDir * 100.0f;			
    float TanLightAngle = tan(3 * PI / 180.0f); //TODO	
    float3 Radiance = Lights[0].Intensity * Lights[0].Color;
    
    FinalColor += DirectLighting(Radiance, LightDir, Normal, ViewDir, Roughness, Metallic, BaseColor);

    // return float4(LightDir, 1.0f);
    return float4(FinalColor, 1.0f);
	
}