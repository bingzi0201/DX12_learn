#include "Common.hlsl"

StructuredBuffer<float> ColumnPrefixSums;
StructuredBuffer<float> GroupColOffset;
Texture2D<float2> EnvironmentCDF_In;
RWTexture2D<float2> EnvironmentCDF_Out;

[numthreads(1, 64, 1)]
void CS(int3 tid : SV_GroupThreadID, int3 did : SV_DispatchThreadID)
{
    uint column = did.x;
    uint groupID = did.y / 64;
    
    float offset = GroupColOffset[groupID];

    float2 cdfData = EnvironmentCDF_In[did.xy];

    float globalPrefix = ColumnPrefixSums[did.y] + offset;

    float total = GroupColOffset[groupsPerColumn - 1] + ColumnPrefixSums[height - 1];

    float normalized = globalPrefix / total;
    
    EnvironmentCDF_Out[did.xy] = float2(cdfData.x, normalized);
}