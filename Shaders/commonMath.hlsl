#ifndef COMMON_MATH_HLSL
#define COMMON_MATH_HLSL

#define PI 3.14159265359f

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

inline float safeInverseLerp(float a, float b, float t)
{
    t = max(t, 0.001f);
    float invT = 1.0f / t;
    return a + (b - a) * invT;
}

float PositiveClampedPow(float X, float Y)
{
    return pow(max(X, 0.0f), Y);
}
float2 PositiveClampedPow(float2 X, float2 Y)
{
    return pow(max(X, float2(0.0f, 0.0f)), Y);
}
float3 PositiveClampedPow(float3 X, float3 Y)
{
    return pow(max(X, float3(0.0f, 0.0f, 0.0f)), Y);
}
float4 PositiveClampedPow(float4 X, float4 Y)
{
    return pow(max(X, float4(0.0f, 0.0f, 0.0f, 0.0f)), Y);
}



#endif // COMMON_MATH_HLSL