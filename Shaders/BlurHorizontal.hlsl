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

[numthreads(N, 1, 1)]
void CSMain(int3 GTid : SV_GroupThreadID
    ,uint3 DTid : SV_DispatchThreadID)
{
    // 왼쪽 가장자리 처리
    if (GTid.x < gBlurRadius)
    {
        int x = max(DTid.x - gBlurRadius, 0);
        gCache[GTid.x] = gInputTexture[int2(x, DTid.y)];
    }

    // 오른쪽 가장자리 처리
    if (GTid.x >= N - gBlurRadius)
    {
        int x = min(DTid.x + gBlurRadius, gInputTexture.Length.x - 1);
        gCache[GTid.x + 2 * gBlurRadius] = gInputTexture[int2(x, DTid.y)];
    }

    // 5 ~ textureSize - 5
    gCache[GTid.x + gBlurRadius] = gInputTexture[min(DTid.xy, gInputTexture.Length.xy - 1)];

    GroupMemoryBarrierWithGroupSync();

    float4 result = float4(0, 0, 0, 0);

    [unroll]
    for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
        uint k = GTid.x + i + gBlurRadius;
        result += weights[i + gBlurRadius] * gCache[k];
    }

    gOutputTexture[DTid.xy] = result;
}


