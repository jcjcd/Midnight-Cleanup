#include "ShaderVariables.hlsl"

cbuffer cbPerLight
{
   SpotLight spotLights[MAX_LIGHTS];

   unsigned int numSpotLights;
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
};

GeometryIn VSMain(VertexIn vin)
{
    GeometryIn vout;
    
    // Transform to world space
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW;

    vout.PosH = mul(posW, gViewProj);
    
    return vout;
}

struct GeometryOut
{
    float4 PosH : SV_POSITION;
    uint RTIndex : SV_RenderTargetArrayIndex;
};

[maxvertexcount(30)]
void GSMain(triangle GeometryIn gin[3], inout TriangleStream<GeometryOut> gout)
{  
    for(uint lightIdx = 0; lightIdx < numSpotLights; ++lightIdx)
    {
        if(spotLights[lightIdx].isOn == 0)
            continue;

        if(spotLights[lightIdx].useShadow == 0)
            continue;

        for(uint faceIndex = 0; faceIndex < 3; ++faceIndex)
        {
            GeometryOut goutData;
            goutData.RTIndex = lightIdx;

            for(int i = 0; i < 3; i++)
            {
                float4 posLight = mul(gin[i].PosW, spotLights[lightIdx].viewProj);
                posLight /= posLight.w;
                posLight.z = saturate(posLight.z);
                goutData.PosH = posLight;
                gout.Append(goutData);
            }

            gout.RestartStrip();
        }
    }
}