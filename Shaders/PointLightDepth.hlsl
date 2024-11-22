#include "ShaderVariables.hlsl"

cbuffer cbPerLight
{
	PointLight pointLights[MAX_LIGHTS];

	unsigned int numPointLights;
};

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float3 TangentL : TANGENT;
    float2 TexC : TEXCOORD0;
};

struct GeometryIn
{
    float4 PosH : SV_POSITION;
    float4 PosW : POSITION;
    float2 TexC : TEXCOORD0;
};

GeometryIn VSMain(VertexIn vin)
{
    GeometryIn vout;
    
    // Transform to world space
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW;

    vout.PosH = mul(posW, gViewProj);
    vout.TexC = vin.TexC;
    
    return vout;
}

struct GeometryOut
{
    float4 PosH : SV_POSITION;
    float4 PosW : POSITION;
    uint RTIndex : SV_RenderTargetArrayIndex;
};

[maxvertexcount(54)]
void GSMain(triangle GeometryIn gin[3], inout TriangleStream<GeometryOut> gout)
{
    for (uint lightIdx = 0; lightIdx < numPointLights; ++lightIdx)
    {
        if(pointLights[lightIdx].isOn == 0)
            continue;

        if(pointLights[lightIdx].useShadow == 0)
            continue;

        for(uint faceIndex = 0; faceIndex < 6; ++faceIndex)
        {
            GeometryOut goutData;
            goutData.RTIndex = lightIdx * 6 + faceIndex;

            for(int i = 0; i < 3; i++)
            {
                goutData.PosW = gin[i].PosW;

                goutData.PosH = mul(gin[i].PosW, pointLights[lightIdx].viewProj[faceIndex]);

                gout.Append(goutData);
            }

            gout.RestartStrip();
        }
    }
}

float4 PSMain(GeometryOut pin) : SV_TARGET
{
    float3 lightVec = pin.PosW.xyz - pointLights[pin.RTIndex / 6].position;
    float lightDistance = length(lightVec);
    lightDistance = lightDistance / pointLights[pin.RTIndex / 6].range;
    return float4(lightDistance, lightDistance, lightDistance, 1.0f);
}