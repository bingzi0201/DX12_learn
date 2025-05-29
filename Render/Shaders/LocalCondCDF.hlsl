#include "Common.hlsl"

Texture2D EquirectangularMap;
RWTexture2D<float2> EnvironmentCDF_Out;
RWStructuredBuffer<float> LocalRowSums;

groupshared float Prefix[64];

[numthreads(64, 1, 1)]
void CS(int3 tid : SV_GroupThreadID, int3 did : SV_DispatchThreadID)
{
    GroupMemoryBarrierWithGroupSync();

    bool active = (did.x < width && did.y < height);

    // compute weight
    float lumi = 0.0f;
    if (active)
    {
        float3 rgb = EquirectangularMap[did.xy];
        float2 uv = (did.xy + 0.5) / float2(width, height);
        uv.y = clamp(uv.y, 0.0, 1.0 - 1e-5);
        float theta = PI * uv.y;
        lumi = dot(rgb, float3(0.2126, 0.7152, 0.0722)) * sin(theta);
    }

    // Hillis-Steele
    Prefix[tid.x] = active ? lumi :0;
    GroupMemoryBarrierWithGroupSync();

    for (uint stride = 1; stride < 64; stride <<= 1)
    {
        GroupMemoryBarrierWithGroupSync();

        if (tid.x >= stride && tid.x < 64)
        {
            Prefix[tid.x] += Prefix[tid.x - stride];
        }
    }

    GroupMemoryBarrierWithGroupSync();
    
    if (tid.x == 63 && active)
    {
        uint groupID = did.x / 64;
        uint bufferIndex = did.y * groupsPerRow + groupID;
        LocalRowSums[bufferIndex] = Prefix[63];
    }
    
    if (active && tid.x < 256)
    {
        EnvironmentCDF_Out[did.xy] = float2(Prefix[tid.x], 0.0f);
    }
}