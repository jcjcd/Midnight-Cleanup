#include "ShaderVariables.hlsl"
#include "SamplerStates.hlsl"

Texture2D gInputTexture;
Texture2D gDownSampledTexture;
RWTexture2D<float4> gOutputTexture;

cbuffer BloomParams
{
    float gThreshold;
    float gScatter;
    bool gUseScatter;
    float pad;
};

[numthreads(16, 16, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height;
    gOutputTexture.GetDimensions(width, height);

    if(DTid.x < width && DTid.y < height)
    {
        float2 uv = (DTid.xy + 0.5f) / float2(width, height);
        float4 inputColor = gInputTexture.SampleLevel(gsamLinearClamp, uv, 0);
        float4 downSampledColor = gDownSampledTexture.SampleLevel(gsamLinearClamp, uv, 0);

        if(gUseScatter)
        {
            gOutputTexture[DTid.xy] = downSampledColor * gScatter + inputColor;
        }
        else
        {
            gOutputTexture[DTid.xy] = downSampledColor + inputColor;
        }
    }
}