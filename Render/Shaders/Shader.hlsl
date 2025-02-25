Texture2D t1 : register(t0);
SamplerState s1 : register(s0);

cbuffer SceneConstantBuffer : register(b0)
{
   float4x4 WorldViewProj;
}

struct VertexIn
{
    float4 PositionL : POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD;
};

struct VertexOut
{
    float4 positionH : SV_Position;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD;
};

VertexOut VS(VertexIn vertex)
{
    VertexOut vout;
    vout.positionH = mul(WorldViewProj, vertex.PositionL);
    vout.color = vertex.color;
    vout.texCoord = vertex.texCoord;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return t1.Sample(s1, pin.texCoord);
    //return pin.color;
}