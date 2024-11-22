// (C) 2019 David Lettier
// lettier.com
#include "SamplerStates.hlsl"

Texture2D colorTexture : register(t0);
Texture2D colorBlurTexture : register(t1);
Texture2D ormTexture : register(t2);

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

float4 PSMain(VertexOut input) : SV_Target
{
    float2 texSize;
    colorTexture.GetDimensions(texSize.x, texSize.y);

    float2 texCoord = input.position.xy / texSize;

    float roughness = ormTexture.Sample(gsamLinearClamp, texCoord).g;
    
    float4 color = colorTexture.Sample(gsamLinearClamp, texCoord);
    float4 colorBlur = colorBlurTexture.Sample(gsamLinearClamp, texCoord);

    float amount = 1.0;

    if (amount <= 0.0)
    {
        return float4(0.0, 0.0, 0.0, 0.0);
    }

    return lerp(color, colorBlur, roughness) * amount;
}
