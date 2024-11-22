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
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 TexC : TEXCOORD0;
    float ClipSpacePosZ : TEXCOORD1;
};

struct PixelOut
{
    float4 reveal : SV_Target0; // r8만 사용
    float4 accum : SV_Target1;  // 다 사용 하지만 해상도때문에 1번에 배치
};

Texture2D gAlbedo : register(t0);

cbuffer cbMaskBuffer
{
    int gRecieveDecal;
    int gReflect;
    int gRefract;
};

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;

    // Transform to world space
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // 비균등 축소 확대가 있을수도 있기 때문에 월드의 역전치행렬을 사용한다..
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorldInvTranspose);
    vout.TangentW = mul(vin.TangentL, (float3x3) gWorld);
    
    // 노말을 정규화한다.
    vout.NormalW = normalize(vout.NormalW);
    vout.TangentW = normalize(vout.TangentW);

    // Transform to homogeneous clip space
    vout.PosH = mul(posW, gViewProj);

    // Output texture coordinates
    vout.TexC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform).xy;

    // Clip space z
    vout.ClipSpacePosZ = vout.PosH.z;
    
    return vout;
}

PixelOut PSMain(VertexOut pin, bool isFrontFace : SV_IsFrontFace)
{
    PixelOut pout = (PixelOut) 0.0f;
    
    // Sample albedo
    float4 color = gAlbedo.Sample(gsamLinearWrap, pin.TexC);
    // 여기서 그냥 하드코딩
    color.a = 0.3;
    float4 fragCoord = float4(pin.PosH.xy / pin.PosH.w, pin.ClipSpacePosZ, 1.0);
    
    
    // Weight function
    float weight = clamp(pow(min(1.0, color.a * 10.0) + 0.01, 3.0) * 1e8 *
                         pow(1.0 - fragCoord.z * 0.9, 3.0), 1e-2, 3e3);

    // Store pixel color accumulation
    pout.accum = float4(color.rgb * color.a, color.a) * weight;

    // Store pixel revealage threshold
    pout.reveal = color.a;
    
    return pout;
}
