#include "SamplerStates.hlsl"


Texture2D uvTexture : register(t0);
Texture2D maskTexture : register(t1);
Texture2D positionFromTexture : register(t2);
Texture2D positionToTexture : register(t3);
Texture2D backgroundColorTexture : register(t4);

cbuffer RefractionAttributes : register(b0)
{
    float4x4 lensView;
    float2 pi;
    float2 gamma;
    float2 sunPosition;
}

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float3 TangentL : TANGENT;
    float2 TexC : TEXCOORD0;
};

struct VertexOut
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;
    
    vout.position = float4(vin.PosL, 1.0f);
	
    // Output texture coordinates
    vout.texCoord = vin.TexC;

    return vout;
}


float4 PSMain(VertexOut input) : SV_TARGET
{
    //float4 tintColor = float4(0.392, 0.537, 0.561, 0.8);
    float4 tintColor = float4(0.112, 0.117, 0.961, 0.1);
    float depthMax = 0.5f;

    float2 texCoord = input.texCoord;

    float4 backgroundColor = backgroundColorTexture.Sample(gsamLinearClamp, texCoord);

    float4 mask = maskTexture.Sample(gsamLinearClamp, texCoord);
    if (mask.b <= 0)
    {
        return backgroundColor;
    }

    float4 uv = uvTexture.Sample(gsamLinearClamp, texCoord);
    if (uv.b <= 0)
    {
        return backgroundColor;
    }

    tintColor.rgb = pow(tintColor.rgb, float3(gamma.x, gamma.x, gamma.x));
    tintColor.rgb *= max(0.2, -1 * sin(sunPosition.x * pi.y));

    float4 positionFrom = positionFromTexture.Sample(gsamLinearClamp, texCoord);
    positionFrom.w = 1.0;
    positionFrom = mul(positionFrom, lensView);
    float4 positionTo = positionToTexture.Sample(gsamLinearClamp, uv.xy);
    positionTo.w = 1.0;
    positionTo = mul(positionTo, lensView);
    backgroundColor = backgroundColorTexture.Sample(gsamLinearClamp, uv.xy);

    float depth = length(positionTo.xyz - positionFrom.xyz);
    float mixture = clamp(depth / depthMax, 0.0, 1.0);

    float3 shallowColor = backgroundColor.rgb;
    float3 deepColor = lerp(shallowColor, tintColor.rgb, tintColor.a);
    float3 foregroundColor = lerp(shallowColor, deepColor, mixture);

    return lerp(float4(0.0, 0.0, 0.0, 0.0), float4(foregroundColor, 1.0), uv.b);
}