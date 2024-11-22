#include "ShaderVariables.hlsl"
#include "SamplerStates.hlsl"

Texture2D equirectangularMap;

struct VertexIn
{
    float3 PosL : POSITION;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosL : POSITION;
};

float2 SampleSphericalMap(float3 v)
{
    float2 invAtan = float2(0.1591, 0.3183);
    float2 uv = float2(atan2(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    uv.y = 1.0 - uv.y; // Flip the y-coordinate
    return uv;
}

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout;
    
    // 국소 정점 위치를 입방체 맵 조회 벡터로 사용한다.
    vout.PosL = vin.PosL;
    
    // 세계 공간으로 변환한다.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    
    // z/w = 1이 되도록(즉, 하늘 구가 항상 먼 평면에 있도록) z = w로 설정한다.
    vout.PosH = mul(posW, gViewProj).xyzw;
    
    return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    float3 normalizedPos = normalize(pin.PosL);
    float2 uv = SampleSphericalMap(normalizedPos);
    float3 color = equirectangularMap.Sample(gsamLinearWrap, uv).rgb;

    return float4(color, 1.0);
}
