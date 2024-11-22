#ifndef COMMON_MATH_HLSL
#define COMMON_MATH_HLSL

#define COMMON_PI 3.14159265359f

inline float3 tofloat3(float v)
{
    return float3(v, v, v);
}

inline float2 tofloat2(float v)
{
    return float2(v, v);
}

inline float4 tofloat4(float v)
{
    return float4(v, v, v, v);
}

#endif // COMMON_MATH_HLSL