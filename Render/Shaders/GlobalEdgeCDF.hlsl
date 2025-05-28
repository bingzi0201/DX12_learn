#include "Utils.hlsl"

cbuffer CB_EnvCDF
{
    uint width;
    uint height;
    uint groupsPerRow;
    uint groupsPerColumn;
};

StructuredBuffer<float> LocalColSums;
RWStructuredBuffer<float> GroupColOffset;

groupshared float sharedSums[64];

[numthreads(64, 1, 1)]
void CS(int3 tid : SV_GroupThreadID, int3 did : SV_DispatchThreadID)
{
    if (tid.x < groupsPerColumn)
    {
        sharedSums[tid.x] = LocalColSums[tid.x];
    }
    else
    {
        sharedSums[tid.x] = 0;
    }
    
    GroupMemoryBarrierWithGroupSync();

    // Hillis-Steele
    for (uint stride = 1; stride < 64; stride <<= 1)
    {
        if (tid.x >= stride && tid.x < groupsPerColumn)
        {
            sharedSums[tid.x] += sharedSums[tid.x - stride];
        }
        GroupMemoryBarrierWithGroupSync();
    }
    
    if (tid.x < groupsPerColumn)
    {
        float prefixSum = (tid.x == 0) ? 0.0f : sharedSums[tid.x - 1];
        GroupColOffset[tid.x] = prefixSum;
    }
  
}