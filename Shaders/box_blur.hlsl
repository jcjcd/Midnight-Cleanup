// (C) 2019 David Lettier
// lettier.com

#include "SamplerStates.hlsl"

Texture2D colorTexture : register(t0);

cbuffer Parameters : register(b0)
{
    float2 parameters; // x: size, y: separation
};


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

    float4 fragColor = colorTexture.Sample(gsamLinearClamp, texCoord);

    int size = (int) parameters.x;
    if (size <= 0)
    {
        return fragColor;
    }

    float separation = parameters.y;
    separation = max(separation, 1.0);

    fragColor.rgb = float3(0.0, 0.0, 0.0);

    float count = 0.0;

    for (int i = -size; i <= size; ++i)
    {
        for (int j = -size; j <= size; ++j)
        {
            fragColor.rgb += colorTexture.Sample(
                gsamLinearClamp,
                (input.position.xy + float2(i, j) * separation) / texSize
            ).rgb;

            count += 1.0;
        }
    }

    fragColor.rgb /= count;

    return fragColor;
}
