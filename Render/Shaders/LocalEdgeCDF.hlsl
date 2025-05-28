#include "Utils.hlsl"

cbuffer CB_EnvCDF
{
    uint width;
    uint height;
    uint groupsPerRow;
    uint groupsPerColumn;
};

StructuredBuffer<float> GlobalRowSums;
RWStructuredBuffer<float> LocalColSums;
RWStructuredBuffer<float> ColumnPrefixSums;

groupshared float Prefix[64];

[numthreads(64, 1, 1)]
void CS(int3 tid : SV_GroupThreadID, int3 did : SV_DispatchThreadID)
{
    uint groupIndex = did.x / 64;
    
    bool active = (groupIndex + tid.x < height);

    if (active)
    {
        Prefix[tid.x] = GlobalRowSums[groupIndex + tid.x];
    }
    else
    {
        Prefix[tid.x] = 0.0f;
    }

    GroupMemoryBarrierWithGroupSync();

    // Hillis-Steele
    for (uint stride = 1; stride < 64; stride <<= 1)
    {
        GroupMemoryBarrierWithGroupSync();
        
        if (tid.x >= stride && tid.x < 64)
        {
            Prefix[tid.x] += Prefix[tid.x - stride];
        }
        
    }

    if (active)
    {
        ColumnPrefixSums[did.x] = Prefix[tid.x];
    }

    if (tid.x == 63)
    {
        LocalColSums[groupIndex] = Prefix[tid.x];
    }
}