#include "ShaderVariables.hlsl"
#include "SamplerStates.hlsl"

struct VertexIn
{
	float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
	float2 TexC    : TEXCOORD;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
	float2 TexC    : TEXCOORD;
};

VertexOut VSMain(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
    
    vout.PosH = float4(vin.PosL, 1.0f);
    vout.NormalW = vin.NormalL; 
    vout.TangentW = vin.NormalL;
    vout.TexC = vin.TexC;
    
    return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    return float4(pin.NormalW, 1.f);

}


