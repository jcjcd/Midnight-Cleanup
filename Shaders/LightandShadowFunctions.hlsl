float CalculateShadowRatio(SamplerComparisonState samShadow, Texture2DArray shadowMap, float4 shadowPosTex, uint index, float shadowSize)
{
    shadowPosTex.xyz /= shadowPosTex.w;

    if (shadowPosTex.x < -1.f
        || shadowPosTex.x > 1.f
        || shadowPosTex.y < -1.f
        || shadowPosTex.y > 1.f
        || shadowPosTex.z < -1.f
        || shadowPosTex.z > 1.f)
    {
        return 1.f;
    }

    shadowPosTex.xy = (shadowPosTex.xy  + 1.0f) * 0.5f;
    shadowPosTex.y = 1 - shadowPosTex.y;
    
    float delta = 1.f / shadowSize; // 그림자 맵 사이즈
    
    float shadowRatio = 0.f;
    
    // 3x3 커널
    const float2 offsets[9] =
    {
        float2(-delta, -delta),
        float2(0, -delta),
        float2(+delta, -delta),
        
        float2(-delta, 0),
        float2(0, 0),
        float2(+delta, 0),
        
        float2(-delta, +delta),
        float2(0, +delta),
        float2(+delta, +delta)
    };
    
    // 판정 결과를 보간해서 그림자 비율을 구함
    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        shadowRatio += shadowMap.SampleCmpLevelZero(samShadow, float3(shadowPosTex.xy + offsets[i], index), shadowPosTex.z).r;
    }
    
    return shadowRatio /= 9.f;
}

float CalculateCascadeShadowRatio(SamplerComparisonState samShadow, Texture2DArray shadowMap, float4 shadowPosTex, uint index, float shadowSize)
{
    if (shadowPosTex.x < -1.f
        || shadowPosTex.x > 1.f
        || shadowPosTex.y < -1.f
        || shadowPosTex.y > 1.f
        || shadowPosTex.z < -1.f
        || shadowPosTex.z > 1.f)
    {
        return 1.f;
    }
    
    float delta = 1.f / shadowSize; // 그림자 맵 사이즈
    
    float shadowRatio = 0.f;
    
    // 3x3 커널
    const float2 offsets[9] =
    {
        float2(-delta, -delta),
        float2(0, -delta),
        float2(+delta, -delta),
        
        float2(-delta, 0),
        float2(0, 0),
        float2(+delta, 0),
        
        float2(-delta, +delta),
        float2(0, +delta),
        float2(+delta, +delta)
    };
    
    // 판정 결과를 보간해서 그림자 비율을 구함
    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        shadowRatio += shadowMap.SampleCmpLevelZero(samShadow, float3(shadowPosTex.xy + offsets[i], index), shadowPosTex.z).r;
    }
    
    return shadowRatio /= 9.f;
}

float CalcDepthInShadow(float3 fragPos, float farPlane, float nearPlane)
{
    const float c1 = farPlane / (farPlane - nearPlane);
    const float c0 = farPlane * nearPlane / (nearPlane - farPlane);
    float3 m = abs(fragPos);
    float major = max(m.x, max(m.y, m.z));
    return (c1 * major + c0) / major;
}

float CalcPointShadowFactor(SamplerComparisonState samShadow, Texture2DArray shadowMap, float4 fragToLight, float4 shadowPosTex, float farPlane, float nearPlane, uint index, float shadowSize)
{
    float currentDepth = CalcDepthInShadow(fragToLight.xyz, farPlane, nearPlane);

    float delta = 1.f / shadowSize; // 그림자 맵 사이즈

    // 3x3 커널
    const float2 offsets[9] =
    {
        float2(-delta, -delta),
        float2(0, -delta),
        float2(+delta, -delta),
        
        float2(-delta, 0),
        float2(0, 0),
        float2(+delta, 0),
        
        float2(-delta, +delta),
        float2(0, +delta),
        float2(+delta, +delta)
    };

    // 판정 결과를 보간해서 그림자 비율을 구함

    float shadowRatio = 0.f;

    shadowPosTex.xyz /= shadowPosTex.w;

    if (shadowPosTex.x < -1.f
        || shadowPosTex.x > 1.f
        || shadowPosTex.y < -1.f
        || shadowPosTex.y > 1.f
        || shadowPosTex.z < -1.f
        || shadowPosTex.z > 1.f)
    {
        return 1.0f;
    }

    shadowPosTex.xy = (shadowPosTex.xy  + 1.0f) * 0.5f;
    shadowPosTex.y = 1 - shadowPosTex.y;

    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        shadowRatio += shadowMap.SampleCmpLevelZero(samShadow, float3(shadowPosTex.xy + offsets[i], index), currentDepth).r;
    }   

    return shadowRatio / 9.f;
}

