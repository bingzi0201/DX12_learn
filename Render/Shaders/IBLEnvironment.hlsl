#include "Common.hlsl"

Texture2D EquirectangularMap;

struct VertexIn
{
    float3 PosL : POSITION;
    float3 Normal : NORMAL;
    float3 TexC : TEXCOORD;
    float2 TangentU : TANGENT;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosL : TEXCOORD0;
};

VertexOut VS(VertexIn vin)
{
    VertexOut Out;
    
    Out.PosL = vin.PosL;
    float4x4 View = gView;
    View[3][0] = View[3][1] = View[3][2] = 0.0f; // remove translation
    Out.PosH = mul(mul(float4(vin.PosL, 1.0f), View), gProj);
    
    return Out;
}

float2 SampleSphericalMap(float3 v)
{
    // (1/2PI, 1/PI)
    const float2 invAtan = float2(0.1591, 0.3183);
    
    float2 uv = float2(atan2(v.z, v.x), asin(v.y));
    uv *= invAtan; // normalize
    uv += 0.5;
    return uv;
}

float4 PS(VertexOut pin) : SV_TARGET
{
    float2 uv = SampleSphericalMap(normalize(pin.PosL));
    float3 color = EquirectangularMap.SampleLevel(gsamLinearClamp, uv, 0).rgb;
    return float4(color, 1.0f);
}