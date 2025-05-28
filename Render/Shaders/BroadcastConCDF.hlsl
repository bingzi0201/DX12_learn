#include "Utils.hlsl"

cbuffer CB_EnvCDF
{
    uint width;
    uint height;
    uint groupsPerRow;
    uint groupsPerColumn;
};

StructuredBuffer<float> GroupPrefixSums;
Texture2D<float2> EnvironmentCDF_In;
RWTexture2D<float2> EnvironmentCDF_Out;
StructuredBuffer<float> GlobalRowSums;

[numthreads(64, 1, 1)]
void CS(int3 tid : SV_GroupThreadID, int3 did : SV_DispatchThreadID)
{
    uint row = did.y;
    uint groupID = did.x / 64;

    uint groupIndex = row * groupsPerRow + groupID;

    float offset = GroupPrefixSums[groupIndex];

    float2 cdfData = EnvironmentCDF_In[did.xy];

    float globalPrefix = cdfData.x + offset;
    
    float total = max(GlobalRowSums[row], 1e-6f);

    float normalized = globalPrefix / total;
    
    EnvironmentCDF_Out[did.xy] = float2(normalized, 0.0f);
    
}