#include "ShaderVariables.hlsl"
#include "SamplerStates.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float3 TangentL : TANGENT;
    float2 TexC : TEXCOORD0;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
	float2 TexC    : TEXCOORD;
};

Texture2D gDiffuseMap;

VertexOut VSMain(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
	
    // Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);

    // Transform to homogeneous clip space.
    vout.PosH = mul(posW, gViewProj);
	
    // Output texture coordinates
    vout.TexC = vin.TexC;

    return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    float4 litColor = gDiffuseMap.Sample(gsamLinearClamp, pin.TexC);
    litColor.a = 1.0f;
    return litColor;
}


