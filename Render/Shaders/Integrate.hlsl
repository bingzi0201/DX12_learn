#include "Common.hlsl"

Texture2D CDFTex;
Texture2D BlueNoiseTex;
TextureCube EnvironmentMap;

cbuffer CB_Frame
{
    uint pixelCount;
    uint frameIndex;
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

// from 128*128 to [0,1)
float2 BlueNoise(uint2 pix)
{
    uint2 coord = (pix + frameIndex) & 127;
    return BlueNoiseTex.Load(int3(coord, 0)).rg;
}

float SolidAngle(int x, int y)
{
    float inverseHeight = 1 / height;
    float inverseWidth = 1 / width;
    
    float dTheta = PI * inverseHeight;
    float dPhi = 2 * PI * inverseWidth;
    float theta0 = PI * y * inverseHeight;
    float theta1 = PI * (y + 1.0) * inverseHeight;
    return dPhi * (cos(theta0) - cos(theta1));
}

void SampleEnvCDF(float2 xi, out uint face,
                  out uint x, out uint y,
                  out float pdf_env, out float3 dir_w)
{

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
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}