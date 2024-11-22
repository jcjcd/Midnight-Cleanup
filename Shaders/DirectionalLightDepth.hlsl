#include "ShaderVariables.hlsl"
#include "SamplerStates.hlsl"

cbuffer cbPerLight
{
   //DirectionalLight directionalLights[MAX_LIGHTS];
    DirectionalLight directionalLights;
    int useAlphaMap;

   //unsigned int numDirectionalLights;
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
    float4 PosW : SV_POSITION;
    float2 TexC : TEXCOORD0;
};

Texture2D gAlpha;

GeometryIn VSMain(VertexIn vin)
{
    GeometryIn vout;
    
    // Transform to world space
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW;

    vout.TexC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform).xy;

    
    return vout;
}

struct GeometryOut
{
    float4 PosH : SV_POSITION;
    float2 TexC : TEXCOORD0;
    uint RTIndex : SV_RenderTargetArrayIndex;
};

[maxvertexcount(12)]
void GSMain(triangle GeometryIn gin[3], inout TriangleStream<GeometryOut> gout)
{
    //for(uint lightIdx = 0; lightIdx < numDirectionalLights; ++lightIdx)
    //{
    //    if(directionalLights[lightIdx].isOn == 0)
    //        continue;
    //
    //    if(directionalLights[lightIdx].useShadow == 0)
    //        continue;
    //
    //   for(uint cascadeIdx = 0; cascadeIdx < 3; ++cascadeIdx)
    //    {
    //        GeometryOut goutData;
    //
    //        for(int i = 0; i < 3; i++)
    //        {
    //            goutData.PosH = mul(gin[i].PosW, directionalLights[lightIdx].viewProj[cascadeIdx]);
    //            goutData.RTIndex = lightIdx * 3 + cascadeIdx;   
    //            gout.Append(goutData);
    //        }
    //
    //        gout.RestartStrip();
    //    }
    //}


    if (directionalLights.isOn == 0)
        return;
    
    if (directionalLights.useShadow == 0)
        return;
    
    for (uint cascadeIdx = 0; cascadeIdx < 4; ++cascadeIdx)
    {
        GeometryOut goutData;
    
        for (int i = 0; i < 3; i++)
        {
            goutData.PosH = mul(gin[i].PosW, directionalLights.viewProj[cascadeIdx]);
            goutData.TexC = gin[i].TexC;
            goutData.RTIndex = cascadeIdx;
            gout.Append(goutData);
        }
    
        gout.RestartStrip();
    }

}

float4 PSMain(GeometryOut pin) : SV_TARGET
{
    if (useAlphaMap == 0)
    {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    float4 opacity = gAlpha.Sample(gsamAnisotropicWrap, pin.TexC);
    clip(opacity.a - 0.333f);
    return float4(pin.PosH.z / pin.PosH.w, pin.PosH.z / pin.PosH.w, pin.PosH.z / pin.PosH.w, pin.PosH.z / pin.PosH.w);

}