#include "commonMath.hlsl"
#include "PBRSupport.hlsl"
#include "ShaderVariables.hlsl"
#include "SamplerStates.hlsl"
#include "normalUnpack.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float3 TangentL : TANGENT;
    float2 TexC : TEXCOORD0;
    float2 TexC1 : TEXCOORD1;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 TexC : TEXCOORD0;
    float2 TexC1 : TEXCOORD1;
    float ClipSpacePosZ : TEXCOORD2;
};

struct PixelOut
{
    float4 Albedo : SV_TARGET0;
    float4 ORM : SV_TARGET1;
    float4 Normal : SV_TARGET2;
    float4 PositionW : SV_TARGET3;
    float4 Emissive : SV_TARGET4;
    float4 Light : SV_TARGET5;
    float4 Mask : SV_TARGET6;
};

cbuffer cbMaskBuffer
{
    int gRecieveDecal;
    int gReflect;
    int gRefract;
};

Texture2D LayerBlend1 : register(t0);
Texture2D LayerBlend2 : register(t1);
Texture2D LayerBlend3 : register(t2);
Texture2D LayerBlend4 : register(t3);

Texture2D LayerAlbedo1 : register(t4);
Texture2D LayerAlbedo2 : register(t5);
Texture2D LayerAlbedo3 : register(t6);
Texture2D LayerAlbedo4 : register(t7);

Texture2D LayerNormal1 : register(t8);
Texture2D LayerNormal2 : register(t9);
Texture2D LayerNormal3 : register(t10);
Texture2D LayerNormal4 : register(t11);

Texture2D LayerORDp1 : register(t12);
Texture2D LayerORDp2 : register(t13);
Texture2D LayerORDp3 : register(t14);
Texture2D LayerORDp4 : register(t15);

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;

    // Transform to world space
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // 비균등 축소 확대가 있을수도 있기 때문에 월드의 역전치행렬을 사용한다.
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorldInvTranspose);
    vout.TangentW = mul(vin.TangentL, (float3x3) gWorld);

    // 노말을 정규화한다.
    vout.NormalW = normalize(vout.NormalW);
    vout.TangentW = normalize(vout.TangentW);

    // Transform to homogeneous clip space
    vout.PosH = mul(posW, gViewProj);

    // Output texture coordinates with tiling
    float tilingFactor = 20.0f; // Example tiling factor
    vout.TexC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform).xy;
    vout.TexC1 = mul(float4(vin.TexC * tilingFactor, 0.0f, 1.0f), gTexTransform).xy;

    // Clip space z
    vout.ClipSpacePosZ = vout.PosH.z;

    return vout;
}

PixelOut PSMain(VertexOut pin, bool isFrontFace : SV_IsFrontFace)
{
    PixelOut pout = (PixelOut) 0.0f;
    const float roughnessMultiplier = 0.75f;

    // 샘플링된 레이어의 블렌드 값들을 가져옵니다.
    float blendWeight1 = LayerBlend1.Sample(gsamAnisotropicWrap, pin.TexC).r;
    float blendWeight2 = LayerBlend2.Sample(gsamAnisotropicWrap, pin.TexC).r;
    float blendWeight3 = LayerBlend3.Sample(gsamAnisotropicWrap, pin.TexC).r;
    float blendWeight4 = LayerBlend4.Sample(gsamAnisotropicWrap, pin.TexC).r;

    // 각 레이어의 가중치를 합하여 정규화합니다.
    float totalWeight = blendWeight1 + blendWeight2 + blendWeight3 + blendWeight4;
    blendWeight1 /= totalWeight;
    blendWeight2 /= totalWeight;
    blendWeight3 /= totalWeight;
    blendWeight4 /= totalWeight;

    // 알베도 색상 블렌딩
    float4 albedo1 = LayerAlbedo1.Sample(gsamAnisotropicWrap, pin.TexC1);
    float4 albedo2 = LayerAlbedo2.Sample(gsamAnisotropicWrap, pin.TexC1);
    float4 albedo3 = LayerAlbedo3.Sample(gsamAnisotropicWrap, pin.TexC1);
    float4 albedo4 = LayerAlbedo4.Sample(gsamAnisotropicWrap, pin.TexC1);
    float4 finalAlbedo = albedo1 * blendWeight1 + albedo2 * blendWeight2 + albedo3 * blendWeight3 + albedo4 * blendWeight4;
    pout.Albedo = finalAlbedo;
    
    // 알베도 톤 매핑
    pout.Albedo.rgb = pout.Albedo.rgb / (pout.Albedo.rgb + tofloat3(1.0f));
    pout.Albedo.rgb = pow(pout.Albedo.rgb, 1.0f / 2.2f);

    // 노멀 맵 블렌딩
    float4 normal1 = LayerNormal1.Sample(gsamAnisotropicWrap, pin.TexC1);
    float4 normal2 = LayerNormal2.Sample(gsamAnisotropicWrap, pin.TexC1);
    float4 normal3 = LayerNormal3.Sample(gsamAnisotropicWrap, pin.TexC1);
    float4 normal4 = LayerNormal4.Sample(gsamAnisotropicWrap, pin.TexC1);
    //float3 normal1Unpacked = UnpackNormalMap(normal1).xyz;
    //float3 normal2Unpacked = UnpackNormalMap(normal2).xyz;
    //float3 normal3Unpacked = UnpackNormalMap(normal3).xyz;
    //float3 normal4Unpacked = UnpackNormalMap(normal4).xyz;
    float3 normal1Unpacked = normal1.xyz;
    float3 normal2Unpacked = normal2.xyz;
    float3 normal3Unpacked = normal3.xyz;
    float3 normal4Unpacked = normal4.xyz;
    float3 blendedNormal = normal1Unpacked * blendWeight1 + normal2Unpacked * blendWeight2 + normal3Unpacked * blendWeight3 + normal4Unpacked * blendWeight4;
    float3 normalW = NormalSampleToWorldSpace(normalize(blendedNormal), pin.NormalW, pin.TangentW);
    pout.Normal = float4(normalW, 1.0f);

    // ORM (Occlusion, Roughness, Metallic) 블렌딩
    float4 ordp1 = LayerORDp1.Sample(gsamAnisotropicWrap, pin.TexC1);
    float4 ordp2 = LayerORDp2.Sample(gsamAnisotropicWrap, pin.TexC1);
    float4 ordp3 = LayerORDp3.Sample(gsamAnisotropicWrap, pin.TexC1);
    float4 ordp4 = LayerORDp4.Sample(gsamAnisotropicWrap, pin.TexC1);
    float4 finalORM = ordp1 * blendWeight1 + ordp2 * blendWeight2 + ordp3 * blendWeight3 + ordp4 * blendWeight4;
    finalORM.g *= roughnessMultiplier;
    finalORM.b = 0.0f;
    pout.ORM = finalORM;

    // 월드 위치 저장
    pout.PositionW = float4(pin.PosW, pin.ClipSpacePosZ);

    // 임시로 방출(Emissive)과 조명(Light)은 0으로 설정
    pout.Emissive = float4(0.0f, 0.0f, 0.0f, 1.0f);
    pout.Light = float4(0.0f, 0.0f, 0.0f, 1.0f);

    // 마스크 정보 설정 (예: 반사, 굴절 등)
    pout.Mask.r = gRecieveDecal;
    pout.Mask.g = gReflect;
    pout.Mask.b = gRefract;
    
    return pout;
}