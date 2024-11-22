#include "commonMath.hlsl"
#include "PBRSupport.hlsl"
#include "ShaderVariables.hlsl"
#include "SamplerStates.hlsl"

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
Texture2D gAlbedo;
Texture2D gNormal;
Texture2D gRMA;
Texture2D gEmissive;

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;

    // Transform to world space
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // ��յ� ��� Ȯ�밡 �������� �ֱ� ������ ������ ����ġ����� ����Ѵ�..
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorldInvTranspose);
    vout.TangentW = mul(vin.TangentL, (float3x3) gWorld);
    
    // �븻�� ����ȭ�Ѵ�.
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

PixelOut PSMain(VertexOut pin, bool isFrontFace : SV_IsFrontFace)  // SV_IsFrontFace�� �ȼ� ���̴��� �Է����� ����
{
    PixelOut pout = (PixelOut) 0.0f;

    pout.Albedo = gAlbedo.Sample(gsamAnisotropicWrap, pin.TexC);
    pout.Normal = gNormal.Sample(gsamAnisotropicWrap, pin.TexC);

    float3 normalW = NormalSampleToWorldSpace(pout.Normal.xyz, pin.NormalW, pin.TangentW);
    if (!isFrontFace)  // isFrontFace ������ �յ� �Ǵ�
    {
        normalW = -normalW;
    }
    pout.Normal.xyz = normalW;

    pout.ORM.r = gRMA.Sample(gsamAnisotropicWrap, pin.TexC).b;
    pout.ORM.g = gRMA.Sample(gsamAnisotropicWrap, pin.TexC).r;
    pout.ORM.b = gRMA.Sample(gsamAnisotropicWrap, pin.TexC).g;
    
    pout.Emissive = gEmissive.Sample(gsamAnisotropicWrap, pin.TexC);
    pout.PositionW = float4(pin.PosW, pin.ClipSpacePosZ);
    
    pout.Mask.r = gRecieveDecal;
    pout.Mask.g = gReflect;
    pout.Mask.b = gRefract;

    return pout;
}