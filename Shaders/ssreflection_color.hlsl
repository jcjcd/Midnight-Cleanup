// (C) 2019 David Lettier
// lettier.com

#include "SamplerStates.hlsl"

Texture2D uvTexture : register(t0);
Texture2D colorTexture : register(t1);
TextureCube skyboxTexture : register(t2);

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

cbuffer TextureAttributes : register(b0)
{
    float2 texSize;
}

cbuffer ViewPosition : register(b1)
{
    float3 viewPosition;
}

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
    int size = 6;
    float separation = 2.0;

    //float2 texSize = texSize;
    float2 texCoord = input.texCoord;

    float4 uv = uvTexture.Sample(gsamLinearClamp, texCoord);

    // Removes holes in the UV map.
    if (uv.b <= 0.0)
    {
        uv = float4(0.0, 0.0, 0.0, 0.0);
        float count = 0.0;

        for (int i = -size; i <= size; ++i)
        {
            for (int j = -size; j <= size; ++j)
            {
                uv += uvTexture.Sample(gsamLinearClamp, (float2(i, j) * separation + input.position.xy) / texSize);
                count += 1.0;
            }
        }

        uv.xyz /= count;
    }

    float4 color = float4(0.0, 0.0, 0.0, 0.0);
    if (uv.b <= 0.0)
    {
        //if (uv.a <= -1.0f)
        //{
        //    color = skyboxTexture.Sample(gsamLinearClamp, uv.xyz);
        //}
        return color;
    }

    color = colorTexture.Sample(gsamLinearClamp, uv.xy);
    float alpha = clamp(uv.b, 0.0, 1.0);

    return float4(lerp(float3(0.0, 0.0, 0.0), color.rgb, alpha), alpha);
}
