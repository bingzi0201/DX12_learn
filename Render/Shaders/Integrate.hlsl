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

/*float SolidAngle(int x, int y)
{
    float invN = 1.0 / pixelCount;
    float u0 = (x) * 2 * invN - 1;
    float v0 = (y) * 2 * invN - 1;
    float u1 = ((x + 1)) * 2 * invN - 1;
    float v1 = ((y + 1)) * 2 * invN - 1;

    float f00 = atan2(u0 * v0, sqrt(u0 * u0 + v0 * v0 + 1));
    float f01 = atan2(u0 * v1, sqrt(u0 * u0 + v1 * v1 + 1));
    float f10 = atan2(u1 * v0, sqrt(u1 * u1 + v0 * v0 + 1));
    float f11 = atan2(u1 * v1, sqrt(u1 * u1 + v1 * v1 + 1));
    return f11 - f10 - f01 + f00; // ΔΩ
}*/

float SolidAngle(int x, int y)
{
    float invN = 1.0f / pixelCount;
    float u = (x + 0.5f) * 2.0f * invN - 1.0f;
    float v = (y + 0.5f) * 2.0f * invN - 1.0f;
    
    float boundary = max(abs(u), abs(v)); // [0,1]
    float correction = 1.0f + 0.1f * boundary * boundary;
    
    float distortion = 1.0f + u * u + v * v;
    return 4.0f * invN * invN / (distortion * sqrt(distortion)) * correction;
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