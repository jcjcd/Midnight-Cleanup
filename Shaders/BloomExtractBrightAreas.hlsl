#include "ShaderVariables.hlsl"
#include "SamplerStates.hlsl"

Texture2D gInputTexture;
RWTexture2D<float4> gOutputTexture;

cbuffer BloomParams
{
    float gThreshold;
    float gScatter;
    float2 pads;
};

float4 ExtractBrightAreas(float4 color, float threshold)
{
    float brightness = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));
    return (brightness > threshold) ? color : float4(0, 0, 0, 0);
}

[numthreads(16, 16, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height;
    gOutputTexture.GetDimensions(width, height);

    if(DTid.x < width && DTid.y < height)
    {
        float2 uv = DTid.xy / float2(width, height);
        float4 color = gInputTexture.SampleLevel(gsamLinearClamp, uv, 0);
        gOutputTexture[DTid.xy] = ExtractBrightAreas(color, gThreshold);
    }
}