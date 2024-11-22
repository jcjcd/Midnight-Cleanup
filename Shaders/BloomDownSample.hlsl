#include "ShaderVariables.hlsl"
#include "SamplerStates.hlsl"

Texture2D gInputTexture;
RWTexture2D<float4> gOutputTexture;

[numthreads(16, 16, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height;
    gOutputTexture.GetDimensions(width, height);

    if(DTid.x < width && DTid.y < height)
    {
        float2 uv = (DTid.xy + 0.5f) / float2(width, height);
        float4 color = gInputTexture.SampleLevel(gsamLinearClamp, uv, 0);
        gOutputTexture[DTid.xy] = color;
    }
}