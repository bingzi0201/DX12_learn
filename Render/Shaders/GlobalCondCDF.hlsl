#include "Utils.hlsl"

cbuffer CB_EnvCDF
{
    uint width;
    uint height;
    uint groupsPerRow;
    uint groupsPerColumn;
};
StructuredBuffer<float> LocalRowSums;
RWStructuredBuffer<float> GroupPrefixSums;
RWStructuredBuffer<float> GlobalRowSums;

groupshared float sharedSums[64];

[numthreads(64, 1, 1)]
void CS(uint3 id : SV_DispatchThreadID)
{

    uint row = id.y;
    uint tid = id.x;

    uint localRowStart = row * groupsPerRow;

    if (tid < groupsPerRow)
    {
        sharedSums[tid] = LocalRowSums[localRowStart + tid];
    }
    else
    {
        sharedSums[tid] = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    // Hillis-Steele
    for (uint stride = 1; stride < 64; stride <<= 1)
    {
        if (tid >= stride && tid < groupsPerRow)
        {
            sharedSums[tid] += sharedSums[tid - stride];
        }
        GroupMemoryBarrierWithGroupSync();
    }

    GroupMemoryBarrierWithGroupSync();

    if (tid < groupsPerRow)
    {
        float prefixSum = (tid == 0) ? 0.0f : sharedSums[tid - 1];
        GroupPrefixSums[localRowStart + tid] = prefixSum;
    }
    
    if (tid.x == groupsPerRow - 1)
    {
        GlobalRowSums[row] = sharedSums[tid];
    }
}