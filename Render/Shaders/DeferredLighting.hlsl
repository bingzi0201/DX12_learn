#include "PBRLighting.hlsl"
#include "Common.hlsl"
#include "LightingUtil.hlsl"

StructuredBuffer<LightParameters> Lights;

Texture2D BaseColorGbuffer;
Texture2D NormalGbuffer;
Texture2D WorldPosGbuffer;
Texture2D OrmGbuffer;
Texture2D EmissiveGbuffer;
TextureCube IBLIrradianceMap;

// for IBL
#define IBL_PREFILTER_ENVMAP_MIP_LEVEL 5
TextureCube IBLPrefilterEnvMaps[IBL_PREFILTER_ENVMAP_MIP_LEVEL];
Texture2D BrdfLUT;

#define DIRECTIONAL_LIGHT_PIXEL_WIDTH       5.0f
#define SPOT_LIGHT_PIXEL_WIDTH              10.0f


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

float3 GetPrefilteredColor(float Roughness, float3 ReflectDir)
{
    float Level = Roughness * (IBL_PREFILTER_ENVMAP_MIP_LEVEL - 1);
    int FloorLevel = floor(Level);
    int CeilLevel = ceil(Level);

    float3 FloorSample = IBLPrefilterEnvMaps[FloorLevel].SampleLevel(gsamLinearClamp, ReflectDir, 0).rgb;
    float3 CeilSample = IBLPrefilterEnvMaps[CeilLevel].SampleLevel(gsamLinearClamp, ReflectDir, 0).rgb;
	
    float3 PrefilteredColor = lerp(FloorSample, CeilSample, (Level - FloorLevel));
    return PrefilteredColor;
}

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
    
    float3 finalColor = 0.0f;

	// Get Gbuffer data
    float3 BaseColor = BaseColorGbuffer.Sample(gsamPointClamp, pin.TexC).rgb;
    float3 Normal = NormalGbuffer.Sample(gsamPointClamp, pin.TexC).rgb;
    float3 WorldPos = WorldPosGbuffer.Sample(gsamPointClamp, pin.TexC).rgb;
    float ShadingModelValue = WorldPosGbuffer.Sample(gsamPointClamp, pin.TexC).a;
    uint ShadingModel = (uint) round(ShadingModelValue * (float) 0xF);
    float Roughness = OrmGbuffer.Sample(gsamPointClamp, pin.TexC).g;
    float Metallic = OrmGbuffer.Sample(gsamPointClamp, pin.TexC).b;
    

    if(ShadingModel == 0)  // Lit
    {        
        
        float3 CameraPosition = gEyePosW;
        float3 ViewDir = normalize(CameraPosition - WorldPos);
        Normal = normalize(Normal);
        float AmbientAccess = 1.0f;
        
        //-------------------------------------------IBL Enviroment Light-----------------------------------
        // Irradiance
        float3 Irradiance = IBLIrradianceMap.SampleLevel(gsamLinearClamp, Normal, 0).rgb;
        //return float4(Irradiance, 1.0f);
        
        // PrefilteredColor
        float3 ReflectDir = reflect(-ViewDir, Normal);
        float3 PrefilteredColor = GetPrefilteredColor(0.5f, ReflectDir);
        //return float4(PrefilteredColor, 1.0f);
            
        // LUT value
        float NoV = dot(Normal, ViewDir);
        float2 LUT = BrdfLUT.SampleLevel(gsamLinearClamp, float2(NoV, Roughness), 0).rg;
            
        finalColor += AmbientLighting(Metallic, BaseColor, Irradiance, PrefilteredColor, LUT, AmbientAccess);
        
        //-----------------------------------------------Direct Light---------------------------------------
	
        float3 LightDir = normalize(-Lights[0].Direction);
        float3 LightPosition = WorldPos + LightDir * 100.0f;
        float TanLightAngle = tan(3 * PI / 180.0f); //TODO	
        float3 Radiance = Lights[0].Intensity * Lights[0].Color;
    
        finalColor += DirectLighting(Radiance, LightDir, Normal, ViewDir, Roughness, Metallic, BaseColor);
        
    }
    
    else if (ShadingModel == 1) //Unlit
    {
        finalColor = BaseColor;
    }
	
    return float4(finalColor, 1.0f);
}