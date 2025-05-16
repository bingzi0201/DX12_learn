#include "Common.hlsl"

TextureCube EnvironmentMap;

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
    float3 TangentU : TANGENT;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosL : TEXCOORD0;
};

VertexOut VS(VertexIn vin)
{
    VertexOut Out;

	// Use local vertex position as cubemap lookup vector.
    Out.PosL = vin.PosL;
	
	// Remove translation from the view matrix
    float4x4 view = gView;
    view[3][0] = view[3][1] = view[3][2] = 0.0f;
	
    Out.PosH = mul(mul(float4(vin.PosL, 1.0f), view), gProj);
		
    return Out;
}

float4 PS(VertexOut pin) : SV_TARGET
{
    float3 irradiance = float3(0.0f, 0.0f, 0.0f);
    float3 integral = float3(0.0f, 0.0f, 0.0f);
    
    float3 normal = normalize(pin.PosL);
    float3 right = float3(0.0f, 1.0f, 0.0f);
    float3 up = cross(normal, right);
    
    float sampleStep = 0.25f;
    float count = 0.0f;
    for (float phi = 0.0f; phi <= 2.0f * PI; phi += sampleStep)
    {
        for (float theta = 0.0f; theta <= 0.5f * PI; theta += sampleStep)
        {
            // Spherical to cartesian (in tangent space)
            float3 tangentSample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
			// Tangent space to world
            float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;
			
            integral += EnvironmentMap.Sample(gsamAnisotropicWrap, sampleVec).rgb * cos(theta) * sin(theta);
            count++;
        }
    }
    
    irradiance = PI * (1.0f / count) * integral;
    
    return float4(irradiance, 1.0f);
}