float CalcPointShadowFactor2(SamplerComparisonState samShadow, Texture2DArray shadowMap, float4 fragToLight, float4 shadowPosTex, float farPlane, float nearPlane, uint index, float shadowSize)
{
    float delta = 1.f / shadowSize; // 그림자 맵 사이즈
    // 3x3 커널
    const float2 offsets[9] =
    {
        float2(-delta, -delta),
        float2(0, -delta),
        float2(+delta, -delta),
        
        float2(-delta, 0),
        float2(0, 0),
        float2(+delta, 0),
        
        float2(-delta, +delta),
        float2(0, +delta),
        float2(+delta, +delta)
    };

    float currentDepth = CalcDepthInShadow(shadowPosTex.xyz, farPlane, nearPlane);
    float shadowRatio = 0.f;
    shadowPosTex.xyz /= shadowPosTex.w;
    
    if (shadowPosTex.x < -1.f
        || shadowPosTex.x > 1.f
        || shadowPosTex.y < -1.f
        || shadowPosTex.y > 1.f
        || shadowPosTex.z < -1.f
        || shadowPosTex.z > 1.f)
    {
        return 1.0f;
    }
    
    shadowPosTex.xy = (shadowPosTex.xy  + 1.0f) * 0.5f;
    shadowPosTex.y = 1 - shadowPosTex.y;                  

    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        shadowRatio += shadowMap.SampleCmpLevelZero(samShadow, float3(shadowPosTex.xy + offsets[i], index), currentDepth); 
    }
    return shadowRatio /= 9.f;
        // It is currently in linear range between [0,1]. Re-transform back to original value
    //closestDepth *= farPlane;

    // Now test for shadows
    //float bias = 0.05;
    //float shadow = currentDepth - bias > shadowRatio ? 1.0 : 0.0;
    //
    //return shadow;

    //float delta = 1.f / shadowSize; // 그림자 맵 사이즈
    //
    //// 3x3 커널
    //const float2 offsets[9] =
    //{
    //    float2(-delta, -delta),
    //    float2(0, -delta),
    //    float2(+delta, -delta),
    //    
    //    float2(-delta, 0),
    //    float2(0, 0),
    //    float2(+delta, 0),
    //    
    //    float2(-delta, +delta),
    //    float2(0, +delta),
    //    float2(+delta, +delta)
    //};
    //
    //// 판정 결과를 보간해서 그림자 비율을 구함
    //
    //float shadowRatio = 0.f;
    //
    //shadowPosTex.xyz /= shadowPosTex.w;
    //
    //if (shadowPosTex.x < -1.f
    //    || shadowPosTex.x > 1.f
    //    || shadowPosTex.y < -1.f
    //    || shadowPosTex.y > 1.f
    //    || shadowPosTex.z < -1.f
    //    || shadowPosTex.z > 1.f)
    //{
    //    return 1.0f;
    //}
    //
    //shadowPosTex.xy = (shadowPosTex.xy  + 1.0f) * 0.5f;
    //shadowPosTex.y = 1 - shadowPosTex.y;                  
    //
    //[unroll]
    //for (int i = 0; i < 9; ++i)
    //{
    //    shadowRatio += shadowMap.SampleCmpLevelZero(samShadow, float3(shadowPosTex.xy + offsets[i], index), currentDepth).r;
    //}   
    //
    //return shadowRatio / 9.f;
}

float PLShadowCalculation(float4 shadowPosTex, float3 fragToLight, SamplerState samShadow, Texture2DArray shadowMap, uint index, float farPlane)
{
    shadowPosTex.xyz /= shadowPosTex.w;
    shadowPosTex.xy = (shadowPosTex.xy  + 1.0f) * 0.5f;
    shadowPosTex.y = 1 - shadowPosTex.y;               

    float closestDepth = shadowMap.Sample(samShadow, float3(shadowPosTex.xy, index), 0).r;
    // It is currently in linear range between [0,1]. Re-transform back to original value
    closestDepth *= farPlane;

    if(closestDepth == 0)
        return 1.0f;
    
    float currentDepth = length(fragToLight);
    // Now test for shadows
    float bias = 0.05f;
    float shadow = currentDepth - bias > closestDepth ? 0.0f : 1.0f;

    return shadow;
}
