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

    
    float3 normalW = normalize(pin.NormalW);
    if (!isFrontFace)  // isFrontFace 값으로 앞뒤 판단
    {
        normalW = -normalW;
    }
    pout.Normal.xyz = normalW;
    
    float3 WorldPos = pin.PosW;
    float3 CameraVector = normalize(gEyePosW - WorldPos);
    float3 ReflectionVector = reflect(-CameraVector, normalW);

    // Perform local lighting and shading calculations
    float Local0 = dot(normalW, ReflectionVector);
    float Local1 = max(0.0, Local0);
    float Local2 = 1.0 - Local1;
    float Local3 = abs(Local2);
    float Local4 = max(Local3, 0.0001);
    float Local5 = pow(Local4, 9.305038); // PreshaderBuffer[3].x = 9.305038
    float Local6 = Local5 * (1.0 - 0.04);
    float Local7 = Local6 + 0.04;
    float3 Local8 = lerp(float3(1.0, 1.0, 1.0), float3(0.0, 0.047368, 1.0), Local7); // PreshaderBuffer[4].xyz and PreshaderBuffer[3].yzw
    float3 Local9 = Local8 * 0.3; // PreshaderBuffer[4].w = 0.300000
    float3 Local10 = Local9 / 0.5f;
    float3 Local11 = lerp(Local10, float3(0.0, 0.0, 0.0), 0.0); // PreshaderBuffer[5].yzw and PreshaderBuffer[5].x
    float Local12 = lerp(0.0, 5.0, Local7); // PreshaderBuffer[7].y and PreshaderBuffer[7].x

    // Assign calculated values to PixelMaterialInputs
    pout.Emissive.rgb = Local11;   // emissive
    pout.Emissive.a = 1.f;   // emissive
    pout.Albedo.a = Local12;    // opacity
    //PixelMaterialInputs.OpacityMask = 1.0;
    pout.Albedo.rgb = float3(0.0, 0.0, 0.0);
    pout.ORM.b = 0.0;
    pout.ORM.g = 0.5;
    pout.ORM.r = 1.0;

    pout.PositionW = float4(pin.PosW, pin.ClipSpacePosZ);
    
    pout.Mask.r = gRecieveDecal;
    pout.Mask.g = gReflect;
    pout.Mask.b = gRefract;

    return pout;
}
