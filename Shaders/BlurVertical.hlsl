#include "ShaderVariables.hlsl"
#include "SamplerStates.hlsl"

cbuffer cbFixed
{
    static const float weights[9] =
    {
        0.013519569015984728,
        0.047662179108871855,
        0.11723004402070096,
        0.20116755999375591,
        0.240841295721373,
        0.20116755999375591,
        0.11723004402070096,
        0.047662179108871855,
        0.013519569015984728
    };
    static const int gBlurRadius = 4;
};

Texture2D gInputTexture;
RWTexture2D<float4> gOutputTexture;

#define N 256
#define CacheSize (N + 2 * gBlurRadius)
groupshared float4 gCache[CacheSize];

[numthreads(1, N, 1)]
void CSMain(int3 GTid : SV_GroupThreadID
    ,uint3 DTid : SV_DispatchThreadID)
{
    // 위쪽 가장자리 처리
    if (GTid.y < gBlurRadius)
    {
        int y = max(DTid.y - gBlurRadius, 0);
        gCache[GTid.y] = gInputTexture[int2(DTid.x, y)];
    }

    // 아래쪽 가장자리 처리
    if (GTid.y >= N - gBlurRadius)
    {
        int y = min(DTid.y + gBlurRadius, gInputTexture.Length.y - 1);
        gCache[GTid.y + 2 * gBlurRadius] = gInputTexture[int2(DTid.x, y)];
    }

    // 5 ~ textureSize - 5
    gCache[GTid.y + gBlurRadius] = gInputTexture[min(DTid.xy, gInputTexture.Length.xy - 1)];

    GroupMemoryBarrierWithGroupSync();

    float4 result = float4(0, 0, 0, 0);

    [unroll]
    for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
        uint k = GTid.y + i + gBlurRadius;
        result += weights[i + gBlurRadius] * gCache[k];
    }

    gOutputTexture[DTid.xy] = result;
}

