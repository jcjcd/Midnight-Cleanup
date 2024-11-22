float3 UnpackNormalmapRGorAG(float4 packednormal)
{
    packednormal.x *= packednormal.w;

    float3 normal;
    normal.xy = packednormal.xy * 2 - 1;
    normal.z = sqrt(1 - saturate(dot(normal.xy, normal.xy)));
    return normal;
}

inline float3 UnpackNormal(float4 packednormal)
{
    return UnpackNormalmapRGorAG(packednormal);
}

float4 UnpackNormalMap(float4 TextureSample)
{
    float3 Unpacked = UnpackNormal(TextureSample);
    Unpacked.y *= -1;
    return float4(Unpacked.xy, Unpacked.z, 1.0f);
}