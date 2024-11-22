#include "commonMath.hlsl"
#include "PBRSupport.hlsl"
#include "ShaderVariables.hlsl"
#include "SamplerStates.hlsl"

cbuffer cbPerLight
{
    float4 gLightPosW[4];
    float4 gLightColor[4];
};

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float3 TangentL : TANGENT;
    float2 TexC : TEXCOORD0;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 TexC : TEXCOORD;
};

Texture2D gAlbedo;
Texture2D gNormal;
Texture2D gMetallic;
Texture2D gRoughness;
Texture2D gAO;


VertexOut VSMain(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;

    // Transform to world space
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;
    
    // 비균등 축소 확대가 있을수도 있기 때문에 월드의 역전치행렬을 사용한다..
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorldInvTranspose);
    vout.TangentW = mul(vin.TangentL, (float3x3) gWorld);

    // Transform to homogeneous clip space
    vout.PosH = mul(posW, gViewProj);

    // Output texture coordinates
    vout.TexC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform).xy;
    
    return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    float3 N = normalize(pin.NormalW);
    float3 V = normalize(gEyePosW - pin.PosW);

    float3 albedo = gAlbedo.Sample(gsamLinearWrap, pin.TexC).rgb;
    albedo = pow(albedo, 2.2f);
    
    float4 normal = gNormal.Sample(gsamLinearWrap, pin.TexC);
    float3 normalW = NormalSampleToWorldSpace(normal.xyz, N, pin.TangentW);
    N = normalW;

    float metallic = gMetallic.Sample(gsamLinearWrap, pin.TexC).r;
    float roughness = gRoughness.Sample(gsamLinearWrap, pin.TexC).r;
    float ao = gAO.Sample(gsamLinearWrap, pin.TexC).r;

    float3 F0 = tofloat3(0.04f);
    F0 = lerp(F0, albedo, metallic);

    //reflection equation
    float3 Lo = tofloat3(0.0f);
    for (int i = 0; i < 4; ++i)
    {
        // calculate per-light radiance
        float3 L = normalize(gLightPosW[i].xyz - pin.PosW);
        float3 H = normalize(V + L);
        float distance = length(gLightPosW[i].xyz - pin.PosW);
        //float attenuation = 1.0f / (distance * distance);
        float3 radiance = gLightColor[i].xyz;
        
        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        float3 kS = F;
        float3 kD = tofloat3(1.0f) - kS;
        kD *= 1.0f - metallic;
        
        float3 numerator = NDF * G * F;
        float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001f;
        float3 specular = numerator / denominator;
        
        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }
    
    // ambient lighting
    //float ao = 1.0f;
    float3 ambient = tofloat3(0.03f) * albedo * ao;
    float3 color = ambient + Lo;
    
    // HDR tonemapping
    color = color / (color + tofloat3(1.0f));
    color = pow(color, 1.0f / 2.2f);
    
    return float4(color, 1.0f);

}
